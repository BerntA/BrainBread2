//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*
Entity Data Descriptions

Each entity has an array which defines it's data in way that is useful for
entity communication, parsing initial values from the map, and save/restore.

each entity has to have the following line in it's class declaration:

DECLARE_DATADESC();

this line defines that it has an m_DataDesc[] array, and declares functions through
which various subsystems can iterate the data.

In it's implementation, each entity has to have:

typedescription_t CBaseEntity::m_DataDesc[] = { ... }

in which all it's data is defined (see below), followed by:


which implements the functions necessary for iterating through an entities data desc.

There are several types of data:

FIELD		: this is a variable which gets saved & loaded from disk
KEY			: this variable can be read in from the map file
GLOBAL		: a global field is actually local; it is saved/restored, but is actually
unique to the entity on each level.
CUSTOM		: the save/restore parsing functions are described by the user.
ARRAY		: an array of values
OUTPUT		: a variable or event that can be connected to other entities (see below)
INPUTFUNC	: maps a string input to a function pointer. Outputs connected to this input
will call the notify function when fired.
INPUT		: maps a string input to a member variable. Outputs connected to this input
will update the input data value when fired.
INPUTNOTIFY	: maps a string input to a member variable/function pointer combo. Outputs
connected to this input will update the data value and call the notify
function when fired.

some of these can overlap.  all the data descriptions usable are:

DEFINE_FIELD(		name,	fieldtype )
DEFINE_KEYFIELD(	name,	fieldtype,	mapname )
DEFINE_KEYFIELD_NOTSAVED(	name,	fieldtype,	mapname )
DEFINE_ARRAY(		name,	fieldtype,	count )
DEFINE_GLOBAL_FIELD(name,	fieldtype )
DEFINE_CUSTOM_FIELD(name,	datafuncs,	mapname )
DEFINE_GLOBAL_KEYFIELD(name,	fieldtype,	mapname )

where:
type is the name of the class (eg. CBaseEntity)
name is the name of the variable in the class (eg. m_iHealth)
fieldtype is the type of data (FIELD_STRING, FIELD_INTEGER, etc)
mapname is the string by which this variable is associated with map file data
count is the number of items in the array
datafuncs is a struct containing function pointers for a custom-defined save/restore/parse

OUTPUTS:

DEFINE_OUTPUT(		outputvar,	outputname )

This maps the string 'outputname' to the COutput-derived member variable outputvar.  In the VMF
file these outputs can be hooked up to inputs (see above).  Whenever the internal state
of an entity changes it will often fire off outputs so that map makers can hook up behaviors.
e.g.  A door entity would have OnDoorOpen, OnDoorClose, OnTouched, etc outputs.
*/


#include "cbase.h"
#include "entitylist.h"
#include "mapentities_shared.h"
#include "eventqueue.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "mempool.h"
#include "tier1/strtools.h"
#include "datacache/imdlcache.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: iterates through a typedescript data block, so it can insert key/value data into the block
// Input  : *pObject - pointer to the struct or class the data is to be insterted into
//			*pFields - description of the data
//			iNumFields - number of fields contained in pFields
//			char *szKeyName - name of the variable to look for
//			char *szValue - value to set the variable to
// Output : Returns true if the variable is found and set, false if the key is not found.
//-----------------------------------------------------------------------------
bool ParseKeyvalue(void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue)
{
	int i;
	typedescription_t 	*pField;

	for (i = 0; i < iNumFields; i++)
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[TD_OFFSET_NORMAL];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for (datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap)
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if (ParseKeyvalue(pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue))
					return true;
			}
		}

		if ((pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*(string_t *)((char *)pObject + fieldOffset)) = AllocPooledString(szValue);
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float *)((char *)pObject + fieldOffset)) = atof(szValue);
				return true;

			case FIELD_BOOLEAN:
				(*(bool *)((char *)pObject + fieldOffset)) = (bool)(atoi(szValue) != 0);
				return true;

			case FIELD_CHARACTER:
				(*(char *)((char *)pObject + fieldOffset)) = (char)atoi(szValue);
				return true;

			case FIELD_SHORT:
				(*(short *)((char *)pObject + fieldOffset)) = (short)atoi(szValue);
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				(*(int *)((char *)pObject + fieldOffset)) = atoi(szValue);
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				UTIL_StringToVector((float *)((char *)pObject + fieldOffset), szValue);
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				UTIL_StringToFloatArray((float *)((char *)pObject + fieldOffset), 16, szValue);
				return true;

			case FIELD_MATRIX3X4_WORLDSPACE:
				UTIL_StringToFloatArray((float *)((char *)pObject + fieldOffset), 12, szValue);
				return true;

			case FIELD_COLOR32:
				UTIL_StringToColor32((color32 *)((char *)pObject + fieldOffset), szValue);
				return true;

			case FIELD_CUSTOM:
			{
				InputOutputFieldInfo_t fieldInfo =
				{
					(char *)pObject + fieldOffset,
					pObject,
					pField
				};
				pField->pIODataOps->Parse(fieldInfo, szValue);
				return true;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning("Bad field in entity!!\n");
				Assert(0);
				break;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: iterates through a typedescript data block, so it can insert key/value data into the block
// Input  : *pObject - pointer to the struct or class the data is to be insterted into
//			*pFields - description of the data
//			iNumFields - number of fields contained in pFields
//			char *szKeyName - name of the variable to look for
//			char *szValue - value to set the variable to
// Output : Returns true if the variable is found and set, false if the key is not found.
//-----------------------------------------------------------------------------
bool ExtractKeyvalue(void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen)
{
	int i;
	typedescription_t 	*pField;

	for (i = 0; i < iNumFields; i++)
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[TD_OFFSET_NORMAL];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for (datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap)
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if (ExtractKeyvalue(pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue, iMaxLen))
					return true;
			}
		}

		if ((pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				Q_strncpy(szValue, ((char *)pObject + fieldOffset), iMaxLen);
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				Q_snprintf(szValue, iMaxLen, "%f", (*(float *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_BOOLEAN:
				Q_snprintf(szValue, iMaxLen, "%d", (*(bool *)((char *)pObject + fieldOffset)) != 0);
				return true;

			case FIELD_CHARACTER:
				Q_snprintf(szValue, iMaxLen, "%d", (*(char *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_SHORT:
				Q_snprintf(szValue, iMaxLen, "%d", (*(short *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				Q_snprintf(szValue, iMaxLen, "%d", (*(int *)((char *)pObject + fieldOffset)));
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				Q_snprintf(szValue, iMaxLen, "%f %f %f",
					((float *)((char *)pObject + fieldOffset))[0],
					((float *)((char *)pObject + fieldOffset))[1],
					((float *)((char *)pObject + fieldOffset))[2]);
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
				return false;

			case FIELD_MATRIX3X4_WORLDSPACE:
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
				return false;

			case FIELD_COLOR32:
				Q_snprintf(szValue, iMaxLen, "%d %d %d %d",
					((int *)((char *)pObject + fieldOffset))[0],
					((int *)((char *)pObject + fieldOffset))[1],
					((int *)((char *)pObject + fieldOffset))[2],
					((int *)((char *)pObject + fieldOffset))[3]);
				return true;

			case FIELD_CUSTOM:
			{
				/*
				InputOutputFieldInfo_t fieldInfo =
				{
				(char *)pObject + fieldOffset,
				pObject,
				pField
				};
				pField->pIODataOps->Parse( fieldInfo, szValue );
				*/
				return false;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning("Bad field in entity!!\n");
				Assert(0);
				break;
			}
		}
	}

	return false;
}

// ID Stamp used to uniquely identify every output
int CEventAction::s_iNextIDStamp = 0;

//-----------------------------------------------------------------------------
// Purpose: Creates an event action and assigns it an unique ID stamp.
// Input  : ActionData - the map file data block descibing the event action.
//-----------------------------------------------------------------------------
CEventAction::CEventAction(const char *ActionData)
{
	m_pNext = NULL;
	m_iIDStamp = ++s_iNextIDStamp;

	m_flDelay = 0;
	m_iTarget = NULL_STRING;
	m_iParameter = NULL_STRING;
	m_iTargetInput = NULL_STRING;
	m_nTimesToFire = EVENT_FIRE_ALWAYS;

	if (ActionData == NULL)
		return;

	char szToken[256];

	//
	// Parse the target name.
	//
	const char *psz = nexttoken(szToken, ActionData, ',', sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_iTarget = AllocPooledString(szToken);
	}

	//
	// Parse the input name.
	//
	psz = nexttoken(szToken, psz, ',', sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_iTargetInput = AllocPooledString(szToken);
	}
	else
	{
		m_iTargetInput = AllocPooledString("Use");
	}

	//
	// Parse the parameter override.
	//
	psz = nexttoken(szToken, psz, ',', sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_iParameter = AllocPooledString(szToken);
	}

	//
	// Parse the delay.
	//
	psz = nexttoken(szToken, psz, ',', sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_flDelay = atof(szToken);
	}

	//
	// Parse the number of times to fire.
	//
	nexttoken(szToken, psz, ',', sizeof(szToken));
	if (szToken[0] != '\0')
	{
		m_nTimesToFire = atoi(szToken);
		if (m_nTimesToFire == 0)
		{
			m_nTimesToFire = EVENT_FIRE_ALWAYS;
		}
	}
}


// this memory pool stores blocks around the size of CEventAction/inputitem_t structs
// can be used for other blocks; will error if to big a block is tried to be allocated
CUtlMemoryPool g_EntityListPool(MAX(sizeof(CEventAction), sizeof(CMultiInputVar::inputitem_t)), 512, CUtlMemoryPool::GROW_FAST, "g_EntityListPool");

#include "tier0/memdbgoff.h"

void *CEventAction::operator new(size_t stAllocateBlock)
{
	return g_EntityListPool.Alloc(stAllocateBlock);
}

void *CEventAction::operator new(size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine)
{
	return g_EntityListPool.Alloc(stAllocateBlock);
}

void CEventAction::operator delete(void *pMem)
{
	g_EntityListPool.Free(pMem);
}

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Returns the highest-valued delay in our list of event actions.
//-----------------------------------------------------------------------------
float CBaseEntityOutput::GetMaxDelay(void)
{
	float flMaxDelay = 0;
	CEventAction *ev = m_ActionList;

	while (ev != NULL)
	{
		if (ev->m_flDelay > flMaxDelay)
		{
			flMaxDelay = ev->m_flDelay;
		}
		ev = ev->m_pNext;
	}

	return(flMaxDelay);
}


//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CBaseEntityOutput::~CBaseEntityOutput()
{
	CEventAction *ev = m_ActionList;
	while (ev != NULL)
	{
		CEventAction *pNext = ev->m_pNext;
		delete ev;
		ev = pNext;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fires the event, causing a sequence of action to occur in other ents.
// Input  : pActivator - Entity that initiated this sequence of actions.
//			pCaller - Entity that is actually causing the event.
//-----------------------------------------------------------------------------
void CBaseEntityOutput::FireOutput(variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	//
	// Iterate through all eventactions and fire them off.
	//
	CEventAction *ev = m_ActionList;
	CEventAction *prev = NULL;

	while (ev != NULL)
	{
		if (ev->m_iParameter == NULL_STRING)
		{
			//
			// Post the event with the default parameter.
			//
			g_EventQueue.AddEvent(STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), Value, ev->m_flDelay + fDelay, pActivator, pCaller, ev->m_iIDStamp);
		}
		else
		{
			//
			// Post the event with a parameter override.
			//
			variant_t ValueOverride;
			ValueOverride.SetString(ev->m_iParameter);
			g_EventQueue.AddEvent(STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), ValueOverride, ev->m_flDelay, pActivator, pCaller, ev->m_iIDStamp);
		}

		if (ev->m_flDelay)
		{
			char szBuffer[256];
			Q_snprintf(szBuffer,
				sizeof(szBuffer),
				"(%0.2f) output: (%s,%s) -> (%s,%s,%.1f)(%s)\n",
				gpGlobals->curtime,
				pCaller ? STRING(pCaller->m_iClassname) : "NULL",
				pCaller ? STRING(pCaller->GetEntityName()) : "NULL",
				STRING(ev->m_iTarget),
				STRING(ev->m_iTargetInput),
				ev->m_flDelay,
				STRING(ev->m_iParameter));

			DevMsg(2, "%s", szBuffer);
		}
		else
		{
			char szBuffer[256];
			Q_snprintf(szBuffer,
				sizeof(szBuffer),
				"(%0.2f) output: (%s,%s) -> (%s,%s)(%s)\n",
				gpGlobals->curtime,
				pCaller ? STRING(pCaller->m_iClassname) : "NULL",
				pCaller ? STRING(pCaller->GetEntityName()) : "NULL", STRING(ev->m_iTarget),
				STRING(ev->m_iTargetInput),
				STRING(ev->m_iParameter));

			DevMsg(2, "%s", szBuffer);
		}

		if (pCaller && pCaller->m_debugOverlays & OVERLAY_MESSAGE_BIT)
		{
			pCaller->DrawOutputOverlay(ev);
		}

		//
		// Remove the event action from the list if it was set to be fired a finite
		// number of times (and has been).
		//
		bool bRemove = false;
		if (ev->m_nTimesToFire != EVENT_FIRE_ALWAYS)
		{
			ev->m_nTimesToFire--;
			if (ev->m_nTimesToFire == 0)
			{
				char szBuffer[256];
				Q_snprintf(szBuffer, sizeof(szBuffer), "Removing from action list: (%s,%s) -> (%s,%s)\n", pCaller ? STRING(pCaller->m_iClassname) : "NULL", pCaller ? STRING(pCaller->GetEntityName()) : "NULL", STRING(ev->m_iTarget), STRING(ev->m_iTargetInput));
				DevMsg(2, "%s", szBuffer);
				bRemove = true;
			}
		}

		if (!bRemove)
		{
			prev = ev;
			ev = ev->m_pNext;
		}
		else
		{
			if (prev != NULL)
			{
				prev->m_pNext = ev->m_pNext;
			}
			else
			{
				m_ActionList = ev->m_pNext;
			}

			CEventAction *next = ev->m_pNext;
			delete ev;
			ev = next;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Parameterless firing of an event
// Input  : pActivator - 
//			pCaller - 
//-----------------------------------------------------------------------------
void COutputEvent::FireOutput(CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	variant_t Val;
	Val.Set(FIELD_VOID, NULL);
	CBaseEntityOutput::FireOutput(Val, pActivator, pCaller, fDelay);
}


void CBaseEntityOutput::ParseEventAction(const char *EventData)
{
	AddEventAction(new CEventAction(EventData));
}

void CBaseEntityOutput::AddEventAction(CEventAction *pEventAction)
{
	pEventAction->m_pNext = m_ActionList;
	m_ActionList = pEventAction;
}

int CBaseEntityOutput::NumberOfElements(void)
{
	int count = 0;
	for (CEventAction *ev = m_ActionList; ev != NULL; ev = ev->m_pNext)
	{
		count++;
	}
	return count;
}

/// Delete every single action in the action list. 
void CBaseEntityOutput::DeleteAllElements(void)
{
	// walk front to back, deleting as we go. We needn't fix up pointers because
	// EVERYTHING will die.

	CEventAction *pNext = m_ActionList;
	// wipe out the head
	m_ActionList = NULL;
	while (pNext)
	{
		CEventAction *strikeThis = pNext;
		pNext = pNext->m_pNext;
		delete strikeThis;
	}

}

/// EVENTS save/restore parsing wrapper

class CEventsIODataOps : public IInputOutputOps
{
	virtual bool Parse(const InputOutputFieldInfo_t &fieldInfo, char const* szValue)
	{
		CBaseEntityOutput *ev = (CBaseEntityOutput*)fieldInfo.pField;
		ev->ParseEventAction(szValue);
		return true;
	}
};

static CEventsIODataOps g_EventsIODataOps;
IInputOutputOps *eventFuncs = &g_EventsIODataOps;

//-----------------------------------------------------------------------------
//			CMultiInputVar implementation
//
// Purpose: holds a list of inputs and their ID tags
//			used for entities that hold inputs from a set of other entities
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: destructor, frees the data list
//-----------------------------------------------------------------------------
CMultiInputVar::~CMultiInputVar()
{
	if (m_InputList)
	{
		while (m_InputList->next != NULL)
		{
			inputitem_t *input = m_InputList->next;
			m_InputList->next = input->next;
			delete input;
		}
		delete m_InputList;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the data set with a new value
// Input  : newVal - the new value to add to or update in the list
//			outputID - the source of the value
//-----------------------------------------------------------------------------
void CMultiInputVar::AddValue(variant_t newVal, int outputID)
{
	// see if it's already in the list
	inputitem_t *inp;
	for (inp = m_InputList; inp != NULL; inp = inp->next)
	{
		// already in list, so just update this link
		if (inp->outputID == outputID)
		{
			inp->value = newVal;
			return;
		}
	}

	// add to start of list
	inp = new inputitem_t;
	inp->value = newVal;
	inp->outputID = outputID;
	if (!m_InputList)
	{
		m_InputList = inp;
		inp->next = NULL;
	}
	else
	{
		inp->next = m_InputList;
		m_InputList = inp;
	}
}


#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// Purpose: allocates memory from the entitylist pool
//-----------------------------------------------------------------------------
void *CMultiInputVar::inputitem_t::operator new(size_t stAllocateBlock)
{
	return g_EntityListPool.Alloc(stAllocateBlock);
}

void *CMultiInputVar::inputitem_t::operator new(size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine)
{
	return g_EntityListPool.Alloc(stAllocateBlock);
}

//-----------------------------------------------------------------------------
// Purpose: frees memory from the entitylist pool
//-----------------------------------------------------------------------------
void CMultiInputVar::inputitem_t::operator delete(void *pMem)
{
	g_EntityListPool.Free(pMem);
}

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//			CEventQueue implementation
//
// Purpose: holds and executes a global prioritized queue of entity actions
//-----------------------------------------------------------------------------
DEFINE_FIXEDSIZE_ALLOCATOR(EventQueuePrioritizedEvent_t, 128, CUtlMemoryPool::GROW_SLOW);

CEventQueue g_EventQueue;

CEventQueue::CEventQueue()
{
	m_Events.m_flFireTime = -FLT_MAX;
	m_Events.m_pNext = NULL;

	Init();
}

CEventQueue::~CEventQueue()
{
	Clear();
}

void CEventQueue::Init(void)
{
	Clear();
}

void CEventQueue::Clear(void)
{
	// delete all the events in the queue
	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;

	while (pe != NULL)
	{
		EventQueuePrioritizedEvent_t *next = pe->m_pNext;
		delete pe;
		pe = next;
	}

	m_Events.m_pNext = NULL;
}

void CEventQueue::Dump(void)
{
	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;

	Msg("Dumping event queue. Current time is: %.2f\n",
		gpGlobals->curtime
		);

	while (pe != NULL)
	{
		EventQueuePrioritizedEvent_t *next = pe->m_pNext;

		Msg("   (%.2f) Target: '%s', Input: '%s', Parameter '%s'. Activator: '%s', Caller '%s'.  \n",
			pe->m_flFireTime,
			STRING(pe->m_iTarget),
			STRING(pe->m_iTargetInput),
			pe->m_VariantValue.String(),
			pe->m_pActivator ? pe->m_pActivator->GetDebugName() : "None",
			pe->m_pCaller ? pe->m_pCaller->GetDebugName() : "None");

		pe = next;
	}

	Msg("Finished dump.\n");
}


//-----------------------------------------------------------------------------
// Purpose: adds the action into the correct spot in the priority queue, targeting entity via string name
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent(const char *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID)
{
	// build the new event
	EventQueuePrioritizedEvent_t *newEvent = new EventQueuePrioritizedEvent_t;
	newEvent->m_flFireTime = gpGlobals->curtime + fireDelay;	// priority key in the priority queue
	newEvent->m_iTarget = MAKE_STRING(target);
	newEvent->m_pEntTarget = NULL;
	newEvent->m_iTargetInput = MAKE_STRING(targetInput);
	newEvent->m_pActivator = pActivator;
	newEvent->m_pCaller = pCaller;
	newEvent->m_VariantValue = Value;
	newEvent->m_iOutputID = outputID;

	AddEvent(newEvent);
}

//-----------------------------------------------------------------------------
// Purpose: adds the action into the correct spot in the priority queue, targeting entity via pointer
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent(CBaseEntity *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID)
{
	// build the new event
	EventQueuePrioritizedEvent_t *newEvent = new EventQueuePrioritizedEvent_t;
	newEvent->m_flFireTime = gpGlobals->curtime + fireDelay;	// primary priority key in the priority queue
	newEvent->m_iTarget = NULL_STRING;
	newEvent->m_pEntTarget = target;
	newEvent->m_iTargetInput = MAKE_STRING(targetInput);
	newEvent->m_pActivator = pActivator;
	newEvent->m_pCaller = pCaller;
	newEvent->m_VariantValue = Value;
	newEvent->m_iOutputID = outputID;

	AddEvent(newEvent);
}

void CEventQueue::AddEvent(CBaseEntity *target, const char *action, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID)
{
	variant_t Value;
	Value.Set(FIELD_VOID, NULL);
	AddEvent(target, action, Value, fireDelay, pActivator, pCaller, outputID);
}


//-----------------------------------------------------------------------------
// Purpose: private function, adds an event into the list
// Input  : *newEvent - the (already built) event to add
//-----------------------------------------------------------------------------
void CEventQueue::AddEvent(EventQueuePrioritizedEvent_t *newEvent)
{
	// loop through the actions looking for a place to insert
	EventQueuePrioritizedEvent_t *pe;
	for (pe = &m_Events; pe->m_pNext != NULL; pe = pe->m_pNext)
	{
		if (pe->m_pNext->m_flFireTime > newEvent->m_flFireTime)
		{
			break;
		}
	}

	Assert(pe);

	// insert
	newEvent->m_pNext = pe->m_pNext;
	newEvent->m_pPrev = pe;
	pe->m_pNext = newEvent;
	if (newEvent->m_pNext)
	{
		newEvent->m_pNext->m_pPrev = newEvent;
	}
}

void CEventQueue::RemoveEvent(EventQueuePrioritizedEvent_t *pe)
{
	Assert(pe->m_pPrev);
	pe->m_pPrev->m_pNext = pe->m_pNext;
	if (pe->m_pNext)
	{
		pe->m_pNext->m_pPrev = pe->m_pPrev;
	}
}


//-----------------------------------------------------------------------------
// Purpose: fires off any events in the queue who's fire time is (or before) the present time
//-----------------------------------------------------------------------------
void CEventQueue::ServiceEvents(void)
{
	if (!CBaseEntity::Debug_ShouldStep())
	{
		return;
	}

	EventQueuePrioritizedEvent_t *pe = m_Events.m_pNext;

	while (pe != NULL && pe->m_flFireTime <= gpGlobals->curtime)
	{
		MDLCACHE_CRITICAL_SECTION();

		bool targetFound = false;

		// find the targets
		if (pe->m_iTarget != NULL_STRING)
		{
			// In the context the event, the searching entity is also the caller
			CBaseEntity *pSearchingEntity = pe->m_pCaller;
			CBaseEntity *target = NULL;
			while (1)
			{
				target = gEntList.FindEntityByName(target, pe->m_iTarget, pSearchingEntity, pe->m_pActivator, pe->m_pCaller);
				if (!target)
					break;

				// pump the action into the target
				target->AcceptInput(STRING(pe->m_iTargetInput), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID);
				targetFound = true;
			}
		}

		// direct pointer
		if (pe->m_pEntTarget != NULL)
		{
			pe->m_pEntTarget->AcceptInput(STRING(pe->m_iTargetInput), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID);
			targetFound = true;
		}

		if (!targetFound)
		{
			// See if we can find a target if we treat the target as a classname
			if (pe->m_iTarget != NULL_STRING)
			{
				CBaseEntity *target = NULL;
				while (1)
				{
					target = gEntList.FindEntityByClassname(target, STRING(pe->m_iTarget));
					if (!target)
						break;

					// pump the action into the target
					target->AcceptInput(STRING(pe->m_iTargetInput), pe->m_pActivator, pe->m_pCaller, pe->m_VariantValue, pe->m_iOutputID);
					targetFound = true;
				}
			}
		}

		if (!targetFound)
		{
			const char *pClass = "", *pName = "";

			// might be NULL
			if (pe->m_pCaller)
			{
				pClass = STRING(pe->m_pCaller->m_iClassname);
				pName = STRING(pe->m_pCaller->GetEntityName());
			}

			char szBuffer[256];
			Q_snprintf(szBuffer, sizeof(szBuffer), "unhandled input: (%s) -> (%s), from (%s,%s); target entity not found\n", STRING(pe->m_iTargetInput), STRING(pe->m_iTarget), pClass, pName);
			DevMsg(2, "%s", szBuffer);
		}

		// remove the event from the list (remembering that the queue may have been added to)
		RemoveEvent(pe);
		delete pe;

		//
		// If we are in debug mode, exit the loop if we have fired the correct number of events.
		//
		if (CBaseEntity::Debug_IsPaused())
		{
			if (!CBaseEntity::Debug_Step())
			{
				break;
			}
		}

		// restart the list (to catch any new items have probably been added to the queue)
		pe = m_Events.m_pNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dumps the contents of the Entity I/O event queue to the console.
//-----------------------------------------------------------------------------
void CC_DumpEventQueue()
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	g_EventQueue.Dump();
}
static ConCommand dumpeventqueue("dumpeventqueue", CC_DumpEventQueue, "Dump the contents of the Entity I/O event queue to the console.");

//-----------------------------------------------------------------------------
// Purpose: Removes all pending events from the I/O queue that were added by the
//			given caller.
//
//			TODO: This is only as reliable as callers are in passing the correct
//				  caller pointer when they fire the outputs. Make more foolproof.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEvents(CBaseEntity *pCaller)
{
	if (!pCaller)
		return;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	while (pCur != NULL)
	{
		bool bDelete = false;
		if (pCur->m_pCaller == pCaller)
		{
			// Pointers match; make sure everything else matches.
			if (!stricmp(STRING(pCur->m_pCaller->GetEntityName()), STRING(pCaller->GetEntityName())) &&
				!stricmp(pCur->m_pCaller->GetClassname(), pCaller->GetClassname()))
			{
				// Found a matching event; delete it from the queue.
				bDelete = true;
			}
		}

		EventQueuePrioritizedEvent_t *pCurSave = pCur;
		pCur = pCur->m_pNext;

		if (bDelete)
		{
			RemoveEvent(pCurSave);
			delete pCurSave;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes all pending events of the specified type from the I/O queue of the specified target
//
//			TODO: This is only as reliable as callers are in passing the correct
//				  caller pointer when they fire the outputs. Make more foolproof.
//-----------------------------------------------------------------------------
void CEventQueue::CancelEventOn(CBaseEntity *pTarget, const char *sInputName)
{
	if (!pTarget)
		return;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	while (pCur != NULL)
	{
		bool bDelete = false;
		if (pCur->m_pEntTarget == pTarget)
		{
			if (!Q_strncmp(STRING(pCur->m_iTargetInput), sInputName, strlen(sInputName)))
			{
				// Found a matching event; delete it from the queue.
				bDelete = true;
			}
		}

		EventQueuePrioritizedEvent_t *pCurSave = pCur;
		pCur = pCur->m_pNext;

		if (bDelete)
		{
			RemoveEvent(pCurSave);
			delete pCurSave;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the target has any pending inputs.
// Input  : *pTarget - 
//			*sInputName - NULL for any input, or a specified one
//-----------------------------------------------------------------------------
bool CEventQueue::HasEventPending(CBaseEntity *pTarget, const char *sInputName)
{
	if (!pTarget)
		return false;

	EventQueuePrioritizedEvent_t *pCur = m_Events.m_pNext;

	while (pCur != NULL)
	{
		if (pCur->m_pEntTarget == pTarget)
		{
			if (!sInputName)
				return true;

			if (!Q_strncmp(STRING(pCur->m_iTargetInput), sInputName, strlen(sInputName)))
				return true;
		}

		pCur = pCur->m_pNext;
	}

	return false;
}

void ServiceEventQueue(void)
{
	VPROF("ServiceEventQueue()");

	g_EventQueue.ServiceEvents();
}

////////////////////////// variant_t implementation //////////////////////////

// BUGBUG: Add support for function pointer save/restore to variants
// BUGBUG: Must pass datamap_t to read/write fields 
void variant_t::Set(fieldtype_t ftype, void *data)
{
	fieldType = ftype;

	switch (ftype)
	{
	case FIELD_BOOLEAN:		bVal = *((bool *)data);				break;
	case FIELD_CHARACTER:	iVal = *((char *)data);				break;
	case FIELD_SHORT:		iVal = *((short *)data);			break;
	case FIELD_INTEGER:		iVal = *((int *)data);				break;
	case FIELD_STRING:		iszVal = *((string_t *)data);		break;
	case FIELD_FLOAT:		flVal = *((float *)data);			break;
	case FIELD_COLOR32:		rgbaVal = *((color32 *)data);		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		vecVal[0] = ((float *)data)[0];
		vecVal[1] = ((float *)data)[1];
		vecVal[2] = ((float *)data)[2];
		break;
	}

	case FIELD_EHANDLE:		eVal = *((EHANDLE *)data);			break;
	case FIELD_CLASSPTR:	eVal = *((CBaseEntity **)data);		break;
	case FIELD_VOID:
	default:
		iVal = 0; fieldType = FIELD_VOID;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Copies the value in the variant into a block of memory
// Input  : *data - the block to write into
//-----------------------------------------------------------------------------
void variant_t::SetOther(void *data)
{
	switch (fieldType)
	{
	case FIELD_BOOLEAN:		*((bool *)data) = bVal != 0;		break;
	case FIELD_CHARACTER:	*((char *)data) = iVal;				break;
	case FIELD_SHORT:		*((short *)data) = iVal;			break;
	case FIELD_INTEGER:		*((int *)data) = iVal;				break;
	case FIELD_STRING:		*((string_t *)data) = iszVal;		break;
	case FIELD_FLOAT:		*((float *)data) = flVal;			break;
	case FIELD_COLOR32:		*((color32 *)data) = rgbaVal;		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		((float *)data)[0] = vecVal[0];
		((float *)data)[1] = vecVal[1];
		((float *)data)[2] = vecVal[2];
		break;
	}

	case FIELD_EHANDLE:		*((EHANDLE *)data) = eVal;			break;
	case FIELD_CLASSPTR:	*((CBaseEntity **)data) = eVal;		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Converts the variant to a new type. This function defines which I/O
//			types can be automatically converted between. Connections that require
//			an unsupported conversion will cause an error message at runtime.
// Input  : newType - the type to convert to
// Output : Returns true on success, false if the conversion is not legal
//-----------------------------------------------------------------------------
bool variant_t::Convert(fieldtype_t newType)
{
	if (newType == fieldType)
	{
		return true;
	}

	//
	// Converting to a null value is easy.
	//
	if (newType == FIELD_VOID)
	{
		Set(FIELD_VOID, NULL);
		return true;
	}

	//
	// FIELD_INPUT accepts the variant type directly.
	//
	if (newType == FIELD_INPUT)
	{
		return true;
	}

	switch (fieldType)
	{
	case FIELD_INTEGER:
	{
		switch (newType)
		{
		case FIELD_FLOAT:
		{
			SetFloat((float)iVal);
			return true;
		}

		case FIELD_BOOLEAN:
		{
			SetBool(iVal != 0);
			return true;
		}
		}
		break;
	}

	case FIELD_FLOAT:
	{
		switch (newType)
		{
		case FIELD_INTEGER:
		{
			SetInt((int)flVal);
			return true;
		}

		case FIELD_BOOLEAN:
		{
			SetBool(flVal != 0);
			return true;
		}
		}
		break;
	}

	//
	// Everyone must convert from FIELD_STRING if possible, since
	// parameter overrides are always passed as strings.
	//
	case FIELD_STRING:
	{
		switch (newType)
		{
		case FIELD_INTEGER:
		{
			if (iszVal != NULL_STRING)
			{
				SetInt(atoi(STRING(iszVal)));
			}
			else
			{
				SetInt(0);
			}
			return true;
		}

		case FIELD_FLOAT:
		{
			if (iszVal != NULL_STRING)
			{
				SetFloat(atof(STRING(iszVal)));
			}
			else
			{
				SetFloat(0);
			}
			return true;
		}

		case FIELD_BOOLEAN:
		{
			if (iszVal != NULL_STRING)
			{
				SetBool(atoi(STRING(iszVal)) != 0);
			}
			else
			{
				SetBool(false);
			}
			return true;
		}

		case FIELD_VECTOR:
		{
			Vector tmpVec = vec3_origin;
			if (sscanf(STRING(iszVal), "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
			{
				// Try sucking out 3 floats with no []s
				sscanf(STRING(iszVal), "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
			}
			SetVector3D(tmpVec);
			return true;
		}

		case FIELD_COLOR32:
		{
			int nRed = 0;
			int nGreen = 0;
			int nBlue = 0;
			int nAlpha = 255;

			sscanf(STRING(iszVal), "%d %d %d %d", &nRed, &nGreen, &nBlue, &nAlpha);
			SetColor32(nRed, nGreen, nBlue, nAlpha);
			return true;
		}

		case FIELD_EHANDLE:
		{
			// convert the string to an entity by locating it by classname
			CBaseEntity *ent = NULL;
			if (iszVal != NULL_STRING)
			{
				// FIXME: do we need to pass an activator in here?
				ent = gEntList.FindEntityByName(NULL, iszVal);
			}
			SetEntity(ent);
			return true;
		}
		}

		break;
	}

	case FIELD_EHANDLE:
	{
		switch (newType)
		{
		case FIELD_STRING:
		{
			// take the entities targetname as the string
			string_t iszStr = NULL_STRING;
			if (eVal != NULL)
			{
				SetString(eVal->GetEntityName());
			}
			return true;
		}
		}
		break;
	}
	}

	// invalid conversion
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: All types must be able to display as strings for debugging purposes.
// Output : Returns a pointer to the string that represents this value.
//
//			NOTE: The returned pointer should not be stored by the caller as
//				  subsequent calls to this function will overwrite the contents
//				  of the buffer!
//-----------------------------------------------------------------------------
const char *variant_t::ToString(void) const
{
	COMPILE_TIME_ASSERT(sizeof(string_t) == sizeof(int));

	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
	{
		return(STRING(iszVal));
	}

	case FIELD_BOOLEAN:
	{
		if (bVal == 0)
		{
			Q_strncpy(szBuf, "false", sizeof(szBuf));
		}
		else
		{
			Q_strncpy(szBuf, "true", sizeof(szBuf));
		}
		return(szBuf);
	}

	case FIELD_INTEGER:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%i", iVal);
		return(szBuf);
	}

	case FIELD_FLOAT:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%g", flVal);
		return(szBuf);
	}

	case FIELD_COLOR32:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
		return(szBuf);
	}

	case FIELD_VECTOR:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
		return(szBuf);
	}

	case FIELD_VOID:
	{
		szBuf[0] = '\0';
		return(szBuf);
	}

	case FIELD_EHANDLE:
	{
		const char *pszName = (Entity()) ? STRING(Entity()->GetEntityName()) : "<<null entity>>";
		Q_strncpy(szBuf, pszName, 512);
		return (szBuf);
	}
	}

	return("No conversion to string");
}

/////////////////////// entitylist /////////////////////

CUtlMemoryPool g_EntListMemPool(sizeof(entitem_t), 256, CUtlMemoryPool::GROW_NONE, "g_EntListMemPool");

#include "tier0/memdbgoff.h"

void *entitem_t::operator new(size_t stAllocateBlock)
{
	return g_EntListMemPool.Alloc(stAllocateBlock);
}

void *entitem_t::operator new(size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine)
{
	return g_EntListMemPool.Alloc(stAllocateBlock);
}

void entitem_t::operator delete(void *pMem)
{
	g_EntListMemPool.Free(pMem);
}

#include "tier0/memdbgon.h"


CEntityList::CEntityList()
{
	m_pItemList = NULL;
	m_iNumItems = 0;
}

CEntityList::~CEntityList()
{
	// remove all items from the list
	entitem_t *next, *e = m_pItemList;
	while (e != NULL)
	{
		next = e->pNext;
		delete e;
		e = next;
	}
	m_pItemList = NULL;
}

void CEntityList::AddEntity(CBaseEntity *pEnt)
{
	// check if it's already in the list; if not, add it
	entitem_t *e = m_pItemList;
	while (e != NULL)
	{
		if (e->hEnt == pEnt)
		{
			// it's already in the list
			return;
		}

		if (e->pNext == NULL)
		{
			// we've hit the end of the list, so tack it on
			e->pNext = new entitem_t;
			e->pNext->hEnt = pEnt;
			e->pNext->pNext = NULL;
			m_iNumItems++;
			return;
		}

		e = e->pNext;
	}

	// empty list
	m_pItemList = new entitem_t;
	m_pItemList->hEnt = pEnt;
	m_pItemList->pNext = NULL;
	m_iNumItems = 1;
}

void CEntityList::DeleteEntity(CBaseEntity *pEnt)
{
	// find the entry in the list and delete it
	entitem_t *prev = NULL, *e = m_pItemList;
	while (e != NULL)
	{
		// delete the link if it's the matching entity OR if the link is NULL
		if (e->hEnt == pEnt || e->hEnt == NULL)
		{
			if (prev)
			{
				prev->pNext = e->pNext;
			}
			else
			{
				m_pItemList = e->pNext;
			}

			delete e;
			m_iNumItems--;

			// REVISIT: Is this correct?  Is this just here to clean out dead EHANDLEs?
			// restart the loop
			e = m_pItemList;
			prev = NULL;
			continue;
		}

		prev = e;
		e = e->pNext;
	}
}
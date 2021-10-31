//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Smart trigger, trigger once/multi with filtrating.
//
//========================================================================================//

#include "cbase.h"
#include "smart_trigger.h"
#include "hl2mp_player.h"
#include "player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// A trigger once & multiple in one with added filters&stuffing.
//-----------------------------------------------------------------------------

BEGIN_DATADESC(CSmartTrigger)

DEFINE_KEYFIELD(m_bTouchOnlyOnce, FIELD_BOOLEAN, "TriggerOnce"),
DEFINE_KEYFIELD(m_iExtraFilter, FIELD_INTEGER, "Filter"),

END_DATADESC()

LINK_ENTITY_TO_CLASS(smart_trigger, CSmartTrigger);

CSmartTrigger::CSmartTrigger(void)
{
	m_bTouchOnlyOnce = false;
	m_iExtraFilter = 0;
}

void CSmartTrigger::Spawn()
{
	AddSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS | SF_TRIGGER_ALLOW_NPCS);
	BaseClass::Spawn();
}

void CSmartTrigger::Touch(CBaseEntity *pOther)
{
	if (m_bDisabled || !IsAllowedToTrigger(pOther, m_iExtraFilter) || !PassesTriggerFilters(pOther))
		return;

	m_OnTouching.FireOutput(this, this);

	// Only once ? If so we delete the entity on fire.
	if (m_bTouchOnlyOnce)
		UTIL_Remove(this);
}

/*static*/ bool CSmartTrigger::IsAllowedToTrigger(CBaseEntity *pOther, int filter)
{
	if (pOther == NULL)
		return false;

	if (filter == 1)
	{
		if (!pOther->IsHuman())
			return false;
	}
	else if (filter == 2)
	{
		if (pOther->Classify() != CLASS_COMBINE)
			return false;
	}
	else if (filter == 3)
	{
		if (!pOther->IsHuman(true))
			return false;
	}
	else if (filter == 4)
	{
		if (!pOther->IsZombie())
			return false;
	}
	else if (filter == 5)
	{
		if (pOther->Classify() != CLASS_ZOMBIE)
			return false;
	}
	else if (filter == 6)
	{
		if (!pOther->IsZombie(true))
			return false;
	}

	return true;
}
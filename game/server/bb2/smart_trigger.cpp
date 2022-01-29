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

enum SmartTriggerFilter
{
	SMART_TRIGGER_FILTER_HUMAN_PLAYERS = 1,
	SMART_TRIGGER_FILTER_MILITARY_NPCS,
	SMART_TRIGGER_FILTER_HUMANS,
	SMART_TRIGGER_FILTER_ZOMBIE_PLAYERS,
	SMART_TRIGGER_FILTER_ZOMBIE_NPCS,
	SMART_TRIGGER_FILTER_ZOMBIES,
	SMART_TRIGGER_FILTER_HUMAN_PLAYERS_NOINFECTED,
};

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

	switch (filter)
	{
	case SMART_TRIGGER_FILTER_HUMAN_PLAYERS:
		return pOther->IsHuman();

	case SMART_TRIGGER_FILTER_MILITARY_NPCS:
		return ((pOther->Classify() == CLASS_COMBINE) || (pOther->Classify() == CLASS_MILITARY));

	case SMART_TRIGGER_FILTER_HUMANS:
		return pOther->IsHuman(true);

	case SMART_TRIGGER_FILTER_ZOMBIE_PLAYERS:
		return pOther->IsZombie();

	case SMART_TRIGGER_FILTER_ZOMBIE_NPCS:
		return (pOther->Classify() == CLASS_ZOMBIE);

	case SMART_TRIGGER_FILTER_ZOMBIES:
		return pOther->IsZombie(true);

	case SMART_TRIGGER_FILTER_HUMAN_PLAYERS_NOINFECTED:
		return (pOther->IsHuman() && (pOther->Classify() != CLASS_PLAYER_INFECTED));
	}

	return true;
}
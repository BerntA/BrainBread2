//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Smart trigger, trigger once/multi with filtrating.
//
//========================================================================================//

#include "cbase.h"
#include "smart_trigger.h"
#include "triggers.h"
#include "hl2mp_player.h"
#include "player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// A trigger once & multiple in one with added filters&stuffing.
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CSmartTrigger ) 

	DEFINE_KEYFIELD( TouchOnlyOnce, FIELD_BOOLEAN, "TriggerOnce" ),
	DEFINE_KEYFIELD( m_iFilter, FIELD_INTEGER, "Filter" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS(smart_trigger, CSmartTrigger);

CSmartTrigger::CSmartTrigger( void )
{
	m_bDisabled = false;
	TouchOnlyOnce = false;
	m_iFilter = 0;
}

void CSmartTrigger::Spawn()
{
	BaseClass::Spawn();
}

void CSmartTrigger::Touch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
		return;

	if ( m_iFilter == 1 )
	{
		if ( !pOther->IsHuman() )
			return;
	}
	else if ( m_iFilter == 2 )
	{
		if ( pOther->Classify() != CLASS_COMBINE )
			return;
	}
	else if ( m_iFilter == 3 )
	{
		if ( !pOther->IsHuman(true) )
			return;
	}
	else if ( m_iFilter == 4 )
	{
		if ( !pOther->IsZombie() )
			return;
	}
	else if ( m_iFilter == 5 )
	{
		if ( pOther->Classify() != CLASS_ZOMBIE )
			return;
	}
	else if ( m_iFilter == 6 )
	{
		if ( !pOther->IsZombie(true) )
			return;
	}

	m_OnTouching.FireOutput(this, this);

	// Only once ? If so we delete the entity on fire.
	if ( TouchOnlyOnce )
		UTIL_Remove( this );
}
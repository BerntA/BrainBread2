//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Smart trigger, trigger once/multi with filtrating.
//
//========================================================================================//

#ifndef TRIGGER_SMART_H
#define TRIGGER_SMART_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "baseentity.h"
#include "triggers.h"
#include "props.h"
#include "player.h"
#include "saverestore_utlvector.h"
#include "GameEventListener.h"

class CSmartTrigger : public CTriggerMultiple
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CSmartTrigger, CTriggerMultiple);

	CSmartTrigger( void );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );

	// Hammer Variables...
	int m_iFilter;
	bool TouchOnlyOnce;
};

#endif // TRIGGER_SMART_H
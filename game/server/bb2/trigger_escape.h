//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Throws the player over to the spec team and prints out a message. In classic the human team will win with at least one human extractor.
//
//========================================================================================//

#ifndef TRIGGER_ESCAPE_H
#define TRIGGER_ESCAPE_H
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

class CTriggerEscape : public CTriggerMultiple
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerEscape, CTriggerMultiple );

	CTriggerEscape();
	virtual ~CTriggerEscape();

	void Spawn();
	void Touch( CBaseEntity *pOther );

	COutputEvent m_OnAddedPlayer;

	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
};

#endif // TRIGGER_ESCAPE_H
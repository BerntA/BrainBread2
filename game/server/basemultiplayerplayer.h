//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#ifndef BASEMULTIPLAYERPLAYER_H
#define BASEMULTIPLAYERPLAYER_H
#pragma once

#include "player.h"
#include "ai_speech.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

class CBaseMultiplayerPlayer : public CBasePlayer
{
	DECLARE_CLASS(CBaseMultiplayerPlayer, CBasePlayer);

public:
	CBaseMultiplayerPlayer();
	virtual ~CBaseMultiplayerPlayer();

	virtual bool CanSpeak(void) { return true; }
};

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CBaseMultiplayerPlayer *ToBaseMultiplayerPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

#if _DEBUG
	return dynamic_cast<CBaseMultiplayerPlayer *>( pEntity );
#else
	return static_cast<CBaseMultiplayerPlayer *>(pEntity);
#endif
}

#endif	// BASEMULTIPLAYERPLAYER_H
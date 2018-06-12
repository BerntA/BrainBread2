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

	virtual bool		CanHearAndReadChatFrom(CBasePlayer *pPlayer);
	virtual bool		CanSpeak(void) { return true; }
	virtual void		Precache(void)
	{
		BaseClass::Precache();
	}

	virtual bool ClientCommand(const CCommand &args);
	virtual void OnAchievementEarned(int iAchievement) {}

	enum
	{
		CHAT_IGNORE_NONE = 0,
		CHAT_IGNORE_ALL,
		CHAT_IGNORE_TEAM,
	};

	virtual float GetConnectionTime(void) { return m_flConnectionTime; }

	// Command rate limiting.
	bool ShouldRunRateLimitedCommand(const CCommand &args);
	bool ShouldRunRateLimitedCommand( const char *pszCommand );

private:
	int m_iIgnoreGlobalChat;
	float m_flConnectionTime;

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float, int>	m_RateLimitLastCommandTimes;
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
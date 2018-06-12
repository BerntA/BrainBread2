//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "mp_shareddefs.h"
#include "basemultiplayerplayer.h"

// Minimum interval between rate-limited commands that players can run.
#define COMMAND_MAX_RATE  0.3

CBaseMultiplayerPlayer::CBaseMultiplayerPlayer()
{
	m_flConnectionTime = gpGlobals->curtime;
}

CBaseMultiplayerPlayer::~CBaseMultiplayerPlayer()
{
}

bool CBaseMultiplayerPlayer::CanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
	// can always hear the console unless we're ignoring all chat
	if (!pPlayer)
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if (m_iIgnoreGlobalChat == CHAT_IGNORE_ALL)
		return false;

	// check if we're ignoring all but teammates
	if (m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship(this, pPlayer) != GR_TEAMMATE)
		return false;

	// can't hear dead players if we're alive
	if (pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE)
		return false;

	return true;
}

bool CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand(const CCommand &args)
{
	return ShouldRunRateLimitedCommand(args[0]);
}

bool CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand(const char *pszCommand)
{
	const char *pcmd = pszCommand;

	int i = m_RateLimitLastCommandTimes.Find(pcmd);
	if (i == m_RateLimitLastCommandTimes.InvalidIndex())
	{
		m_RateLimitLastCommandTimes.Insert(pcmd, gpGlobals->curtime);
		return true;
	}
	else if ((gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < COMMAND_MAX_RATE)
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

bool CBaseMultiplayerPlayer::ClientCommand(const CCommand &args)
{
	const char *pcmd = args[0];

	if (FStrEq(pcmd, "ignoremsg"))
	{
		if (ShouldRunRateLimitedCommand(args))
		{
			m_iIgnoreGlobalChat = (m_iIgnoreGlobalChat + 1) % 3;
			switch (m_iIgnoreGlobalChat)
			{
			case CHAT_IGNORE_NONE:
				ClientPrint(this, HUD_PRINTTALK, "#Accept_All_Messages");
				break;
			case CHAT_IGNORE_ALL:
				ClientPrint(this, HUD_PRINTTALK, "#Ignore_Broadcast_Messages");
				break;
			case CHAT_IGNORE_TEAM:
				ClientPrint(this, HUD_PRINTTALK, "#Ignore_Broadcast_Team_Messages");
				break;
			default:
				break;
			}
		}
		return true;
	}

	return BaseClass::ClientCommand(args);
}
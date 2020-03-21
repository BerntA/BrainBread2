//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMPLAY_GAMERULES_H
#define TEAMPLAY_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "multiplay_gamerules.h"

#ifdef CLIENT_DLL
	#define CTeamplayRules C_TeamplayRules
#else
	#include "takedamageinfo.h"
#endif

#define MAX_TEAMS 5

class CTeamplayRules : public CMultiplayRules
{
public:
	DECLARE_CLASS( CTeamplayRules, CMultiplayRules );

#ifdef CLIENT_DLL

#else

	CTeamplayRules();
	virtual ~CTeamplayRules() { }

	virtual void Precache( void );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual bool IsTeamplay( void );
	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );	
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void DeathNotice(CBaseEntity *pVictim, const CTakeDamageInfo &info);
	virtual const char *GetGameDescription( void ) { return "BrainBread 2"; }  // this is the game name that gets seen in the server browser
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void Think ( void );
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib );
	virtual void ClientDisconnected( edict_t *pClient );
#endif
};

inline CTeamplayRules* TeamplayGameRules()
{
	return static_cast<CTeamplayRules*>(g_pGameRules);
}

#endif // TEAMPLAY_GAMERULES_H

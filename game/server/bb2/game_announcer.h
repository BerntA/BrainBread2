//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Deathmatch Announcer - Not really an 'entity'.
//
//========================================================================================//

#ifndef GAME_ANNOUNCER_H
#define GAME_ANNOUNCER_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "baseentity.h"

class CGameAnnouncer;
class CGameAnnouncer
{
public:
	void Reset(void);
	void RemoveItemsForPlayer(int index);
	bool HandleClientAnnouncerSound(const CTakeDamageInfo &info, CBaseEntity *pVictim, CBaseEntity *pKiller);
	void DeathNotice(const CTakeDamageInfo &info, CBaseEntity *pVictim, CBaseEntity *pKiller);

private:
	bool m_bFirstBlood;
	float m_flIsBusy;
};

extern CGameAnnouncer *GameAnnouncer;

#endif // GAME_ANNOUNCER_H
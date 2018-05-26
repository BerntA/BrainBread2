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

#include "baseentity.h"

class CGameAnnouncer;
class CGameAnnouncer
{
public:
	void Reset(void);
	void RemoveItemsForPlayer(int index);
	bool HandleClientAnnouncerSound(const CTakeDamageInfo &info, CBaseEntity *pVictim, CBaseEntity *pKiller);
	void DeathNotice(const CTakeDamageInfo &info, CBaseEntity *pVictim, CBaseEntity *pKiller);
	void Think(int timeleft);

private:
	bool m_bFirstBlood;
	float m_flIsBusy;
	int m_iLastTime;
};

extern CGameAnnouncer *GameAnnouncer;

#endif // GAME_ANNOUNCER_H
//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: Player Specific Achiev. Statistics.
//
//========================================================================================//

#ifndef PLAYER_ACHIEV_STATS_H
#define PLAYER_ACHIEV_STATS_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "takedamageinfo.h"
#include "GameBase_Shared.h"

class CHL2MP_Player;
class CPlayerAchievStats
{
public:
	CPlayerAchievStats(CHL2MP_Player *pOuter);
	~CPlayerAchievStats();

	bool IsValid(void);
	void OnSpawned(void);
	void OnDeath(const CTakeDamageInfo &info);
	void OnKilled(CBaseEntity *pVictim, CBaseEntity *pInflictor, const CTakeDamageInfo &info, int hitgroup);
	void OnTookDamage(const CTakeDamageInfo &info);
	void OnDidDamage(const CTakeDamageInfo &info);
	void OnPickupItem(const DataInventoryItem_Base_t *item);

private:
	CHL2MP_Player *m_pOuter;
	CPlayerAchievStats(const CPlayerAchievStats &);

protected:
	int m_iHealthKitsUsed;
	int m_iZombieKicks, m_iZombiePunches, m_iZombieUppercuts;
	int m_iTotalHeadshots, m_iZombiePlayerHeadshots;
};

#endif // PLAYER_ACHIEV_STATS_H
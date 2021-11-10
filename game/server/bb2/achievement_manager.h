//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Achievement & Stat Handler
//
//========================================================================================//

#ifndef SHARED_ACHIEVEMENT_HANDLER_H
#define SHARED_ACHIEVEMENT_HANDLER_H

#ifdef _WIN32
#pragma once
#endif

#include <steam/steam_api.h>
#include "gameinterface.h"
#include "hl2mp_player.h"
#include "achievement_shareddefs.h"

namespace AchievementManager
{
	void AnnounceAchievement(int plIndex, const char *pcAchievement, int iAchievementType = 0);
	bool WriteToAchievement(CHL2MP_Player *pPlayer, const char *szAchievement, int iAchievementType = 0);
	bool WriteToStat(CHL2MP_Player *pPlayer, const char *szStat, int iForceValue = 0, bool bAddTo = false);
	bool WriteToStatPvP(CHL2MP_Player *pPlayer, const char *szStat);
	bool IsGlobalStatsAllowed(void);
	bool CanLoadSteamStats(CHL2MP_Player *pPlayer);
	bool CanWrite(CHL2MP_Player *pClient, const char *szParam, int iAchievementType = 0);
	int GetMaxValueForStat(const char *szStat);
}

extern const char *pszGameSkills[40]; // Skill stat definition array.

#endif // SHARED_ACHIEVEMENT_HANDLER_H
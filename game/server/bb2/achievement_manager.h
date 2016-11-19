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

class CAchievementManager
{
public:
	CAchievementManager();
	virtual ~CAchievementManager();

	void AnnounceAchievement(int plIndex, const char *pcAchievement, int iAchievementType = 0);
	bool WriteToAchievement(CHL2MP_Player *pPlayer, const char *szAchievement, int iAchievementType = 0);
	bool WriteToStat(CHL2MP_Player *pPlayer, const char *szStat, int iForceValue = 0);
	bool SaveGlobalStats(CHL2MP_Player *pPlayer);
	bool LoadGlobalStats(CHL2MP_Player *pPlayer);
	int GetMaxValueForStat(const char *szStat);
	int GetIndexOfAchievement(const char *pcAchievement);

private:

	bool CanSetupProfile(void);
	bool CanWrite(CHL2MP_Player *pClient, const char *param = NULL, bool bIsStat = false);
	bool CanWriteToType(const char *param, int iType);
};

extern const char *pszGameSkills[40]; // Skill stat definition array.

#endif // SHARED_ACHIEVEMENT_HANDLER_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Achievement & Stat Handler
//
//========================================================================================//

#include "cbase.h"
#include "achievement_manager.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"

static globalStatItem_t szGameStats[] =
{
	{ "BBX_ST_XP_CURRENT", 0 },
	{ "BBX_ST_XP_LEFT", 65 },
	{ "BBX_ST_LEVEL", 1 },
	{ "BBX_ST_TALENTS", 0 },
	{ "BBX_ST_ZM_POINTS", 0 },
};

const char *pszGameSkills[40] =
{
	"BBX_ST_AGI_SPEED",
	"BBX_ST_AGI_ACROBATICS",
	"BBX_ST_AGI_SLIDE",
	"BBX_ST_AGI_SNIPER_MASTER",
	"BBX_ST_AGI_ENHANCED_REFLEXES",
	"BBX_ST_AGI_MELEE_SPEED",
	"BBX_ST_AGI_LIGHTWEIGHT",
	"BBX_ST_AGI_WEIGHTLESS",
	"BBX_ST_AGI_HEALTH_REGEN",
	"BBX_ST_AGI_REALITY_PHASE",

	"BBX_ST_STR_HEALTH",
	"BBX_ST_STR_IMPENETRABLE",
	"BBX_ST_STR_PAINKILLER",
	"BBX_ST_STR_LIFE_LEECH",
	"BBX_ST_STR_POWER_KICK",
	"BBX_ST_STR_BLEED",
	"BBX_ST_STR_CRIPPLING_BLOW",
	"BBX_ST_STR_ARMOR_MASTER",
	"BBX_ST_STR_MELEE_MASTER",
	"BBX_ST_STR_BLOOD_RAGE",

	"BBX_ST_PRO_RIFLE_MASTER",
	"BBX_ST_PRO_SHOTGUN_MASTER",
	"BBX_ST_PRO_PISTOL_MASTER",
	"BBX_ST_PRO_RESOURCEFUL",
	"BBX_ST_PRO_BLAZING_AMMO",
	"BBX_ST_PRO_COLDSNAP",
	"BBX_ST_PRO_SHOUT_AND_SPRAY",
	"BBX_ST_PRO_EMPOWERED_BULLETS",
	"BBX_ST_PRO_MAGAZINE_REFILL",
	"BBX_ST_PRO_GUNSLINGER",

	"BBX_ST_ZM_HEALTH",
	"BBX_ST_ZM_DAMAGE",
	"BBX_ST_ZM_DAMAGE_REDUCTION",
	"BBX_ST_ZM_SPEED",
	"BBX_ST_ZM_JUMP",
	"BBX_ST_ZM_LEAP",
	"BBX_ST_ZM_DEATH",
	"BBX_ST_ZM_LIFE_LEECH",
	"BBX_ST_ZM_HEALTH_REGEN",
	"BBX_ST_ZM_MASS_INVASION",
};

void AchievementManager::AnnounceAchievement(int plIndex, const char *pcAchievement, int iAchievementType)
{
	IGameEvent *event = gameeventmanager->CreateEvent("game_achievement");
	if (event)
	{
		event->SetString("ach_str", pcAchievement);
		event->SetInt("index", plIndex);
		event->SetInt("type", iAchievementType);
		gameeventmanager->FireEvent(event);
	}
}

// Set an achievement here.
bool AchievementManager::WriteToAchievement(CHL2MP_Player *pPlayer, const char *szAchievement, int iAchievementType)
{
	if (!CanWrite(pPlayer, szAchievement, iAchievementType))
	{
		DevMsg("Failed to write to the achievement %s!\n", szAchievement);
		return false;
	}

	CSteamID pSteamClient;
	if (!pPlayer->GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %i\n", pPlayer->GetUserID());
		return false;
	}

	bool bAchieved = false;
	steamgameserverapicontext->SteamGameServerStats()->GetUserAchievement(pSteamClient, szAchievement, &bAchieved);
	if (bAchieved)
		return false;

	// Do the change.
	if (steamgameserverapicontext->SteamGameServerStats()->SetUserAchievement(pSteamClient, szAchievement))
	{
		// Store the change.
		steamgameserverapicontext->SteamGameServerStats()->StoreUserStats(pSteamClient);

		// Send Event:
		AnnounceAchievement(pPlayer->entindex(), szAchievement, iAchievementType);

		// Give Some Reward:
		const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(szAchievement);
		if (pAchiev && pAchiev->reward)
			pPlayer->CanLevelUp(pAchiev->reward, NULL);

		return true;
	}

	return false;
}

// Write to a stat
bool AchievementManager::WriteToStat(CHL2MP_Player *pPlayer, const char *szStat, int iForceValue, bool bAddTo)
{
	if (!CanWrite(pPlayer, szStat))
	{
		DevMsg("Failed to write to the stat %s!\n", szStat);
		return false;
	}

	CSteamID pSteamClient;
	if (!pPlayer->GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %i\n", pPlayer->GetUserID());
		return false;
	}

	int iCurrentValue = 0, iMaxValue = GetMaxValueForStat(szStat);
	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pSteamClient, szStat, &iCurrentValue);

	// Make sure we're not above the max value!
	if (iMaxValue <= iCurrentValue)
		return false;

	if (iForceValue > 0)
	{
		if (bAddTo)
			iCurrentValue += iForceValue;
		else
			iCurrentValue = iForceValue;
	}
	else
		iCurrentValue++;

	// Give us achievements if the stats related to certain achievs have been surpassed.
	for (int i = 0; i < ACHIEVEMENTS::GetNumAchievements(); i++)
	{
		const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(i);
		if (pAchiev && (pAchiev->value <= iCurrentValue) && pAchiev->szAchievement && pAchiev->szAchievement[0] && !strcmp(szStat, pAchiev->szStat))
			WriteToAchievement(pPlayer, pAchiev->szAchievement);
	}

	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, szStat, iCurrentValue);
	steamgameserverapicontext->SteamGameServerStats()->StoreUserStats(pSteamClient);
	return true;
}

bool AchievementManager::WriteToStatPvP(CHL2MP_Player *pPlayer, const char *szStat)
{
	// Make sure we have loaded stats, have PvP mode on, and have a server context!
	if (
		!pPlayer || !pPlayer->HasLoadedStats() || pPlayer->IsBot() || !GameBaseServer()->CanEditSteamStats() ||
		!steamgameserverapicontext || !steamgameserverapicontext->SteamGameServerStats() ||
		!HL2MPRules() || !HL2MPRules()->IsFastPacedGameplay()
		)
		return false;

	CSteamID pSteamClient;
	if (!pPlayer->GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %i\n", pPlayer->GetUserID());
		return false;
	}

	int iCurrentValue;
	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pSteamClient, szStat, &iCurrentValue);
	iCurrentValue++;
	iCurrentValue = clamp(iCurrentValue, 0, 9999999);

	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, szStat, iCurrentValue);
	steamgameserverapicontext->SteamGameServerStats()->StoreUserStats(pSteamClient);
	return true;
}

bool AchievementManager::IsGlobalStatsAllowed(void)
{
	// Make sure that our interfaces have been locked and loaded.
	// Are we using stats, and has cheats NOT been on?
	return (steamgameserverapicontext && steamgameserverapicontext->SteamGameServerStats() && GameBaseServer() && (GameBaseServer()->CanStoreSkills() == PROFILE_GLOBAL));
}

bool AchievementManager::CanLoadSteamStats(CHL2MP_Player *pPlayer)
{
	if (!engine->IsDedicatedServer() || !pPlayer || pPlayer->m_bHasReadProfileData || pPlayer->IsBot() || !steamgameserverapicontext || !steamgameserverapicontext->SteamGameServerStats())
		return false;

	CSteamID pSteamClient;
	if (!pPlayer->GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %s\n", pPlayer->GetPlayerName());
		return false;
	}

	SteamAPICall_t apiCall = steamgameserverapicontext->SteamGameServerStats()->RequestUserStats(pSteamClient);
	pPlayer->m_SteamCallResultRequestPlayerStats.Set(apiCall, pPlayer, &CHL2MP_Player::OnReceivedSteamStats);
	return true;
}

// Can we write to this achiev/stat?
bool AchievementManager::CanWrite(CHL2MP_Player *pClient, const char *szParam, int iAchievementType)
{
	if (!pClient || !pClient->m_bHasReadProfileData || pClient->IsBot() || !IsGlobalStatsAllowed() || !szParam || !szParam[0])
		return false;

	if (iAchievementType == ACHIEVEMENT_TYPE_MAP)
	{
		for (int i = 0; i < ACHIEVEMENTS::GetNumAchievements(); i++)
		{
			const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(i);
			if (pAchiev && (iAchievementType == pAchiev->type) && !strcmp(pAchiev->szAchievement, szParam))
				return true;
		}

		return false;
	}

	return true;
}

// Get the highest value for this stat. (if it is defined for multiple achievements it will have different max values)
int AchievementManager::GetMaxValueForStat(const char *szStat)
{
	int iValue = 0;
	for (int i = 0; i < ACHIEVEMENTS::GetNumAchievements(); i++)
	{
		const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(i);
		if (pAchiev && (iValue < pAchiev->value) && !strcmp(szStat, pAchiev->szStat))
			iValue = pAchiev->value;
	}
	return iValue;
}

CON_COMMAND(dev_reset_stats, "Reset Stats")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pPlayer)
		return;

	if (!pPlayer->IsGroupIDFlagActive(GROUPID_IS_DEVELOPER) && !pPlayer->IsGroupIDFlagActive(GROUPID_IS_TESTER))
	{
		ClientPrint(pPlayer, HUD_PRINTCONSOLE, "You must be a developer or tester to use this command!\n");
		return;
	}

	if (GameBaseServer()->CanStoreSkills() != PROFILE_GLOBAL)
	{
		ClientPrint(pPlayer, HUD_PRINTCONSOLE, "This function can only be used on servers which allow global saving!\n");
		return;
	}

	CSteamID pSteamClient;
	if (!pPlayer->GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %i\n", pPlayer->GetUserID());
		return;
	}

	// BB2 SKILL TREE - Base
	int iLevel = clamp(GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iLevel, 1, MAX_PLAYER_LEVEL);
	pPlayer->SetPlayerLevel(iLevel);
	pPlayer->m_BB2Local.m_iSkill_Talents = GameBaseShared()->GetSharedGameDetails()->CalculatePointsForLevel(iLevel);
	pPlayer->m_BB2Local.m_iSkill_XPLeft = (GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iXPIncreasePerLevel * iLevel);
	pPlayer->m_BB2Local.m_iSkill_XPCurrent = 0;
	pPlayer->m_BB2Local.m_iZombieCredits = 0;

	for (int i = 0; i < MAX_SKILL_ARRAY; i++)
		pPlayer->m_BB2Local.m_iPlayerSkills.Set(i, 0);

	for (int i = 0; i < ACHIEVEMENTS::GetNumAchievements(); i++)
	{
		const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(i);
		if (pAchiev && pAchiev->szStat && pAchiev->szStat[0])
			steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, pAchiev->szStat, 0);
	}

	for (int i = 0; i < _ARRAYSIZE(szGameStats); i++)
		steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, szGameStats[i].szStat, szGameStats[i].defaultValue);

	for (int i = 0; i < _ARRAYSIZE(pszGameSkills); i++)
		steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, pszGameSkills[i], 0);

	steamgameserverapicontext->SteamGameServerStats()->StoreUserStats(pSteamClient);

	ClientPrint(pPlayer, HUD_PRINTCONSOLE, "You've reset all your stats!\n");
};

CON_COMMAND(bb2_reset_skills, "Allows a player to reset his/her human skills back to default, will not reset the level or XP, points will be restored!")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pPlayer || ((pPlayer->LastTimePlayerTalked() + 1.0f) >= gpGlobals->curtime))
		return;

	pPlayer->NotePlayerTalked();

	if (GameBaseServer()->CanStoreSkills() <= PROFILE_NONE)
	{
		ClientPrint(pPlayer, HUD_PRINTCONSOLE, "This function can only be used on servers which allow stat saving!\n");
		return;
	}

	pPlayer->m_BB2Local.m_iSkill_Talents = GameBaseShared()->GetSharedGameDetails()->CalculatePointsForLevel(pPlayer->GetPlayerLevel());
	for (int i = 0; i < PLAYER_SKILL_ZOMBIE_HEALTH; i++)
		pPlayer->m_BB2Local.m_iPlayerSkills.Set(i, 0);

	ClientPrint(pPlayer, HUD_PRINTCONSOLE, "You've reset all your skills!\n");
};
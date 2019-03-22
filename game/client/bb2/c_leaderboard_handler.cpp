//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles leaderboard information, fetches leaderboard info and such.
//
//========================================================================================//

#include "cbase.h"
#include "c_leaderboard_handler.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLeaderboardHandler::CLeaderboardHandler()
{
	m_hGlobalLeaderboardHandle = NULL;
	m_bIsLoading = false;
	Reset();
}

CLeaderboardHandler::~CLeaderboardHandler()
{
}

void CLeaderboardHandler::Reset(void)
{
	m_flTimeToUpload = 0.0f;
	FetchLeaderboardHandle();
}

void CLeaderboardHandler::UploadLeaderboardStats(bool bDelay)
{
	if (bDelay)
	{
		m_flTimeToUpload = engine->Time() + 0.15f;
		return;
	}

	if (m_hGlobalLeaderboardHandle != NULL)
	{
		int32 iLevel = 0, iKills = 0, iDeaths = 0, iScore = 0;
		steamapicontext->SteamUserStats()->GetStat("BBX_ST_LEVEL", &iLevel);
		steamapicontext->SteamUserStats()->GetStat("BBX_ST_KILLS", &iKills);
		steamapicontext->SteamUserStats()->GetStat("BBX_ST_DEATHS", &iDeaths);

		iScore = (iLevel + iKills) - iDeaths;
		if (iScore < 0)
			iScore = 0;

		int32 details[] =
		{
			iLevel,
			iKills,
			iDeaths,
		};

		steamapicontext->SteamUserStats()->UploadLeaderboardScore(m_hGlobalLeaderboardHandle, k_ELeaderboardUploadScoreMethodForceUpdate, iScore, details, _ARRAYSIZE(details));
	}
}

void CLeaderboardHandler::OnUpdate(void)
{
	if ((m_flTimeToUpload > 0.0f) && (engine->Time() >= m_flTimeToUpload))
	{
		UploadLeaderboardStats();
		m_flTimeToUpload = 0.0f;
	}
}

void CLeaderboardHandler::FetchLeaderboardHandle(void)
{
	m_hGlobalLeaderboardHandle = NULL;

	if (!steamapicontext || (steamapicontext && !steamapicontext->SteamUserStats()))
		return;

	SteamAPICall_t hSteamAPICall = NULL;
	hSteamAPICall = steamapicontext->SteamUserStats()->FindLeaderboard("Global");

	if (hSteamAPICall != NULL)
		m_SteamCallResultFindLeaderboard.Set(hSteamAPICall, this, &CLeaderboardHandler::OnFindLeaderboard);
}

void CLeaderboardHandler::FetchLeaderboardResults(int iOffset)
{
	if (m_bIsLoading || (m_hGlobalLeaderboardHandle == NULL))
		return;

	m_bIsLoading = true;

	SteamAPICall_t hSteamAPICall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries(m_hGlobalLeaderboardHandle, k_ELeaderboardDataRequestGlobal, 1 + iOffset, MAX_LEADERBOARD_ENTRIES + iOffset);
	m_callResultDownloadEntries.Set(hSteamAPICall, this, &CLeaderboardHandler::OnLeaderboardDownloadedEntries);
}

void CLeaderboardHandler::OnFindLeaderboard(LeaderboardFindResult_t *pFindLeaderboardResult, bool bIOFailure)
{
	// See if we encountered an error during the call
	if (!pFindLeaderboardResult->m_bLeaderboardFound || bIOFailure)
		return;

	// Only allow the Global scores.
	const char *pchName = steamapicontext->SteamUserStats()->GetLeaderboardName(pFindLeaderboardResult->m_hSteamLeaderboard);
	if (strcmp(pchName, "Global"))
		return;

	m_hGlobalLeaderboardHandle = pFindLeaderboardResult->m_hSteamLeaderboard;
}

void CLeaderboardHandler::OnLeaderboardDownloadedEntries(LeaderboardScoresDownloaded_t *pLeaderboardScoresDownloaded, bool bIOFailure)
{
	int iEntries = pLeaderboardScoresDownloaded->m_cEntryCount;
	int m_iItemIndex = 0;

	for (int index = 0; index < iEntries; index++)
	{
		LeaderboardEntry_t pEntry;
		int32 details[3];
		if (steamapicontext->SteamUserStats()->GetDownloadedLeaderboardEntry(pLeaderboardScoresDownloaded->m_hSteamLeaderboardEntries, index, &pEntry, details, _ARRAYSIZE(details)))
		{
			CSteamID pSteamID = pEntry.m_steamIDUser;

			char pszPlayerName[32];
			char pszSteamID[80];
			const char *playerName = steamapicontext->SteamFriends()->GetFriendPersonaName(pSteamID);
			if (!(playerName && playerName[0]))
				playerName = "Unnamed";

			Q_strncpy(pszPlayerName, playerName, 32);
			Q_snprintf(pszSteamID, 80, "%llu", pSteamID.ConvertToUint64());

			GameBaseClient->AddScoreboardItem(pszSteamID, pszPlayerName, details[0], details[1], details[2], m_iItemIndex);
			m_iItemIndex++;
		}
	}

	m_bIsLoading = false;
	GameBaseClient->ScoreboardRefreshComplete(steamapicontext->SteamUserStats()->GetLeaderboardEntryCount(m_hGlobalLeaderboardHandle));
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles leaderboard information, fetches leaderboard info and such.
//
//========================================================================================//

#include "cbase.h"
#include "c_leaderboard_handler.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CUtlVector<CLeaderboardHandler*> g_pLeaderboards;

CLeaderboardHandler::CLeaderboardHandler(const char *leaderboardName)
{
	g_pLeaderboards.AddToTail(this);
	Q_strncpy(m_pchLeaderboardName, leaderboardName, sizeof(m_pchLeaderboardName));
	m_hLeaderboardHandle = NULL;
	m_bIsLoading = false;
	OnReset();
}

CLeaderboardHandler::~CLeaderboardHandler()
{
	g_pLeaderboards.FindAndRemove(this);
}

void CLeaderboardHandler::OnReset(void)
{
	m_flTimeToUpload = 0.0f;
	FetchLeaderboardHandle();
}

void CLeaderboardHandler::OnUpdate(void)
{
	if ((m_flTimeToUpload > 0.0f) && (engine->Time() >= m_flTimeToUpload) && (m_hLeaderboardHandle != NULL))
	{
		int32 iArg1 = 0, iArg2 = 0, iArg3 = 0;
		GetLeaderboardStats(iArg1, iArg2, iArg3);
		int32 details[] = { iArg1, iArg2, iArg3, };
		int32 iScore = (iArg1 + iArg2) - iArg3;
		steamapicontext->SteamUserStats()->UploadLeaderboardScore(m_hLeaderboardHandle, k_ELeaderboardUploadScoreMethodForceUpdate, MAX(iScore, 0), details, _ARRAYSIZE(details));
		m_flTimeToUpload = 0.0f;
	}
}

void CLeaderboardHandler::FetchLeaderboardHandle(void)
{
	m_hLeaderboardHandle = NULL;

	if (!steamapicontext || !steamapicontext->SteamUserStats())
		return;

	SteamAPICall_t hSteamAPICall = steamapicontext->SteamUserStats()->FindLeaderboard(m_pchLeaderboardName);
	if (hSteamAPICall != NULL)
		m_SteamCallResultFindLeaderboard.Set(hSteamAPICall, this, &CLeaderboardHandler::OnFindLeaderboard);
}

void CLeaderboardHandler::FetchLeaderboardResults(int iOffset)
{
	if (m_bIsLoading || (m_hLeaderboardHandle == NULL))
		return;

	m_bIsLoading = true;
	SteamAPICall_t hSteamAPICall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries(m_hLeaderboardHandle, k_ELeaderboardDataRequestGlobal, 1 + iOffset, MAX_LEADERBOARD_ENTRIES + iOffset);
	m_callResultDownloadEntries.Set(hSteamAPICall, this, &CLeaderboardHandler::OnLeaderboardDownloadedEntries);
}

void CLeaderboardHandler::OnFindLeaderboard(LeaderboardFindResult_t *pFindLeaderboardResult, bool bIOFailure)
{
	// See if we encountered an error during the call
	if (!pFindLeaderboardResult->m_bLeaderboardFound || bIOFailure)
		return;

	// Only allow the selected leaderboard scores.
	const char *pchName = steamapicontext->SteamUserStats()->GetLeaderboardName(pFindLeaderboardResult->m_hSteamLeaderboard);
	if (strcmp(pchName, m_pchLeaderboardName))
		return;

	m_hLeaderboardHandle = pFindLeaderboardResult->m_hSteamLeaderboard;
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
	GameBaseClient->ScoreboardRefreshComplete(steamapicontext->SteamUserStats()->GetLeaderboardEntryCount(m_hLeaderboardHandle));
}

/*static*/void CLeaderboardHandler::UploadLeaderboardStats(void)
{
	const float flTimeNow = engine->Time();
	for (int i = 0; i < g_pLeaderboards.Count(); i++)
		g_pLeaderboards[i]->m_flTimeToUpload = (flTimeNow + 0.15f);
}

/*static*/void CLeaderboardHandler::Update(void)
{
	for (int i = 0; i < g_pLeaderboards.Count(); i++)
		g_pLeaderboards[i]->OnUpdate();
}

/*static*/void CLeaderboardHandler::Reset(void)
{
	for (int i = 0; i < g_pLeaderboards.Count(); i++)
		g_pLeaderboards[i]->OnReset();
}

/*static*/void CLeaderboardHandler::FetchLeaderboardResults(const char *name, int iOffset)
{
	if (!name || !name[0])
		return;

	for (int i = 0; i < g_pLeaderboards.Count(); i++)
	{
		if (!strcmp(g_pLeaderboards[i]->m_pchLeaderboardName, name))
		{
			g_pLeaderboards[i]->FetchLeaderboardResults(iOffset);
			break;
		}
	}
}
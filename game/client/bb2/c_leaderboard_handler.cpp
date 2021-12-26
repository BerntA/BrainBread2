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
static int m_gIndex = 0;
static int m_gCurrentIndex = -1;
static float m_gUploadDelay = 0.0f;

CLeaderboardHandler::CLeaderboardHandler(const char *leaderboardName)
{
	g_pLeaderboards.AddToTail(this);
	Q_strncpy(m_pchLeaderboardName, leaderboardName, sizeof(m_pchLeaderboardName));
	m_hLeaderboardHandle = NULL;
	m_bIsLoading = m_bIsUploading = false;
	m_iIndex = m_gIndex++;
}

CLeaderboardHandler::~CLeaderboardHandler()
{
	g_pLeaderboards.FindAndRemove(this);
}

void CLeaderboardHandler::OnUpdate(void)
{
	if ((m_iIndex == m_gCurrentIndex) && !m_bIsUploading && (m_hLeaderboardHandle != NULL) && (engine->Time() > m_gUploadDelay))
	{
		int32 iArg1 = 0, iArg2 = 0, iArg3 = 0;
		GetLeaderboardStats(iArg1, iArg2, iArg3);
		int32 details[] = { iArg1, iArg2, iArg3, };
		int32 iScore = (iArg1 + iArg2) - iArg3;
		SteamAPICall_t hSteamAPICall = steamapicontext->SteamUserStats()->UploadLeaderboardScore(m_hLeaderboardHandle, k_ELeaderboardUploadScoreMethodForceUpdate, MAX(iScore, 0), details, _ARRAYSIZE(details));
		m_callResultUpload.Set(hSteamAPICall, this, &CLeaderboardHandler::OnLeaderboardUploadedEntry);
		m_bIsUploading = true;
	}
}

void CLeaderboardHandler::FetchLeaderboardHandle(void)
{
	SteamAPICall_t hSteamAPICall = (steamapicontext && steamapicontext->SteamUserStats()) ? steamapicontext->SteamUserStats()->FindLeaderboard(m_pchLeaderboardName) : NULL;
	m_callResultFind.Set(hSteamAPICall, this, &CLeaderboardHandler::OnFindLeaderboard);
}

void CLeaderboardHandler::FetchLeaderboardResults(int iOffset)
{
	if (m_bIsLoading || (m_hLeaderboardHandle == NULL))
		return;

	m_bIsLoading = true;
	SteamAPICall_t hSteamAPICall = steamapicontext->SteamUserStats()->DownloadLeaderboardEntries(m_hLeaderboardHandle, k_ELeaderboardDataRequestGlobal, 1 + iOffset, MAX_LEADERBOARD_ENTRIES + iOffset);
	m_callResultDownload.Set(hSteamAPICall, this, &CLeaderboardHandler::OnLeaderboardDownloadedEntries);
}

void CLeaderboardHandler::OnFindLeaderboard(LeaderboardFindResult_t *pData, bool bIOFailure)
{
	// See if we encountered an error during the call
	if (!pData->m_bLeaderboardFound || bIOFailure)
		return;

	// Only allow the selected leaderboard scores.
	const char *pchName = steamapicontext->SteamUserStats()->GetLeaderboardName(pData->m_hSteamLeaderboard);
	if (strcmp(pchName, m_pchLeaderboardName))
		return;

	m_hLeaderboardHandle = pData->m_hSteamLeaderboard;
}

void CLeaderboardHandler::OnLeaderboardDownloadedEntries(LeaderboardScoresDownloaded_t *pData, bool bIOFailure)
{
	int iEntries = pData->m_cEntryCount;
	int m_iItemIndex = 0;

	for (int index = 0; index < iEntries; index++)
	{
		LeaderboardEntry_t pEntry;
		int32 details[3];
		if (steamapicontext->SteamUserStats()->GetDownloadedLeaderboardEntry(pData->m_hSteamLeaderboardEntries, index, &pEntry, details, _ARRAYSIZE(details)))
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

void CLeaderboardHandler::OnLeaderboardUploadedEntry(LeaderboardScoreUploaded_t *pData, bool bIOFailure)
{
	m_bIsUploading = false;
	m_gCurrentIndex++;
	m_gUploadDelay = (engine->Time() + 0.15f);
}

/*static*/void CLeaderboardHandler::InitHandle(void)
{
	for (int i = 0; i < g_pLeaderboards.Count(); i++)
		g_pLeaderboards[i]->FetchLeaderboardHandle();
}

/*static*/void CLeaderboardHandler::UploadLeaderboardStats(void)
{
	m_gCurrentIndex = 0;
	m_gUploadDelay = 0.0f;
	for (int i = 0; i < g_pLeaderboards.Count(); i++)
		g_pLeaderboards[i]->m_bIsUploading = false;
}

/*static*/void CLeaderboardHandler::Update(void)
{
	for (int i = 0; i < g_pLeaderboards.Count(); i++)
		g_pLeaderboards[i]->OnUpdate();
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
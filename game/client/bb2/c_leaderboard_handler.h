//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles leaderboard information, fetches leaderboard info and such.
//
//========================================================================================//

#ifndef LEADERBOARD_HANDLER_H
#define LEADERBOARD_HANDLER_H

#ifdef _WIN32
#pragma once
#endif

#include <steam/steam_api.h>

#define MAX_LEADERBOARD_ENTRIES 5
#define TIME_TO_FULL_UPDATE 10.0f

class CLeaderboardHandler
{
public:
	CLeaderboardHandler();
	~CLeaderboardHandler();

	void FetchLeaderboardHandle(void);
	void FetchLeaderboardResults(int iOffset = 0);
	void OnUpdate(void);
	void Reset(void);

private:
	bool m_bIsLoading;

	SteamLeaderboard_t m_hGlobalLeaderboardHandle;

	void OnFindLeaderboard(LeaderboardFindResult_t *pFindLearderboardResult, bool bIOFailure);
	CCallResult<CLeaderboardHandler, LeaderboardFindResult_t> m_SteamCallResultFindLeaderboard;

	void OnLeaderboardDownloadedEntries(LeaderboardScoresDownloaded_t *pLeaderboardScoresDownloaded, bool bIOFailure);
	CCallResult<CLeaderboardHandler, LeaderboardScoresDownloaded_t> m_callResultDownloadEntries;

	float m_flScoreUpdateTime;
};

#endif // LEADERBOARD_HANDLER_H
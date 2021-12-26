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

class CLeaderboardHandler
{
public:
	DECLARE_CLASS_NOBASE(CLeaderboardHandler);

	CLeaderboardHandler(const char *leaderboardName);
	virtual ~CLeaderboardHandler();

	static void InitHandle(void);
	static void UploadLeaderboardStats(void);
	static void Update(void);
	static void FetchLeaderboardResults(const char *name, int iOffset);

protected:
	virtual void GetLeaderboardStats(int32 &arg1, int32 &arg2, int32 &arg3) { }

	void FetchLeaderboardHandle(void);
	void FetchLeaderboardResults(int iOffset = 0);

	void OnUpdate(void);

private:
	bool m_bIsLoading;
	bool m_bIsUploading;
	int m_iIndex;
	char m_pchLeaderboardName[32];
	SteamLeaderboard_t m_hLeaderboardHandle;

	void OnFindLeaderboard(LeaderboardFindResult_t *pData, bool bIOFailure);
	CCallResult<CLeaderboardHandler, LeaderboardFindResult_t> m_callResultFind;

	void OnLeaderboardDownloadedEntries(LeaderboardScoresDownloaded_t *pData, bool bIOFailure);
	CCallResult<CLeaderboardHandler, LeaderboardScoresDownloaded_t> m_callResultDownload;

	void OnLeaderboardUploadedEntry(LeaderboardScoreUploaded_t *pData, bool bIOFailure);
	CCallResult<CLeaderboardHandler, LeaderboardScoreUploaded_t> m_callResultUpload;

	CLeaderboardHandler(const CLeaderboardHandler &); // not defined	
};

#endif // LEADERBOARD_HANDLER_H
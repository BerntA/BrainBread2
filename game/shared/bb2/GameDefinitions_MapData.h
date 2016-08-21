//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handle map specific data on the server & client. @UGC stuff.
//
//========================================================================================//

#ifndef GAME_DEFINITIONS_MAPDATA_H
#define GAME_DEFINITIONS_MAPDATA_H

#ifdef _WIN32
#pragma once
#endif

#include <steam/steam_api.h>

enum MapVerifications
{
	MAP_VERIFIED_UNKNOWN = 0,
	MAP_VERIFIED_WHITELISTED,
	MAP_VERIFIED_OFFICIAL,
};

#ifdef CLIENT_DLL
struct gameMapItem_t
{
	char pszMapName[32];
	char pszMapTitle[32];
	char pszMapDescription[256];
	char pszMapExtraInfo[256];
	bool bExclude;

	float flScore;
	int iMapVerification;
};
#else
struct gameMapItem_t
{
	char pszMapName[32];
	int iMapVerification;
	unsigned long long ulFileSize;
};
#endif

class CGameDefinitionsMapData
{
public:
	CGameDefinitionsMapData();
	~CGameDefinitionsMapData();

	void ReloadDataForMap(void);
	void ParseDataForMap(const char *map);
	void CleanupMapData(void);

	void FetchMapData(void);
	int GetMapIndex(const char *pszMap);
	bool IsMapWhiteListed(const char *pszMap);

#ifndef CLIENT_DLL
	bool VerifyMapFile(const char *map, unsigned long long mapSize);
#else
	void GetMapInfo(const char *map, gameMapItem_t &item, KeyValues *pkvData = NULL);
#endif

	void OnReceiveUGCQueryResultsAll(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure);
	UGCQueryHandle_t queryHandler;
	CCallResult< CGameDefinitionsMapData, SteamUGCQueryCompleted_t > m_SteamCallResultUGCQuery;

	CUtlVector<gameMapItem_t> pszGameMaps;
};

#endif // GAME_DEFINITIONS_MAPDATA_H
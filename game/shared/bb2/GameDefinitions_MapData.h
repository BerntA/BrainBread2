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
#define MAX_MAP_PREVIEW_IMAGES 5
struct gameMapItem_t
{
	gameMapItem_t()
	{
		numLoadingScreens = 0;
		numImagePreviews = 0;
		for (int i = 0; i < MAX_MAP_PREVIEW_IMAGES; i++)
			Q_strncpy(pszImagePreview[i], "", MAX_WEAPON_STRING);
	}

	char pszMapName[32];
	char pszMapTitle[32];
	char pszMapDescription[256];
	char pszMapExtraInfo[256];
	bool bExclude;

	float flScore;
	int iMapVerification;
	PublishedFileId_t workshopID;

	char pszImagePreview[MAX_MAP_PREVIEW_IMAGES][MAX_WEAPON_STRING];
	int numImagePreviews;
	int numLoadingScreens;

	const char *GetImagePreview(int index)
	{
		if (index < 0 || index >= MAX_MAP_PREVIEW_IMAGES)
			return "steam_default_avatar";

		return pszImagePreview[index];
	}
};
#else
struct gameMapItem_t
{
	char pszMapName[32];
	int iMapVerification;
	unsigned long long ulFileSize;
	PublishedFileId_t workshopID;
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

	bool FetchMapData(void);
	bool QueryUGC(int page = 1);
	gameMapItem_t *GetMapData(const char *pszMap);
	int GetMapIndex(const char *pszMap);
	bool IsMapWhiteListed(const char *pszMap);

#ifndef CLIENT_DLL
	bool VerifyMapFile(const char *map, unsigned long long mapSize);
	CUtlVector<PublishedFileId_t> pszWorkshopItemList;
#else
	void GetMapInfo(const char *map, gameMapItem_t &item, KeyValues *pkvData = NULL);
	void GetMapImageData(const char *map, gameMapItem_t &item);
#endif

	void OnReceiveUGCQueryResultsAll(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure);
	UGCQueryHandle_t queryHandler;
	CCallResult< CGameDefinitionsMapData, SteamUGCQueryCompleted_t > m_SteamCallResultUGCQuery;

	CUtlVector<gameMapItem_t> pszGameMaps;

protected:
	int m_iMatchingItems;
	int m_iCurrentPage;
	bool m_bSatRequestInfo;
};

#endif // GAME_DEFINITIONS_MAPDATA_H
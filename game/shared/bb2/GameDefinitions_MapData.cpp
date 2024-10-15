//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handle map specific data on the server & client. @UGC stuff.
//
//========================================================================================//

#include "cbase.h"
#include "GameDefinitions_MapData.h"
#include "hl2mp_gamerules.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "GameBase_Shared.h"

#ifndef CLIENT_DLL
#include "gameinterface.h"
#include "GameBase_Server.h"

#define STEAM_API_INTERFACE steamgameserverapicontext
#else
#include "vgui/ILocalize.h"
#include "vgui/VGUI.h"
#include "music_system.h"

#define STEAM_API_INTERFACE steamapicontext

const char *GetMapInfoToken(int mode)
{
	const char *unicodeToken = "#MAP_INFO_OBJECTIVE";
	switch (mode)
	{
	case MODE_ARENA:
		unicodeToken = "#MAP_INFO_ARENA";
		break;
	case MODE_DEATHMATCH:
		unicodeToken = "#MAP_INFO_DEATHMATCH";
		break;
	case MODE_ELIMINATION:
		unicodeToken = "#MAP_INFO_ELIMINATION";
		break;
	}

	return unicodeToken;
}
#endif

CGameDefinitionsMapData::CGameDefinitionsMapData()
{
	queryHandler = NULL;
	FetchMapData();
}

CGameDefinitionsMapData::~CGameDefinitionsMapData()
{
	pszGameMaps.Purge();
#ifndef CLIENT_DLL
	pszWorkshopItemList.Purge();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: This will reparse map specific scripts for the active map.
//-----------------------------------------------------------------------------
void CGameDefinitionsMapData::ReloadDataForMap(void)
{
#ifdef CLIENT_DLL
	if (!engine->IsInGame())
		return;

	char pchMapName[128];
	Q_FileBase(engine->GetLevelName(), pchMapName, 128);
	ParseDataForMap(pchMapName);
#else
	if (HL2MPRules())
		ParseDataForMap(HL2MPRules()->szCurrentMap);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Whenever you load a new map, parse specific data for the loaded map here.
//-----------------------------------------------------------------------------
void CGameDefinitionsMapData::ParseDataForMap(const char *map)
{
	CleanupMapData();

	char pchMapPath[80];
	Q_snprintf(pchMapPath, 80, "data/maps/%s", map);

	KeyValues *pkvMapData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, pchMapPath);
	if (pkvMapData)
	{
		KeyValues *pkvQuestData = pkvMapData->FindKey("QuestData");
		KeyValues *pkvModelOverrideData = pkvMapData->FindKey("ModelOverrideData");
		KeyValues *pkvInventoryData = pkvMapData->FindKey("InventoryData");

		if (GameBaseShared()->GetSharedQuestData())
			GameBaseShared()->GetSharedQuestData()->ParseQuestData(pkvQuestData);

		if (GameBaseShared()->GetNPCData())
			GameBaseShared()->GetNPCData()->LoadNPCOverrideData(pkvModelOverrideData);

		if (GameBaseShared()->GetSharedGameDetails())
			GameBaseShared()->GetSharedGameDetails()->ParseInventoryData(pkvInventoryData, true);

#ifdef CLIENT_DLL
		if (GetMusicSystem)
			GetMusicSystem->ParseMapMusicData(pkvMapData->FindKey("MusicData"));
#endif

		pkvMapData->deleteThis();
	}

	// Load festive overrides
#ifndef CLIENT_DLL
	{
		const int iFestiveEvent = GameBaseShared()->GetFestiveEvent();
		KeyValues* pkvFestiveData = NULL;

		switch (iFestiveEvent)
		{
		case FESTIVE_EVENT_HALLOWEEN:
			pkvFestiveData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/festive_halloween_overrides");
			break;

		case FESTIVE_EVENT_CHRISTMAS:
			pkvFestiveData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/festive_christmas_overrides");
			break;
		}

		if (pkvFestiveData)
		{
			if (GameBaseShared()->GetNPCData())
				GameBaseShared()->GetNPCData()->LoadNPCOverrideData(pkvFestiveData);

			pkvFestiveData->deleteThis();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Cleanup map specific parsed stuff before parsing new stuff.
//-----------------------------------------------------------------------------
void CGameDefinitionsMapData::CleanupMapData(void)
{
	if (GameBaseShared()->GetSharedQuestData())
		GameBaseShared()->GetSharedQuestData()->CleanupQuestData();

	if (GameBaseShared()->GetNPCData())
		GameBaseShared()->GetNPCData()->CleanupOverrideData();

	if (GameBaseShared()->GetSharedGameDetails())
		GameBaseShared()->GetSharedGameDetails()->RemoveMapInventoryItems();

#ifdef CLIENT_DLL
	if (GetMusicSystem)
		GetMusicSystem->CleanupMapMusic();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Load official maps, custom maps and workshop maps. Parse data.
//-----------------------------------------------------------------------------
bool CGameDefinitionsMapData::FetchMapData(void)
{
	pszGameMaps.Purge();
#ifndef CLIENT_DLL
	pszWorkshopItemList.Purge();
#endif

	m_iMatchingItems = 0;
	m_iCurrentPage = 1;
	m_bSatRequestInfo = false;

	// Fetch the official map list!
	KeyValues *pkvOfficialMapData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/checksums", true);
	if (pkvOfficialMapData)
	{
		KeyValues *pkvMapField = pkvOfficialMapData->FindKey("Maps");
		if (pkvMapField)
		{
			for (KeyValues *sub = pkvMapField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				gameMapItem_t mapItem;

#ifdef CLIENT_DLL
				GetMapInfo(sub->GetName(), mapItem);
#else
				mapItem.ulFileSize = ((unsigned long long)atoll(sub->GetString()));
#endif

				mapItem.iMapVerification = MAP_VERIFIED_OFFICIAL;
				mapItem.workshopID = 0;
				Q_strncpy(mapItem.pszMapName, sub->GetName(), 32);

				pszGameMaps.AddToTail(mapItem);
			}
		}

		pkvOfficialMapData->deleteThis();
	}

#ifndef CLIENT_DLL
	if (GameBaseShared() && GameBaseShared()->GetServerWorkshopData())
		GameBaseShared()->GetServerWorkshopData()->GetListOfAddons(pszWorkshopItemList, true);

	m_iMatchingItems = pszWorkshopItemList.Count();
	m_bSatRequestInfo = true;
#else
	// Add other unknown custom maps (non workshop)...
	char pszFileName[80];
	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx("data/maps/*.txt", "MOD", &findHandle);
	while (pFilename)
	{
		Q_snprintf(pszFileName, 80, "data/maps/%s", pFilename);
		pszFileName[strlen(pszFileName) - 4] = 0; // strip the file extension!

		KeyValues *pkvMapData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, pszFileName);
		if (pkvMapData)
		{
			const char *mapName = pkvMapData->GetString("Name");
			if (GetMapIndex(mapName) == -1)
			{
				gameMapItem_t mapItem;
				GetMapInfo(mapName, mapItem, pkvMapData);
				mapItem.iMapVerification = MAP_VERIFIED_UNKNOWN;
				mapItem.workshopID = 0;
				Q_strncpy(mapItem.pszMapName, mapName, 32);
				pszGameMaps.AddToTail(mapItem);
			}

#ifdef CLIENT_DLL
			if (GetMusicSystem)
				GetMusicSystem->ParseLoadingMusic(mapName, pkvMapData->FindKey("MusicData"));
#endif

			pkvMapData->deleteThis();
		}

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
#endif

	return QueryUGC(m_iCurrentPage);
}

bool CGameDefinitionsMapData::QueryUGC(int page)
{
#ifndef CLIENT_DLL
	if (!STEAM_API_INTERFACE || (STEAM_API_INTERFACE && !STEAM_API_INTERFACE->SteamUGC()) || !engine->IsDedicatedServer())
#else
	if (!STEAM_API_INTERFACE || (STEAM_API_INTERFACE && !STEAM_API_INTERFACE->SteamUGC()))
#endif
	{
#ifdef CLIENT_DLL
		Warning("Client SteamAPI Interface is unavailable!\n");
#else
		Warning("Server SteamAPI Interface is unavailable!\n");
#endif
		return false;
	}

	if (queryHandler)
	{
		STEAM_API_INTERFACE->SteamUGC()->ReleaseQueryUGCRequest(queryHandler);
		queryHandler = NULL;
	}

#ifdef CLIENT_DLL
	AccountID_t steamAccountID = STEAM_API_INTERFACE->SteamUser()->GetSteamID().GetAccountID();
	queryHandler = STEAM_API_INTERFACE->SteamUGC()->CreateQueryUserUGCRequest(steamAccountID, k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_Items, k_EUserUGCListSortOrder_TitleAsc, (AppId_t)382990, (AppId_t)346330, page);
#else
	page--;
	if ((pszWorkshopItemList.Count() <= 0) || (pszWorkshopItemList.Count() <= (page * 50)))
		return false;

	queryHandler = STEAM_API_INTERFACE->SteamUGC()->CreateQueryUGCDetailsRequest(&pszWorkshopItemList[(page * 50)], pszWorkshopItemList.Count());
#endif

	STEAM_API_INTERFACE->SteamUGC()->SetReturnKeyValueTags(queryHandler, true);
	SteamAPICall_t apiCallback = STEAM_API_INTERFACE->SteamUGC()->SendQueryUGCRequest(queryHandler);
	m_SteamCallResultUGCQuery.Set(apiCallback, this, &CGameDefinitionsMapData::OnReceiveUGCQueryResultsAll);

	return true;
}

gameMapItem_t* CGameDefinitionsMapData::GetMapData(const char* pszMap)
{
	int index = GetMapIndex(pszMap);
	return ((index != -1) ? &pszGameMaps[index] : NULL);
}

int CGameDefinitionsMapData::GetMapIndex(const char* pszMap)
{
	for (int i = 0; i < pszGameMaps.Count(); i++)
	{
		if (!strcmp(pszGameMaps[i].pszMapName, pszMap))
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Is this map whitelisted?
//-----------------------------------------------------------------------------
bool CGameDefinitionsMapData::IsMapWhiteListed(const char *pszMap)
{
	for (int i = 0; i < pszGameMaps.Count(); i++)
	{
		if ((pszGameMaps[i].iMapVerification >= MAP_VERIFIED_WHITELISTED) && !strcmp(pszGameMaps[i].pszMapName, pszMap))
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Fetch workshop items and their details
//-----------------------------------------------------------------------------
void CGameDefinitionsMapData::OnReceiveUGCQueryResultsAll(SteamUGCQueryCompleted_t* pCallback, bool bIOFailure)
{
	if (!m_bSatRequestInfo)
	{
		m_iMatchingItems = pCallback->m_unTotalMatchingResults;
		m_bSatRequestInfo = true;
		Msg("Loaded %i workshop addons.\n", pCallback->m_unTotalMatchingResults);
	}

	CUtlVector<WorkshopMapKVPair> mapKVPair;
	char mapTagKey[WORKSHOP_KV_PAIR_SIZE], mapTagValue[WORKSHOP_KV_PAIR_SIZE];
	int pairIndex = 0;

	for (uint32 i = 0; i < pCallback->m_unNumResultsReturned; ++i)
	{
		mapKVPair.Purge();
		SteamUGCDetails_t WorkshopItem;
		if (!STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCResult(pCallback->m_handle, i, &WorkshopItem))
			continue;

		uint32 uAmountOfKeys = STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCNumKeyValueTags(pCallback->m_handle, i);
		if (uAmountOfKeys == 0) continue;

		// Scan for map_name keys, populate vector list:
		for (uint32 items = 0; items < uAmountOfKeys; items++)
		{
			STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCKeyValueTag(pCallback->m_handle, i, items, mapTagKey, WORKSHOP_KV_PAIR_SIZE, mapTagValue, WORKSHOP_KV_PAIR_SIZE);
			if (strcmp(mapTagKey, "map_name") != 0) continue;

			WorkshopMapKVPair pair;
			Q_strncpy(pair.map, mapTagValue, sizeof(pair.map));
			pair.size = 0ULL;
			mapKVPair.AddToTail(pair);
		}

		// Scan for map_size keys:
		pairIndex = 0;
		for (uint32 items = 0; items < uAmountOfKeys; items++)
		{
			STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCKeyValueTag(pCallback->m_handle, i, items, mapTagKey, WORKSHOP_KV_PAIR_SIZE, mapTagValue, WORKSHOP_KV_PAIR_SIZE);
			if (strcmp(mapTagKey, "map_size") != 0) continue;

			mapKVPair[pairIndex].size = ((unsigned long long)atoll(mapTagValue));
			pairIndex++;
		}

		if (mapKVPair.Count() == 0) continue;

		int iVerification = MAP_VERIFIED_WHITELISTED; // Every custom map is now whitelisted.
		//if (Q_strstr(WorkshopItem.m_rgchTags, "Whitelisted"))
		//	iVerification = MAP_VERIFIED_WHITELISTED;

		for (int items = 0; items < mapKVPair.Count(); items++)
		{
			const WorkshopMapKVPair& pair = mapKVPair[items];
			int iExistingMapIndex = GetMapIndex(pair.map);

#ifndef CLIENT_DLL
			if (iExistingMapIndex != -1)
			{
				pszGameMaps[iExistingMapIndex].ulFileSize = pair.size;
				pszGameMaps[iExistingMapIndex].iMapVerification = iVerification;
				pszGameMaps[iExistingMapIndex].workshopID = WorkshopItem.m_nPublishedFileId;
			}
			else
			{
				gameMapItem_t mapItem;
				mapItem.ulFileSize = pair.size;
				mapItem.iMapVerification = iVerification;
				mapItem.workshopID = WorkshopItem.m_nPublishedFileId;
				Q_strncpy(mapItem.pszMapName, pair.map, 32);
				pszGameMaps.AddToTail(mapItem);
			}
#else
			if (iExistingMapIndex != -1)
			{
				pszGameMaps[iExistingMapIndex].iMapVerification = iVerification;
				pszGameMaps[iExistingMapIndex].workshopID = WorkshopItem.m_nPublishedFileId;
			}
			else
			{
				gameMapItem_t mapItem;
				Q_strncpy(mapItem.pszMapTitle, ((!(WorkshopItem.m_rgchTitle && WorkshopItem.m_rgchTitle[0])) ? pair.map : WorkshopItem.m_rgchTitle), 32);
				Q_strncpy(mapItem.pszMapDescription, WorkshopItem.m_rgchDescription, 256);
				Q_strncpy(mapItem.pszMapExtraInfo, "", 256);
				mapItem.iMapVerification = iVerification;
				mapItem.workshopID = WorkshopItem.m_nPublishedFileId;
				mapItem.bExclude = false;
				Q_strncpy(mapItem.pszMapName, pair.map, 32);
				GetMapImageData(mapItem.pszMapName, mapItem);
				pszGameMaps.AddToTail(mapItem);
			}
#endif
		}
	}

	// Cleanup
	if (queryHandler)
	{
		STEAM_API_INTERFACE->SteamUGC()->ReleaseQueryUGCRequest(queryHandler);
		queryHandler = NULL;
	}

	m_iMatchingItems -= pCallback->m_unNumResultsReturned;
	if (m_iMatchingItems > 0)
	{
		m_iCurrentPage++;
		QueryUGC(m_iCurrentPage);
		return;
	}

#ifndef CLIENT_DLL
	GameBaseServer()->CheckMapData();
	GameBaseServer()->LoadServerTags();
#endif
}

#ifndef CLIENT_DLL
// Check if this map is registered in our map list, if so check if its filesize is the same as in our list.
bool CGameDefinitionsMapData::VerifyMapFile(const char* map, unsigned long long mapSize)
{
	int index = GetMapIndex(map);
	if ((index == -1) || (mapSize <= 0))
		return false;
	return (mapSize == pszGameMaps[index].ulFileSize);
}
#else
void CGameDefinitionsMapData::GetMapInfo(const char *map, gameMapItem_t &item, KeyValues *pkvData)
{
	GetMapImageData(map, item);
	bool bShouldDelete = false;
	KeyValues *pkvInfo = pkvData;
	if (!pkvInfo)
	{
		char pchPathToMap[80];
		Q_snprintf(pchPathToMap, 80, "data/maps/%s", map);
		pkvInfo = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, pchPathToMap);
		if (!pkvInfo)
		{
			Q_strncpy(item.pszMapTitle, "", 32);
			Q_strncpy(item.pszMapDescription, "", 256);
			Q_strncpy(item.pszMapExtraInfo, "", 256);
			item.bExclude = true;
			Warning("Unable to read map data for map '%s'!\n", map);
			return;
		}

		bShouldDelete = true;
	}

	Q_strncpy(item.pszMapTitle, pkvInfo->GetString("Title"), 32);
	Q_strncpy(item.pszMapDescription, pkvInfo->GetString("Description"), 256);
	item.bExclude = (pkvInfo->GetInt("Exclude") >= 1);

	// Fetch the extra info:
	const char *unicodeToken = GetMapInfoToken(GetGamemodeForMap(map));
	wchar_t wszMapGamemodeString[32];
	g_pVGuiLocalize->ConvertANSIToUnicode(GetGamemodeNameForPrefix(map), wszMapGamemodeString, sizeof(wszMapGamemodeString));

	wchar_t wszArg1[16], wszArg2[16], wszArg3[16], wszArg4[16];
	g_pVGuiLocalize->ConvertANSIToUnicode(pkvInfo->GetString("arg1", ""), wszArg1, sizeof(wszArg1));
	g_pVGuiLocalize->ConvertANSIToUnicode(pkvInfo->GetString("arg2", ""), wszArg2, sizeof(wszArg2));
	g_pVGuiLocalize->ConvertANSIToUnicode(pkvInfo->GetString("arg3", ""), wszArg3, sizeof(wszArg3));
	g_pVGuiLocalize->ConvertANSIToUnicode(pkvInfo->GetString("arg4", ""), wszArg4, sizeof(wszArg4));

	wchar_t wszInfo[256];
	g_pVGuiLocalize->ConstructString(wszInfo, sizeof(wszInfo), g_pVGuiLocalize->Find(unicodeToken), 5, wszMapGamemodeString, wszArg1, wszArg2, wszArg3, wszArg4);
	g_pVGuiLocalize->ConvertUnicodeToANSI(wszInfo, item.pszMapExtraInfo, 256);

	if (bShouldDelete)
		pkvInfo->deleteThis();
}

void CGameDefinitionsMapData::GetMapImageData(const char *map, gameMapItem_t &item)
{
	int numImagePreviews = 0, numLoadingScreens = 0;
	FileFindHandle_t findHandle;

	const char *pFilename = filesystem->FindFirstEx("materials/vgui/maps/*.vmt", "MOD", &findHandle);
	while (pFilename)
	{
		if (numImagePreviews >= MAX_MAP_PREVIEW_IMAGES)
			break;

		if (Q_stristr(pFilename, map))
		{
			Q_snprintf(item.pszImagePreview[numImagePreviews], MAX_WEAPON_STRING, "maps/%s_%i", map, numImagePreviews);
			numImagePreviews++;
		}

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);

	pFilename = filesystem->FindFirstEx("materials/vgui/loading/*.vmt", "MOD", &findHandle);
	while (pFilename)
	{
		if (Q_stristr(pFilename, map))
			numLoadingScreens++;

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);

	item.numImagePreviews = numImagePreviews;
	item.numLoadingScreens = numLoadingScreens;
}
#endif
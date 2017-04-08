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
		KeyValues *pkvMusicData = pkvMapData->FindKey("MusicData");
		if (GetMusicSystem)
			GetMusicSystem->ParseMapMusicData(pkvMusicData);
#endif

		pkvMapData->deleteThis();
	}
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
void CGameDefinitionsMapData::FetchMapData(void)
{
	pszGameMaps.Purge();

	// Fetch the official map list!
	KeyValues *pkvOfficialMapData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/officialmaps");
	if (pkvOfficialMapData)
	{
		// Verify that this is our list, password style.
		if (!strcmp(pkvOfficialMapData->GetName(), "OfficialMapDataXfGZ"))
		{
			for (KeyValues *sub = pkvOfficialMapData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				gameMapItem_t mapItem;

#ifdef CLIENT_DLL
				GetMapInfo(sub->GetName(), mapItem);
				mapItem.flScore = 0.0f;
#else
				mapItem.ulFileSize = (unsigned long long)atoll(sub->GetString());
#endif

				mapItem.iMapVerification = MAP_VERIFIED_OFFICIAL;
				Q_strncpy(mapItem.pszMapName, sub->GetName(), 32);

				pszGameMaps.AddToTail(mapItem);
			}
		}

		pkvOfficialMapData->deleteThis();
	}

#ifdef CLIENT_DLL
	// Add other unknown custom maps (non workshop)...
	char pszFileName[80];
	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx("data/maps/*.*", "MOD", &findHandle);
	while (pFilename)
	{
		bool bIsDirectory = filesystem->IsDirectory(pFilename, "MOD");
		if ((strlen(pFilename) > 4) && !bIsDirectory)
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
					mapItem.flScore = 0.0f;
					mapItem.iMapVerification = MAP_VERIFIED_UNKNOWN;
					Q_strncpy(mapItem.pszMapName, mapName, 32);
					pszGameMaps.AddToTail(mapItem);
				}

				pkvMapData->deleteThis();
			}
		}

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
#endif

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
		return;
	}

	if (queryHandler)
	{
		STEAM_API_INTERFACE->SteamUGC()->ReleaseQueryUGCRequest(queryHandler);
		queryHandler = NULL;
	}

	AccountID_t steamAccountID = NULL;
#ifdef CLIENT_DLL
	steamAccountID = STEAM_API_INTERFACE->SteamUser()->GetSteamID().GetAccountID();
	queryHandler = STEAM_API_INTERFACE->SteamUGC()->CreateQueryUserUGCRequest(steamAccountID, k_EUserUGCList_Subscribed, k_EUGCMatchingUGCType_Items, k_EUserUGCListSortOrder_TitleAsc, (AppId_t)382990, (AppId_t)346330, 1);
#else
	steamAccountID = STEAM_API_INTERFACE->SteamGameServer()->GetSteamID().GetAccountID();
	queryHandler = STEAM_API_INTERFACE->SteamUGC()->CreateQueryAllUGCRequest(k_EUGCQuery_RankedByVote, k_EUGCMatchingUGCType_Items, (AppId_t)382990, (AppId_t)346330, 1);
	STEAM_API_INTERFACE->SteamUGC()->AddRequiredTag(queryHandler, "Whitelisted");
#endif

	STEAM_API_INTERFACE->SteamUGC()->SetReturnKeyValueTags(queryHandler, true);

	SteamAPICall_t apiCallback = STEAM_API_INTERFACE->SteamUGC()->SendQueryUGCRequest(queryHandler);
	m_SteamCallResultUGCQuery.Set(apiCallback, this, &CGameDefinitionsMapData::OnReceiveUGCQueryResultsAll);
}

int CGameDefinitionsMapData::GetMapIndex(const char *pszMap)
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
	bool bWhitelisted = false;
	for (int i = 0; i < pszGameMaps.Count(); i++)
	{
		if (!strcmp(pszGameMaps[i].pszMapName, pszMap) && (pszGameMaps[i].iMapVerification >= MAP_VERIFIED_WHITELISTED))
		{
			bWhitelisted = true;
			break;
		}
	}

	return bWhitelisted;
}

//-----------------------------------------------------------------------------
// Purpose: Fetch workshop items and their details
//-----------------------------------------------------------------------------
void CGameDefinitionsMapData::OnReceiveUGCQueryResultsAll(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure)
{
	for (uint32 i = 0; i < pCallback->m_unNumResultsReturned; ++i)
	{
		SteamUGCDetails_t WorkshopItem;
		if (STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCResult(pCallback->m_handle, i, &WorkshopItem))
		{
			// This is not a map item therefore we skip it...
			if (!Q_strstr(WorkshopItem.m_rgchTags, "Elimination") &&
				!Q_strstr(WorkshopItem.m_rgchTags, "Arena") &&
				!Q_strstr(WorkshopItem.m_rgchTags, "Classic") &&
				!Q_strstr(WorkshopItem.m_rgchTags, "Objective") &&
				!Q_strstr(WorkshopItem.m_rgchTags, "Story Mode") &&
				!Q_strstr(WorkshopItem.m_rgchTags, "Deathmatch") &&
				!Q_strstr(WorkshopItem.m_rgchTags, "Custom"))
				continue;

			uint32 uAmountOfKeys = STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCNumKeyValueTags(pCallback->m_handle, i);
			for (uint32 items = 0; items < uAmountOfKeys; items += 2)
			{
				int iVerification = MAP_VERIFIED_UNKNOWN;
				if (Q_strstr(WorkshopItem.m_rgchTags, "Whitelisted"))
					iVerification = MAP_VERIFIED_WHITELISTED;

				char mapNameKey[32], mapNameValue[32], mapSizeKey[32], mapSizeValue[128];
				STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCKeyValueTag(pCallback->m_handle, i, items, mapNameKey, 32, mapNameValue, 32);
				STEAM_API_INTERFACE->SteamUGC()->GetQueryUGCKeyValueTag(pCallback->m_handle, i, (items + 1), mapSizeKey, 32, mapSizeValue, 128);

				if (strlen(mapNameValue) <= 0)
					continue;

#ifndef CLIENT_DLL
				if (strlen(mapSizeValue) <= 0)
					continue;

				if (GetMapIndex(mapNameValue) != -1)
					continue;

				unsigned long long mapFileSize = (unsigned long long)atoll(mapSizeValue);
				gameMapItem_t mapItem;
				mapItem.ulFileSize = mapFileSize;
				mapItem.iMapVerification = iVerification;
				Q_strncpy(mapItem.pszMapName, mapNameValue, 32);
				pszGameMaps.AddToTail(mapItem);
#else
				int iExistingMapIndex = GetMapIndex(mapNameValue);
				if (iExistingMapIndex != -1)
				{
					pszGameMaps[iExistingMapIndex].flScore = WorkshopItem.m_flScore;
					pszGameMaps[iExistingMapIndex].iMapVerification = iVerification;
				}
				else
				{
					gameMapItem_t mapItem;
					Q_strncpy(mapItem.pszMapTitle, WorkshopItem.m_rgchTitle, 32);
					Q_strncpy(mapItem.pszMapDescription, WorkshopItem.m_rgchDescription, 256);
					Q_strncpy(mapItem.pszMapExtraInfo, "", 256);
					mapItem.iMapVerification = iVerification;
					mapItem.bExclude = false;
					mapItem.flScore = WorkshopItem.m_flScore;
					Q_strncpy(mapItem.pszMapName, mapNameValue, 32);
					pszGameMaps.AddToTail(mapItem);
				}
#endif
			}
		}
	}

	// Cleanup
	if (queryHandler)
	{
		STEAM_API_INTERFACE->SteamUGC()->ReleaseQueryUGCRequest(queryHandler);
		queryHandler = NULL;
	}

#ifndef CLIENT_DLL
	GameBaseServer()->LoadServerTags();
#endif
}

#ifndef CLIENT_DLL
// Check if this map is registered in our map list, if so check if its filesize is the same as in our list.
bool CGameDefinitionsMapData::VerifyMapFile(const char *map, unsigned long long mapSize)
{
	int index = GetMapIndex(map);
	if (index == -1)
		return false;

	if (mapSize <= 0)
		return false;

	return (mapSize == pszGameMaps[index].ulFileSize);
}
#else
void CGameDefinitionsMapData::GetMapInfo(const char *map, gameMapItem_t &item, KeyValues *pkvData)
{
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
#endif
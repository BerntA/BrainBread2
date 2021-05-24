//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handle server-side workshop downloading/updating, specifically for gameservers using SteamCMD.
//
//========================================================================================//

#ifndef GAME_DEFINITIONS_WORKSHOP_H
#define GAME_DEFINITIONS_WORKSHOP_H

#ifdef _WIN32
#pragma once
#endif

#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>

class CGameDefinitionsWorkshop;
class CGameDefinitionsWorkshop
{
public:
	CGameDefinitionsWorkshop();
	~CGameDefinitionsWorkshop();

	void Initialize();
	void DownloadThink();
	void UpdateDownloadedItems();
	void DownloadCollection(PublishedFileId_t itemID);

	void AddItemToList(PublishedFileId_t itemID);
	void RemoveItemFromList(PublishedFileId_t itemID);
	bool DownloadNextItemInList(void);
	int GetItemInList(PublishedFileId_t itemID);
	int GetLastItemInList(void) { return (m_pWorkshopItemDownloadList.Count() - 1); }

	// Populate a list with the available workshop IDs found in the workshop folder.
	void GetListOfAddons(CUtlVector<PublishedFileId_t> &list, bool bMapsOnly = false);

	const char *GetWorkshopDir() { return pszWorkshopDir; }

protected:
	STEAM_GAMESERVER_CALLBACK(CGameDefinitionsWorkshop, DownloadedItem, DownloadItemResult_t, m_CallbackItemDownloaded);

	UGCQueryHandle_t ugcQueryHandle;
	void OnReceiveUGCQueryUGCDetails(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure);
	CCallResult< CGameDefinitionsWorkshop, SteamUGCQueryCompleted_t > m_SteamCallResultUGCQuery;

private:
	char pszWorkshopDir[2048];
	float m_flDownloadInfoDelay;
	int m_iProcessed;
	int m_iProcessCount;
	CUtlVector<PublishedFileId_t> m_pWorkshopItemDownloadList;
};

#endif // GAME_DEFINITIONS_WORKSHOP_H
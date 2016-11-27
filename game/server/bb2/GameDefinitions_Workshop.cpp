//=========       Copyright � Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handle server-side workshop downloading/updating, specifically for gameservers using SteamCMD.
//
//========================================================================================//

#include "cbase.h"
#include "GameDefinitions_Workshop.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static bool m_bLoaded = false;
static bool m_bDownloadListOfItems = false;
static PublishedFileId_t m_currentFileID = 0;

CGameDefinitionsWorkshop::CGameDefinitionsWorkshop() : m_CallbackItemDownloaded(this, &CGameDefinitionsWorkshop::DownloadedItem)
{
	m_bLoaded = false;
	m_bDownloadListOfItems = false;
	pszWorkshopDir[0] = 0;
	m_currentFileID = 0;
	m_flDownloadInfoDelay = 0.0f;
	ugcQueryHandle = NULL;

	char pchGameDir[1024];
	engine->GetGameDir(pchGameDir, 1024);

	Q_snprintf(pszWorkshopDir, 2048, "%s\\workshop", pchGameDir);
	Q_FixSlashes(pszWorkshopDir);

	m_pWorkshopItemDownloadList.Purge();
}

CGameDefinitionsWorkshop::~CGameDefinitionsWorkshop()
{
	m_bLoaded = false;
	m_bDownloadListOfItems = false;
	m_currentFileID = 0;
	m_flDownloadInfoDelay = 0.0f;
	m_pWorkshopItemDownloadList.Purge();
}

void CGameDefinitionsWorkshop::Initialize()
{
	if (m_bLoaded)
		return;

	if (!engine->IsDedicatedServer())
		return;

	if (!steamgameserverapicontext)
		return;

	if (!steamgameserverapicontext->SteamUGC())
		return;

	steamgameserverapicontext->SteamUGC()->BInitWorkshopForGameServer((DepotId_t)346330, pszWorkshopDir);

	m_bLoaded = true;

	Msg("Steam Workshop has been fully initialized!\n");
}

void CGameDefinitionsWorkshop::DownloadThink()
{
	if (!m_bLoaded)
		return;

	if (m_flDownloadInfoDelay <= engine->Time())
	{
		m_flDownloadInfoDelay = engine->Time() + 0.2f;
		if (m_currentFileID)
		{
			uint64 bytesReceived = 0, totalBytesToReceive = 0;
			if (steamgameserverapicontext->SteamUGC()->GetItemDownloadInfo(m_currentFileID, &bytesReceived, &totalBytesToReceive))
			{
				if (totalBytesToReceive)
					Msg("Downloading item '%llu':\nProgress: %llu / %llu\n", ((uint64)m_currentFileID), bytesReceived, totalBytesToReceive);
			}
		}
	}
}

void CGameDefinitionsWorkshop::UpdateDownloadedItems()
{
	if (!m_bLoaded || m_bDownloadListOfItems)
		return;

	m_currentFileID = 0;
	m_pWorkshopItemDownloadList.Purge();

	KeyValues *pkvWorkshopData = new KeyValues("Data");
	if (pkvWorkshopData->LoadFromFile(filesystem, "workshop/appworkshop_346330.acf", "MOD"))
	{
		KeyValues *pkvInstalledItems = pkvWorkshopData->FindKey("WorkshopItemsInstalled");
		if (pkvInstalledItems)
		{
			for (KeyValues *sub = pkvInstalledItems->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				uint64 itemID = atoll(sub->GetName());
				if (itemID)
					AddItemToList(((PublishedFileId_t)itemID));
			}
		}
	}
	pkvWorkshopData->deleteThis();

	DownloadNextItemInList();
}

void CGameDefinitionsWorkshop::DownloadCollection(PublishedFileId_t itemID)
{
	if (!m_bLoaded || m_bDownloadListOfItems || (ugcQueryHandle != NULL))
		return;

	ugcQueryHandle = steamgameserverapicontext->SteamUGC()->CreateQueryUGCDetailsRequest(&itemID, 1);
	steamgameserverapicontext->SteamUGC()->SetReturnChildren(ugcQueryHandle, true);

	SteamAPICall_t apiCallback = steamgameserverapicontext->SteamUGC()->SendQueryUGCRequest(ugcQueryHandle);
	m_SteamCallResultUGCQuery.Set(apiCallback, this, &CGameDefinitionsWorkshop::OnReceiveUGCQueryUGCDetails);
}

void CGameDefinitionsWorkshop::OnReceiveUGCQueryUGCDetails(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure)
{
	SteamUGCDetails_t WorkshopItem;
	if (steamgameserverapicontext->SteamUGC()->GetQueryUGCResult(pCallback->m_handle, 0, &WorkshopItem))
	{
		if (WorkshopItem.m_eFileType == k_EWorkshopFileTypeCollection)
		{
			m_currentFileID = 0;
			m_pWorkshopItemDownloadList.Purge();

			uint32 childrenCount = WorkshopItem.m_unNumChildren;
			if (childrenCount)
			{
				PublishedFileId_t *childrenIDs = new PublishedFileId_t[childrenCount];
				if (steamgameserverapicontext->SteamUGC()->GetQueryUGCChildren(pCallback->m_handle, 0, childrenIDs, childrenCount))
				{
					for (uint32 child = 0; child < childrenCount; ++child)
						AddItemToList(childrenIDs[child]);

					DownloadNextItemInList();
				}
				delete[] childrenIDs;
			}
		}
		else
			Msg("Item '%llu' is not a collection!\n", WorkshopItem.m_nPublishedFileId);
	}

	if (ugcQueryHandle)
	{
		steamgameserverapicontext->SteamUGC()->ReleaseQueryUGCRequest(ugcQueryHandle);
		ugcQueryHandle = NULL;
	}
}

void CGameDefinitionsWorkshop::AddItemToList(PublishedFileId_t itemID)
{
	int index = GetItemInList(itemID);
	if (index != -1)
		return;

	m_pWorkshopItemDownloadList.AddToTail(itemID);
}

void CGameDefinitionsWorkshop::RemoveItemFromList(PublishedFileId_t itemID)
{
	int index = GetItemInList(itemID);
	if (index == -1)
		return;

	m_pWorkshopItemDownloadList.Remove(index);
}

bool CGameDefinitionsWorkshop::DownloadNextItemInList(void)
{
	if (m_pWorkshopItemDownloadList.Count())
	{
		m_bDownloadListOfItems = true;
		m_currentFileID = m_pWorkshopItemDownloadList[GetLastItemInList()];
		steamgameserverapicontext->SteamUGC()->DownloadItem(m_currentFileID, true);
		return true;
	}

	return false;
}

int CGameDefinitionsWorkshop::GetItemInList(PublishedFileId_t itemID)
{
	for (int i = (m_pWorkshopItemDownloadList.Count() - 1); i >= 0; i--)
	{
		if (m_pWorkshopItemDownloadList[i] == itemID)
			return i;
	}

	return -1;
}

void CGameDefinitionsWorkshop::DownloadedItem(DownloadItemResult_t *pItem)
{
	if (!m_bLoaded)
		return;

	Msg("Item '%llu' download result: %i\n", pItem->m_nPublishedFileId, pItem->m_eResult);

	if (m_currentFileID == pItem->m_nPublishedFileId)
	{
		if (m_bDownloadListOfItems)
		{
			RemoveItemFromList(m_currentFileID);
			if (DownloadNextItemInList())
				return;
		}

		m_bDownloadListOfItems = false;
		m_currentFileID = 0;
	}
}

CON_COMMAND(workshop_help, "Display some help & tips related to gameserver workshop stuff.")
{
	if (UTIL_GetCommandClientIndex() != 0)
		return;

	if (!engine->IsDedicatedServer())
	{
		Msg("The 'workshop_*' commands are meant for dedicated servers only!\n");
		return;
	}

	Msg("To use the workshop for gameserver please refer to these commands: (write these in the console or launch the server with +<command here>)\n");
	Msg("workshop_download_item <publishedID> - Downloads the specified item.\n");
	Msg("workshop_download_collection <collectionID> - Downloads an entire collection of workshop items.\n");
	Msg("workshop_update_items - Updates all of your downloaded items.\n");
	Msg("Whenever you download or update workshop items make sure to restart the server so that the items will load properly!\n");
}

CON_COMMAND(workshop_download_item, "Download some item on the workshop.")
{
	if (!m_bLoaded || m_bDownloadListOfItems)
		return;

	if (UTIL_GetCommandClientIndex() != 0)
		return;

	if (args.ArgC() != 2)
		return;

	PublishedFileId_t itemID = ((PublishedFileId_t)atoll(args[1]));
	if (!itemID)
		return;

	Msg("Trying to download item: '%llu'\n", ((uint64)itemID));
	steamgameserverapicontext->SteamUGC()->DownloadItem(itemID, true);
	m_currentFileID = itemID;
}

CON_COMMAND(workshop_download_collection, "Downloads a collection of workshop items.")
{
	if (!m_bLoaded || m_bDownloadListOfItems)
		return;

	if (UTIL_GetCommandClientIndex() != 0)
		return;

	if (!GameBaseShared()->GetServerWorkshopData())
		return;

	if (args.ArgC() != 2)
		return;

	PublishedFileId_t itemID = ((PublishedFileId_t)atoll(args[1]));
	if (!itemID)
		return;

	Msg("Trying to download collection: '%llu'\n", ((uint64)itemID));
	GameBaseShared()->GetServerWorkshopData()->DownloadCollection(itemID);
}

CON_COMMAND(workshop_update_items, "Updates downloaded items if possible.")
{
	if (!m_bLoaded || m_bDownloadListOfItems)
		return;

	if (UTIL_GetCommandClientIndex() != 0)
		return;

	if (!GameBaseShared()->GetServerWorkshopData())
		return;

	Msg("Trying to update downloaded workshop items...\n");
	GameBaseShared()->GetServerWorkshopData()->UpdateDownloadedItems();
}
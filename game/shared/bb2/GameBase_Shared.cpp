//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Shared Game Handler: Handles .bbd files for data parsing, reading, storing, etc...
//
//========================================================================================//

#include "cbase.h"
#include "GameBase_Shared.h"
#include "hl2mp_gamerules.h"
#include "particle_parse.h"

#ifndef CLIENT_DLL
#include "items.h"
#include "baseentity.h"
#include "GameBase_Server.h"
#include "html_data_handler.h"
#else
#include "fmod_manager.h"
#include "hud_macros.h"
#endif

#ifdef CLIENT_DLL
ConVar bb2_inventory_item_debugging("bb2_inventory_item_debugging", "0", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Must be enabled in order for bb2_inventory_ convars to take effect.");
ConVar bb2_inventory_origin_x("bb2_inventory_origin_x", "0", FCVAR_CLIENTDLL, "Changes the origin of all model previews for inventory items.");
ConVar bb2_inventory_origin_y("bb2_inventory_origin_y", "0", FCVAR_CLIENTDLL, "Changes the origin of all model previews for inventory items.");
ConVar bb2_inventory_origin_z("bb2_inventory_origin_z", "0", FCVAR_CLIENTDLL, "Changes the origin of all model previews for inventory items.");
ConVar bb2_inventory_model("bb2_inventory_model", "models/weapons/rifles/ak47/a_ak47.mdl", FCVAR_CLIENTDLL, "Overrides the model on every model preview for inventory items.");
ConVar bb2_inventory_angle_x("bb2_inventory_angle_x", "0", FCVAR_CLIENTDLL, "Changes the angle of all model previews for inventory items.");
ConVar bb2_inventory_angle_y("bb2_inventory_angle_y", "0", FCVAR_CLIENTDLL, "Changes the angle of all model previews for inventory items.");
ConVar bb2_inventory_angle_z("bb2_inventory_angle_z", "0", FCVAR_CLIENTDLL, "Changes the angle of all model previews for inventory items.");

static void __MsgFunc_InventoryUpdate(bf_read &msg)
{
	int iAction = msg.ReadShort();

	if (iAction == INV_ACTION_PURGE)
	{
		GameBaseShared()->GetGameInventory().Purge();
		return;
	}

	uint iItemID = msg.ReadShort();
	bool bMapItem = ((msg.ReadShort() >= 1));

	char pszEntityLink[80];
	pszEntityLink[0] = 0;

	if (iAction >= INV_ACTION_ADD)
		msg.ReadString(pszEntityLink, 80);

	int itemIndex = GameBaseShared()->GetInventoryItemIndex(iItemID, bMapItem);
	if (itemIndex == -1)
	{
		if (iAction == INV_ACTION_REMOVE)
			return;
	}

	if (iAction == INV_ACTION_REMOVE)
		GameBaseShared()->GetGameInventory().Remove(itemIndex);
	else if (iAction == INV_ACTION_ADD)
	{
		InventoryItem_t invItem;
		invItem.m_iItemID = iItemID;
		invItem.bIsMapItem = bMapItem;
		Q_strncpy(invItem.szEntityLink, pszEntityLink, MAX_WEAPON_STRING);
		GameBaseShared()->GetGameInventory().AddToTail(invItem);
	}
}
#endif

CGameBaseShared gGameBaseShared;
CGameBaseShared* GameBaseShared()
{
	return &gGameBaseShared;
}

CGameBaseShared::CGameBaseShared()
{
}

CGameBaseShared::~CGameBaseShared()
{
}

void CGameBaseShared::Init()
{
#ifdef CLIENT_DLL
	m_pMusicSystem = new CMusicSystem();
#endif

	m_pSharedGameDefinitions = NULL;
	m_pSharedGameMapData = NULL;
	m_pSharedNPCData = NULL;
	LoadBase();

	m_pSharedQuestData = new CGameDefinitionsQuestData();

#ifdef CLIENT_DLL
	HOOK_MESSAGE(InventoryUpdate);
	m_pSteamServers = new CSteamServerLister();
#else
	m_pAchievementManager = new CAchievementManager();
	m_pServerWorkshopData = new CGameDefinitionsWorkshop();
	m_pPlayerLoadoutHandler = new CPlayerLoadoutHandler();
#endif
}

void CGameBaseShared::LoadBase()
{
	CheckGameVersion();

	// Not NULL? Release.
	if (m_pSharedGameDefinitions)
		delete m_pSharedGameDefinitions;

	if (m_pSharedGameMapData)
		delete m_pSharedGameMapData;

	if (m_pSharedNPCData)
		delete m_pSharedNPCData;

	// We load our base values and such:
	m_pSharedGameDefinitions = new CGameDefinitionsShared();
	if (m_pSharedGameDefinitions)
	{
#ifdef CLIENT_DLL
		Msg("Client loaded the game base successfully!\n");
#else
		Msg("Server loaded the game base successfully!\n");
#endif
	}

	m_pSharedGameMapData = new CGameDefinitionsMapData();
	m_pSharedNPCData = new CGameDefinitionsNPC();

#ifdef CLIENT_DLL
	if (m_pMusicSystem)
		m_pMusicSystem->ParseMusicData();
#endif
}

void CGameBaseShared::Release()
{
	if (m_pSharedGameDefinitions)
		delete m_pSharedGameDefinitions;

	m_pSharedGameDefinitions = NULL;

	if (m_pSharedGameMapData)
		delete m_pSharedGameMapData;

	m_pSharedGameMapData = NULL;

	if (m_pSharedNPCData)
		delete m_pSharedNPCData;

	m_pSharedNPCData = NULL;

	if (m_pSharedQuestData)
		delete m_pSharedQuestData;

#ifdef CLIENT_DLL
	pszTemporaryInventoryList.Purge();

	if (m_pSteamServers)
		delete m_pSteamServers;

	if (m_pMusicSystem)
		delete m_pMusicSystem;
#else
	pszInventoryList.Purge();

	if (m_pAchievementManager)
		delete m_pAchievementManager;

	if (m_pServerWorkshopData)
		delete m_pServerWorkshopData;

	if (m_pPlayerLoadoutHandler)
		delete m_pPlayerLoadoutHandler;
#endif
}

void CGameBaseShared::CheckGameVersion(void)
{
	const char *szResult = "Invalid";
	KeyValues *pkvData = ReadEncryptedKVFile(filesystem, "Version", GetEncryptionKey(), true);
	if (pkvData)
	{
		szResult = ReadAndAllocStringValue(pkvData, "Game");
		pkvData->deleteThis();
	}

	Q_strncpy(pszGameVersion, szResult, 32);
}

////////////////////////////////////////////////
// Purpose:
// Decrypt and return a readable format @ a bbd file.
///////////////////////////////////////////////
KeyValues *CGameBaseShared::ReadEncryptedKeyValueFile(IFileSystem *filesystem, const char *filePath)
{
	bool bEncryption = false;

	char szFile[MAX_WEAPON_STRING];
	Q_strncpy(szFile, filePath, MAX_WEAPON_STRING);

	char szFileWithExtension[MAX_WEAPON_STRING];
	Q_snprintf(szFileWithExtension, MAX_WEAPON_STRING, "%s.txt", szFile);

	if (!filesystem->FileExists(szFileWithExtension, "MOD"))
	{
		bEncryption = true;

		Q_snprintf(szFileWithExtension, MAX_WEAPON_STRING, "%s.bbd", szFile);

		if (!filesystem->FileExists(szFileWithExtension, "MOD"))
		{
			Warning("Couldn't find the %s for .bbd or .txt!\n", szFile);
			return NULL;
		}
	}

	KeyValues *pKV = new KeyValues("BB2Data");

	// If we're not going to read an encrypted file, read a regular one.
	if (!bEncryption)
	{
		if (!pKV->LoadFromFile(filesystem, szFileWithExtension, "MOD"))
		{
			pKV->deleteThis();
			return NULL;
		}

		return pKV;
	}

	FileHandle_t f = filesystem->Open(szFileWithExtension, "rb", "MOD");
	if (!f)
	{
		pKV->deleteThis();
		return NULL;
	}

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	char *buffer = (char*)MemAllocScratch(fileSize + 1);

	Assert(buffer);

	filesystem->Read(buffer, fileSize, f); // read into local buffer
	buffer[fileSize] = 0; // null terminate file as EOF
	filesystem->Close(f);	// close file after reading

	UTIL_DecodeICE((unsigned char*)buffer, fileSize, GetEncryptionKey());

	bool retOK = pKV->LoadFromBuffer(szFileWithExtension, buffer, filesystem);

	MemFreeScratch();

	if (!retOK)
	{
		pKV->deleteThis();
		return NULL;
	}

	return pKV;
}

////////////////////////////////////////////////
// Purpose:
// Encrypt the output file. Read it into memory, encrypt and re-save it.
///////////////////////////////////////////////
void CGameBaseShared::EncryptKeyValueFile(const char *filePath, const char *fileContent)
{
	char szContent[256];
	Q_strncpy(szContent, fileContent, 256);

	FileHandle_t tempData = g_pFullFileSystem->Open(filePath, "w", "MOD");
	if (tempData != FILESYSTEM_INVALID_HANDLE)
	{
		g_pFullFileSystem->Write(szContent, strlen(szContent), tempData);
		g_pFullFileSystem->Close(tempData);
	}

	FileHandle_t f = filesystem->Open(filePath, "rb", "MOD");

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	char *buffer = (char*)MemAllocScratch(fileSize + 1);

	Assert(buffer);

	filesystem->Read(buffer, fileSize, f); // read into local buffer
	buffer[fileSize] = 0; // null terminate file as EOF
	filesystem->Close(f);	// close file after reading

	UTIL_EncryptICE((unsigned char*)buffer, fileSize, GetEncryptionKey());

	// Write our new stuff:
	FileHandle_t SaveDataFile = g_pFullFileSystem->Open(filePath, "wb");
	if (SaveDataFile != FILESYSTEM_INVALID_HANDLE)
	{
		g_pFullFileSystem->Write(buffer, fileSize, SaveDataFile);
		g_pFullFileSystem->Close(SaveDataFile);
	}

	MemFreeScratch();
}

////////////////////////////////////////////////
// Purpose:
// Get the time input returned into a formatted string. Example: 4h, 34min, 20sec....
///////////////////////////////////////////////
const char *CGameBaseShared::GetTimeString(int iHoursPlayed)
{
	if (iHoursPlayed <= 0)
		return "N/A";

	int iCurrentValue = iHoursPlayed;

	int iYears = 0;
	int iMonths = 0;
	int iWeeks = 0;
	int iDays = 0;
	int iHours = 0;

	while (iCurrentValue >= TIME_STRING_YEAR)
	{
		iCurrentValue -= TIME_STRING_YEAR;
		iYears++;
	}

	while (iCurrentValue >= TIME_STRING_MONTH)
	{
		iCurrentValue -= TIME_STRING_MONTH;
		iMonths++;
	}

	while (iCurrentValue >= TIME_STRING_WEEK)
	{
		iCurrentValue -= TIME_STRING_WEEK;
		iWeeks++;
	}

	while (iCurrentValue >= TIME_STRING_DAY)
	{
		iCurrentValue -= TIME_STRING_DAY;
		iDays++;
	}

	iHours = iCurrentValue;

#ifdef CLIENT_DLL
	return VarArgs("%iY, %iM, %iW, %iD, %iH", iYears, iMonths, iWeeks, iDays, iHours);
#else
	return UTIL_VarArgs("%iY, %iM, %iW, %iD, %iH", iYears, iMonths, iWeeks, iDays, iHours);
#endif
}

////////////////////////////////////////////////
// Purpose:
// Read a file and return its contents.
///////////////////////////////////////////////
void CGameBaseShared::GetFileContent(const char *path, char *buf, int size)
{
	Q_strncpy(buf, "", size);

	FileHandle_t f = filesystem->Open(path, "rb", "MOD");
	if (f)
	{
		int fileSize = filesystem->Size(f);
		unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 2);

		char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
		Assert(buffer);

		// read into local buffer
		bool bRetOK = (((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f) != 0);

		filesystem->Close(f);	// close file after reading

		if (bRetOK)
		{
			buffer[fileSize] = 0; // null terminate file as EOF
			buffer[fileSize + 1] = 0; // double NULL terminating in case this is a unicode file

			Q_strncpy(buf, buffer, size);
		}

		((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
	}
	else
		Warning("Unable to read file: %s\n", path);
}

////////////////////////////////////////////////
// Purpose:
// Used by firebullets in baseentity_shared. Returns the new damage to take depending on how far away the victim is from the start pos.
///////////////////////////////////////////////
float CGameBaseShared::GetDropOffDamage(const Vector &vecStart, const Vector &vecEnd, float damage, float minDist)
{
	// If min dist is zero we don't want drop off!
	if (minDist <= 0)
		return damage;

	// If the dist traveled is not longer than minDist we don't care...
	float distanceTraveled = fabs((vecEnd - vecStart).Length());
	if (distanceTraveled <= minDist)
		return damage;

	return ((minDist / distanceTraveled) * damage);
}

////////////////////////////////////////////////
// Purpose:
// Get the raw sequence duration for any activity, the default SequenceDuration function only returns the duration of an active sequence, this func returns the duration regardless of that.
///////////////////////////////////////////////
float CGameBaseShared::GetSequenceDuration(CStudioHdr *ptr, int sequence)
{
	float duration = 0.0f;

	if (ptr)
	{
		int sequences = ptr->GetNumSeq();
		for (int i = 0; i < sequences; i++)
		{
			mstudioseqdesc_t &seqdesc = ptr->pSeqdesc(i);
			mstudioanimdesc_t &animdesc = ptr->pAnimdesc(ptr->iRelativeAnim(i, seqdesc.anim(0, 0)));
			if (seqdesc.activity == sequence)
			{
				float numFrames = ((float)animdesc.numframes);
				duration = (numFrames / animdesc.fps);
				break;
			}
		}
	}

	return duration;
}

////////////////////////////////////////////////
// Purpose:
// Spawn the bleedout effect.
///////////////////////////////////////////////
void CGameBaseShared::DispatchBleedout(CBaseEntity *pEntity)
{
	if (!pEntity)
		return;

	Vector vecDown;
	AngleVectors(pEntity->GetLocalAngles(), NULL, NULL, &vecDown);
	VectorNormalize(vecDown);
	vecDown *= -1;

	Vector vecStart = pEntity->GetLocalOrigin();
	Vector vecEnd = vecStart + vecDown * MAX_TRACE_LENGTH;
	Vector mins = Vector(-60, -60, 0);
	Vector maxs = Vector(60, 60, 12);

	trace_t tr;
	CTraceFilterNoNPCsOrPlayer filter(pEntity, COLLISION_GROUP_DEBRIS);

	UTIL_TraceHull(vecStart, vecEnd, mins, maxs, MASK_SOLID_BRUSHONLY, &filter, &tr);
	if (tr.DidHitWorld() && !tr.allsolid && !tr.IsDispSurface() && (tr.fraction != 1.0f) && (tr.plane.normal.z == 1.0f))
	{
		QAngle qAngle(0, 0, 0);
		DispatchParticleEffect(GameBaseShared()->GetSharedGameDetails()->GetBleedoutParticle(), tr.endpos, qAngle, pEntity);
	}
}

////////////////////////////////////////////////
// Purpose:
// Returns details for the item. If it exists...
///////////////////////////////////////////////
KeyValues *CGameBaseShared::GetInventoryItemDetails(uint itemID, bool bIsMapItem)
{
	if (bIsMapItem)
	{
		KeyValues *pkvRet = NULL;
		char pchPath[128];
		pchPath[0] = 0;

#ifdef CLIENT_DLL
		char pchMap[128];
		Q_FileBase(engine->GetLevelName(), pchMap, 128);
		Q_snprintf(pchPath, 128, "data/maps/%s", pchMap);
#else
		if (HL2MPRules())
			Q_snprintf(pchPath, 128, "data/maps/%s", HL2MPRules()->szCurrentMap);
#endif

		KeyValues *pkvData = ReadEncryptedKeyValueFile(filesystem, pchPath);
		if (!pkvData)
			return NULL;

		KeyValues *pkvInventoryData = pkvData->FindKey("InventoryData");
		if (pkvInventoryData)
			pkvRet = pkvInventoryData->MakeCopy();

		pkvData->deleteThis();

		return pkvRet;
	}

	return ReadEncryptedKeyValueFile(filesystem, "data/game/inventory_items");
}

////////////////////////////////////////////////
// Purpose:
// Returns the keyvalue for the input id.
///////////////////////////////////////////////
KeyValues *CGameBaseShared::GetInventoryItemByID(KeyValues *pkvDetails, uint iID)
{
	char szID[32];
	Q_snprintf(szID, 32, "%u", iID);

	return pkvDetails->FindKey(szID);
}

////////////////////////////////////////////////
// Purpose:
// Helps our GUI to figure out what kind of rarity this item has.
///////////////////////////////////////////////
const char *CGameBaseShared::GetInventoryItemRarity(int iRarity)
{
	switch (iRarity)
	{
	case RARITY_COMMON:
		return "common";
	case RARITY_RARE:
		return "rare";
	case RARITY_LEGENDARY:
		return "legendary";
	default:
		return "common";
	}
}

////////////////////////////////////////////////
// Purpose:
// Helps our GUI to figure out what type of item this is.
///////////////////////////////////////////////
const char *CGameBaseShared::GetInventoryItemType(int iType)
{
	switch (iType)
	{
	case TYPE_MISC:
		return "Misc";
	case TYPE_OBJECTIVE:
		return "Quest";
	case TYPE_ATTACHMENT:
		return "Attachment";
	case TYPE_ARMOR:
		return "Equipment";
	case TYPE_SPECIAL:
		return "Unknown";
	default:
		return "N/A";
	}
}

#ifdef CLIENT_DLL
////////////////////////////////////////////////
// Purpose:
// Return the index of some inv. item.
///////////////////////////////////////////////
int CGameBaseShared::GetInventoryItemIndex(uint itemID, bool bIsMapItem)
{
	for (int i = 0; i < pszTemporaryInventoryList.Count(); i++)
	{
		if ((pszTemporaryInventoryList[i].m_iItemID == itemID) && (pszTemporaryInventoryList[i].bIsMapItem == bIsMapItem))
			return i;
	}

	return -1;
}
#else
////////////////////////////////////////////////
// Purpose:
// Add an inv. item to the deisred player.
///////////////////////////////////////////////
void CGameBaseShared::AddInventoryItem(int iPlayerIndex, uint iItemID, const char *szEntLink, bool bIsMapItem)
{
	InventoryServerItem_t pInvItem;
	pInvItem.m_iPlayerIndex = iPlayerIndex;
	pInvItem.m_iItemID = iItemID;
	pInvItem.bIsMapItem = bIsMapItem;
	Q_strncpy(pInvItem.szEntityLink, szEntLink, MAX_WEAPON_STRING);
	pszInventoryList.AddToTail(pInvItem);

	DevMsg(2, "Player %i picked up inventory item %u!\n", iPlayerIndex, iItemID);

	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(iPlayerIndex));
	if (pClient)
	{
		CSingleUserRecipientFilter filter(pClient);
		filter.MakeReliable();
		UserMessageBegin(filter, "InventoryUpdate");
		WRITE_SHORT(INV_ACTION_ADD);
		WRITE_SHORT(iItemID);
		WRITE_SHORT((bIsMapItem ? 1 : 0));
		WRITE_STRING(pInvItem.szEntityLink);
		MessageEnd();

		const DataInventoryItem_Base_t *itemData = GetSharedGameDetails()->GetInventoryData(iItemID, bIsMapItem);
		int iType = itemData ? itemData->iType : 0;
		int iSubType = itemData ? itemData->iSubType : 0;

		ComputePlayerWeight(pClient);
		if ((iType == TYPE_OBJECTIVE) && ((iSubType == TYPE_REMOVABLE_GLOW) || (iSubType == TYPE_VITAL)))
			pClient->SetGlowMode(GLOW_MODE_GLOBAL);

		if ((iItemID == 1) && (!bIsMapItem))
		{
			GetAchievementManager()->WriteToAchievement(pClient, "ACH_SURVIVOR_CAPTURE_BRIEFCASE");

			// Announce Pickup... 'hihi'
			if ((random->RandomInt(0, 2) == 1) || pClient->GetNearbyTeammates())
				HL2MPRules()->EmitSoundToClient(pClient, "PickupBriefcase", BB2_SoundTypes::TYPE_PLAYER, pClient->GetSoundsetGender());
		}
	}
}

////////////////////////////////////////////////
// Purpose:
// Tries to use the inventory item, if it succeed we call OnSuccess if not OnFailure kinda.
///////////////////////////////////////////////
bool CGameBaseShared::UseInventoryItem(int iPlayerIndex, uint iItemID, bool bIsMapItem, bool bAutoConsume, bool bDelayedUse, int forceIndex)
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(iPlayerIndex));
	if (!pClient)
		return false;

	int plrInvItemIndex = -1;
	if ((forceIndex != -1) && (forceIndex >= 0) && (forceIndex < pszInventoryList.Count()))
		plrInvItemIndex = forceIndex;
	else
		plrInvItemIndex = GetInventoryItemForPlayer(iPlayerIndex, iItemID, bIsMapItem);

	if ((plrInvItemIndex == -1) && !bAutoConsume)
		return false;

	const DataInventoryItem_Base_t *itemData = GetSharedGameDetails()->GetInventoryData(iItemID, bIsMapItem);
	if (itemData == NULL)
		return false;

	bool bShouldRemoveItem = true;
	int iType = itemData->iType;
	int iSubType = itemData->iSubType;

	//
	// We have to check which kind of item was used before we make an appropriate action.
	//
	if ((iType == TYPE_OBJECTIVE) && !bAutoConsume)
	{
		if (strlen(pszInventoryList[plrInvItemIndex].szEntityLink) <= 0)
		{
			Warning("No entity link for obj. itemID %u\n", iItemID);
			return false;
		}

		CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, pszInventoryList[plrInvItemIndex].szEntityLink);
		if (!pEntity)
		{
			Warning("Couldn't find the entity link %s\n", pszInventoryList[plrInvItemIndex].szEntityLink);
			return false;
		}

		inputdata_t inData;
		inData.pActivator = (CBaseEntity*)pClient;

		// If we're not within the required range fire the 'failure' output.
		if (pEntity->GetAbsOrigin().DistTo(pClient->GetAbsOrigin()) >= MAX_ENTITY_LINK_RANGE)
		{
			pEntity->InputFireInventoryObjectiveFail(inData);
			return false;
		}

		// We're within the X range! 
		pEntity->InputFireInventoryObjectiveSuccess(inData);

		// Check if our item should be removed now:
		if (iSubType < TYPE_REMOVABLE)
			bShouldRemoveItem = false;
	}
	else if (iType == TYPE_MISC)
	{
		if ((iSubType == TYPE_HEALTHKIT) || (iSubType == TYPE_HEALTHVIAL) ||
			(iSubType == TYPE_FOOD))
		{
			if (pClient->m_iHealth >= pClient->GetMaxHealth())
				return false;

			if (bDelayedUse)
				return false;

			float flHealthToAdd = (float)GetSharedGameDetails()->GetInventoryMiscDataValue(iSubType);
			flHealthToAdd = (((float)pClient->GetMaxHealth() / 100.0f) * flHealthToAdd);

			if (pClient->IsHuman() && (pClient->GetSkillValue(PLAYER_SKILL_HUMAN_PAINKILLER) > 0))
				flHealthToAdd += ((flHealthToAdd / 100.0f) * pClient->GetSkillValue(PLAYER_SKILL_HUMAN_PAINKILLER, TEAM_HUMANS));

			pClient->TakeHealth(flHealthToAdd, DMG_GENERIC);

			if (random->RandomInt(0, 2) == 1)
				HL2MPRules()->EmitSoundToClient(pClient, "PickupHealth", BB2_SoundTypes::TYPE_PLAYER, pClient->GetSoundsetGender());
		}
		else if ((iSubType == TYPE_ARMOR_LARGE) || (iSubType == TYPE_ARMOR_MEDIUM) || (iSubType == TYPE_ARMOR_SMALL))
		{
			int iArmorType = (iSubType - 4);

			if ((pClient->m_BB2Local.m_iActiveArmorType <= TYPE_NONE) || (bDelayedUse && (pClient->m_BB2Local.m_iActiveArmorType != iArmorType)))
				pClient->ApplyArmor(100.0f, iArmorType);
			else if ((pClient->m_BB2Local.m_iActiveArmorType == iArmorType) && (pClient->ArmorValue() < 100))
				pClient->ApplyArmor(100.0f, iArmorType);
			else
				return false;
		}
	}
	else
	{
		Warning("Item %u has an unknown sub-type!\n", iItemID);
		return false;
	}

	// Check if our item should be removed now:
	if (bShouldRemoveItem && !bAutoConsume)
	{
		CSingleUserRecipientFilter filter(pClient);
		filter.MakeReliable();
		UserMessageBegin(filter, "InventoryUpdate");
		WRITE_SHORT(INV_ACTION_REMOVE);
		WRITE_SHORT(iItemID);
		WRITE_SHORT((bIsMapItem ? 1 : 0));
		MessageEnd();

		pszInventoryList.Remove(plrInvItemIndex);
		ComputePlayerWeight(pClient);
	}

	if (!HasObjectiveGlowItems(pClient))
		pClient->SetGlowMode(GLOW_MODE_NONE);

	return true;
}

////////////////////////////////////////////////
// Purpose:
// Remove items with the linked player index, then creates them on the server. (happens when a player disconnect or drops an item...)
///////////////////////////////////////////////
void CGameBaseShared::RemoveInventoryItem(int iPlayerIndex, const Vector &vecAbsOrigin, int iType, uint iItemID, bool bDeleteItem, int forceIndex)
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(iPlayerIndex));

	bool bOnlyDoOneIteration = false;
	int i = (pszInventoryList.Count() - 1);
	if ((forceIndex != -1) && (forceIndex >= 0) && (forceIndex <= i))
	{
		i = forceIndex;
		bOnlyDoOneIteration = true;
	}

	for (; i >= 0; i--)
	{
		if (pszInventoryList[i].m_iPlayerIndex == iPlayerIndex)
		{
			if (iType != -1)
			{
				int iMapItem = (pszInventoryList[i].bIsMapItem ? 1 : 0);
				if ((iItemID != pszInventoryList[i].m_iItemID) || (iType != iMapItem))
					continue;
			}

			uint iID = pszInventoryList[i].m_iItemID;
			const char *szModel = GetSharedGameDetails()->GetInventoryItemModel(iID, pszInventoryList[i].bIsMapItem);

			if (!bDeleteItem)
			{
				CItem *pEntity = (CItem*)CreateEntityByName("inventory_item");
				if (pEntity)
				{
					int iCollisionGroup = pClient ? pClient->GetCollisionGroup() : COLLISION_GROUP_WEAPON;
					Vector vecStartPos = vecAbsOrigin + Vector(0, 0, 20);
					Vector vecEndPos = vecAbsOrigin;
					Vector vecDir = (vecEndPos - vecStartPos);
					VectorNormalize(vecDir);

					trace_t tr;
					CTraceFilterNoNPCsOrPlayer trFilter(pClient, iCollisionGroup);
					UTIL_TraceLine(vecStartPos, vecEndPos + (vecDir * MAX_TRACE_LENGTH), MASK_SHOT, &trFilter, &tr);

					Vector endPoint = tr.endpos;
					pEntity->SetLocalOrigin(endPoint);
					pEntity->SetItem(szModel, iID, pszInventoryList[i].szEntityLink, pszInventoryList[i].bIsMapItem);
					pEntity->Spawn();
		 
					const model_t *pModel = modelinfo->GetModel(pEntity->GetModelIndex());
					if (pModel)
					{
						Vector mins, maxs;
						modelinfo->GetModelBounds(pModel, mins, maxs);
						endPoint.z += maxs.z + 4;
					}
					pEntity->SetLocalOrigin(endPoint);
				}
			}

			if (pClient)
			{
				CSingleUserRecipientFilter filter(pClient);
				filter.MakeReliable();
				UserMessageBegin(filter, "InventoryUpdate");
				WRITE_SHORT(INV_ACTION_REMOVE);
				WRITE_SHORT(iID);
				WRITE_SHORT((pszInventoryList[i].bIsMapItem ? 1 : 0));
				MessageEnd();
			}

			pszInventoryList.Remove(i);
			ComputePlayerWeight(pClient);

			// We only want to remove 1 item at a time if Id is above 0.
			if ((iItemID > 0) || bOnlyDoOneIteration)
				break;
		}
	}

	if (!pClient)
		return;

	if (!HasObjectiveGlowItems(pClient))
		pClient->SetGlowMode(GLOW_MODE_NONE);
}

////////////////////////////////////////////////
// Purpose:
// Remove all inventory items on the client & server.
///////////////////////////////////////////////
void CGameBaseShared::RemoveInventoryItems(void)
{
	pszInventoryList.Purge();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient)
			continue;

		CSingleUserRecipientFilter filter(pClient);
		filter.MakeReliable();
		UserMessageBegin(filter, "InventoryUpdate");
		WRITE_SHORT(INV_ACTION_PURGE);
		MessageEnd();
	}
}

////////////////////////////////////////////////
// Purpose:
// Get the amount of items for the desired player.
///////////////////////////////////////////////
int CGameBaseShared::GetInventoryItemCountForPlayer(int iPlayerIndex)
{
	int iCount = 0;
	for (int i = (pszInventoryList.Count() - 1); i >= 0; i--)
	{
		if (pszInventoryList[i].m_iPlayerIndex == iPlayerIndex)
			iCount++;
	}

	return iCount;
}

////////////////////////////////////////////////
// Purpose:
// Get the inventory item index inside the inv. list for the desired player.
///////////////////////////////////////////////
int CGameBaseShared::GetInventoryItemForPlayer(int iPlayerIndex, uint iItemID, bool bIsMapItem)
{
	for (int i = (pszInventoryList.Count() - 1); i >= 0; i--)
	{
		if ((pszInventoryList[i].m_iPlayerIndex == iPlayerIndex) && (pszInventoryList[i].m_iItemID == iItemID) && (pszInventoryList[i].bIsMapItem == bIsMapItem))
			return i;
	}

	return -1;
}

////////////////////////////////////////////////
// Purpose:
// Does this user have any vital / rem _ glow items?
///////////////////////////////////////////////
bool CGameBaseShared::HasObjectiveGlowItems(CHL2MP_Player *pClient)
{
	if (!pClient)
		return false;

	bool bFound = false;
	for (int i = (pszInventoryList.Count() - 1); i >= 0; i--)
	{
		if (pszInventoryList[i].m_iPlayerIndex == pClient->entindex())
		{
			const DataInventoryItem_Base_t *itemData = GetSharedGameDetails()->GetInventoryData(pszInventoryList[i].m_iItemID, pszInventoryList[i].bIsMapItem);
			if (!itemData)
				continue;

			if ((itemData->iType == TYPE_OBJECTIVE) && ((itemData->iSubType == TYPE_REMOVABLE_GLOW) || (itemData->iSubType == TYPE_VITAL)))
			{
				bFound = true;
				break;
			}
		}
	}

	return bFound;
}

////////////////////////////////////////////////
// Purpose:
// Called when the player kills any entity.
// Achievement progressing.
///////////////////////////////////////////////
void CGameBaseShared::EntityKilledByPlayer(CBaseEntity *pKiller, CBaseEntity *pVictim, CBaseEntity *pInflictor)
{
	if (!pKiller || !pVictim || !pInflictor)
		return;

	if (!pKiller->IsPlayer())
		return;

	CHL2MP_Player *pClient = ToHL2MPPlayer(pKiller);
	if (!pClient)
		return;

	if (pClient->IsBot())
		return;

	if (!GetAchievementManager())
		return;

	if (pClient->GetTeamNumber() == TEAM_HUMANS)
	{
		if (FClassnameIs(pVictim, "npc_walker") || FClassnameIs(pVictim, "npc_runner"))
		{
			GetAchievementManager()->WriteToAchievement(pClient, "ACH_ZOMBIE_FIRST_BLOOD");
			GetAchievementManager()->WriteToStat(pClient, "BBX_KI_ZOMBIES");
		}
		else if (FClassnameIs(pVictim, "npc_fred"))
		{
			GetAchievementManager()->WriteToAchievement(pClient, "ACH_SURVIVOR_KILL_FRED");
		}
		else if (pVictim->IsZombie())
		{
			// TODO...
		}

		CBaseCombatWeapon *pWeapon = pClient->GetActiveWeapon();
		if (pWeapon && FClassnameIs(pInflictor, "player"))
		{
			if (FClassnameIs(pWeapon, "weapon_beretta"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_BERETTA");
			else if (FClassnameIs(pWeapon, "weapon_ak47"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_AK74");
			else if (FClassnameIs(pWeapon, "weapon_g36c"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_G36C");
			else if (FClassnameIs(pWeapon, "weapon_m9_bayonet"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_M9PHROBIS");
			else if (FClassnameIs(pWeapon, "weapon_fireaxe"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_FIREAXE");
			else if (FClassnameIs(pWeapon, "weapon_remington"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_870");
			else if (FClassnameIs(pWeapon, "weapon_sawedoff"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_SAWOFF");
			else if (FClassnameIs(pWeapon, "weapon_hands"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_FISTS");
			else if (FClassnameIs(pWeapon, "weapon_machete"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_MACHETE");
			else if (FClassnameIs(pWeapon, "weapon_baseballbat"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_BASEBALLBAT");
			else if (FClassnameIs(pWeapon, "weapon_famas"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_FAMAS");
			else if (FClassnameIs(pWeapon, "weapon_sledgehammer"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_SLEDGEHAMMER");
			else if (FClassnameIs(pWeapon, "weapon_hatchet"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_HATCHET");
			else if (FClassnameIs(pWeapon, "weapon_minigun"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_MINIGUN");
			else if (FClassnameIs(pWeapon, "weapon_mp7"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_HKMP7");
			else if (FClassnameIs(pWeapon, "weapon_mac11"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_MAC11");
			else if (FClassnameIs(pWeapon, "weapon_microuzi"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_UZI");
			else if (FClassnameIs(pWeapon, "weapon_glock17"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_GLOCK17");
			else if (FClassnameIs(pWeapon, "weapon_remington700"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_REM700");
			else if (FClassnameIs(pWeapon, "weapon_winchester1894"))
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_TRAPPER");

			if (pWeapon->IsAkimboWeapon())
				GetAchievementManager()->WriteToStat(pClient, "BBX_KI_AKIMBO");
		}
		else if (FClassnameIs(pInflictor, "npc_grenade_frag"))
			GetAchievementManager()->WriteToStat(pClient, "BBX_KI_EXPLOSIVES");
		else if (FClassnameIs(pInflictor, "prop_propane_explosive"))
		{
			GetAchievementManager()->WriteToAchievement(pClient, "ACH_GM_SURVIVAL_PROPANE");
			GetAchievementManager()->WriteToStat(pClient, "BBX_KI_EXPLOSIVES");
		}
		else if (FClassnameIs(pInflictor, "prop_thrown_brick"))
			GetAchievementManager()->WriteToAchievement(pClient, "ACH_WEP_BRICK");

		if (!pClient->GetPerkFlags())
			pClient->m_iNumPerkKills++;
	}
	else
	{
		if (pVictim->IsHuman())
			GetAchievementManager()->WriteToAchievement(pClient, "ACH_ZOMBIE_KILL_HUMAN");
	}

	GetAchievementManager()->WriteToStat(pClient, "BBX_ST_KILLS");
}

////////////////////////////////////////////////
// Purpose:
// The game is over, we're changing map! Give achievs?
///////////////////////////////////////////////
void CGameBaseShared::OnGameOver(float timeLeft, int iWinner)
{
	if ((GameBaseServer()->CanStoreSkills() != PROFILE_GLOBAL) || (iWinner != TEAM_HUMANS))
		return;

	int iPlayersInGame = (HL2MPRules()->GetTeamSize(TEAM_HUMANS) + HL2MPRules()->GetTeamSize(TEAM_DECEASED));
	const char *currMap = HL2MPRules()->szCurrentMap;
	bool bTimeOut = (timeLeft <= 0.0f);
	bool bCanGiveMapAchiev = (!bTimeOut && ((iPlayersInGame >= 4)));

	const char *pAchievement = NULL;
	if (bCanGiveMapAchiev)
	{
		if (!strcmp(currMap, "bbc_laststand"))
			pAchievement = "ACH_MAP_LASTSTAND";
		else if (!strcmp(currMap, "bbc_termoil"))
			pAchievement = "ACH_MAP_TERMOIL";
		else if (!strcmp(currMap, "bba_rooftop"))
			pAchievement = "ACH_MAP_ROOFTOP";
		else if (!strcmp(currMap, "bba_colosseum"))
			pAchievement = "ACH_MAP_COLOSSEUM";
		else if (!strcmp(currMap, "bba_cargo"))
			pAchievement = "ACH_MAP_CARGO";
		else if (!strcmp(currMap, "bba_barracks"))
			pAchievement = "ACH_MAP_BARRACKS_ARENA";
		else if (!strcmp(currMap, "bba_devilscrypt"))
			pAchievement = "ACH_MAP_DEVILSCRYPT";
		else if (!strcmp(currMap, "bba_swamptrouble"))
			pAchievement = "ACH_MAP_SWAMPTROUBLE";
		else if (!strcmp(currMap, "bba_salvage"))
			pAchievement = "ACH_MAP_SALVAGE";
	}

	if (GetAchievementManager() && !bTimeOut)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
			if (!pPlayer)
				continue;

			if (pPlayer->IsBot() || !pPlayer->HasFullySpawned())
				continue;

			if (pAchievement != NULL)
				GetAchievementManager()->WriteToAchievement(pPlayer, pAchievement);

			const char *pSpecialAchievement = NULL;
			if (!pPlayer->HasPlayerUsedFirearm() && (pPlayer->GetTotalScore() > 0))
			{
				if (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE)
					pSpecialAchievement = "ACH_OBJ_NOFIREARMS";
				else if (HL2MPRules()->GetCurrentGamemode() == MODE_ARENA)
					pSpecialAchievement = "ACH_ENDGAME_NOFIREARMS";
			}

			if (pSpecialAchievement == NULL)
				continue;

			GetAchievementManager()->WriteToAchievement(pPlayer, pSpecialAchievement);
		}
	}
}

////////////////////////////////////////////////
// Purpose:
// Handle client and server commands.
///////////////////////////////////////////////
bool CGameBaseShared::ClientCommand(const CCommand &args)
{
	return false;
}

////////////////////////////////////////////////
// Purpose:
// Notify everyone that we have a new player in our game!
///////////////////////////////////////////////
void CGameBaseShared::NewPlayerConnection(bool bState)
{
	IGameEvent *event = gameeventmanager->CreateEvent("player_connection");
	if (event)
	{
		event->SetBool("state", bState); // False - Connected, True - Disconnected.
		gameeventmanager->FireEvent(event);
	}
}

void CGameBaseShared::ComputePlayerWeight(CHL2MP_Player *pPlayer)
{
	if (!pPlayer)
		return;

	float m_flWeight = 0.0f;

	for (int i = 0; i < pszInventoryList.Count(); i++)
	{
		if (pszInventoryList[i].m_iPlayerIndex != pPlayer->entindex())
			continue;

		const DataInventoryItem_Base_t *itemData = GetSharedGameDetails()->GetInventoryData(pszInventoryList[i].m_iItemID, pszInventoryList[i].bIsMapItem);
		if (!itemData)
			continue;

		m_flWeight += ((float)itemData->iWeight);
	}

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if (!pWeapon)
			continue;

		if ((pWeapon->GetSlot() >= MAX_WEAPON_SLOTS) || !pWeapon->VisibleInWeaponSelection())
			continue;

		m_flWeight += pWeapon->GetWpnData().m_flPhysicalWeight;
	}

	if (pPlayer->m_BB2Local.m_iActiveArmorType > 0)
		m_flWeight += (float)GameBaseShared()->GetSharedGameDetails()->GetInventoryArmorDataValue("weight", pPlayer->m_BB2Local.m_iActiveArmorType);

	if (pPlayer->IsHuman() && (pPlayer->GetSkillValue(PLAYER_SKILL_HUMAN_LIGHTWEIGHT) > 0))
		m_flWeight -= ((m_flWeight / 100) * pPlayer->GetSkillValue(PLAYER_SKILL_HUMAN_LIGHTWEIGHT, TEAM_HUMANS));

	if ((m_flWeight <= 0) || (pPlayer->GetTeamNumber() == TEAM_DECEASED))
		m_flWeight = 0.0f;

	pPlayer->m_BB2Local.m_flCarryWeight = m_flWeight;
}
#endif

#ifndef CLIENT_DLL
CON_COMMAND(reload_gamebase_server, "Reload the game base content.(full reparse)")
{
	if (!sv_cheats)
		return;

	if (!sv_cheats->GetBool())
		return;

	GameBaseShared()->LoadBase();

	GameBaseShared()->GetSharedGameDetails()->Precache();
	GameBaseShared()->GetNPCData()->Precache();
	GameBaseShared()->GetSharedMapData()->ReloadDataForMap();

	LoadSharedData();
};

CON_COMMAND_F(bb2_give_armor_type, "Give Temp Armor Type", FCVAR_CHEAT)
{
	if (args.ArgC() != 2)
		return;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient)
			continue;

		if (pClient->m_BB2Local.m_iActiveArmorType)
			pClient->ApplyArmor(0, pClient->m_BB2Local.m_iActiveArmorType);

		pClient->ApplyArmor(100.0f, atoi(args[1]));
	}
};
#else
CON_COMMAND(reload_gamebase_client, "Reload the game base content.(full reparse)")
{
	ConVarRef cheats("sv_cheats");

	if (!cheats.GetBool())
		return;

	GameBaseShared()->LoadBase();

	GameBaseShared()->GetSharedGameDetails()->Precache();
	GameBaseShared()->GetNPCData()->Precache();
	GameBaseShared()->GetSharedMapData()->ReloadDataForMap();
};
#endif
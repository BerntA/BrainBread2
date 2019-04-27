//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: NPC Data Handler Class - Stores Info about npcs (precaches) at server startup.
//
//========================================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "GameDefinitions_NPC.h"
#include "GameBase_Shared.h"
#include "gibs_shared.h"

CGameDefinitionsNPC::CGameDefinitionsNPC()
{
	LoadNPCData();
}

CGameDefinitionsNPC::~CGameDefinitionsNPC()
{
	Cleanup();
	CleanupOverrideData();
}

void CGameDefinitionsNPC::Cleanup(void)
{
	for (int i = (pszNPCItems.Count() - 1); i >= 0; --i)
	{
		if (pszNPCItems[i])
			delete pszNPCItems[i];
	}

	pszNPCItems.Purge();
}

void CGameDefinitionsNPC::CleanupOverrideData(void)
{
	for (int i = (m_pModelOverrideItems.Count() - 1); i >= 0; --i)
	{
		if (m_pModelOverrideItems[i])
			delete m_pModelOverrideItems[i];
	}

	m_pModelOverrideItems.Purge();
}

bool CGameDefinitionsNPC::LoadNPCData(void)
{
	Cleanup();

	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx("data/npc/*.*", "MOD", &findHandle);
	while (pFilename)
	{
		if (strlen(pFilename) > 4)
		{
			char npcName[MAX_MAP_NAME_SAVE];
			Q_strncpy(npcName, pFilename, MAX_MAP_NAME_SAVE);
			npcName[strlen(npcName) - 4] = 0; // strip the file extension!

			char filePath[MAX_WEAPON_STRING];
			Q_snprintf(filePath, MAX_WEAPON_STRING, "data/npc/%s", pFilename);
			filePath[strlen(filePath) - 4] = 0; // null terminate here so we remove the file extension part.

			KeyValues *npcData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, filePath);
			if (npcData)
			{
				CNPCDataItem *npcItem = new CNPCDataItem();
				Q_strncpy(npcItem->szNPCName, npcName, MAX_MAP_NAME_SAVE);

				npcItem->iHealthMin = npcData->GetInt("HealthMin", 100);
				npcItem->iHealthMax = npcData->GetInt("HealthMax", 100);

				npcItem->iSlashDamageMin = npcData->GetInt("DamageSingleMin", 1);
				npcItem->iSlashDamageMax = npcData->GetInt("DamageSingleMax", 1);

				npcItem->iDoubleSlashDamageMin = npcData->GetInt("DamageBothMin", 1);
				npcItem->iDoubleSlashDamageMax = npcData->GetInt("DamageBothMax", 1);

				npcItem->flSpeedFactorMin = npcData->GetFloat("SpeedMin", 1.0f);
				npcItem->flSpeedFactorMax = npcData->GetFloat("SpeedMax", 1.0f);

				npcItem->iKickDamageMin = npcData->GetInt("KickDamageMin");
				npcItem->iKickDamageMax = npcData->GetInt("KickDamageMax");

				npcItem->iXP = npcData->GetInt("XP");
				npcItem->flHealthScale = npcData->GetFloat("HealthScale", 10.0f);
				npcItem->flDamageScale = npcData->GetFloat("DamageScale", 10.0f);
				npcItem->flRange = npcData->GetFloat("Range", 50.0f);

				KeyValues *pkvMdl = npcData->FindKey("Models");
				if (pkvMdl)
				{
					for (KeyValues *sub = pkvMdl->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						NPCModelItem_t modelItem;
						Q_strncpy(modelItem.szModelPath, sub->GetName(), MAX_WEAPON_STRING);

						// GIB Info:
						Q_strncpy(modelItem.szGibHead, sub->GetString("head"), MAX_WEAPON_STRING);
						Q_strncpy(modelItem.szGibArmRight, sub->GetString("arms_right"), MAX_WEAPON_STRING);
						Q_strncpy(modelItem.szGibArmLeft, sub->GetString("arms_left"), MAX_WEAPON_STRING);
						Q_strncpy(modelItem.szGibLegRight, sub->GetString("legs_right"), MAX_WEAPON_STRING);
						Q_strncpy(modelItem.szGibLegLeft, sub->GetString("legs_left"), MAX_WEAPON_STRING);

						modelItem.iSkinMax = sub->GetInt("skin_max");
						modelItem.iSkinMin = sub->GetInt("skin_min");

						npcItem->pszModelList.AddToTail(modelItem);
					}
				}

				KeyValues *pkvWeapons = npcData->FindKey("WeaponDefinitions");
				if (pkvWeapons)
				{
					for (KeyValues *sub = pkvWeapons->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						NPCWeaponItem_t weaponItem;
						Q_strncpy(weaponItem.szWeaponClass, sub->GetName(), 32);
						
						if (sub->FindKey("damage"))
							weaponItem.flDamageMin = weaponItem.flDamageMax = MAX(sub->GetFloat("damage"), 0.1f);
						else if (sub->FindKey("damage_min") && sub->FindKey("damage_max"))
						{
							weaponItem.flDamageMin = MAX(sub->GetFloat("damage_min"), 0.1f);
							weaponItem.flDamageMax = MAX(sub->GetFloat("damage_max"), 0.1f);
						}
						else
							weaponItem.flDamageMin = weaponItem.flDamageMax = 1.0f;

						weaponItem.flDamageScale[DAMAGE_SCALE_TO_PLAYER] = sub->GetFloat("scale_player", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_PLAYER_ZOMBIE] = sub->GetFloat("scale_player_zombie", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_ZOMBIES] = sub->GetFloat("scale_npc_zombies", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_HUMANS] = sub->GetFloat("scale_npc_humans", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_ZOMBIE_BOSSES] = sub->GetFloat("scale_npc_zombie_bosses", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_HUMAN_BOSSES] = sub->GetFloat("scale_npc_human_bosses", 1.0f);
						npcItem->pszWeaponList.AddToTail(weaponItem);
					}
				}

				KeyValues *pkvLimbs = npcData->FindKey("LimbData");
				if (pkvLimbs)
				{
					for (KeyValues *sub = pkvLimbs->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						NPCLimbItem_t limbItem;
						Q_strncpy(limbItem.szLimb, sub->GetName(), 32);
						limbItem.flScale = sub->GetFloat("scale", 1.0f);
						limbItem.flHealth = sub->GetFloat("health", 10.0f);
						npcItem->pszLimbList.AddToTail(limbItem);
					}
				}

				pszNPCItems.AddToTail(npcItem);

				npcData->deleteThis();
			}
		}

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);

	return true;
}

bool CGameDefinitionsNPC::Precache(void)
{
#ifdef CLIENT_DLL
	if (!engine->IsInGame())
		return false;
#endif

	for (int i = 0; i < pszNPCItems.Count(); i++)
	{
		for (int modelItems = 0; modelItems < pszNPCItems[i]->pszModelList.Count(); modelItems++)
		{
			CBaseAnimating::PrecacheModel(pszNPCItems[i]->pszModelList[modelItems].szModelPath);

			// Check if there's any gibs:
			if (pszNPCItems[i]->pszModelList[modelItems].szGibHead && pszNPCItems[i]->pszModelList[modelItems].szGibHead[0])
				CBaseAnimating::PrecacheModel(pszNPCItems[i]->pszModelList[modelItems].szGibHead);

			if (pszNPCItems[i]->pszModelList[modelItems].szGibArmLeft && pszNPCItems[i]->pszModelList[modelItems].szGibArmLeft[0])
				CBaseAnimating::PrecacheModel(pszNPCItems[i]->pszModelList[modelItems].szGibArmLeft);

			if (pszNPCItems[i]->pszModelList[modelItems].szGibArmRight && pszNPCItems[i]->pszModelList[modelItems].szGibArmRight[0])
				CBaseAnimating::PrecacheModel(pszNPCItems[i]->pszModelList[modelItems].szGibArmRight);

			if (pszNPCItems[i]->pszModelList[modelItems].szGibLegLeft && pszNPCItems[i]->pszModelList[modelItems].szGibLegLeft[0])
				CBaseAnimating::PrecacheModel(pszNPCItems[i]->pszModelList[modelItems].szGibLegLeft);

			if (pszNPCItems[i]->pszModelList[modelItems].szGibLegRight && pszNPCItems[i]->pszModelList[modelItems].szGibLegRight[0])
				CBaseAnimating::PrecacheModel(pszNPCItems[i]->pszModelList[modelItems].szGibLegRight);
		}
	}

	for (int i = 0; i < m_pModelOverrideItems.Count(); i++)
	{
		for (int modelItems = 0; modelItems < m_pModelOverrideItems[i]->pszModelList.Count(); modelItems++)
		{
			CBaseAnimating::PrecacheModel(m_pModelOverrideItems[i]->pszModelList[modelItems].szModelPath);

			// Check if there's any gibs:
			if (m_pModelOverrideItems[i]->pszModelList[modelItems].szGibHead && m_pModelOverrideItems[i]->pszModelList[modelItems].szGibHead[0])
				CBaseAnimating::PrecacheModel(m_pModelOverrideItems[i]->pszModelList[modelItems].szGibHead);

			if (m_pModelOverrideItems[i]->pszModelList[modelItems].szGibArmLeft && m_pModelOverrideItems[i]->pszModelList[modelItems].szGibArmLeft[0])
				CBaseAnimating::PrecacheModel(m_pModelOverrideItems[i]->pszModelList[modelItems].szGibArmLeft);

			if (m_pModelOverrideItems[i]->pszModelList[modelItems].szGibArmRight && m_pModelOverrideItems[i]->pszModelList[modelItems].szGibArmRight[0])
				CBaseAnimating::PrecacheModel(m_pModelOverrideItems[i]->pszModelList[modelItems].szGibArmRight);

			if (m_pModelOverrideItems[i]->pszModelList[modelItems].szGibLegLeft && m_pModelOverrideItems[i]->pszModelList[modelItems].szGibLegLeft[0])
				CBaseAnimating::PrecacheModel(m_pModelOverrideItems[i]->pszModelList[modelItems].szGibLegLeft);

			if (m_pModelOverrideItems[i]->pszModelList[modelItems].szGibLegRight && m_pModelOverrideItems[i]->pszModelList[modelItems].szGibLegRight[0])
				CBaseAnimating::PrecacheModel(m_pModelOverrideItems[i]->pszModelList[modelItems].szGibLegRight);
		}
	}

	return true;
}

int CGameDefinitionsNPC::GetIndex(const char *name)
{
	for (int i = 0; i < pszNPCItems.Count(); ++i)
	{
		if (!strcmp(pszNPCItems[i]->szNPCName, name))
			return i;
	}

	return -1;
}

CNPCDataItem *CGameDefinitionsNPC::GetNPCData(const char *name)
{
	int index = GetIndex(name);
	if (index != -1)
		return pszNPCItems[index];

	return NULL;
}

int CGameDefinitionsNPC::GetSkin(const char *name, const char *model)
{
	int index = GetIndex(name);
	if (index == -1)
		return 0;

	int overrideIndex = GetOverridedModelIndexForNPC(name);
	if (overrideIndex != -1)
	{
		for (int i = 0; i < m_pModelOverrideItems[overrideIndex]->pszModelList.Count(); i++)
		{
			if (!strcmp(model, m_pModelOverrideItems[overrideIndex]->pszModelList[i].szModelPath))
				return (random->RandomInt(m_pModelOverrideItems[overrideIndex]->pszModelList[i].iSkinMin, m_pModelOverrideItems[overrideIndex]->pszModelList[i].iSkinMax));
		}

		return 0;
	}

	for (int i = 0; i < pszNPCItems[index]->pszModelList.Count(); i++)
	{
		if (!strcmp(model, pszNPCItems[index]->pszModelList[i].szModelPath))
			return (random->RandomInt(pszNPCItems[index]->pszModelList[i].iSkinMin, pszNPCItems[index]->pszModelList[i].iSkinMax));
	}

	return 0;
}

float CGameDefinitionsNPC::GetFirearmDamage(const char *name, const char *weapon)
{
	float flDamage = 1.0f;

	int index = GetIndex(name);
	if (index != -1)
	{
		for (int i = 0; i < pszNPCItems[index]->pszWeaponList.Count(); i++)
		{
			if (!strcmp(weapon, pszNPCItems[index]->pszWeaponList[i].szWeaponClass))
				return (random->RandomFloat(pszNPCItems[index]->pszWeaponList[i].flDamageMin, pszNPCItems[index]->pszWeaponList[i].flDamageMax));
		}
	}

	return flDamage;
}

float CGameDefinitionsNPC::GetFirearmDamageScale(const char *name, const char *weapon, int entityType)
{
	float flScale = 1.0f;

	int index = GetIndex(name);
	if (index != -1)
	{
		for (int i = 0; i < pszNPCItems[index]->pszWeaponList.Count(); i++)
		{
			if ((entityType >= 0 && entityType < NUM_DAMAGE_SCALES) && !strcmp(weapon, pszNPCItems[index]->pszWeaponList[i].szWeaponClass))
				return pszNPCItems[index]->pszWeaponList[i].flDamageScale[entityType];
		}
	}

	return flScale;
}

float CGameDefinitionsNPC::GetLimbData(const char *name, const char *limb, bool bHealth)
{
	float flValue = ((bHealth) ? 10.0f : 1.0f);

	int index = GetIndex(name);
	if (index != -1)
	{
		for (int i = 0; i < pszNPCItems[index]->pszLimbList.Count(); i++)
		{
			if (!strcmp(limb, pszNPCItems[index]->pszLimbList[i].szLimb))
				return (bHealth ? pszNPCItems[index]->pszLimbList[i].flHealth : pszNPCItems[index]->pszLimbList[i].flScale);
		}
	}

	return flValue;
}

const char *CGameDefinitionsNPC::GetModel(const char *name)
{
	int index = GetIndex(name);
	if (index == -1)
		return "";

	int overrideIndex = GetOverridedModelIndexForNPC(name);
	if (overrideIndex != -1)
		return (m_pModelOverrideItems[overrideIndex]->pszModelList[random->RandomInt(0, (m_pModelOverrideItems[overrideIndex]->pszModelList.Count() - 1))].szModelPath);

	return (pszNPCItems[index]->pszModelList[random->RandomInt(0, (pszNPCItems[index]->pszModelList.Count() - 1))].szModelPath);
}

const char *CGameDefinitionsNPC::GetModelForGib(const char *name, const char *gib, const char *model)
{
	int index = GetIndex(name);
	if (index == -1)
		return "";

	NPCModelItem_t *item = NULL;
	int overrideIndex = GetOverridedModelIndexForNPC(name);
	if (overrideIndex == -1)
	{
		for (int i = 0; i < pszNPCItems[index]->pszModelList.Count(); i++)
		{
			if (!strcmp(model, pszNPCItems[index]->pszModelList[i].szModelPath))
			{
				item = &pszNPCItems[index]->pszModelList[i];
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < m_pModelOverrideItems[overrideIndex]->pszModelList.Count(); i++)
		{
			if (!strcmp(model, m_pModelOverrideItems[overrideIndex]->pszModelList[i].szModelPath))
			{
				item = &m_pModelOverrideItems[overrideIndex]->pszModelList[i];
				break;
			}
		}
	}

	if (item != NULL)
	{
		if (!strcmp(gib, "head"))
			return item->szGibHead;
		else if (!strcmp(gib, "arms_left"))
			return item->szGibArmLeft;
		else if (!strcmp(gib, "arms_right"))
			return item->szGibArmRight;
		else if (!strcmp(gib, "legs_left"))
			return item->szGibLegLeft;
		else if (!strcmp(gib, "legs_right"))
			return item->szGibLegRight;
	}

	return "";
}

bool CGameDefinitionsNPC::DoesNPCExist(const char *name)
{
	return (GetIndex(name) != -1);
}

bool CGameDefinitionsNPC::DoesNPCHaveGibForLimb(const char *name, const char *model, int gibID)
{
	int index = GetIndex(name);
	if (index == -1)
		return false;

	NPCModelItem_t *item = NULL;
	int overrideIndex = GetOverridedModelIndexForNPC(name);
	if (overrideIndex == -1)
	{
		for (int i = 0; i < pszNPCItems[index]->pszModelList.Count(); i++)
		{
			if (!strcmp(model, pszNPCItems[index]->pszModelList[i].szModelPath))
			{
				item = &pszNPCItems[index]->pszModelList[i];
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < m_pModelOverrideItems[overrideIndex]->pszModelList.Count(); i++)
		{
			if (!strcmp(model, m_pModelOverrideItems[overrideIndex]->pszModelList[i].szModelPath))
			{
				item = &m_pModelOverrideItems[overrideIndex]->pszModelList[i];
				break;
			}
		}
	}

	if (item != NULL)
	{
		if ((gibID == GIB_NO_HEAD) && item->szGibHead && item->szGibHead[0])
			return true;

		if ((gibID == GIB_NO_ARM_LEFT) && item->szGibArmLeft && item->szGibArmLeft[0])
			return true;

		if ((gibID == GIB_NO_ARM_RIGHT) && item->szGibArmRight && item->szGibArmRight[0])
			return true;

		if ((gibID == GIB_NO_LEG_LEFT) && item->szGibLegLeft && item->szGibLegLeft[0])
			return true;

		if ((gibID == GIB_NO_LEG_RIGHT) && item->szGibLegRight && item->szGibLegRight[0])
			return true;
	}

	return false;
}

void CGameDefinitionsNPC::LoadNPCOverrideData(KeyValues *pkvData)
{
	if (pkvData)
	{
		for (KeyValues *sub = pkvData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			CNPCOverrideModelData *overrideItem = new CNPCOverrideModelData(sub->GetName());
			for (KeyValues *model = sub->GetFirstSubKey(); model; model = model->GetNextKey())
			{
				NPCModelItem_t modelItem;
				Q_strncpy(modelItem.szModelPath, model->GetName(), MAX_WEAPON_STRING);

				// GIB Info:
				Q_strncpy(modelItem.szGibHead, model->GetString("head"), MAX_WEAPON_STRING);
				Q_strncpy(modelItem.szGibArmRight, model->GetString("arms_right"), MAX_WEAPON_STRING);
				Q_strncpy(modelItem.szGibArmLeft, model->GetString("arms_left"), MAX_WEAPON_STRING);
				Q_strncpy(modelItem.szGibLegRight, model->GetString("legs_right"), MAX_WEAPON_STRING);
				Q_strncpy(modelItem.szGibLegLeft, model->GetString("legs_left"), MAX_WEAPON_STRING);

				modelItem.iSkinMax = model->GetInt("skin_max");
				modelItem.iSkinMin = model->GetInt("skin_min");

				overrideItem->pszModelList.AddToTail(modelItem);
			}

			m_pModelOverrideItems.AddToTail(overrideItem);
		}
	}
}

int CGameDefinitionsNPC::GetOverridedModelIndexForNPC(const char *name)
{
	for (int i = 0; i < m_pModelOverrideItems.Count(); ++i)
	{
		if (!strcmp(m_pModelOverrideItems[i]->szNPCName, name))
			return i;
	}

	return -1;
}

const CNPCOverrideModelData *CGameDefinitionsNPC::GetOverridedModelDataForNPC(const char *name)
{
	int index = GetOverridedModelIndexForNPC(name);
	if (index == -1)
		return NULL;

	return m_pModelOverrideItems[index];
}

const NPCModelItem_t *CNPCDataItem::GetModelItem(int index)
{
	const CNPCOverrideModelData *overrideData = GameBaseShared()->GetNPCData()->GetOverridedModelDataForNPC(szNPCName);

	int mdlCount = overrideData ? overrideData->pszModelList.Count() : pszModelList.Count();
	if (mdlCount <= 0)
		return NULL;

	int actualIndex = index;
	if (actualIndex == -1)
		actualIndex = random->RandomInt(0, (mdlCount - 1));

	if (actualIndex < 0 || actualIndex >= mdlCount)
		return NULL;

	return (overrideData ? &overrideData->pszModelList[actualIndex] : &pszModelList[actualIndex]);
}
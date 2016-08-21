//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: NPC Data Handler Class - Stores Info about npcs (precaches) at server startup.
//
//========================================================================================//

#ifndef GAME_DEFINITIONS_NPC_H
#define GAME_DEFINITIONS_NPC_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "GameDefinitions_Shared.h"

// Everything here is original values we need when we scale up or down the npcs depending on the player count in the server! We also use these when npcs spawn.
struct NPCModelItem_t
{
	char szModelPath[MAX_WEAPON_STRING];
	// GIBS:
	char szGibHead[MAX_WEAPON_STRING];
	char szGibArmLeft[MAX_WEAPON_STRING];
	char szGibArmRight[MAX_WEAPON_STRING];
	char szGibLegLeft[MAX_WEAPON_STRING];
	char szGibLegRight[MAX_WEAPON_STRING];

	int iSkinMax;
	int iSkinMin;
};

struct NPCWeaponItem_t
{
	char szWeaponClass[32];
	float flDamage;
	float flDamageScale[NUM_DAMAGE_SCALES];
};

struct NPCLimbItem_t
{
	char szLimb[32];
	float flScale;
	float flHealth;
};

class CNPCDataItem
{
public:

	CNPCDataItem()
	{
		pszModelList.Purge();
		pszWeaponList.Purge();
		pszLimbList.Purge();
	}

	~CNPCDataItem()
	{
		pszModelList.Purge();
		pszWeaponList.Purge();
		pszLimbList.Purge();
	}

	char szNPCName[MAX_MAP_NAME_SAVE];
	int iHealth;
	int iSlashDamage;
	int iDoubleSlashDamage;
	int iKickDamage;
	int iXP;
	float flHealthScale;
	float flDamageScale;
	float flRange;
	CUtlVector<NPCModelItem_t> pszModelList;
	CUtlVector<NPCWeaponItem_t> pszWeaponList;
	CUtlVector<NPCLimbItem_t> pszLimbList;
};

class CNPCOverrideModelData
{
public:

	CNPCOverrideModelData(const char *name)
	{
		Q_strncpy(szNPCName, name, MAX_MAP_NAME_SAVE);
		pszModelList.Purge();
	}

	~CNPCOverrideModelData()
	{
		pszModelList.Purge();
	}

	char szNPCName[MAX_MAP_NAME_SAVE];
	CUtlVector<NPCModelItem_t> pszModelList;
};

class CGameDefinitionsNPC
{
public:
	CGameDefinitionsNPC();
	~CGameDefinitionsNPC();

	void Cleanup(void);
	void CleanupOverrideData(void);
	bool LoadNPCData(void);
	bool Precache(void);

	int GetHealth(const char *name);
	int GetSlashDamage(const char *name);
	int GetDoubleSlashDamage(const char *name);
	int GetKickDamage(const char *name);
	int GetXP(const char *name);
	int GetSkin(const char *name, const char *model);
	float GetScale(const char *name, bool bDamage);
	float GetRange(const char *name);
	float GetFirearmDamage(const char *name, const char *weapon);
	float GetFirearmDamageScale(const char *name, const char *weapon, int entityType);
	float GetLimbData(const char *name, const char *limb, bool bHealth = false);
	const char *GetModel(const char *name);
	const char *GetModelForGib(const char *name, const char *gib, const char *model);

	// Misc
	bool DoesNPCExist(const char *name);
	bool DoesNPCHaveGibForLimb(const char *name, const char *model, int gibID);

	// Override Logic
	void LoadNPCOverrideData(KeyValues *pkvData = NULL);
	int GetOverridedModelIndexForNPC(const char *name);

private:

	int GetIndex(const char *name);
	CUtlVector<CNPCDataItem*> pszNPCItems;
	CUtlVector<CNPCOverrideModelData*> m_pModelOverrideItems;
};

#endif // GAME_DEFINITIONS_NPC_H
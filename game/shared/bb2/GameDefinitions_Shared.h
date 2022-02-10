//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Data Handler Class : Keeps information about the player and npc sound sets.
//
//========================================================================================//

#ifndef GAME_DEFINITIONS_SHARED_H
#define GAME_DEFINITIONS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "achievement_shareddefs.h"

#ifdef CLIENT_DLL
#include "ComboList.h"
#endif

#define PLAYER_GIB_GROUPS_MAX 5

enum SharedDamageScaleTypes
{
	DAMAGE_SCALE_TO_PLAYER = 0,
	DAMAGE_SCALE_TO_PLAYER_ZOMBIE,
	DAMAGE_SCALE_TO_NPC_ZOMBIES,
	DAMAGE_SCALE_TO_NPC_HUMANS,
	DAMAGE_SCALE_TO_NPC_ZOMBIE_BOSSES,
	DAMAGE_SCALE_TO_NPC_HUMAN_BOSSES,
	NUM_DAMAGE_SCALES,
};

enum ChatCommandType
{
	CHAT_CMD_VOICE = 1,
	CHAT_CMD_TEAM,
	CHAT_CMD_ALL,
};

enum VoiceCommandTypes
{
	VOICE_COMMAND_AGREE = 0,
	VOICE_COMMAND_DISAGREE,
	VOICE_COMMAND_FOLLOW,
	VOICE_COMMAND_TAKEPOINT,
	VOICE_COMMAND_NOWEP,
	VOICE_COMMAND_OUTOFAMMO,
	VOICE_COMMAND_READY,
	VOICE_COMMAND_LOOK,
	VOICE_COMMAND_EXIT,
};

enum SkillSoundCueList
{
	SKILL_SOUND_CUE_AMMO_BLAZE = 1,
	SKILL_SOUND_CUE_AMMO_FROST,
	SKILL_SOUND_CUE_AMMO_REFILL,
	SKILL_SOUND_CUE_AMMO_PENETRATE,

	SKILL_SOUND_CUE_MELEE_BLEED,
	SKILL_SOUND_CUE_MELEE_STUN,

	SKILL_SOUND_CUE_LIFE_LEECH,
};

struct DataGamemodeItem_Shared_t
{
	float flXPRoundWinArena;

	float flXPGameWinObjective;
	float flXPGameWinArena;

	float flXPScaleFactor;
	float flXPScaleFactorMinAvgLvL;
	float flXPScaleFactorMaxAvgLvL;

	float flZombieCreditsPercentToLose;
	float flArenaHardModeXPMultiplier;

	int iKillsRequiredToPerk;

	int iDefaultZombieCredits;
	float flAmmoResupplyTime;

	int iMaxAmmoReplenishWithinInterval;
	float flMaxAmmoReplensihInterval;
	float flAmmoReplenishPenalty;
};

struct DataPlayerItem_Shared_t
{
	int iMaxLevel;
	int iXPIncreasePerLevel;
	int iTeamBonusDamageIncrease;
	int iTeamBonusXPIncrease;
	int iLevel;
	int iInfectionStartPercent;
	float flInfectionDuration;
	float flPerkTime;
};

struct DataPlayerItem_Player_Shared_t
{
	int iHealth;
	int iArmor;
	int iArmorType;
	float flSpeed;
	float flJumpHeight;
	float flLeapLength;
	float flHealthRegenerationRate;
	int iTeam;
	int iGameMode;
};

struct DataPlayerItem_Player_PowerupItem_t
{
	char pchName[32];
	char pchModelPath[MAX_WEAPON_STRING];
	char pchActivationSoundScript[64];

	int iFlag;
	float flHealth;
	float flSpeed;
	float flJumpHeight;
	float flLeapLength;
	float flHealthRegenerationRate;
	float flPerkDuration;
	float flExtraFactor;

#ifdef CLIENT_DLL
	CHudTexture *pIconPowerupActive;
	CHudTexture *pIconPowerupInactive;
#endif
};

struct DataPlayerItem_MiscSkillInfo_t
{
	float flBleedDuration;
	float flBleedFrequency;

	float flNPCBurnDuration;
	float flNPCBurnDamage;
	float flNPCBurnFrequency;

	float flPlayerBurnDuration;
	float flPlayerBurnDamage;
	float flPlayerBurnFrequency;

	float flStunDuration;

	float flSlowDownDuration;
	float flSlowDownPercent;

	float flKickDamage;
	float flKickRange;
	float flKickKnockbackForce;
	float flKickCooldown;

	float flSlideLength;
	float flSlideSpeed;
	float flSlideCooldown;

	float flBloodRageRegenRate;
};

struct DataPlayerItem_Humans_Skills_t
{
	float flAgilitySpeed;
	float flAgilityJump;
	float flAgilityLeap;
	float flAgilitySlide;
	float flAgilityEnhancedReflexes;
	float flAgilityMeleeSpeed;
	float flAgilityLightweight;
	float flAgilityWeightless;
	float flAgilityHealthRegen;
	float flAgilityRealityPhase;

	float flStrengthHealth;
	float flStrengthImpenetrable;
	float flStrengthPainkiller;
	float flStrengthLifeLeech;
	float flStrengthPowerKick;
	float flStrengthBleed;
	float flStrengthCripplingBlow;
	float flStrengthArmorMaster;
	float flStrengthBloodRage;

	float flFirearmResourceful;
	float flFirearmBlazingAmmo;
	float flFirearmColdsnap;
	float flFirearmEmpoweredBullets;
	float flFirearmMagazineRefill;
	float flFirearmGunslinger;
};

struct DataPlayerItem_Zombies_Skills_t
{
	float flHealth;
	float flDamage;
	float flDamageReduction;
	float flSpeed;
	float flJump;
	float flLeap;
	float flDeath;
	float flLifeLeech;
	float flHealthRegen;
	float flMassInvasion;
};

struct DataPlayerItem_ZombieRageMode_t
{
	float flHealth;
	float flHealthRegen;
	float flSpeed;
	float flJump;
	float flLeap;
	float flDuration; // How long shall the rage last?
	float flRequiredDamageThreshold; // Damage required to activate zomb rage.
	float flTimeUntilBarDepletes; // Time until bar will start to decrease, attack frequently or else you risk losing the threshold you built up!
	float flDepletionRate; // The rate of the depletion, once started.
};

struct DataPlayerItem_Shared_WeaponInfo_t
{
	char szWeaponClass[32];
	float flDamageScale[NUM_DAMAGE_SCALES];
	int iTeam;
	int iGameMode;
};

struct DataPlayerItem_Shared_LimbInfo_t
{
	char szLimb[32];
	float flScale;
	float flHealth;
	int iTeam;
	int iGameMode;
};

#ifdef CLIENT_DLL
struct DataPlayerItem_Survivor_Shared_t
{
	char szSurvivorName[MAX_MAP_NAME];
	bool bGender;

	// Models
	char szHumanHandsPath[MAX_WEAPON_STRING];
	char szHumanModelPath[MAX_WEAPON_STRING];
	char szHumanBodyPath[MAX_WEAPON_STRING];

	char szZombieHandsPath[MAX_WEAPON_STRING];
	char szZombieModelPath[MAX_WEAPON_STRING];
	char szZombieBodyPath[MAX_WEAPON_STRING];

	// Gibs
	char szHumanGibHead[MAX_WEAPON_STRING];
	char szHumanGibArmLeft[MAX_WEAPON_STRING];
	char szHumanGibArmRight[MAX_WEAPON_STRING];
	char szHumanGibLegLeft[MAX_WEAPON_STRING];
	char szHumanGibLegRight[MAX_WEAPON_STRING];

	char szDeceasedGibHead[MAX_WEAPON_STRING];
	char szDeceasedGibArmLeft[MAX_WEAPON_STRING];
	char szDeceasedGibArmRight[MAX_WEAPON_STRING];
	char szDeceasedGibLegLeft[MAX_WEAPON_STRING];
	char szDeceasedGibLegRight[MAX_WEAPON_STRING];

	char szFriendlySurvivorName[MAX_MAP_NAME];
	char szFriendlyDescription[128];
	char szSequence[MAX_MAP_NAME];

	// Customization
	int iSkins;
	int iSpecialHeadItems;
	int iSpecialBodyItems;
	int iSpecialRightLegItems;
	int iSpecialLeftLegItems;

	// Camera Data
	QAngle angAngles;
	Vector vecPosition;

	const model_t *m_pClientModelPtrHuman;
	const model_t *m_pClientModelPtrHumanHands;
	const model_t *m_pClientModelPtrHumanBody;

	const model_t *m_pClientModelPtrZombie;
	const model_t *m_pClientModelPtrZombieHands;
	const model_t *m_pClientModelPtrZombieBody;

	const model_t *m_pClientModelPtrGibsHuman[PLAYER_GIB_GROUPS_MAX];
	const model_t *m_pClientModelPtrGibsZombie[PLAYER_GIB_GROUPS_MAX];
};
#endif

struct DataInventoryItem_Base_t
{
	uint iItemID;
	bool bIsMapItem;

	int iType;
	int iSubType;
	int iWeight;
	int iSkin;
	char szModelPath[MAX_WEAPON_STRING];

#ifdef CLIENT_DLL
	int iHUDTextureID;
	bool bShouldRenderIcon;
	char szTitle[MAX_WEAPON_STRING];
#else
	int iLevelReq;

	char szSoundScriptSuccess[32];
	char szSoundScriptFailure[32];
	char szSoundScriptExchange[32];

	Color clGlowColor;
	bool bGlobalGlow;
	bool bAutoConsume;
	bool bUnique;

	bool bEnableObjectiveIcon;
	char szObjectiveIconTexture[MAX_WEAPON_STRING];

	bool bHasEntityLink;
	char szEntityLink[64];

	float flScale;
	QAngle angOffset;

	bool bDisableRotationFX;
#endif
};

struct DataInventoryItem_Misc_t
{
	uint iItemID;
	int iValue;
};

struct DataInventoryItem_Armor_t
{
	uint iItemID;
	int iReductionPercent;
	int iWeight;
};

struct DataSoundPrefixItem_t
{
	int iID;
	int iType;
	char szScriptName[MAX_MAP_NAME];
	char szFriendlyName[MAX_MAP_NAME];
	char szSurvivorLink[MAX_MAP_NAME]; // Only used for zombie & human sound sets.
};

struct DataBloodParticleItem_t
{
	char szDefault[MAX_MAP_NAME];
	char szExtreme[MAX_MAP_NAME];
};

struct DataGibBloodParticleItem_t
{
	char szDefault[MAX_MAP_NAME];
	char szExtreme[MAX_MAP_NAME];
	char szLimb[MAX_MAP_NAME];
};

struct DataParticleItem_t
{
	char szDefault[MAX_MAP_NAME];
};

struct DataExplosiveItem_t
{
	int iType;
	char szParticle[MAX_MAP_NAME];
	float flRadius;
	float flPlayerDamage;
	float flNPCDamage;
};

struct DataPenetrationItem_t
{
	unsigned short material;
	float depth;
};

struct DataLoadingTipsItem_t
{
	char pchToken[MAX_MAP_NAME];
	char pchIconPath[MAX_MAP_NAME];
};

class CGameDefinitionsShared
{
public:
	CGameDefinitionsShared();
	~CGameDefinitionsShared();

	void Cleanup(void);
	bool LoadData(void);
	bool Precache(void);
#ifdef CLIENT_DLL
	void LoadClientModels(void);
#endif

	// Gamemode Data:
	const DataGamemodeItem_Shared_t *GetGamemodeData(void) { return &pszGamemodeData; }
	int CalculatePointsForLevel(int lvl);

	// Player Data:
	const DataPlayerItem_Shared_t *GetPlayerSharedData(void);
	const DataPlayerItem_Player_Shared_t *GetPlayerGameModeData(int iTeam);
	const DataPlayerItem_MiscSkillInfo_t *GetPlayerMiscSkillData(void);
	const DataPlayerItem_Humans_Skills_t *GetPlayerHumanSkillData(void);
	const DataPlayerItem_Zombies_Skills_t *GetPlayerZombieSkillData(void);
	const DataPlayerItem_ZombieRageMode_t *GetPlayerZombieRageData(void);
	const DataPlayerItem_Player_PowerupItem_t *GetPlayerPowerupData(const char *powerupName);
	const DataPlayerItem_Player_PowerupItem_t *GetPlayerPowerupData(int powerupFlag);
	float GetPlayerSharedValue(const char *name, int iTeam);
	float GetPlayerSkillValue(int iSkillType, int iTeam, int iSubType);

	int GetIndexForPowerup(const char *name) const;
	float GetPlayerFirearmDamageScale(const char *weapon, int entityType, int team);
	float GetPlayerLimbData(const char *limb, int team, bool bHealth = false);

	// Player Model Data:
#ifdef CLIENT_DLL
	void ParseCharacterFile(const char *file);
	const DataPlayerItem_Survivor_Shared_t *GetSurvivorDataForIndex(int index);
	const DataPlayerItem_Survivor_Shared_t *GetSurvivorDataForIndex(const char *name, bool bNoDefault = false);
	CUtlVector<DataPlayerItem_Survivor_Shared_t> &GetSurvivorDataList(void) { return pszPlayerSurvivorData; }
	const model_t *GetPlayerGibModelPtrForGibID(const DataPlayerItem_Survivor_Shared_t &data, bool bHuman, int gibID);
	const char *GetPlayerGibForModel(const char *survivor, bool bHuman, const char *gib);
	bool DoesPlayerHaveGibForLimb(const char *survivor, bool bHuman, int gibID);
#endif

	// Inventory Data
	void ParseInventoryData(KeyValues *pkvData = NULL, bool bIsMapItem = false);
	void RemoveMapInventoryItems(void);
	int GetInventoryMiscDataValue(uint itemID);
	int GetInventoryArmorDataValue(const char *name, uint itemID);
	bool DoesInventoryItemExist(uint itemID, bool bIsMapItem);
	int GetInventoryItemIndex(uint itemID, bool bIsMapItem);
	const char *GetInventoryItemModel(uint itemID, bool bIsMapItem);
	const DataInventoryItem_Base_t *GetInventoryData(uint itemID, bool bIsMapItem);
	CUtlVector<DataInventoryItem_Base_t> &GetInventoryItemList(void) { return pszItemSharedData; }

	// Sound Data
#ifdef CLIENT_DLL
	void ParseSoundsetFile(const char *file);
	const char *GetSoundPrefix(int iType, int index, const char *survivor = "survivor1");
	int GetConVarValueForEntitySoundType(int iType);
	const char *GetEntityNameFromEntitySoundType(int iType);
	int GetEntitySoundTypeFromEntityName(const char *name);
	void AddSoundScriptItems(vgui::ComboList *pList, int iType);
	void AddSoundScriptItems(vgui::ComboList *pList, int iType, const char *survivorLink);
	const char *GetSoundPrefixForChoosenItem(int iType, const char *survivorLink, const char *friendlyName);
	int GetSelectedSoundsetItemID(vgui::ComboList *pList, int iType, const char *survivorLink, const char *script);
	const char *GetPlayerSoundsetPrefix(int iType, const char *survivorLink, const char *script);
	const DataLoadingTipsItem_t *GetRandomLoadingTip(void);
#endif

	// Particle Data
	const char *GetBloodParticle(bool bExtremeGore = false);
	const char *GetHeadshotParticle(bool bExtremeGore = false);
	const char *GetBleedoutParticle(bool bExtremeGore = false);
	const char *GetBloodExplosionMist(bool bExtremeGore = false);
	const char *GetGibParticleForLimb(const char *limb, bool bExtremeGore = false);

	// Explosion Data
	const DataExplosiveItem_t *GetExplosiveDataForType(int type);
	const char *GetExplosionParticle(int type);
	CUtlVector<DataExplosiveItem_t> &GetExplosiveData(void) { return pszExplosionData; }

private:

	// Player Data
	DataGamemodeItem_Shared_t pszGamemodeData;
	DataPlayerItem_Shared_t pszPlayerSharedData;
	DataPlayerItem_MiscSkillInfo_t pszPlayerMiscSkillData;
	DataPlayerItem_Humans_Skills_t pszHumanSkillData;
	DataPlayerItem_Zombies_Skills_t pszZombieSkillData;
	DataPlayerItem_ZombieRageMode_t pszZombieRageModeData;
	CUtlVector<DataPlayerItem_Player_Shared_t> pszPlayerData;
#ifdef CLIENT_DLL
	CUtlVector<DataPlayerItem_Survivor_Shared_t> pszPlayerSurvivorData;
#endif
	CUtlVector<DataPlayerItem_Shared_WeaponInfo_t> pszPlayerWeaponData;
	CUtlVector<DataPlayerItem_Shared_LimbInfo_t> pszPlayerLimbData;
	CUtlVector<DataPlayerItem_Player_PowerupItem_t> pszPlayerPowerupData;

	// Inventory Data
	CUtlVector<DataInventoryItem_Base_t> pszItemSharedData;
	CUtlVector<DataInventoryItem_Misc_t> pszItemMiscData;
	CUtlVector<DataInventoryItem_Armor_t> pszItemArmorData;

	// Sound Data
#ifdef CLIENT_DLL
	CUtlVector<DataLoadingTipsItem_t> pszLoadingTipData;
	CUtlVector<DataSoundPrefixItem_t> pszSoundPrefixesData;
	int GetNextIndexForSoundSet(int iType, const char *survivorLink);
#endif

	// Particle Data
	CUtlVector<DataBloodParticleItem_t> pszBloodParticleData;
	CUtlVector<DataBloodParticleItem_t> pszHeadshotParticleData;
	CUtlVector<DataBloodParticleItem_t> pszBleedoutParticleData;
	CUtlVector<DataBloodParticleItem_t> pszBloodExplosionParticleData;
	CUtlVector<DataGibBloodParticleItem_t> pszGibParticleData;

	// Explosion Data
	CUtlVector<DataExplosiveItem_t> pszExplosionData;
};

extern const char *GetVoiceCommandString(int command);
extern const char *GetVoiceCommandChatMessage(int command);
extern const char *GetTeamPerkName(int perk);

extern const char *GetGamemodeName(int gamemode);
extern const char *GetGamemodeNameForPrefix(const char *map);
extern int GetGamemodeForMap(const char *map);

namespace ACHIEVEMENTS
{
	const achievementStatItem_t *GetAchievementItem(int index);
	const achievementStatItem_t *GetAchievementItem(const char *str);
	int GetNumAchievements(void);
}

extern const DataPenetrationItem_t *GetPenetrationDataForMaterial(unsigned short material);
extern Vector TryPenetrateSurface(trace_t *tr, ITraceFilter *filter);

const char* COM_GetModDirectory();
void RecreateSoundScriptsManifest(void);

#endif // GAME_DEFINITIONS_SHARED_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Shared Game Handler: Handles .bbd files for data parsing, reading, storing, etc...
//
//========================================================================================//

#ifndef GAME_BASE_SHARED_H
#define GAME_BASE_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "filesystem.h"
#include "GameDefinitions_Shared.h"
#include "skills_shareddefs.h"
#include "GameDefinitions_MapData.h"
#include "GameDefinitions_NPC.h"
#include "GameDefinitions_QuestData.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "c_steam_server_lister.h"
#include "music_system.h"
#else
#include "hl2mp_player.h"
#include "achievement_manager.h"
#include "GameDefinitions_Workshop.h"
#include "player_loadout_handler.h"
#endif

#define MAX_INVENTORY_ITEM_COUNT 12
#define MAX_OBJECTIVE_EXPERIENCE 0.02f

#define DELAYED_USE_TIME 1.5f
#define MAX_MELEE_LAGCOMP_DIST 300.0f

// Min. required distance in order to activate/use an inv. item.
#define MAX_ENTITY_LINK_RANGE 140.0f

// If the player is further away than this from every npc then fade out all boss bars!
#define MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_BOSS 700.0f
#define MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_REGULAR 190.0f

// If your character surpasses this weight in additional equipment we will disable wep selection and other stuff as well,
// = Minigun mode.
#define MAX_PLAYER_CARRY_WEIGHT 50.0f // kg

/// Time Definitions in hours.
#define TIME_STRING_YEAR 8760
#define TIME_STRING_MONTH 730
#define TIME_STRING_WEEK 168
#define TIME_STRING_DAY 24

#ifdef CLIENT_DLL
#define CGameBaseShared C_GameBaseShared

extern ConVar bb2_preview_debugging;
extern ConVar bb2_preview_origin_x;
extern ConVar bb2_preview_origin_y;
extern ConVar bb2_preview_origin_z;
extern ConVar bb2_preview_angle_x;
extern ConVar bb2_preview_angle_y;
extern ConVar bb2_preview_angle_z;
#endif

enum ServerConnection_Direct
{
	CONNECTION_WAITING = 0,
	CONNECTION_SEARCH_INTERNET,
	CONNECTION_SEARCH_LAN,
	CONNECTION_DEFAULT,
};

enum InventoryItem_Type
{
	TYPE_MISC = 0, // Usable in-game. Such as armor, health kits, food...
	TYPE_OBJECTIVE, // Objective (usable) like briefcase, keys, etc... Boltcutters, anything.
	TYPE_ATTACHMENT, // Can only be applied to a weapon.
	TYPE_ARMOR, // Can be weilded, granted by achievements/use of an substantial amount of tokens.
	TYPE_SPECIAL, // Random, trading cards ? / EASTER EGG's?
};

enum Misc_Type
{
	TYPE_HEALTHKIT = 0, // Big healthkit.
	TYPE_HEALTHVIAL, // Medium health boost.
	TYPE_FOOD, // Food, drink, etc... Gives a medium boost.
	TYPE_DRUG, // Alcohol, drugs, etc... Gives full hp boost but 'fucks' you up.
	TYPE_SYRINGE, // Small health boost.
	TYPE_ARMOR_SMALL, // Reduces little damage.
	TYPE_ARMOR_MEDIUM, // Reduces a decent amount of damage.
	TYPE_ARMOR_LARGE, // Reduces more damage.
};

enum Attachment_Type
{
	TYPE_SILENCER = 0, // SUPPRESS SOUND
	TYPE_FLASHLIGHT, // FOR ILLUMINATION
	TYPE_STOCK, // HOLDING STOCK / SHOULDER STOCK FOR BETTER ACCURACY AND LESS RECOIL
	TYPE_SIGHT, // STANDARD RED DOT STYLE
	TYPE_SCOPE, // USES DYNAMIC SCOPE + ZOOM
	TYPE_BARREL, // BARREL-STOCK TO REDUCE SMOKE AND MUZZLE FLASH (LESS VISIBILITY) NOTICE: CAN'T BE USED WITH SILENCER.
	TYPE_CALIBER, // CALIBER TYPE, BIGGER DAMAGE.
};

enum Equipment_Type // You may only wield two unique equipment at a time.
{
	TYPE_HELMET = 0, // HATS, COOL HELMS. INCREASES BASE STATS. DEPENDS ON RARITY / LEVEL REQ. + TOKEN COST
	TYPE_TRINKET, // HIGH TOKEN COST, INCREASES BASE STATS. (FREE TRINKET ON ACOMPLISHING ALL ACHIEVS)
};

enum Objective_Type
{
	TYPE_KEY = 0, // Actually the same as type_box for now...
	TYPE_BOX, // Regular Item
	TYPE_VITAL, // Vital item = important item = glows the player who has this item. (global)
	TYPE_REMOVABLE, // WHEN THIS ITEM HAS BE USED IT WILL BE REMOVED FROM THE INVENTORY. MUST BE USED FOR OBJECTS WHICH YOU INTEND TO REMOVE ON USE.
	TYPE_REMOVABLE_GLOW, // SAME AS REMOVABLE BUT WILL GLOW THE CARRIER LIKE VITAL.
};

enum ObjectiveScaling_Type
{
	OBJ_SCALING_NONE = 0,
	OBJ_SCALING_ALL,
	OBJ_SCALING_FIXED, // Excludes time.
	OBJ_SCALING_TIMEONLY, // Excludes kills.
};

enum InventoryActions
{
	INV_ACTION_REMOVE = 0,
	INV_ACTION_PURGE,
	INV_ACTION_ADD,
	INV_ACTION_UPD,
};

enum InventoryCheckType
{
	INV_CHECK_USE = 1,
	INV_CHECK_DROP,
	INV_CHECK_DELETE,
};

enum
{
	GROUPID_IS_DEVELOPER = 0x001,

	GROUPID_IS_TESTER = 0x002,
	GROUPID_IS_DONATOR = 0x004,

	MAX_GROUPID_BITS = 3
};

enum
{
	MAT_OVERLAY_BLOOD = 0x001,
	MAT_OVERLAY_SPAWNPROTECTION = 0x002,
	MAT_OVERLAY_CRIPPLED = 0x004,
	MAT_OVERLAY_BURNING = 0x008,
	MAT_OVERLAY_COLDSNAP = 0x010,
	MAT_OVERLAY_BLEEDING = 0x020,
	MAX_MAT_OVERLAYS_BITS = 6
};

enum
{
	GAME_TIP_KEYBIND = 0,
	GAME_TIP_DEFAULT = 0,
	GAME_TIP_INFO,
	GAME_TIP_TIP,
	GAME_TIP_WARNING,
	GAME_TIP_DEFEND,
};

struct InventoryItem_t
{
#ifndef CLIENT_DLL
	int m_iPlayerIndex;
#endif
	uint m_iItemID;
	bool bIsMapItem;
};

struct ObjectiveItem_t
{
	int m_iIndex;
	int m_iTeam;
	int m_iStatus;
	int m_iKillsLeft;
	int m_iIconLocationIndex;
	float m_flTime;
	char szObjective[80];
	char szIconTexture[80];
	int m_nDisplayFlags;
	float m_flLerp;
};

enum ObjectiveItemFlags
{
	OBJ_FADED_IN = 0x001,
	OBJ_IS_VISIBILE = 0x002,
	OBJ_IS_FINISHED = 0x004,
	OBJ_FADING_OUT = 0x008,
};

// Armor types for the player, each type has a 100% 'hp/durability' value however type 2 is stronger than type 0. It reduces more damage and lasts slightly longer.
enum PlayerArmorTypes
{
	TYPE_NONE = 0,
	TYPE_LIGHT,
	TYPE_MED,
	TYPE_HEAVY,
	TYPE_SUPER_HEAVY, // Never got implemented... =(
};

enum ExplosiveTypes_t
{
	EXPLOSIVE_TYPE_GRENADE = 0,
	EXPLOSIVE_TYPE_PROPANE,
};

enum MeleeAttackTypes_t
{
	MELEE_TYPE_BLUNT = 1, // Stab, Club, Smack, etc...
	MELEE_TYPE_SLASH, // Slash, Strike, etc...
	MELEE_TYPE_BASH_BLUNT, // Bash Strike... Push back strike!
	MELEE_TYPE_BASH_SLASH, // Bash Slash + push back.
};

enum ClientAttachmentTypes_t
{
	CLIENT_ATTACHMENT_WEAPON = 0,
};

class CGameBaseShared
{
public:

	void Init();
	void LoadBase();
	void Release();

	const unsigned char *GetEncryptionKey(void) { return (unsigned char *)"F3QxBzK6"; }

	// Game Base
	KeyValues *ReadEncryptedKeyValueFile(IFileSystem *filesystem, const char *filePath, bool bEncryption = false);
	void EncryptKeyValueFile(const char *filePath, const char *fileContent);
	const char *GetFileChecksum(const char *relativePath);

	// Base Stat / Skill / Player Values: Base Models / Survivors / Character Profiles:
	CGameDefinitionsShared *GetSharedGameDetails(void) { return m_pSharedGameDefinitions; }
	CGameDefinitionsMapData *GetSharedMapData(void) { return m_pSharedGameMapData; }
	CGameDefinitionsNPC *GetNPCData(void) { return m_pSharedNPCData; }
	CGameDefinitionsQuestData *GetSharedQuestData(void) { return m_pSharedQuestData; }

	// Misc
	const char *GetTimeString(int iHoursPlayed);
	void GetFileContent(const char *path, char *buf, int size);
	float GetDropOffDamage(const Vector &vecStart, const Vector &vecEnd, float damage, float minDist);
	float GetSequenceDuration(CStudioHdr *ptr, int sequence);
	float GetPlaybackSpeedThirdperson(CHL2MP_Player *pClient, int viewmodelActivity, int thirdpersonActivity);

	// Bleeding Dispatches
	void DispatchBleedout(CBaseEntity *pEntity);

#ifdef CLIENT_DLL
	// Inventory Accessors
	CUtlVector<InventoryItem_t> &GetGameInventory() { return pszTemporaryInventoryList; }
	int GetInventoryItemIndex(uint itemID, bool bIsMapItem);

	// Steam Server Accessor
	CSteamServerLister *GetSteamServerManager() { return m_pSteamServers; }
#else
	// Inventory
	CUtlVector<InventoryItem_t> &GetServerInventory() { return pszInventoryList; }
	void AddInventoryItem(int iPlayerIndex, const DataInventoryItem_Base_t *itemData, bool bIsMapItem);
	void RemoveInventoryItem(int iPlayerIndex, const Vector &vecAbsOrigin, int iType = -1, uint iItemID = 0, bool bDeleteItem = false, int forceIndex = -1);
	void RemoveInventoryItems(void);
	bool UseInventoryItem(int iPlayerIndex, uint iItemID, bool bIsMapItem, bool bAutoConsume = false, bool bDelayedUse = false, int forceIndex = -1);
	int GetInventoryItemCountForPlayer(int iPlayerIndex);
	int GetInventoryItemForPlayer(int iPlayerIndex, uint iItemID, bool bIsMapItem);
	bool HasObjectiveGlowItems(CHL2MP_Player *pClient);

	// Achievement Checks
	void EntityKilledByPlayer(CBaseEntity *pKiller, CBaseEntity *pVictim, CBaseEntity *pInflictor, int forcedWeaponID = WEAPON_ID_NONE);
	void OnGameOver(float timeLeft, int iWinner);

	// Workshop Handler:
	CGameDefinitionsWorkshop *GetServerWorkshopData(void) { return m_pServerWorkshopData; }

	// Loadout Handler:
	CPlayerLoadoutHandler *GetPlayerLoadoutHandler(void) { return m_pPlayerLoadoutHandler; }

	// Server Commands and Client Commands checks.
	bool ClientCommand(const CCommand &args);

	// Events
	void NewPlayerConnection(bool bState = false);
	int GetNumActivePlayers(void) { return m_iNumActivePlayers; }
	int GetAveragePlayerLevel(void) { return m_iAvgPlayerLvL; }

	float GetPlayerScaledValue(float min, float max)
	{
		float flScaling = ((float)GetNumActivePlayers()) / (MAX_PLAYERS - 2.0f);
		float flNewValue = 0.0f;

		flScaling = clamp(flScaling, 0.0f, 1.0f);
		flNewValue = min + (flScaling * (max - min));

		return clamp(flNewValue, min, max);
	}

	// Misc
	void ComputePlayerWeight(CHL2MP_Player *pPlayer);
#endif

private:

#ifdef CLIENT_DLL
	CSteamServerLister *m_pSteamServers;
	CMusicSystem *m_pMusicSystem;
	CUtlVector<InventoryItem_t> pszTemporaryInventoryList; // Can be organized from the game, stores temp items, some will be lost on death. (obj. items)
#else
	// Stores ALL items on the server (all player items in one list) MAX items = 12(maxclients) * 12(maxitems pr user).
	CUtlVector<InventoryItem_t> pszInventoryList;

	// Server Workshop Handler:
	CGameDefinitionsWorkshop *m_pServerWorkshopData;
	// Server Loadout Handler:
	CPlayerLoadoutHandler *m_pPlayerLoadoutHandler;

	int m_iNumActivePlayers;
	int m_iAvgPlayerLvL;
#endif

	// Shared Game Data Info
	CGameDefinitionsShared *m_pSharedGameDefinitions;
	CGameDefinitionsMapData *m_pSharedGameMapData;
	CGameDefinitionsNPC *m_pSharedNPCData;
	CGameDefinitionsQuestData *m_pSharedQuestData;
};

void ClearCharVectorList(CUtlVector<char*> &list);
extern CGameBaseShared* GameBaseShared();

#endif // GAME_BASE_SHARED_H
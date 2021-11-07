//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Data Handler Class : Keeps information about the player and npc sound sets.
//
//========================================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "GameDefinitions_Shared.h"
#include "GameBase_Shared.h"
#include "hl2mp_gamerules.h"
#include "gibs_shared.h"
#include "decals.h"
#include "icommandline.h"

#ifdef CLIENT_DLL
#include "hud.h"
#include "hudelement.h"
#include "vgui/ISurface.h"
#include "c_world.h"

// Soundsets
static ConVar bb2_sound_player_human("bb2_sound_player_human", "Pantsman", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player sound set prefix.");
static ConVar bb2_sound_player_deceased("bb2_sound_player_deceased", "Default", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player sound set prefix.");

ConVar bb2_sound_zombie("bb2_sound_zombie", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Your sound choice for this npc.");
ConVar bb2_sound_military("bb2_sound_military", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Your sound choice for this npc.");
ConVar bb2_sound_bandit("bb2_sound_bandit", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Your sound choice for this npc.");
ConVar bb2_sound_fred("bb2_sound_fred", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Your sound choice for this npc.");
ConVar bb2_sound_announcer("bb2_sound_announcer", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Your sound choice for the deathmatch announcer.");

// Player Customization
static ConVar bb2_survivor_choice("bb2_survivor_choice", "survivor1", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player model.");
static ConVar bb2_survivor_choice_skin("bb2_survivor_choice_skin", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player model skin.");
static ConVar bb2_survivor_choice_extra_head("bb2_survivor_choice_extra_head", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player model bodygroup for extra head.");
static ConVar bb2_survivor_choice_extra_body("bb2_survivor_choice_extra_body", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player model bodygroup for extra body.");
static ConVar bb2_survivor_choice_extra_leg_right("bb2_survivor_choice_extra_leg_right", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player model bodygroup for extra right leg.");
static ConVar bb2_survivor_choice_extra_leg_left("bb2_survivor_choice_extra_leg_left", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Selected player model bodygroup for extra left leg.");

// Misc
static ConVar bb2_weapon_autoreload("bb2_weapon_autoreload", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Enables auto reloading, if possible.");

#define CLIENT_MODEL_START_INDEX 8192 // Make sure we don't collide with anything edict related! Must be > 4096!
struct ClientModelItem
{
	ClientModelItem(int id, const model_t *mdl)
	{
		this->id = id;
		this->model = mdl;
	}

	~ClientModelItem()
	{
		model = NULL;
	}

	int id;
	const model_t *model;
};
static CUtlVector<ClientModelItem> m_pClientModelList;

static const model_t *LoadClientModel(const char *path)
{
	int newID = (CLIENT_MODEL_START_INDEX + m_pClientModelList.Count());

	const model_t *model = engine->LoadModel(path);
	m_pClientModelList.AddToTail(ClientModelItem(newID, model));
	return model;
}

int LookupClientModelIndex(const model_t *model)
{
	int count = m_pClientModelList.Count();
	if (count)
	{
		for (int i = 0; i < count; i++)
		{
			ClientModelItem *item = &m_pClientModelList[i];
			if (item->model == model)
				return item->id;
		}
	}

	return -1;
}

const model_t *LookupClientModelPointer(int index)
{
	int count = m_pClientModelList.Count();
	if (count && (index >= CLIENT_MODEL_START_INDEX))
	{
		for (int i = 0; i < count; i++)
		{
			ClientModelItem *item = &m_pClientModelList[i];
			if (item->id == index)
				return item->model;
		}
	}

	return modelinfo->GetModel(index); // Revert to default!
}
#else
#include "GameBase_Server.h"
#endif

// Gamemode data
static const char *playerGamemodeFiles[] =
{
	"data/game/game_base_player_objective",
	"data/game/game_base_player_elimination",
	"data/game/game_base_player_arena",
	"data/game/game_base_player_deathmatch",
};

CGameDefinitionsShared::CGameDefinitionsShared()
{
	LoadData();
}

CGameDefinitionsShared::~CGameDefinitionsShared()
{
	Cleanup();
}

void CGameDefinitionsShared::Cleanup(void)
{
#ifdef CLIENT_DLL
	for (int i = (pszItemSharedData.Count() - 1); i >= 0; i--)
	{
		if (pszItemSharedData[i].iHUDTextureID != -1)
			vgui::surface()->DestroyTextureID(pszItemSharedData[i].iHUDTextureID);
	}

	pszLoadingTipData.Purge();
	pszSoundPrefixesData.Purge();
	pszPlayerSurvivorData.Purge();
	m_pClientModelList.Purge();
#endif

	pszPlayerData.Purge();
	pszPlayerLimbData.Purge();
	pszPlayerWeaponData.Purge();
	pszPlayerPowerupData.Purge();
	pszItemSharedData.Purge();
	pszItemMiscData.Purge();
	pszItemArmorData.Purge();
	pszBloodParticleData.Purge();
	pszHeadshotParticleData.Purge();
	pszBleedoutParticleData.Purge();
	pszBloodExplosionParticleData.Purge();
	pszGibParticleData.Purge();
	pszExplosionData.Purge();
}

bool CGameDefinitionsShared::LoadData(void)
{
	Cleanup();
	KeyValues *pkvParseData = NULL, *pkvOther = NULL;

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/game_base_player_shared");
	if (pkvParseData)
	{
		pkvOther = pkvParseData->FindKey("Shared");
		if (pkvOther)
		{
			pszPlayerSharedData.iMaxLevel = pkvOther->GetInt("MaxLevel", MAX_PLAYER_LEVEL);
			pszPlayerSharedData.iXPIncreasePerLevel = pkvOther->GetInt("XPPerLevel", 65);
			pszPlayerSharedData.iTeamBonusDamageIncrease = pkvOther->GetInt("TeamBonusDamageIncrease", 1);
			pszPlayerSharedData.iTeamBonusXPIncrease = pkvOther->GetInt("TeamBonusXPIncrease", 1);
			pszPlayerSharedData.flPerkTime = pkvOther->GetFloat("PerkTime", 10.0f);
			pszPlayerSharedData.iLevel = pkvOther->GetInt("Level", 1);
			pszPlayerSharedData.iInfectionStartPercent = pkvOther->GetInt("InfectionStart", 15);
			pszPlayerSharedData.flInfectionDuration = pkvOther->GetFloat("InfectionTime", 30.0f);
		}

		pkvOther = pkvParseData->FindKey("HumanSkills");
		if (pkvOther)
		{
			pszHumanSkillData.flAgilitySpeed = pkvOther->GetFloat("Speed", 3.0f);
			pszHumanSkillData.flAgilityJump = pkvOther->GetFloat("Jump", 3.0f);
			pszHumanSkillData.flAgilityLeap = pkvOther->GetFloat("Leap", 5.0f);
			pszHumanSkillData.flAgilitySlide = pkvOther->GetFloat("Slide", 5.0f);
			pszHumanSkillData.flAgilityEnhancedReflexes = pkvOther->GetFloat("Reflexes", 4.0f);
			pszHumanSkillData.flAgilityMeleeSpeed = pkvOther->GetFloat("MeleeSpeed", 2.0f);
			pszHumanSkillData.flAgilityLightweight = pkvOther->GetFloat("Lightweight", 2.0f);
			pszHumanSkillData.flAgilityWeightless = pkvOther->GetFloat("Weightless", 4.0f);
			pszHumanSkillData.flAgilityHealthRegen = pkvOther->GetFloat("HealthRegen", 0.35f);
			pszHumanSkillData.flAgilityRealityPhase = pkvOther->GetFloat("RealityPhase", 3.0f);

			pszHumanSkillData.flStrengthHealth = pkvOther->GetFloat("Health", 10.0f);
			pszHumanSkillData.flStrengthImpenetrable = pkvOther->GetFloat("Impenetrable", 2.0f);
			pszHumanSkillData.flStrengthPainkiller = pkvOther->GetFloat("Painkiller", 5.0f);
			pszHumanSkillData.flStrengthLifeLeech = pkvOther->GetFloat("LifeLeech", 1.0f);
			pszHumanSkillData.flStrengthPowerKick = pkvOther->GetFloat("PowerKick", 10.0f);
			pszHumanSkillData.flStrengthBleed = pkvOther->GetFloat("Bleed", 5.0f);
			pszHumanSkillData.flStrengthCripplingBlow = pkvOther->GetFloat("CripplingBlow", 2.0f);
			pszHumanSkillData.flStrengthArmorMaster = pkvOther->GetFloat("ArmorMaster", 2.0f);
			pszHumanSkillData.flStrengthBloodRage = pkvOther->GetFloat("BloodRage", 2.0f);

			pszHumanSkillData.flFirearmResourceful = pkvOther->GetFloat("Resourceful", 3.0f);
			pszHumanSkillData.flFirearmBlazingAmmo = pkvOther->GetFloat("BlazingAmmo", 2.0f);
			pszHumanSkillData.flFirearmColdsnap = pkvOther->GetFloat("Coldsnap", 2.0f);
			pszHumanSkillData.flFirearmEmpoweredBullets = pkvOther->GetFloat("EmpoweredBullets", 4.0f);
			pszHumanSkillData.flFirearmMagazineRefill = pkvOther->GetFloat("MagazineRefill", 1.0f);
			pszHumanSkillData.flFirearmGunslinger = pkvOther->GetFloat("Gunslinger", 2.0f);
		}

		pkvOther = pkvParseData->FindKey("ZombieSkills");
		if (pkvOther)
		{
			pszZombieSkillData.flHealth = pkvOther->GetFloat("Health", 5.0f);
			pszZombieSkillData.flDamage = pkvOther->GetFloat("Damage", 2.0f);
			pszZombieSkillData.flDamageReduction = pkvOther->GetFloat("DamageReduction", 3.0f);
			pszZombieSkillData.flSpeed = pkvOther->GetFloat("Speed", 4.0f);
			pszZombieSkillData.flJump = pkvOther->GetFloat("Jump", 3.0f);
			pszZombieSkillData.flLeap = pkvOther->GetFloat("Leap", 1.0f);
			pszZombieSkillData.flDeath = pkvOther->GetFloat("Death", 2.0f);
			pszZombieSkillData.flLifeLeech = pkvOther->GetFloat("LifeLeech", 2.0f);
			pszZombieSkillData.flHealthRegen = pkvOther->GetFloat("HealthRegen", 4.0f);
			pszZombieSkillData.flMassInvasion = pkvOther->GetFloat("MassInvasion", 1.0f);
		}

		pkvOther = pkvParseData->FindKey("ZombieRageMode");
		if (pkvOther)
		{
			pszZombieRageModeData.flHealth = pkvOther->GetFloat("Health", 50.0f);
			pszZombieRageModeData.flHealthRegen = pkvOther->GetFloat("HealthRegen", 5.0f);
			pszZombieRageModeData.flSpeed = pkvOther->GetFloat("Speed", 25.0f);
			pszZombieRageModeData.flJump = pkvOther->GetFloat("Jump", 5.0f);
			pszZombieRageModeData.flLeap = pkvOther->GetFloat("Leap", 5.0f);
			pszZombieRageModeData.flDuration = pkvOther->GetFloat("Duration", 15.0f);
			pszZombieRageModeData.flRequiredDamageThreshold = pkvOther->GetFloat("DamageRequired", 250.0f);
			pszZombieRageModeData.flTimeUntilBarDepletes = pkvOther->GetFloat("TimeUntilBarDepletes", 3.0f);
			pszZombieRageModeData.flDepletionRate = pkvOther->GetFloat("DepletionRate", 10.0f);
		}

		pkvOther = pkvParseData->FindKey("MiscSkillInfo");
		if (pkvOther)
		{
			pszPlayerMiscSkillData.flBleedDuration = pkvOther->GetFloat("BleedDuration");
			pszPlayerMiscSkillData.flBleedFrequency = pkvOther->GetFloat("BleedFrequency");

			pszPlayerMiscSkillData.flNPCBurnDuration = pkvOther->GetFloat("BurnDurationNPC");
			pszPlayerMiscSkillData.flNPCBurnDamage = pkvOther->GetFloat("BurnDamageNPC");
			pszPlayerMiscSkillData.flNPCBurnFrequency = pkvOther->GetFloat("BurnFrequencyNPC");

			pszPlayerMiscSkillData.flPlayerBurnDuration = pkvOther->GetFloat("BurnDurationPlayer");
			pszPlayerMiscSkillData.flPlayerBurnDamage = pkvOther->GetFloat("BurnDamagePlayer");
			pszPlayerMiscSkillData.flPlayerBurnFrequency = pkvOther->GetFloat("BurnFrequencyPlayer");

			pszPlayerMiscSkillData.flStunDuration = pkvOther->GetFloat("StunDuration");
			pszPlayerMiscSkillData.flSlowDownDuration = pkvOther->GetFloat("SlowDownDuration");
			pszPlayerMiscSkillData.flSlowDownPercent = pkvOther->GetFloat("SlowDownPercent");

			pszPlayerMiscSkillData.flKickDamage = pkvOther->GetFloat("KickDamage");
			pszPlayerMiscSkillData.flKickRange = pkvOther->GetFloat("KickRange");
			pszPlayerMiscSkillData.flKickKnockbackForce = pkvOther->GetFloat("KickForce");
			pszPlayerMiscSkillData.flKickCooldown = pkvOther->GetFloat("KickCooldown");

			pszPlayerMiscSkillData.flSlideLength = pkvOther->GetFloat("SlideLength");
			pszPlayerMiscSkillData.flSlideSpeed = pkvOther->GetFloat("SlideSpeed");
			pszPlayerMiscSkillData.flSlideCooldown = pkvOther->GetFloat("SlideCooldown");
		}

		pkvOther = pkvParseData->FindKey("ItemArmor");
		if (pkvOther)
		{
			for (KeyValues *sub = pkvOther->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				DataInventoryItem_Armor_t item;
				item.iItemID = (uint)atol(sub->GetName());
				item.iWeight = sub->GetInt("weight");
				item.iReductionPercent = sub->GetInt("reduction");
				pszItemArmorData.AddToTail(item);
			}
		}

		pkvOther = pkvParseData->FindKey("ItemHealth");
		if (pkvOther)
		{
			for (KeyValues *sub = pkvOther->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				DataInventoryItem_Misc_t item;
				item.iItemID = (uint)atol(sub->GetName());
				item.iValue = sub->GetInt();
				pszItemMiscData.AddToTail(item);
			}
		}

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/game_base_player_shared!\n");

	for (int i = 0; i < _ARRAYSIZE(playerGamemodeFiles); i++)
	{
		pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, playerGamemodeFiles[i]);
		if (pkvParseData)
		{
			int gamemode = (i + 1);

			KeyValues *pkvDefault = pkvParseData->FindKey("Humans");
			if (pkvDefault)
			{
				DataPlayerItem_Player_Shared_t playerItem;
				playerItem.iHealth = pkvDefault->GetInt("Health", 100);
				playerItem.iArmor = pkvDefault->GetInt("Armor", 0);
				playerItem.iArmorType = pkvDefault->GetInt("ArmorType", 0);
				playerItem.flSpeed = pkvDefault->GetFloat("Speed", 155.0f);
				playerItem.flJumpHeight = pkvDefault->GetFloat("Jump", 27.0f);
				playerItem.flLeapLength = pkvDefault->GetFloat("Leap", 0.0f);
				playerItem.flHealthRegenerationRate = pkvDefault->GetFloat("HealthRegen", 0.0f);
				playerItem.iTeam = TEAM_HUMANS;
				playerItem.iGameMode = gamemode;
				pszPlayerData.AddToTail(playerItem);

				KeyValues *pkvShared = pkvDefault->FindKey("WeaponDefinitions");
				if (pkvShared)
				{
					for (KeyValues *sub = pkvShared->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						DataPlayerItem_Shared_WeaponInfo_t weaponItem;
						Q_strncpy(weaponItem.szWeaponClass, sub->GetName(), 32);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_PLAYER] = sub->GetFloat("scale_player", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_PLAYER_ZOMBIE] = sub->GetFloat("scale_player_zombie", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_ZOMBIES] = sub->GetFloat("scale_npc_zombies", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_HUMANS] = sub->GetFloat("scale_npc_humans", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_ZOMBIE_BOSSES] = sub->GetFloat("scale_npc_zombie_bosses", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_HUMAN_BOSSES] = sub->GetFloat("scale_npc_human_bosses", 1.0f);
						weaponItem.iTeam = TEAM_HUMANS;
						weaponItem.iGameMode = gamemode;
						pszPlayerWeaponData.AddToTail(weaponItem);
					}
				}

				pkvShared = pkvDefault->FindKey("LimbData");
				if (pkvShared)
				{
					for (KeyValues *sub = pkvShared->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						DataPlayerItem_Shared_LimbInfo_t limbItem;
						Q_strncpy(limbItem.szLimb, sub->GetName(), 32);
						limbItem.flScale = sub->GetFloat("scale", 1.0f);
						limbItem.flHealth = sub->GetFloat("health", 10.0f);
						limbItem.iTeam = TEAM_HUMANS;
						limbItem.iGameMode = gamemode;
						pszPlayerLimbData.AddToTail(limbItem);
					}
				}
			}

			pkvDefault = pkvParseData->FindKey("Zombies");
			if (pkvDefault)
			{
				DataPlayerItem_Player_Shared_t playerItem;
				playerItem.iHealth = pkvDefault->GetInt("Health", 125);
				playerItem.iArmor = pkvDefault->GetInt("Armor", 0);
				playerItem.iArmorType = pkvDefault->GetInt("ArmorType", 0);
				playerItem.flSpeed = pkvDefault->GetFloat("Speed", 175.0f);
				playerItem.flJumpHeight = pkvDefault->GetFloat("Jump", 25.0f);
				playerItem.flLeapLength = pkvDefault->GetFloat("Leap", 1.0f);
				playerItem.flHealthRegenerationRate = pkvDefault->GetFloat("HealthRegen", 5.0f);
				playerItem.iTeam = TEAM_DECEASED;
				playerItem.iGameMode = gamemode;
				pszPlayerData.AddToTail(playerItem);

				KeyValues *pkvShared = pkvDefault->FindKey("WeaponDefinitions");
				if (pkvShared)
				{
					for (KeyValues *sub = pkvShared->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						DataPlayerItem_Shared_WeaponInfo_t weaponItem;
						Q_strncpy(weaponItem.szWeaponClass, sub->GetName(), 32);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_PLAYER] = sub->GetFloat("scale_player", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_PLAYER_ZOMBIE] = sub->GetFloat("scale_player_zombie", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_ZOMBIES] = sub->GetFloat("scale_npc_zombies", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_HUMANS] = sub->GetFloat("scale_npc_humans", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_ZOMBIE_BOSSES] = sub->GetFloat("scale_npc_zombie_bosses", 1.0f);
						weaponItem.flDamageScale[DAMAGE_SCALE_TO_NPC_HUMAN_BOSSES] = sub->GetFloat("scale_npc_human_bosses", 1.0f);
						weaponItem.iTeam = TEAM_DECEASED;
						weaponItem.iGameMode = gamemode;
						pszPlayerWeaponData.AddToTail(weaponItem);
					}
				}

				pkvShared = pkvDefault->FindKey("LimbData");
				if (pkvShared)
				{
					for (KeyValues *sub = pkvShared->GetFirstSubKey(); sub; sub = sub->GetNextKey())
					{
						DataPlayerItem_Shared_LimbInfo_t limbItem;
						Q_strncpy(limbItem.szLimb, sub->GetName(), 32);
						limbItem.flScale = sub->GetFloat("scale", 1.0f);
						limbItem.flHealth = sub->GetFloat("health", 10.0f);
						limbItem.iTeam = TEAM_DECEASED;
						limbItem.iGameMode = gamemode;
						pszPlayerLimbData.AddToTail(limbItem);
					}
				}
			}

			pkvParseData->deleteThis();
		}
		else
			Warning("Failed to parse: gamemode specific data!\n");
	}

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/game_base_player_powerups");
	if (pkvParseData)
	{
		for (KeyValues *sub = pkvParseData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			const char *powerupName = sub->GetName();

			DataPlayerItem_Player_PowerupItem_t item;
			Q_strncpy(item.pchName, powerupName, 32);
			Q_strncpy(item.pchModelPath, sub->GetString("Model"), MAX_WEAPON_STRING);
			Q_strncpy(item.pchActivationSoundScript, sub->GetString("Soundscript"), 64);

			item.iFlag = 0;
			if (!strcmp(powerupName, "Critical"))
				item.iFlag = PERK_POWERUP_CRITICAL;
			else if (!strcmp(powerupName, "Cheetah"))
				item.iFlag = PERK_POWERUP_CHEETAH;
			else if (!strcmp(powerupName, "Predator"))
				item.iFlag = PERK_POWERUP_PREDATOR;
			else if (!strcmp(powerupName, "Painkiller"))
				item.iFlag = PERK_POWERUP_PAINKILLER;
			else if (!strcmp(powerupName, "Nanites"))
				item.iFlag = PERK_POWERUP_NANITES;

			if (item.iFlag <= 0)
				continue;

			item.flHealth = sub->GetFloat("Health", 0.0f);
			item.flSpeed = sub->GetFloat("Speed", 0.0f);
			item.flJumpHeight = sub->GetFloat("Jump", 0.0f);
			item.flLeapLength = sub->GetFloat("Leap", 0.0f);
			item.flHealthRegenerationRate = sub->GetFloat("HealthRegen", 0.0f);
			item.flExtraFactor = sub->GetFloat("ExtraFactor", 0.0f);
			item.flPerkDuration = sub->GetFloat("Duration", 10.0f);

#ifdef CLIENT_DLL
			char pchPowerupIcon[64];

			Q_snprintf(pchPowerupIcon, 64, "powerup_%s_active", powerupName);
			Q_strlower(pchPowerupIcon);
			item.pIconPowerupActive = gHUD.GetIcon(pchPowerupIcon);

			Q_snprintf(pchPowerupIcon, 64, "powerup_%s_inactive", powerupName);
			Q_strlower(pchPowerupIcon);
			item.pIconPowerupInactive = gHUD.GetIcon(pchPowerupIcon);
#endif

			pszPlayerPowerupData.AddToTail(item);
		}

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/game_base_player_powerups!\n");

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/gamemode_shared");
	if (pkvParseData)
	{
		pszGamemodeData.flXPRoundWinArena = clamp(pkvParseData->GetFloat("round_win_arena", 1.0f), 0.0f, 2.5f);

		pszGamemodeData.flXPGameWinObjective = clamp(pkvParseData->GetFloat("game_win_objective", 5.0f), 0.0f, 8.0f);
		pszGamemodeData.flXPGameWinArena = clamp(pkvParseData->GetFloat("game_win_arena", 2.5f), 0.0f, 4.0f);

		pszGamemodeData.flXPScaleFactor = pkvParseData->GetFloat("xp_scale_factor", 3.0f);
		pszGamemodeData.flXPScaleFactorMinAvgLvL = pkvParseData->GetFloat("xp_scale_factor_min_avg_lvl", 50.0f);
		pszGamemodeData.flXPScaleFactorMaxAvgLvL = pkvParseData->GetFloat("xp_scale_factor_max_avg_lvl", 100.0f);

		pszGamemodeData.flZombieCreditsPercentToLose = clamp(pkvParseData->GetFloat("zombie_credits_lost_death", 50.0f), 0.0f, 100.0f);
		pszGamemodeData.flArenaHardModeXPMultiplier = MAX(pkvParseData->GetFloat("arena_hardmode_multiplier", 2.0f), 1.0f);

		pszGamemodeData.iKillsRequiredToPerk = pkvParseData->GetInt("perk_kills_required", 50);

		pszGamemodeData.iDefaultZombieCredits = pkvParseData->GetInt("zombie_credits_start", 10);
		pszGamemodeData.flAmmoResupplyTime = pkvParseData->GetFloat("ammo_resupply_time", 30.0f);

		pszGamemodeData.iMaxAmmoReplenishWithinInterval = pkvParseData->GetInt("max_ammo_replenishes", 4);
		pszGamemodeData.flMaxAmmoReplensihInterval = pkvParseData->GetFloat("ammo_replenish_interval", 60.0f);
		pszGamemodeData.flAmmoReplenishPenalty = pkvParseData->GetFloat("ammo_replenish_penalty", 120.0f);

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/gamemode_shared!\n");

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/game_base_shared");
	if (pkvParseData)
	{
		KeyValues *pkvParticles = pkvParseData->FindKey("Particles");
		if (pkvParticles)
		{
			KeyValues *pkvBloodField = pkvParticles->FindKey("Blood");
			if (pkvBloodField)
			{
				for (KeyValues *sub = pkvBloodField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataBloodParticleItem_t item;
					Q_strncpy(item.szDefault, sub->GetName(), MAX_MAP_NAME);
					Q_strncpy(item.szExtreme, sub->GetString(), MAX_MAP_NAME);
					pszBloodParticleData.AddToTail(item);
				}
			}
			else
				Warning("Failed to load blood particles!\n");

			pkvBloodField = pkvParticles->FindKey("Headshot");
			if (pkvBloodField)
			{
				for (KeyValues *sub = pkvBloodField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataBloodParticleItem_t item;
					Q_strncpy(item.szDefault, sub->GetName(), MAX_MAP_NAME);
					Q_strncpy(item.szExtreme, sub->GetString(), MAX_MAP_NAME);
					pszHeadshotParticleData.AddToTail(item);
				}
			}
			else
				Warning("Failed to load headshot particles!\n");

			pkvBloodField = pkvParticles->FindKey("Bleedout");
			if (pkvBloodField)
			{
				for (KeyValues *sub = pkvBloodField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataBloodParticleItem_t item;
					Q_strncpy(item.szDefault, sub->GetName(), MAX_MAP_NAME);
					Q_strncpy(item.szExtreme, sub->GetString(), MAX_MAP_NAME);
					pszBleedoutParticleData.AddToTail(item);
				}
			}
			else
				Warning("Failed to load bleedout/blood-pool particles!\n");

			pkvBloodField = pkvParticles->FindKey("BloodExplosion");
			if (pkvBloodField)
			{
				for (KeyValues *sub = pkvBloodField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataBloodParticleItem_t item;
					Q_strncpy(item.szDefault, sub->GetName(), MAX_MAP_NAME);
					Q_strncpy(item.szExtreme, sub->GetString(), MAX_MAP_NAME);
					pszBloodExplosionParticleData.AddToTail(item);
				}
			}
			else
				Warning("Failed to load blood mist / blood explosion particles!\n");

			pkvBloodField = pkvParticles->FindKey("GibBlood");
			if (pkvBloodField)
			{
				for (KeyValues *sub = pkvBloodField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataGibBloodParticleItem_t item;
					Q_strncpy(item.szLimb, sub->GetName(), MAX_MAP_NAME);
					Q_strncpy(item.szDefault, sub->GetString("default"), MAX_MAP_NAME);
					Q_strncpy(item.szExtreme, sub->GetString("extreme"), MAX_MAP_NAME);
					pszGibParticleData.AddToTail(item);
				}
			}
			else
				Warning("Failed to load gib impact/blood particles!\n");

			pkvBloodField = pkvParticles->FindKey("ExplosionData");
			if (pkvBloodField)
			{
				for (KeyValues *sub = pkvBloodField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataExplosiveItem_t item;
					item.iType = atoi(sub->GetName());
					Q_strncpy(item.szParticle, sub->GetString("Particle"), MAX_MAP_NAME);
					item.flRadius = sub->GetFloat("Radius");
					item.flPlayerDamage = sub->GetFloat("PlayerDamage");
					item.flNPCDamage = sub->GetFloat("NPCDamage");
					pszExplosionData.AddToTail(item);
				}
			}
			else
				Warning("Failed to load explosion particles & data!\n");
		}
		else
			Warning("Failed to parse: data/game/game_base_shared!\n");

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/game_base_shared!\n");

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/inventory_items");
	if (pkvParseData)
	{
		ParseInventoryData(pkvParseData);
		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/inventory_items!\n");

#ifdef CLIENT_DLL
	ParseSoundsetFile("data/soundsets/default.txt"); // Load default soundsets first!

	char filePath[MAX_WEAPON_STRING];
	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx("data/soundsets/*.txt", "MOD", &findHandle);
	while (pFilename)
	{
		if (Q_strcmp(pFilename, "default.txt"))
		{
			Q_snprintf(filePath, MAX_WEAPON_STRING, "data/soundsets/%s", pFilename);
			ParseSoundsetFile(filePath);
		}
		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/settings/tips");
	if (pkvParseData)
	{
		for (KeyValues *sub = pkvParseData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			DataLoadingTipsItem_t item;
			Q_strncpy(item.pchToken, sub->GetString(), MAX_MAP_NAME);
			Q_strncpy(item.pchIconPath, sub->GetName(), MAX_MAP_NAME);
			pszLoadingTipData.AddToTail(item);
		}

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/settings/tips!\n");

	pFilename = filesystem->FindFirstEx("data/characters/*.txt", "MOD", &findHandle);
	while (pFilename)
	{
		Q_snprintf(filePath, MAX_WEAPON_STRING, "data/characters/%s", pFilename);
		ParseCharacterFile(filePath);
		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
#endif

	return true;
}

// Precache Stuff
bool CGameDefinitionsShared::Precache(void)
{
#ifdef CLIENT_DLL
	if (!engine->IsInGame())
		return false;
#endif

	for (int i = 0; i < pszBloodParticleData.Count(); i++)
	{
		PrecacheParticleSystem(pszBloodParticleData[i].szDefault);
		PrecacheParticleSystem(pszBloodParticleData[i].szExtreme);
	}

	for (int i = 0; i < pszBleedoutParticleData.Count(); i++)
	{
		PrecacheParticleSystem(pszBleedoutParticleData[i].szDefault);
		PrecacheParticleSystem(pszBleedoutParticleData[i].szExtreme);
	}

	for (int i = 0; i < pszHeadshotParticleData.Count(); i++)
	{
		PrecacheParticleSystem(pszHeadshotParticleData[i].szDefault);
		PrecacheParticleSystem(pszHeadshotParticleData[i].szExtreme);
	}

	for (int i = 0; i < pszBloodExplosionParticleData.Count(); i++)
	{
		PrecacheParticleSystem(pszBloodExplosionParticleData[i].szDefault);
		PrecacheParticleSystem(pszBloodExplosionParticleData[i].szExtreme);
	}

	for (int i = 0; i < pszGibParticleData.Count(); i++)
	{
		PrecacheParticleSystem(pszGibParticleData[i].szDefault);
		PrecacheParticleSystem(pszGibParticleData[i].szExtreme);
	}

	for (int i = 0; i < pszExplosionData.Count(); i++)
	{
		PrecacheParticleSystem(pszExplosionData[i].szParticle);
	}

	for (int i = 0; i < pszItemSharedData.Count(); i++)
	{
#ifndef CLIENT_DLL
		CBaseAnimating::PrecacheScriptSound(pszItemSharedData[i].szSoundScriptSuccess);
		CBaseAnimating::PrecacheScriptSound(pszItemSharedData[i].szSoundScriptFailure);
		if (pszItemSharedData[i].szSoundScriptExchange && pszItemSharedData[i].szSoundScriptExchange[0])
			CBaseAnimating::PrecacheScriptSound(pszItemSharedData[i].szSoundScriptExchange);
#endif
		CBaseAnimating::PrecacheModel(pszItemSharedData[i].szModelPath);
	}

	for (int i = 0; i < pszPlayerPowerupData.Count(); i++)
	{
		if (pszPlayerPowerupData[i].pchModelPath && pszPlayerPowerupData[i].pchModelPath[0])
			CBaseAnimating::PrecacheModel(pszPlayerPowerupData[i].pchModelPath);

		if (pszPlayerPowerupData[i].pchActivationSoundScript && pszPlayerPowerupData[i].pchActivationSoundScript[0])
			CBaseAnimating::PrecacheScriptSound(pszPlayerPowerupData[i].pchActivationSoundScript);
	}

	// Legacy Particles
	PrecacheParticleSystem("blood_impact_red_01");
	PrecacheParticleSystem("blood_impact_green_01");
	PrecacheParticleSystem("blood_impact_yellow_01");

	// Other
	PrecacheParticleSystem("headshot");
	PrecacheParticleSystem("Rocket");
	PrecacheParticleSystem("water_splash_01"); // TFO Rain Effects
	PrecacheParticleSystem("bb2_item_spawn");
	PrecacheParticleSystem("blood_mist");

	// Helms
	PrecacheParticleSystem("helm_aurora_parent_green");
	PrecacheParticleSystem("helm_aurora_parent_orange");
	PrecacheParticleSystem("helm_aurora_parent_purple");
	PrecacheParticleSystem("helm_halo01");

	// Impacts
	PrecacheParticleSystem("impact_concrete");
	PrecacheParticleSystem("impact_dirt");
	PrecacheParticleSystem("impact_metal");
	PrecacheParticleSystem("impact_computer");
	PrecacheParticleSystem("impact_wood");
	PrecacheParticleSystem("impact_glass");

	PrecacheParticleSystem("water_splash_01");
	PrecacheParticleSystem("water_splash_02");
	PrecacheParticleSystem("water_splash_03");

	PrecacheParticleSystem("weapon_muzzle_smoke_b");

	PrecacheParticleSystem("flame_tiny");
	PrecacheParticleSystem("weapon_flame_fire_1");


	return true;
}

int CGameDefinitionsShared::CalculatePointsForLevel(int lvl)
{
	if (lvl <= 1)
		return 0;

	if (lvl >= MAX_PLAYER_LEVEL)
		return MAX_PLAYER_TALENTS;

	float points = MIN((lvl - 1), 98) + ((lvl >= 100) ? 2 : 0);
	if (lvl > 100)
	{
		float flLvL = ((float)lvl) - 100.0f;
		points += floor((clamp((flLvL / (MAX_PLAYER_LEVEL - 100.0f)), 0.0f, 1.0f) * (MAX_PLAYER_TALENTS - 100.0f)));
	}

	return ((int)points);
}

#ifdef CLIENT_DLL
void CGameDefinitionsShared::LoadClientModels(void)
{
	m_pClientModelList.Purge();

	int numCharacters = pszPlayerSurvivorData.Count();
	if (numCharacters <= 0)
		return;

	for (int i = 0; i < numCharacters; i++)
	{
		DataPlayerItem_Survivor_Shared_t &item = pszPlayerSurvivorData[i];

		item.m_pClientModelPtrHuman = LoadClientModel(item.szHumanModelPath);
		item.m_pClientModelPtrHumanHands = LoadClientModel(item.szHumanHandsPath);
		item.m_pClientModelPtrHumanBody = LoadClientModel(item.szHumanBodyPath);

		item.m_pClientModelPtrZombie = LoadClientModel(item.szZombieModelPath);
		item.m_pClientModelPtrZombieHands = LoadClientModel(item.szZombieHandsPath);
		item.m_pClientModelPtrZombieBody = LoadClientModel(item.szZombieBodyPath);

		const char *humanGibs[PLAYER_GIB_GROUPS_MAX] = {
			item.szHumanGibHead,
			item.szHumanGibArmLeft,
			item.szHumanGibArmRight,
			item.szHumanGibLegLeft,
			item.szHumanGibLegRight
		};

		const char *zombieGibs[PLAYER_GIB_GROUPS_MAX] = {
			item.szDeceasedGibHead,
			item.szDeceasedGibArmLeft,
			item.szDeceasedGibArmRight,
			item.szDeceasedGibLegLeft,
			item.szDeceasedGibLegRight
		};

		for (int i = 0; i < PLAYER_GIB_GROUPS_MAX; i++)
		{
			item.m_pClientModelPtrGibsHuman[i] = NULL;
			if (!(humanGibs[i] && humanGibs[i][0]))
				continue;

			item.m_pClientModelPtrGibsHuman[i] = LoadClientModel(humanGibs[i]);
		}

		for (int i = 0; i < PLAYER_GIB_GROUPS_MAX; i++)
		{
			item.m_pClientModelPtrGibsZombie[i] = NULL;
			if (!(zombieGibs[i] && zombieGibs[i][0]))
				continue;

			item.m_pClientModelPtrGibsZombie[i] = LoadClientModel(zombieGibs[i]);
		}
	}
}
#endif

// Player Data:
const DataPlayerItem_Shared_t *CGameDefinitionsShared::GetPlayerSharedData(void)
{
	return &pszPlayerSharedData;
}

const DataPlayerItem_Player_Shared_t *CGameDefinitionsShared::GetPlayerGameModeData(int iTeam)
{
	Assert(pszPlayerData.Count() > 0);

	for (int i = 0; i < pszPlayerData.Count(); i++)
	{
		if (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == pszPlayerData[i].iGameMode) && (iTeam == pszPlayerData[i].iTeam))
			return &pszPlayerData[i];
	}

	return &pszPlayerData[0];
}

const DataPlayerItem_MiscSkillInfo_t *CGameDefinitionsShared::GetPlayerMiscSkillData(void)
{
	return &pszPlayerMiscSkillData;
}

const DataPlayerItem_Humans_Skills_t *CGameDefinitionsShared::GetPlayerHumanSkillData(void)
{
	return &pszHumanSkillData;
}

const DataPlayerItem_Zombies_Skills_t *CGameDefinitionsShared::GetPlayerZombieSkillData(void)
{
	return &pszZombieSkillData;
}

const DataPlayerItem_ZombieRageMode_t *CGameDefinitionsShared::GetPlayerZombieRageData(void)
{
	return &pszZombieRageModeData;
}

#ifdef CLIENT_DLL
void CGameDefinitionsShared::ParseCharacterFile(const char *file)
{
	KeyValues *survivorData = new KeyValues("CharacterInfo");
	if (survivorData->LoadFromFile(filesystem, file, "MOD"))
	{
		for (KeyValues *sub = survivorData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			DataPlayerItem_Survivor_Shared_t item;
			Q_strncpy(item.szSurvivorName, sub->GetName(), MAX_MAP_NAME);
			item.bGender = (sub->GetInt("gender", 1) >= 1); // TRUE = male, FALSE = female...

			// Model Info
			Q_strncpy(item.szHumanModelPath, sub->GetString("modelname"), MAX_WEAPON_STRING);
			Q_strncpy(item.szHumanHandsPath, sub->GetString("hands"), MAX_WEAPON_STRING);
			Q_strncpy(item.szHumanBodyPath, sub->GetString("body"), MAX_WEAPON_STRING);

			Q_strncpy(item.szZombieModelPath, sub->GetString("modelname_zmb"), MAX_WEAPON_STRING);
			Q_strncpy(item.szZombieHandsPath, sub->GetString("hands_zmb"), MAX_WEAPON_STRING);
			Q_strncpy(item.szZombieBodyPath, sub->GetString("body_zmb"), MAX_WEAPON_STRING);

			// Gib Info
			KeyValues *pkvGibInfo = sub->FindKey("GibInfoHuman");
			Q_strncpy(item.szHumanGibHead, pkvGibInfo ? pkvGibInfo->GetString("head") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szHumanGibArmRight, pkvGibInfo ? pkvGibInfo->GetString("arms_right") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szHumanGibArmLeft, pkvGibInfo ? pkvGibInfo->GetString("arms_left") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szHumanGibLegRight, pkvGibInfo ? pkvGibInfo->GetString("legs_right") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szHumanGibLegLeft, pkvGibInfo ? pkvGibInfo->GetString("legs_left") : "", MAX_WEAPON_STRING);

			pkvGibInfo = sub->FindKey("GibInfoZombie");
			Q_strncpy(item.szDeceasedGibHead, pkvGibInfo ? pkvGibInfo->GetString("head") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szDeceasedGibArmRight, pkvGibInfo ? pkvGibInfo->GetString("arms_right") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szDeceasedGibArmLeft, pkvGibInfo ? pkvGibInfo->GetString("arms_left") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szDeceasedGibLegRight, pkvGibInfo ? pkvGibInfo->GetString("legs_right") : "", MAX_WEAPON_STRING);
			Q_strncpy(item.szDeceasedGibLegLeft, pkvGibInfo ? pkvGibInfo->GetString("legs_left") : "", MAX_WEAPON_STRING);

			Q_strncpy(item.szFriendlySurvivorName, sub->GetString("name"), MAX_MAP_NAME);
			Q_strncpy(item.szFriendlyDescription, sub->GetString("description"), 128);
			Q_strncpy(item.szSequence, sub->GetString("sequence"), MAX_MAP_NAME);

			// Customization Info
			KeyValues *pkvCustomizationInfo = sub->FindKey("CustomizationInfo");
			item.iSkins = pkvCustomizationInfo ? pkvCustomizationInfo->GetInt("skins") : 0;
			item.iSpecialHeadItems = pkvCustomizationInfo ? pkvCustomizationInfo->GetInt("extra_head") : 0;
			item.iSpecialBodyItems = pkvCustomizationInfo ? pkvCustomizationInfo->GetInt("extra_body") : 0;
			item.iSpecialLeftLegItems = pkvCustomizationInfo ? pkvCustomizationInfo->GetInt("extra_leg_left") : 0;
			item.iSpecialRightLegItems = pkvCustomizationInfo ? pkvCustomizationInfo->GetInt("extra_leg_right") : 0;

			// Camera Info
			item.vecPosition = Vector(sub->GetFloat("origin_x"), sub->GetFloat("origin_y"), sub->GetFloat("origin_z"));
			item.angAngles = QAngle(sub->GetFloat("angles_x"), sub->GetFloat("angles_y"), sub->GetFloat("angles_z"));

			item.m_pClientModelPtrHuman = engine->LoadModel(item.szHumanModelPath);
			item.m_pClientModelPtrZombie = engine->LoadModel(item.szZombieModelPath);

			pszPlayerSurvivorData.AddToTail(item);
		}
	}
	else
		Warning("Failed to parse custom: %s!\n", file);
	survivorData->deleteThis();
}

const DataPlayerItem_Survivor_Shared_t *CGameDefinitionsShared::GetSurvivorDataForIndex(int index)
{
	if ((index >= 0) && (index < pszPlayerSurvivorData.Count()))
		return &pszPlayerSurvivorData[index];

	// Return a default, if we couldn't find the default military mdl, return the fist item:
	int itemCount = pszPlayerSurvivorData.Count();
	if (itemCount)
	{
		const char *defaultHuman = DEFAULT_PLAYER_MODEL(TEAM_HUMANS);
		for (int i = 0; i < itemCount; i++)
		{
			if (!strcmp(pszPlayerSurvivorData[i].szHumanModelPath, defaultHuman))
				return &pszPlayerSurvivorData[i];
		}

		return &pszPlayerSurvivorData[0];
	}

	return NULL;
}

const DataPlayerItem_Survivor_Shared_t *CGameDefinitionsShared::GetSurvivorDataForIndex(const char *name, bool bNoDefault)
{
	int itemCount = pszPlayerSurvivorData.Count();
	if (itemCount)
	{
		for (int i = 0; i < itemCount; i++)
		{
			if (!strcmp(pszPlayerSurvivorData[i].szSurvivorName, name))
				return &pszPlayerSurvivorData[i];
		}

		// Return a default, if we couldn't find the default military mdl, return the fist item:
		if (bNoDefault == false)
		{
			const char *defaultHuman = DEFAULT_PLAYER_MODEL(TEAM_HUMANS);
			for (int i = 0; i < itemCount; i++)
			{
				if (!strcmp(pszPlayerSurvivorData[i].szHumanModelPath, defaultHuman))
					return &pszPlayerSurvivorData[i];
			}

			return &pszPlayerSurvivorData[0];
		}
	}

	return NULL;
}
#endif

const DataPlayerItem_Player_PowerupItem_t *CGameDefinitionsShared::GetPlayerPowerupData(const char *powerupName)
{
	int index = GetIndexForPowerup(powerupName);
	if (index != -1)
		return &pszPlayerPowerupData[index];

	return NULL;
}

const DataPlayerItem_Player_PowerupItem_t *CGameDefinitionsShared::GetPlayerPowerupData(int powerupFlag)
{
	for (int i = 0; i < pszPlayerPowerupData.Count(); ++i)
	{
		if (pszPlayerPowerupData[i].iFlag == powerupFlag)
			return &pszPlayerPowerupData[i];
	}

	return NULL;
}

float CGameDefinitionsShared::GetPlayerSharedValue(const char *name, int iTeam)
{
	if (iTeam == TEAM_HUMANS)
	{
		if (!strcmp(name, "MaxLevel"))
			return (float)pszPlayerSharedData.iMaxLevel;
		else if (!strcmp(name, "XPPerLevel"))
			return (float)pszPlayerSharedData.iXPIncreasePerLevel;
		else if (!strcmp(name, "TeamBonusDamageIncrease"))
			return (float)pszPlayerSharedData.iTeamBonusDamageIncrease;
		else if (!strcmp(name, "TeamBonusXPIncrease"))
			return (float)pszPlayerSharedData.iTeamBonusXPIncrease;
		else if (!strcmp(name, "PerkTime"))
			return pszPlayerSharedData.flPerkTime;
		else if (!strcmp(name, "Level"))
			return (float)pszPlayerSharedData.iLevel;
		else if (!strcmp(name, "InfectionStart"))
			return (float)pszPlayerSharedData.iInfectionStartPercent;
		else if (!strcmp(name, "InfectionTime"))
			return pszPlayerSharedData.flInfectionDuration;
	}

	if (!HL2MPRules())
		return 0.0f;

	int currentGamemode = HL2MPRules()->GetCurrentGamemode();

	for (int i = 0; i < pszPlayerData.Count(); i++)
	{
		if ((pszPlayerData[i].iTeam == iTeam) && (pszPlayerData[i].iGameMode == currentGamemode))
		{
			if (!strcmp(name, "Health"))
				return (float)pszPlayerData[i].iHealth;
			else if (!strcmp(name, "Armor"))
				return (float)pszPlayerData[i].iArmor;
			else if (!strcmp(name, "ArmorType"))
				return (float)pszPlayerData[i].iArmorType;
			else if (!strcmp(name, "Speed"))
				return pszPlayerData[i].flSpeed;
			else if (!strcmp(name, "Jump"))
				return pszPlayerData[i].flJumpHeight;
			else if (!strcmp(name, "Leap"))
				return pszPlayerData[i].flLeapLength;
			else if (!strcmp(name, "HealthRegen"))
				return pszPlayerData[i].flHealthRegenerationRate;
		}
	}

	return 0.0f;
}

float CGameDefinitionsShared::GetPlayerSkillValue(int iSkillType, int iTeam, int iSubType)
{
	if (iTeam == TEAM_HUMANS)
	{
		switch (iSkillType)
		{
		case PLAYER_SKILL_HUMAN_SPEED:
			return pszHumanSkillData.flAgilitySpeed;
		case PLAYER_SKILL_HUMAN_ACROBATICS:
		{
			if (iSubType == PLAYER_SKILL_HUMAN_JUMP)
				return pszHumanSkillData.flAgilityJump;
			else if (iSubType == PLAYER_SKILL_HUMAN_LEAP)
				return pszHumanSkillData.flAgilityLeap;

			break;
		}
		case PLAYER_SKILL_HUMAN_SLIDE:
			return pszHumanSkillData.flAgilitySlide;
		case PLAYER_SKILL_HUMAN_ENHANCED_REFLEXES:
			return pszHumanSkillData.flAgilityEnhancedReflexes;
		case PLAYER_SKILL_HUMAN_MELEE_SPEED:
			return pszHumanSkillData.flAgilityMeleeSpeed;
		case PLAYER_SKILL_HUMAN_LIGHTWEIGHT:
			return pszHumanSkillData.flAgilityLightweight;
		case PLAYER_SKILL_HUMAN_WEIGHTLESS:
			return pszHumanSkillData.flAgilityWeightless;
		case PLAYER_SKILL_HUMAN_HEALTHREGEN:
			return pszHumanSkillData.flAgilityHealthRegen;
		case PLAYER_SKILL_HUMAN_REALITY_PHASE:
			return pszHumanSkillData.flAgilityRealityPhase;

		case PLAYER_SKILL_HUMAN_HEALTH:
			return pszHumanSkillData.flStrengthHealth;
		case PLAYER_SKILL_HUMAN_IMPENETRABLE:
			return pszHumanSkillData.flStrengthImpenetrable;
		case PLAYER_SKILL_HUMAN_PAINKILLER:
			return pszHumanSkillData.flStrengthPainkiller;
		case PLAYER_SKILL_HUMAN_LIFE_LEECH:
			return pszHumanSkillData.flStrengthLifeLeech;
		case PLAYER_SKILL_HUMAN_POWER_KICK:
			return pszHumanSkillData.flStrengthPowerKick;
		case PLAYER_SKILL_HUMAN_BLEED:
			return pszHumanSkillData.flStrengthBleed;
		case PLAYER_SKILL_HUMAN_CRIPPLING_BLOW:
			return pszHumanSkillData.flStrengthCripplingBlow;
		case PLAYER_SKILL_HUMAN_ARMOR_MASTER:
			return pszHumanSkillData.flStrengthArmorMaster;
		case PLAYER_SKILL_HUMAN_BLOOD_RAGE:
			return pszHumanSkillData.flStrengthBloodRage;

		case PLAYER_SKILL_HUMAN_RESOURCEFUL:
			return pszHumanSkillData.flFirearmResourceful;
		case PLAYER_SKILL_HUMAN_BLAZING_AMMO:
			return pszHumanSkillData.flFirearmBlazingAmmo;
		case PLAYER_SKILL_HUMAN_COLDSNAP:
			return pszHumanSkillData.flFirearmColdsnap;
		case PLAYER_SKILL_HUMAN_EMPOWERED_BULLETS:
			return pszHumanSkillData.flFirearmEmpoweredBullets;
		case PLAYER_SKILL_HUMAN_MAGAZINE_REFILL:
			return pszHumanSkillData.flFirearmMagazineRefill;
		case PLAYER_SKILL_HUMAN_GUNSLINGER:
			return pszHumanSkillData.flFirearmGunslinger;
		}
	}
	else if (iTeam == TEAM_DECEASED)
	{
		switch (iSkillType)
		{
		case PLAYER_SKILL_ZOMBIE_HEALTH:
			return pszZombieSkillData.flHealth;
		case PLAYER_SKILL_ZOMBIE_DAMAGE:
			return pszZombieSkillData.flDamage;
		case PLAYER_SKILL_ZOMBIE_DAMAGE_REDUCTION:
			return pszZombieSkillData.flDamageReduction;
		case PLAYER_SKILL_ZOMBIE_SPEED:
			return pszZombieSkillData.flSpeed;
		case PLAYER_SKILL_ZOMBIE_JUMP:
			return pszZombieSkillData.flJump;
		case PLAYER_SKILL_ZOMBIE_LEAP:
			return pszZombieSkillData.flLeap;
		case PLAYER_SKILL_ZOMBIE_DEATH:
			return pszZombieSkillData.flDeath;
		case PLAYER_SKILL_ZOMBIE_LIFE_LEECH:
			return pszZombieSkillData.flLifeLeech;
		case PLAYER_SKILL_ZOMBIE_HEALTH_REGEN:
			return pszZombieSkillData.flHealthRegen;
		case PLAYER_SKILL_ZOMBIE_MASS_INVASION:
			return pszZombieSkillData.flMassInvasion;
		}
	}

	return 0.0f;
}

int CGameDefinitionsShared::GetIndexForPowerup(const char *name) const
{
	for (int i = 0; i < pszPlayerPowerupData.Count(); ++i)
	{
		if (!strcmp(pszPlayerPowerupData[i].pchName, name))
			return i;
	}

	return -1;
}

#ifdef CLIENT_DLL
const model_t *CGameDefinitionsShared::GetPlayerGibModelPtrForGibID(const DataPlayerItem_Survivor_Shared_t &data, bool bHuman, int gibID)
{
	switch (gibID)
	{
	case GIB_NO_HEAD:
		return (bHuman ? data.m_pClientModelPtrGibsHuman[0] : data.m_pClientModelPtrGibsZombie[0]);		
	case GIB_NO_ARM_LEFT:
		return (bHuman ? data.m_pClientModelPtrGibsHuman[1] : data.m_pClientModelPtrGibsZombie[1]);
	case GIB_NO_ARM_RIGHT:
		return (bHuman ? data.m_pClientModelPtrGibsHuman[2] : data.m_pClientModelPtrGibsZombie[2]);
	case GIB_NO_LEG_LEFT:
		return (bHuman ? data.m_pClientModelPtrGibsHuman[3] : data.m_pClientModelPtrGibsZombie[3]);
	case GIB_NO_LEG_RIGHT:
		return (bHuman ? data.m_pClientModelPtrGibsHuman[4] : data.m_pClientModelPtrGibsZombie[4]);
	}

	return NULL;
}

const char *CGameDefinitionsShared::GetPlayerGibForModel(const char *survivor, bool bHuman, const char *gib)
{
	const DataPlayerItem_Survivor_Shared_t *data = GetSurvivorDataForIndex(survivor);
	if (data == NULL)
		return "";

	if (!strcmp(gib, "head"))
		return (bHuman ? data->szHumanGibHead : data->szDeceasedGibHead);
	else if (!strcmp(gib, "arms_left"))
		return (bHuman ? data->szHumanGibArmLeft : data->szDeceasedGibArmLeft);
	else if (!strcmp(gib, "arms_right"))
		return (bHuman ? data->szHumanGibArmRight : data->szDeceasedGibArmRight);
	else if (!strcmp(gib, "legs_left"))
		return (bHuman ? data->szHumanGibLegLeft : data->szDeceasedGibLegLeft);
	else if (!strcmp(gib, "legs_right"))
		return (bHuman ? data->szHumanGibLegRight : data->szDeceasedGibLegRight);

	return "";
}

bool CGameDefinitionsShared::DoesPlayerHaveGibForLimb(const char *survivor, bool bHuman, int gibID)
{
	const DataPlayerItem_Survivor_Shared_t *data = GetSurvivorDataForIndex(survivor);
	if (data == NULL)
		return false;

	if ((gibID == GIB_NO_HEAD) && (bHuman ? (data->m_pClientModelPtrGibsHuman[0] != NULL) : (data->m_pClientModelPtrGibsZombie[0] != NULL)))
		return true;

	if ((gibID == GIB_NO_ARM_LEFT) && (bHuman ? (data->m_pClientModelPtrGibsHuman[1] != NULL) : (data->m_pClientModelPtrGibsZombie[1] != NULL)))
		return true;

	if ((gibID == GIB_NO_ARM_RIGHT) && (bHuman ? (data->m_pClientModelPtrGibsHuman[2] != NULL) : (data->m_pClientModelPtrGibsZombie[2] != NULL)))
		return true;

	if ((gibID == GIB_NO_LEG_LEFT) && (bHuman ? (data->m_pClientModelPtrGibsHuman[3] != NULL) : (data->m_pClientModelPtrGibsZombie[3] != NULL)))
		return true;

	if ((gibID == GIB_NO_LEG_RIGHT) && (bHuman ? (data->m_pClientModelPtrGibsHuman[4] != NULL) : (data->m_pClientModelPtrGibsZombie[4] != NULL)))
		return true;

	return false;
}
#endif

float CGameDefinitionsShared::GetPlayerFirearmDamageScale(const char *weapon, int entityType, int team)
{
	if (!HL2MPRules())
		return 1.0f;

	float flScale = 1.0f;
	int currentGamemode = HL2MPRules()->GetCurrentGamemode();

	for (int i = 0; i < pszPlayerWeaponData.Count(); i++)
	{
		if ((entityType >= 0) && (entityType < NUM_DAMAGE_SCALES) && (pszPlayerWeaponData[i].iTeam == team) && (pszPlayerWeaponData[i].iGameMode == currentGamemode) && !strcmp(pszPlayerWeaponData[i].szWeaponClass, weapon))
			return pszPlayerWeaponData[i].flDamageScale[entityType];
	}

	return flScale;
}

float CGameDefinitionsShared::GetPlayerLimbData(const char *limb, int team, bool bHealth)
{
	float flValue = ((bHealth) ? 10.0f : 1.0f);

	if (!HL2MPRules())
		return flValue;

	int currentGamemode = HL2MPRules()->GetCurrentGamemode();

	for (int i = 0; i < pszPlayerLimbData.Count(); i++)
	{
		if ((pszPlayerLimbData[i].iTeam == team) && (pszPlayerLimbData[i].iGameMode == currentGamemode) && !strcmp(pszPlayerLimbData[i].szLimb, limb))
			return (bHealth ? pszPlayerLimbData[i].flHealth : pszPlayerLimbData[i].flScale);
	}

	return flValue;
}

// Inventory Data
void CGameDefinitionsShared::ParseInventoryData(KeyValues *pkvData, bool bIsMapItem)
{
	if (pkvData)
	{
		for (KeyValues *sub = pkvData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			DataInventoryItem_Base_t item;
			item.iItemID = (uint)atol(sub->GetName());
			item.iType = sub->GetInt("Type");
			item.iSubType = sub->GetInt("SubType");
			item.iWeight = sub->GetInt("Weight");
			item.iSkin = 0;
			item.bIsMapItem = bIsMapItem;

#ifndef CLIENT_DLL
			item.flScale = 1.0f;
			item.angOffset = QAngle(0, 0, 0);
			item.bDisableRotationFX = (sub->GetInt("DisableRotationFX") >= 1);
#endif

			KeyValues *pkvModel = sub->FindKey("model");
			if (pkvModel)
			{
				Q_strncpy(item.szModelPath, pkvModel->GetString("modelname"), MAX_WEAPON_STRING);
				item.iSkin = pkvModel->GetInt("skin");
#ifndef CLIENT_DLL
				item.flScale = pkvModel->GetFloat("scale", 1.0f);
				item.angOffset = QAngle(
					pkvModel->GetFloat("ang_off_x"),
					pkvModel->GetFloat("ang_off_y"),
					pkvModel->GetFloat("ang_off_z"));
#endif
			}

#ifdef CLIENT_DLL
			item.bShouldRenderIcon = false;
			const char *hudIconPath = sub->GetString("HUDIconTexture");
			int hudTextureID = -1;
			if (hudIconPath && hudIconPath[0])
			{
				item.bShouldRenderIcon = true;
				char pchFilePath[MAX_WEAPON_STRING];
				Q_snprintf(pchFilePath, MAX_WEAPON_STRING, "materials/%s.vmt", hudIconPath);
				if (filesystem->FileExists(pchFilePath, "MOD"))
				{
					hudTextureID = vgui::surface()->CreateNewTextureID();
					vgui::surface()->DrawSetTextureFile(hudTextureID, hudIconPath, true, false);
				}
			}
			item.iHUDTextureID = hudTextureID;	
			Q_strncpy(item.szTitle, sub->GetString("Title"), MAX_WEAPON_STRING);
#else
			item.iLevelReq = sub->GetInt("LevelReq");

			Q_strncpy(item.szSoundScriptSuccess, sub->GetString("PickupSound", "ItemShared.Pickup"), 32);
			Q_strncpy(item.szSoundScriptFailure, sub->GetString("DenySound", "ItemShared.Deny"), 32);
			Q_strncpy(item.szSoundScriptExchange, sub->GetString("ExchangeSound"), 32);

			const char *pszObjIconTexture = sub->GetString("ObjectiveIconTexture");

			item.bGlobalGlow = false;
			item.bEnableObjectiveIcon = (pszObjIconTexture && pszObjIconTexture[0]);
			item.bAutoConsume = sub->GetBool("AutoConsume");
			Q_strncpy(item.szObjectiveIconTexture, pszObjIconTexture, MAX_WEAPON_STRING);

			KeyValues *pkvColor = sub->FindKey("GlobalGlow");
			if (pkvColor)
			{
				item.bGlobalGlow = true;
				item.clGlowColor = Color(
					pkvColor->GetInt("r", 255),
					pkvColor->GetInt("g", 255),
					pkvColor->GetInt("b", 255),
					pkvColor->GetInt("a", 255)
					);
			}
			else
				item.clGlowColor = sub->GetColor("LocalGlow");

			const char *pszEntityLink = sub->GetString("EntityLink");
			item.bHasEntityLink = (pszEntityLink && pszEntityLink[0]);
			Q_strncpy(item.szEntityLink, pszEntityLink, 64);
#endif

			pszItemSharedData.AddToTail(item);
		}
	}
}

void CGameDefinitionsShared::RemoveMapInventoryItems(void)
{
	for (int i = (pszItemSharedData.Count() - 1); i >= 0; i--)
	{
		if (pszItemSharedData[i].bIsMapItem)
		{
#ifdef CLIENT_DLL
			if (pszItemSharedData[i].iHUDTextureID != -1)
				vgui::surface()->DestroyTextureID(pszItemSharedData[i].iHUDTextureID);
#endif

			pszItemSharedData.Remove(i);
		}
	}
}

int CGameDefinitionsShared::GetInventoryMiscDataValue(uint itemID)
{
	for (int i = 0; i < pszItemMiscData.Count(); ++i)
	{
		if (pszItemMiscData[i].iItemID == itemID)
			return pszItemMiscData[i].iValue;
	}

	return 0;
}

int CGameDefinitionsShared::GetInventoryArmorDataValue(const char *name, uint itemID)
{
	for (int i = 0; i < pszItemArmorData.Count(); ++i)
	{
		if (pszItemArmorData[i].iItemID == itemID)
		{
			if (!strcmp(name, "weight"))
				return pszItemArmorData[i].iWeight;
			else if (!strcmp(name, "reduction"))
				return pszItemArmorData[i].iReductionPercent;
		}
	}

	return 0;
}

bool CGameDefinitionsShared::DoesInventoryItemExist(uint itemID, bool bIsMapItem)
{
	return (GetInventoryItemIndex(itemID, bIsMapItem) != -1);
}

int CGameDefinitionsShared::GetInventoryItemIndex(uint itemID, bool bIsMapItem)
{
	for (int i = 0; i < pszItemSharedData.Count(); ++i)
	{
		if ((pszItemSharedData[i].iItemID == itemID) && (pszItemSharedData[i].bIsMapItem == bIsMapItem))
			return i;
	}

	return -1;
}

const char *CGameDefinitionsShared::GetInventoryItemModel(uint itemID, bool bIsMapItem)
{
	int index = GetInventoryItemIndex(itemID, bIsMapItem);
	if (index == -1)
		return "";

	return pszItemSharedData[index].szModelPath;
}

const DataInventoryItem_Base_t *CGameDefinitionsShared::GetInventoryData(uint itemID, bool bIsMapItem)
{
	int index = GetInventoryItemIndex(itemID, bIsMapItem);
	if (index == -1)
		return NULL;

	return &pszItemSharedData[index];
}

// Sound Data
#ifdef CLIENT_DLL
void CGameDefinitionsShared::ParseSoundsetFile(const char *file)
{
	KeyValues *soundPrefixData = new KeyValues("CustomSoundSet");
	if (soundPrefixData->LoadFromFile(filesystem, file, "MOD"))
	{
		for (KeyValues *sub = soundPrefixData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			int npcType = GetEntitySoundTypeFromEntityName(sub->GetName());
			if (npcType == BB2_SoundTypes::TYPE_UNKNOWN)
				continue;

			const char *charLink = sub->GetString("CharacterLink");
			DataSoundPrefixItem_t item;
			item.iType = npcType;
			item.iID = GetNextIndexForSoundSet(npcType, charLink);
			Q_strncpy(item.szFriendlyName, sub->GetString("Name"), MAX_MAP_NAME);
			Q_strncpy(item.szScriptName, sub->GetString("Prefix"), MAX_MAP_NAME);
			Q_strncpy(item.szSurvivorLink, charLink, MAX_MAP_NAME);
			pszSoundPrefixesData.AddToTail(item);
		}
	}
	else
		Warning("Failed to parse custom: %s!\n", file);
	soundPrefixData->deleteThis();
}

const char *CGameDefinitionsShared::GetSoundPrefix(int iType, int index, const char *survivor)
{
	const char *szDefault = "";
	for (int i = 0; i < pszSoundPrefixesData.Count(); ++i)
	{
		if (pszSoundPrefixesData[i].iType != iType)
			continue;

		// Save the default soundset script in case we can't find the desired one. Fallback to this one if necessary.
		if (!(szDefault && szDefault[0]))
			szDefault = pszSoundPrefixesData[i].szScriptName;

		if (pszSoundPrefixesData[i].iID == index)
			return pszSoundPrefixesData[i].szScriptName;
	}

	return szDefault;
}

int CGameDefinitionsShared::GetConVarValueForEntitySoundType(int iType)
{
	if (iType == BB2_SoundTypes::TYPE_ZOMBIE)
		return bb2_sound_zombie.GetInt();
	else if (iType == BB2_SoundTypes::TYPE_FRED)
		return bb2_sound_fred.GetInt();
	else if (iType == BB2_SoundTypes::TYPE_SOLDIER)
		return bb2_sound_military.GetInt();
	else if (iType == BB2_SoundTypes::TYPE_BANDIT)
		return bb2_sound_bandit.GetInt();
	else if (iType == BB2_SoundTypes::TYPE_PLAYER)
		return bb2_sound_player_human.GetInt();
	else if (iType == BB2_SoundTypes::TYPE_DECEASED)
		return bb2_sound_player_deceased.GetInt();
	else if (iType == BB2_SoundTypes::TYPE_ANNOUNCER)
		return bb2_sound_announcer.GetInt();

	DevMsg(2, "Unable to find cvar value for type %i!\n", iType);
	return -1;
}

const char *CGameDefinitionsShared::GetEntityNameFromEntitySoundType(int iType)
{
	const char *name = "UNKNOWN";
	if (iType == BB2_SoundTypes::TYPE_ZOMBIE)
		name = "Walker";
	else if (iType == BB2_SoundTypes::TYPE_FRED)
		name = "Fred";
	else if (iType == BB2_SoundTypes::TYPE_SOLDIER)
		name = "Military";
	else if (iType == BB2_SoundTypes::TYPE_BANDIT)
		name = "Bandit";
	else if (iType == BB2_SoundTypes::TYPE_PLAYER)
		return "HumanPlayer";
	else if (iType == BB2_SoundTypes::TYPE_DECEASED)
		return "ZombiePlayer";
	else if (iType == BB2_SoundTypes::TYPE_ANNOUNCER)
		return "Announcer";

	return name;
}

int CGameDefinitionsShared::GetEntitySoundTypeFromEntityName(const char *name)
{
	if (Q_stristr(name, "Fred"))
		return BB2_SoundTypes::TYPE_FRED;
	else if (Q_stristr(name, "Walker"))
		return BB2_SoundTypes::TYPE_ZOMBIE;
	else if (Q_stristr(name, "Military"))
		return BB2_SoundTypes::TYPE_SOLDIER;
	else if (Q_stristr(name, "Bandit"))
		return BB2_SoundTypes::TYPE_BANDIT;
	else if (Q_stristr(name, "HumanPlayer"))
		return BB2_SoundTypes::TYPE_PLAYER;
	else if (Q_stristr(name, "ZombiePlayer"))
		return BB2_SoundTypes::TYPE_DECEASED;
	else if (Q_stristr(name, "Announcer"))
		return BB2_SoundTypes::TYPE_ANNOUNCER;

	return BB2_SoundTypes::TYPE_UNKNOWN;
}

int CGameDefinitionsShared::GetNextIndexForSoundSet(int iType, const char *survivorLink)
{
	int index = 0;
	for (int i = 0; i < pszSoundPrefixesData.Count(); i++)
	{
		if (survivorLink && survivorLink[0])
		{
			if (strcmp(survivorLink, pszSoundPrefixesData[i].szSurvivorLink))
				continue;
		}

		if (pszSoundPrefixesData[i].iType == iType)
			index++;
	}

	return index;
}

void CGameDefinitionsShared::AddSoundScriptItems(vgui::ComboList *pList, int iType)
{
	if (!pList)
		return;

	for (int i = 0; i < pszSoundPrefixesData.Count(); i++)
	{
		if (pszSoundPrefixesData[i].iType == iType)
			pList->GetComboBox()->AddItem(pszSoundPrefixesData[i].szFriendlyName, NULL);
	}
}

void CGameDefinitionsShared::AddSoundScriptItems(vgui::ComboList *pList, int iType, const char *survivorLink)
{
	if (!pList)
		return;

	for (int i = 0; i < pszSoundPrefixesData.Count(); i++)
	{
		if ((pszSoundPrefixesData[i].iType == iType) && !strcmp(survivorLink, pszSoundPrefixesData[i].szSurvivorLink))
			pList->GetComboBox()->AddItem(pszSoundPrefixesData[i].szFriendlyName, NULL);
	}
}

const char *CGameDefinitionsShared::GetSoundPrefixForChoosenItem(int iType, const char *survivorLink, const char *friendlyName)
{
	for (int i = 0; i < pszSoundPrefixesData.Count(); i++)
	{
		if ((pszSoundPrefixesData[i].iType == iType) && !strcmp(survivorLink, pszSoundPrefixesData[i].szSurvivorLink) && !strcmp(friendlyName, pszSoundPrefixesData[i].szFriendlyName))
			return pszSoundPrefixesData[i].szScriptName;
	}

	if (iType == BB2_SoundTypes::TYPE_PLAYER)
		return "Pantsman";

	return "Default";
}

int CGameDefinitionsShared::GetSelectedSoundsetItemID(vgui::ComboList *pList, int iType, const char *survivorLink, const char *script)
{
	for (int list = 0; list < pList->GetComboBox()->GetItemCount(); list++)
	{
		char friendlyName[MAX_MAP_NAME];
		pList->GetComboBox()->GetItemText(list, friendlyName, MAX_MAP_NAME);

		for (int i = 0; i < pszSoundPrefixesData.Count(); i++)
		{
			if ((pszSoundPrefixesData[i].iType == iType) && !strcmp(survivorLink, pszSoundPrefixesData[i].szSurvivorLink) && !strcmp(friendlyName, pszSoundPrefixesData[i].szFriendlyName)
				&& !strcmp(script, pszSoundPrefixesData[i].szScriptName))
				return list;
		}
	}

	return 0;
}

const char *CGameDefinitionsShared::GetPlayerSoundsetPrefix(int iType, const char *survivorLink, const char *script)
{
	const char *characterDefaultSet = "";
	for (int i = 0; i < pszSoundPrefixesData.Count(); ++i)
	{
		if ((pszSoundPrefixesData[i].iType != iType) || (strcmp(survivorLink, pszSoundPrefixesData[i].szSurvivorLink)))
			continue;

		if (!(characterDefaultSet && characterDefaultSet[0]))
			characterDefaultSet = pszSoundPrefixesData[i].szScriptName;

		if (!strcmp(script, pszSoundPrefixesData[i].szScriptName))
			return pszSoundPrefixesData[i].szScriptName;
	}

	if (characterDefaultSet && characterDefaultSet[0])
		return characterDefaultSet;

	if (iType == BB2_SoundTypes::TYPE_PLAYER)
		return "Pantsman";

	return "Default";
}

const DataLoadingTipsItem_t *CGameDefinitionsShared::GetRandomLoadingTip(void)
{
	int numTips = pszLoadingTipData.Count();
	if (numTips <= 0)
		return NULL;

	return (&pszLoadingTipData[random->RandomInt(0, (numTips - 1))]);
}
#endif

// Particle Data
const char *CGameDefinitionsShared::GetBloodParticle(bool bExtremeGore)
{
	if (!pszBloodParticleData.Count())
		return "blood_impact_red_01";

	int index = (random->RandomInt(0, (pszBloodParticleData.Count() - 1)));
	return (bExtremeGore ? pszBloodParticleData[index].szExtreme : pszBloodParticleData[index].szDefault);
}

const char *CGameDefinitionsShared::GetHeadshotParticle(bool bExtremeGore)
{
	if (!pszHeadshotParticleData.Count())
		return "blood_impact_red_01";

	int index = (random->RandomInt(0, (pszHeadshotParticleData.Count() - 1)));
	return (bExtremeGore ? pszHeadshotParticleData[index].szExtreme : pszHeadshotParticleData[index].szDefault);
}

const char *CGameDefinitionsShared::GetBleedoutParticle(bool bExtremeGore)
{
	if (!pszBleedoutParticleData.Count())
		return "blood_impact_red_01";

	int index = (random->RandomInt(0, (pszBleedoutParticleData.Count() - 1)));
	return (bExtremeGore ? pszBleedoutParticleData[index].szExtreme : pszBleedoutParticleData[index].szDefault);
}

const char *CGameDefinitionsShared::GetBloodExplosionMist(bool bExtremeGore)
{
	if (!pszBloodExplosionParticleData.Count())
		return "blood_impact_red_01";

	int index = (random->RandomInt(0, (pszBloodExplosionParticleData.Count() - 1)));
	return (bExtremeGore ? pszBloodExplosionParticleData[index].szExtreme : pszBloodExplosionParticleData[index].szDefault);
}

const char *CGameDefinitionsShared::GetGibParticleForLimb(const char *limb, bool bExtremeGore)
{
	if (!pszGibParticleData.Count())
		return "blood_impact_red_01";

	for (int i = 0; i < pszGibParticleData.Count(); i++)
	{
		if (!strcmp(limb, pszGibParticleData[i].szLimb))
			return ((bExtremeGore) ? pszGibParticleData[i].szExtreme : pszGibParticleData[i].szDefault);
	}

	return "blood_impact_red_01";
}

const DataExplosiveItem_t *CGameDefinitionsShared::GetExplosiveDataForType(int type)
{
	for (int i = 0; i < pszExplosionData.Count(); i++)
	{
		if (pszExplosionData[i].iType == type)
			return &pszExplosionData[i];
	}

	return NULL;
}

const char *CGameDefinitionsShared::GetExplosionParticle(int type)
{
	const DataExplosiveItem_t *data = GetExplosiveDataForType(type);
	if (data == NULL)
		return "";

	return data->szParticle;
}

// Other Misc Extern Globals:

const char *GetVoiceCommandString(int command)
{
	switch (command)
	{
	case VOICE_COMMAND_AGREE:
		return "Agree";
	case VOICE_COMMAND_DISAGREE:
		return "Disagree";
	case VOICE_COMMAND_FOLLOW:
		return "Follow";
	case VOICE_COMMAND_TAKEPOINT:
		return "TakePoint";
	case VOICE_COMMAND_NOWEP:
		return "NoWeapon";
	case VOICE_COMMAND_OUTOFAMMO:
		return "NoAmmo";
	case VOICE_COMMAND_READY:
		return "Ready";
	case VOICE_COMMAND_LOOK:
		return "Look";
	default:
		return "";
	}
}

const char *GetVoiceCommandChatMessage(int command)
{
	switch (command)
	{
	case VOICE_COMMAND_AGREE:
		return "#Voice_Agree";
	case VOICE_COMMAND_DISAGREE:
		return "#Voice_Disagree";
	case VOICE_COMMAND_FOLLOW:
		return "#Voice_Follow";
	case VOICE_COMMAND_TAKEPOINT:
		return "#Voice_TakePoint";
	case VOICE_COMMAND_NOWEP:
		return "#Voice_NoWeapon";
	case VOICE_COMMAND_OUTOFAMMO:
		return "#Voice_OutOfAmmo";
	case VOICE_COMMAND_READY:
		return "#Voice_Ready";
	case VOICE_COMMAND_LOOK:
		return "#Voice_Look";
	default:
		return "";
	}
}

const char *GetTeamPerkName(int perk)
{
	switch (perk)
	{
	case TEAM_HUMAN_PERK_UNLIMITED_AMMO:
		return "#TEAM_PERK_UNLIMITED_AMMO";
	case TEAM_DECEASED_PERK_INCREASED_STRENGTH:
		return "#TEAM_PERK_INCREASED_DAMAGE";
	default:
		return "";
	}
}

const char *GetGamemodeName(int gamemode)
{
#ifdef CLIENT_DLL
	if (GetClientWorldEntity() && GetClientWorldEntity()->m_bIsStoryMap)
		return "Story";
#else
	if (GameBaseServer()->IsStoryMode())
		return "Story";
#endif

	switch (gamemode)
	{
	case MODE_OBJECTIVE:
		return "Objective";
	case MODE_ARENA:
		return "Arena";
	case MODE_ELIMINATION:
		return "Elimination";
	case MODE_DEATHMATCH:
		return "Deathmatch";
	}

	return "Story";
}

const char *GetGamemodeNameForPrefix(const char *map)
{
	const char *pszGameDescription = "Story";
	if (Q_stristr(map, "bbe_"))
		pszGameDescription = "Elimination";
	else if (Q_stristr(map, "bba_"))
		pszGameDescription = "Arena";
	else if (Q_stristr(map, "bbd_"))
		pszGameDescription = "Deathmatch";
	else if (Q_stristr(map, "bbc_"))
		pszGameDescription = "Objective";

	return pszGameDescription;
}

int GetGamemodeForMap(const char *map)
{
	int mode = MODE_OBJECTIVE;
	if (Q_stristr(map, "bbe_"))
		mode = MODE_ELIMINATION;
	else if (Q_stristr(map, "bba_"))
		mode = MODE_ARENA;
	else if (Q_stristr(map, "bbd_"))
		mode = MODE_DEATHMATCH;

	return mode;
}

namespace ACHIEVEMENTS
{
	static const achievementStatItem_t GAME_STAT_AND_ACHIEVEMENT_DATA[] =
	{
		{ "ACH_TUTORIAL_COMPLETE", "", 0, ACHIEVEMENT_TYPE_MAP, 0, false, NULL },

		// Level
		{ "ACH_LEVEL_5", "BBX_ST_LEVEL", 5, ACHIEVEMENT_TYPE_DEFAULT, 25, false, NULL },
		{ "ACH_LEVEL_50", "BBX_ST_LEVEL", 50, ACHIEVEMENT_TYPE_DEFAULT, 500, false, NULL },
		{ "ACH_LEVEL_100", "BBX_ST_LEVEL", 100, ACHIEVEMENT_TYPE_DEFAULT, 1000, false, NULL },
		{ "ACH_LEVEL_150", "BBX_ST_LEVEL", 150, ACHIEVEMENT_TYPE_DEFAULT, 2500, false, NULL },
		{ "ACH_LEVEL_350", "BBX_ST_LEVEL", 350, ACHIEVEMENT_TYPE_DEFAULT, 5000, false, NULL },
		{ "ACH_LEVEL_500", "BBX_ST_LEVEL", 500, ACHIEVEMENT_TYPE_DEFAULT, 0, false, NULL },

		// Objectives / Sweetness
		{ "ACH_SURVIVOR_CAPTURE_BRIEFCASE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 50, false, NULL },
		{ "ACH_SURVIVOR_KILL_FRED", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 100, false, NULL },
		{ "ACH_ZOMBIE_KILL_HUMAN", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 50, false, NULL },
		{ "ACH_ZOMBIE_FIRST_BLOOD", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 25, false, NULL },
		{ "ACH_SKILL_PERK_ROCKET", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 25, false, NULL },

		// Zombie Demolishing
		{ "ACH_SURVIVOR_KILL_25_ZOMBIES", "BBX_KI_ZOMBIES", 25, ACHIEVEMENT_TYPE_DEFAULT, 50, false, NULL },
		{ "ACH_SURVIVOR_KILL_100_ZOMBIES", "BBX_KI_ZOMBIES", 100, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		{ "ACH_SURVIVOR_KILL_1000_ZOMBIES", "BBX_KI_ZOMBIES", 1000, ACHIEVEMENT_TYPE_DEFAULT, 1000, false, NULL },
		{ "ACH_SURVIVOR_KILL_20000_ZOMBIES", "BBX_KI_ZOMBIES", 20000, ACHIEVEMENT_TYPE_DEFAULT, 10000, false, NULL },

		// Bandit Carnage
		{ "ACH_BANDIT_KILL_500", "BBX_KI_BANDITS", 500, ACHIEVEMENT_TYPE_DEFAULT, 750, false, NULL },
		{ "ACH_BANDIT_KILL_1000", "BBX_KI_BANDITS", 1000, ACHIEVEMENT_TYPE_DEFAULT, 1250, false, NULL },

		// Luck
		{ "ACH_GM_SURVIVAL_PROPANE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 70, false, NULL },
		{ "ACH_WEAPON_GRENADE_FAIL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 25, false, NULL },

		// Pistols & Revolvers
		{ "ACH_WEP_BERETTA", "BBX_KI_BERETTA", 150, ACHIEVEMENT_TYPE_DEFAULT, 250, false, NULL },
		{ "ACH_WEP_DEAGLE", "BBX_KI_DEAGLE", 200, ACHIEVEMENT_TYPE_DEFAULT, 400, false, NULL },
		//{ "ACH_WEP_1911", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		//{ "ACH_WEP_SAUER", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		//{ "ACH_WEP_HK45", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		{ "ACH_WEP_GLOCK", "BBX_KI_GLOCK17", 200, ACHIEVEMENT_TYPE_DEFAULT, 300, false, NULL },
		//{ "ACH_WEP_USP", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		//{ "ACH_WEP_357", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		{ "ACH_WEP_DUAL", "BBX_KI_AKIMBO", 500, ACHIEVEMENT_TYPE_DEFAULT, 750, false, NULL },

		// SMGs
		{ "ACH_WEP_UZI", "BBX_KI_UZI", 300, ACHIEVEMENT_TYPE_DEFAULT, 285, false, NULL },
		{ "ACH_WEP_HKMP5", "BBX_KI_HKMP5", 300, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		//{ "ACH_WEP_HKSMG2", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		{ "ACH_WEP_HKMP7", "BBX_KI_HKMP7", 300, ACHIEVEMENT_TYPE_DEFAULT, 250, false, NULL },
		{ "ACH_WEP_MAC11", "BBX_KI_MAC11", 300, ACHIEVEMENT_TYPE_DEFAULT, 350, false, NULL },

		// Rifles
		{ "ACH_WEP_FAMAS", "BBX_KI_FAMAS", 240, ACHIEVEMENT_TYPE_DEFAULT, 150, false, NULL },
		{ "ACH_WEP_AK74", "BBX_KI_AK74", 300, ACHIEVEMENT_TYPE_DEFAULT, 150, false, NULL },
		{ "ACH_WEP_G36C", "BBX_KI_G36C", 300, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		//{ "ACH_WEP_M4", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },

		// Shotguns
		{ "ACH_WEP_870", "BBX_KI_870", 210, ACHIEVEMENT_TYPE_DEFAULT, 150, false, NULL },
		{ "ACH_WEP_BENELLIM4", "BBX_KI_BENELLIM4", 275, ACHIEVEMENT_TYPE_DEFAULT, 185, false, NULL },
		{ "ACH_WEP_DOUBLEBARREL", "BBX_KI_SAWOFF", 200, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		{ "ACH_WEP_WINCHESTER", "BBX_KI_TRAPPER", 240, ACHIEVEMENT_TYPE_DEFAULT, 400, false, NULL },

		// Special & Snipers
		{ "ACH_WEP_MINIGUN", "BBX_KI_MINIGUN", 12000, ACHIEVEMENT_TYPE_DEFAULT, 3000, false, NULL },
		{ "ACH_WEP_FLAMETHROWER", "BBX_KI_FLAMETHROWER", 340, ACHIEVEMENT_TYPE_DEFAULT, 750, false, NULL },
		//{ "ACH_WEP_50CAL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		{ "ACH_WEP_700", "BBX_KI_REM700", 150, ACHIEVEMENT_TYPE_DEFAULT, 500, false, NULL },
		//{ "ACH_WEP_CROSSBOW", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		//{ "ACH_WEP_STONER", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },

		// Explosives
		{ "ACH_WEP_EXPLOSIVE", "BBX_KI_EXPLOSIVES", 200, ACHIEVEMENT_TYPE_DEFAULT, 450, false, NULL },

		// Melee
		{ "ACH_WEAPON_AXE", "BBX_KI_FIREAXE", 50, ACHIEVEMENT_TYPE_DEFAULT, 300, false, NULL },
		{ "ACH_WEAPON_KNIFE", "BBX_KI_M9PHROBIS", 35, ACHIEVEMENT_TYPE_DEFAULT, 150, false, NULL },
		{ "ACH_WEP_BASEBALLBAT", "BBX_KI_BASEBALLBAT", 80, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		{ "ACH_WEP_HATCHET", "BBX_KI_HATCHET", 120, ACHIEVEMENT_TYPE_DEFAULT, 240, false, NULL },
		{ "ACH_WEP_SLEDGE", "BBX_KI_SLEDGEHAMMER", 280, ACHIEVEMENT_TYPE_DEFAULT, 500, false, NULL },
		{ "ACH_WEP_MACHETE", "BBX_KI_MACHETE", 280, ACHIEVEMENT_TYPE_DEFAULT, 300, false, NULL },
		{ "ACH_WEP_BRICKED", "BBX_KI_BRICK", 99, ACHIEVEMENT_TYPE_DEFAULT, 250, false, NULL },
		{ "ACH_WEP_TRICKSTER", "BBX_KI_KICK", 35, ACHIEVEMENT_TYPE_DEFAULT, 300, false, NULL },
		{ "ACH_WEP_FISTS", "BBX_KI_FISTS", 100, ACHIEVEMENT_TYPE_DEFAULT, 400, false, NULL },

		// Special End Game:
		{ "ACH_GM_ARENA_WIN", "", 0, ACHIEVEMENT_TYPE_MAP, 250, false, NULL },
		{ "ACH_ENDGAME_NOFIREARMS", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 1000, false, NULL },
		{ "ACH_OBJ_NOFIREARMS", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 1000, false, NULL },

		// Objectives & Quests Progression, + story.
		{ "ACH_QUEST_PROG_5", "BBX_QUESTS", 5, ACHIEVEMENT_TYPE_DEFAULT, 500, false, NULL },
		{ "ACH_QUEST_PROG_25", "BBX_QUESTS", 25, ACHIEVEMENT_TYPE_DEFAULT, 1000, false, NULL },

		// Misc, Custom, Community Recommendations...
		{ "ACH_RAZ_HEALTH_ADDICT", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		{ "ACH_RAZ_SWEEPER", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 200, false, NULL },
		{ "ACH_RAZ_MAZELTOV", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, NULL },
		{ "ACH_RAZ_KUNG_FLU", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, NULL },
		{ "ACH_RAZ_TRAUMA", "BBX_RZ_HEADSHOT", 250, ACHIEVEMENT_TYPE_DEFAULT, 1000, false, NULL },
		{ "ACH_RAZ_PRESCRIBED_PAIN", "BBX_RZ_PAIN", 1000000, ACHIEVEMENT_TYPE_DEFAULT, 2500, false, NULL },

		// Maps:
		{ "ACH_MAP_LASTSTAND", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bbc_laststand" },
		{ "ACH_MAP_TERMOIL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bbc_termoil" },
		{ "ACH_MAP_MECKLENBURG", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 750, false, "bbc_mecklenburg" },
		{ "ACH_MAP_COMPOUND", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 250, false, "bbc_compound" },
		{ "ACH_MAP_NIGHTCLUB", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 750, false, "bbc_nightclub" },
		{ "ACH_MAP_SWAMPTROUBLE_OBJ", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 750, false, "bbc_swamptrouble" },
		//{ "ACH_MAP_COLTEC_C", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 750, true, "bbc_coltec" },
		//{ "ACH_MAP_IKROM", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 1000, true, "bbc_ikrom" },
		{ "ACH_MAP_ROOFTOP", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_rooftop" },
		{ "ACH_MAP_COLOSSEUM", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_colosseum" },
		{ "ACH_MAP_BARRACKS_ARENA", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_barracks" },
		{ "ACH_MAP_CARGO", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_cargo" },
		{ "ACH_MAP_DEVILSCRYPT", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_devilscrypt" },
		{ "ACH_MAP_SWAMPTROUBLE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_swamptrouble" },
		{ "ACH_MAP_SALVAGE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 2000, false, "bba_salvage" },
		{ "ACH_MAP_CARNAGE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_carnage" },
		{ "ACH_MAP_COLTEC_A", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false, "bba_coltec" },
		{ "ACH_MAP_ISLAND_A", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 250, false, "bba_island" },

		// Hidden:
		{ "ACH_SECRET_WATCHYOURSTEP", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true, NULL },
		{ "ACH_SECRET_TFO", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true, NULL },
		//{ "ACH_AREA_MACHETE", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true, NULL },
		//{ "ACH_AREA_TEA", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true, NULL },
		{ "ACH_SECRET_TURTLE", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true, NULL },
		{ "ACH_SECRET_GIVEALL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		{ "ACH_WEP_BRICK", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 150, true, NULL },

		// Stats Only + hidden (no achievs) - used for leaderboards...
		{ "", "BBX_ST_KILLS", 9999999, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
		{ "", "BBX_ST_DEATHS", 9999999, ACHIEVEMENT_TYPE_DEFAULT, 0, true, NULL },
	};

	const achievementStatItem_t *GetAchievementItem(int index)
	{
		if ((index < 0) || (index >= _ARRAYSIZE(GAME_STAT_AND_ACHIEVEMENT_DATA)))
			return NULL;
		return &GAME_STAT_AND_ACHIEVEMENT_DATA[index];
	}

	const achievementStatItem_t *GetAchievementItem(const char *str)
	{
		if (str && str[0])
		{
			for (int i = 0; i < _ARRAYSIZE(GAME_STAT_AND_ACHIEVEMENT_DATA); i++)
			{
				if (!strcmp(GAME_STAT_AND_ACHIEVEMENT_DATA[i].szAchievement, str))
					return &GAME_STAT_AND_ACHIEVEMENT_DATA[i];
			}
		}
		return NULL;
	}

	int GetNumAchievements(void) { return _ARRAYSIZE(GAME_STAT_AND_ACHIEVEMENT_DATA); }
}

#define PENETRATION_DATA_SIZE 12
DataPenetrationItem_t PENETRATION_DATA_LIST[PENETRATION_DATA_SIZE] =
{
	{ CHAR_TEX_WOOD, 9.0f },
	{ CHAR_TEX_GRATE, 6.0f },
	{ CHAR_TEX_CONCRETE, 4.0f },
	{ CHAR_TEX_TILE, 5.0f },
	{ CHAR_TEX_COMPUTER, 5.0f },
	{ CHAR_TEX_GLASS, 8.0f },
	{ CHAR_TEX_VENT, 4.0f },
	{ CHAR_TEX_METAL, 5.0f },
	{ CHAR_TEX_PLASTIC, 8.0f },
	{ CHAR_TEX_BLOODYFLESH, 16.0f },
	{ CHAR_TEX_FLESH, 16.0f },
	{ CHAR_TEX_DIRT, 6.0f },
};

const DataPenetrationItem_t *GetPenetrationDataForMaterial(unsigned short material)
{
	for (int i = 0; i < PENETRATION_DATA_SIZE; i++)
	{
		if (PENETRATION_DATA_LIST[i].material == material)
			return &PENETRATION_DATA_LIST[i];
	}

	return NULL;
}

Vector TryPenetrateSurface(trace_t *tr, ITraceFilter *filter)
{
	if (tr && filter && strcmp(tr->surface.name, "tools/toolsblockbullets"))
	{
		surfacedata_t *p_penetrsurf = physprops->GetSurfaceData(tr->surface.surfaceProps);
		if (p_penetrsurf)
		{
			const DataPenetrationItem_t *penetrationInfo = GetPenetrationDataForMaterial(p_penetrsurf->game.material);
			if (penetrationInfo)
			{
				Vector vecDir = (tr->endpos - tr->startpos);
				VectorNormalize(vecDir);

				Vector vecNewStart = tr->endpos + vecDir * penetrationInfo->depth;
				trace_t trPeneTest;
				UTIL_TraceLine(vecNewStart, vecNewStart + vecDir * MAX_TRACE_LENGTH, MASK_SHOT, filter, &trPeneTest);
				if (!trPeneTest.startsolid)
					return vecNewStart;
			}
		}
	}

	return vec3_invalid;
}

const char* COM_GetModDirectory()
{
	static char modDir[MAX_PATH];
	if (Q_strlen(modDir) == 0)
	{
		const char* gamedir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue("-defaultgamedir", "hl2"));
		Q_strncpy(modDir, gamedir, sizeof(modDir));
		if (strchr(modDir, '/') || strchr(modDir, '\\'))
		{
			Q_StripLastDir(modDir, sizeof(modDir));
			int dirlen = Q_strlen(modDir);
			Q_strncpy(modDir, gamedir + dirlen, sizeof(modDir) - dirlen);
		}
	}

	return modDir;
}
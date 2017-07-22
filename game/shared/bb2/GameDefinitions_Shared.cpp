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

#else
#include "GameBase_Server.h"
#endif

// Gamemode data
const char *playerGamemodeFiles[] =
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

	pszSoundPrefixesData.Purge();
#endif

	pszPlayerData.Purge();
	pszPlayerLimbData.Purge();
	pszPlayerWeaponData.Purge();
	pszPlayerSurvivorData.Purge();
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

	KeyValues *pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/game_base_player_shared");
	if (pkvParseData)
	{
		KeyValues *pkvShared = pkvParseData->FindKey("Shared");
		if (pkvShared)
		{
			pszPlayerSharedData.iMaxLevel = pkvShared->GetInt("MaxLevel", MAX_PLAYER_LEVEL);
			pszPlayerSharedData.iXPIncreasePerLevel = pkvShared->GetInt("XPPerLevel", 65);
			pszPlayerSharedData.iTeamBonusDamageIncrease = pkvShared->GetInt("TeamBonusDamageIncrease", 1);
			pszPlayerSharedData.iTeamBonusXPIncrease = pkvShared->GetInt("TeamBonusXPIncrease", 1);
			pszPlayerSharedData.flPerkTime = pkvShared->GetFloat("PerkTime", 10.0f);
			pszPlayerSharedData.iLevel = pkvShared->GetInt("Level", 1);
			pszPlayerSharedData.iInfectionStartPercent = pkvShared->GetInt("InfectionStart", 15);
			pszPlayerSharedData.flInfectionDuration = pkvShared->GetFloat("InfectionTime", 30.0f);
		}

		KeyValues *pkvSkills = pkvParseData->FindKey("HumanSkills");
		if (pkvSkills)
		{
			pszHumanSkillData.flAgilitySpeed = pkvSkills->GetFloat("Speed", 3.0f);
			pszHumanSkillData.flAgilityJump = pkvSkills->GetFloat("Jump", 3.0f);
			pszHumanSkillData.flAgilityLeap = pkvSkills->GetFloat("Leap", 5.0f);
			pszHumanSkillData.flAgilitySlide = pkvSkills->GetFloat("Slide", 5.0f);
			pszHumanSkillData.flAgilityEnhancedReflexes = pkvSkills->GetFloat("Reflexes", 4.0f);
			pszHumanSkillData.flAgilityMeleeSpeed = pkvSkills->GetFloat("MeleeSpeed", 2.0f);
			pszHumanSkillData.flAgilityLightweight = pkvSkills->GetFloat("Lightweight", 2.0f);
			pszHumanSkillData.flAgilityWeightless = pkvSkills->GetFloat("Weightless", 4.0f);
			pszHumanSkillData.flAgilityHealthRegen = pkvSkills->GetFloat("HealthRegen", 0.35f);
			pszHumanSkillData.flAgilityRealityPhase = pkvSkills->GetFloat("RealityPhase", 3.0f);

			pszHumanSkillData.flStrengthHealth = pkvSkills->GetFloat("Health", 10.0f);
			pszHumanSkillData.flStrengthImpenetrable = pkvSkills->GetFloat("Impenetrable", 2.0f);
			pszHumanSkillData.flStrengthPainkiller = pkvSkills->GetFloat("Painkiller", 5.0f);
			pszHumanSkillData.flStrengthLifeLeech = pkvSkills->GetFloat("LifeLeech", 1.0f);
			pszHumanSkillData.flStrengthPowerKick = pkvSkills->GetFloat("PowerKick", 10.0f);
			pszHumanSkillData.flStrengthBleed = pkvSkills->GetFloat("Bleed", 5.0f);
			pszHumanSkillData.flStrengthCripplingBlow = pkvSkills->GetFloat("CripplingBlow", 2.0f);
			pszHumanSkillData.flStrengthArmorMaster = pkvSkills->GetFloat("ArmorMaster", 2.0f);
			pszHumanSkillData.flStrengthBloodRage = pkvSkills->GetFloat("BloodRage", 2.0f);

			pszHumanSkillData.flFirearmResourceful = pkvSkills->GetFloat("Resourceful", 3.0f);
			pszHumanSkillData.flFirearmBlazingAmmo = pkvSkills->GetFloat("BlazingAmmo", 2.0f);
			pszHumanSkillData.flFirearmColdsnap = pkvSkills->GetFloat("Coldsnap", 2.0f);
			pszHumanSkillData.flFirearmEmpoweredBullets = pkvSkills->GetFloat("EmpoweredBullets", 4.0f);
			pszHumanSkillData.flFirearmMagazineRefill = pkvSkills->GetFloat("MagazineRefill", 1.0f);
			pszHumanSkillData.flFirearmGunslinger = pkvSkills->GetFloat("Gunslinger", 2.0f);
		}

		pkvSkills = pkvParseData->FindKey("ZombieSkills");
		if (pkvSkills)
		{
			pszZombieSkillData.flHealth = pkvSkills->GetFloat("Health", 5.0f);
			pszZombieSkillData.flDamage = pkvSkills->GetFloat("Damage", 2.0f);
			pszZombieSkillData.flDamageReduction = pkvSkills->GetFloat("DamageReduction", 3.0f);
			pszZombieSkillData.flSpeed = pkvSkills->GetFloat("Speed", 4.0f);
			pszZombieSkillData.flJump = pkvSkills->GetFloat("Jump", 3.0f);
			pszZombieSkillData.flLeap = pkvSkills->GetFloat("Leap", 1.0f);
			pszZombieSkillData.flDeath = pkvSkills->GetFloat("Death", 2.0f);
			pszZombieSkillData.flLifeLeech = pkvSkills->GetFloat("LifeLeech", 2.0f);
			pszZombieSkillData.flHealthRegen = pkvSkills->GetFloat("HealthRegen", 4.0f);
			pszZombieSkillData.flMassInvasion = pkvSkills->GetFloat("MassInvasion", 1.0f);
		}

		pkvSkills = pkvParseData->FindKey("ZombieRageMode");
		if (pkvSkills)
		{
			pszZombieRageModeData.flHealth = pkvSkills->GetFloat("Health", 50.0f);
			pszZombieRageModeData.flHealthRegen = pkvSkills->GetFloat("HealthRegen", 5.0f);
			pszZombieRageModeData.flSpeed = pkvSkills->GetFloat("Speed", 25.0f);
			pszZombieRageModeData.flJump = pkvSkills->GetFloat("Jump", 5.0f);
			pszZombieRageModeData.flLeap = pkvSkills->GetFloat("Leap", 5.0f);
		}

		pkvSkills = pkvParseData->FindKey("MiscSkillInfo");
		if (pkvSkills)
		{
			pszPlayerMiscSkillData.flBleedDuration = pkvSkills->GetFloat("BleedDuration");
			pszPlayerMiscSkillData.flBleedFrequency = pkvSkills->GetFloat("BleedFrequency");
			pszPlayerMiscSkillData.flBurnDuration = pkvSkills->GetFloat("BurnDuration");
			pszPlayerMiscSkillData.flBurnDamage = pkvSkills->GetFloat("BurnDamage");
			pszPlayerMiscSkillData.flBurnFrequency = pkvSkills->GetFloat("BurnFrequency");
			pszPlayerMiscSkillData.flStunDuration = pkvSkills->GetFloat("StunDuration");
			pszPlayerMiscSkillData.flSlowDownDuration = pkvSkills->GetFloat("SlowDownDuration");
			pszPlayerMiscSkillData.flSlowDownPercent = pkvSkills->GetFloat("SlowDownPercent");

			pszPlayerMiscSkillData.flKickDamage = pkvSkills->GetFloat("KickDamage");
			pszPlayerMiscSkillData.flKickRange = pkvSkills->GetFloat("KickRange");
			pszPlayerMiscSkillData.flKickKnockbackForce = pkvSkills->GetFloat("KickForce");
			pszPlayerMiscSkillData.flKickCooldown = pkvSkills->GetFloat("KickCooldown");

			pszPlayerMiscSkillData.flSlideLength = pkvSkills->GetFloat("SlideLength");
			pszPlayerMiscSkillData.flSlideSpeed = pkvSkills->GetFloat("SlideSpeed");
			pszPlayerMiscSkillData.flSlideCooldown = pkvSkills->GetFloat("SlideCooldown");
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

			pszPlayerPowerupData.AddToTail(item);
		}

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/game_base_player_powerups!\n");

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/gamemode_shared");
	if (pkvParseData)
	{
		pszGamemodeData.iXPRoundWinArena = pkvParseData->GetInt("round_win_arena", 100);
		pszGamemodeData.iXPRoundWinElimination = pkvParseData->GetInt("round_win_elimination", 50);

		pszGamemodeData.iXPGameWinObjective = pkvParseData->GetInt("game_win_objective", 150);
		pszGamemodeData.iXPGameWinArena = pkvParseData->GetInt("game_win_arena", 1000);
		pszGamemodeData.iXPGameWinElimination = pkvParseData->GetInt("game_win_elimination", 500);
		pszGamemodeData.iXPGameWinDeathmatch = pkvParseData->GetInt("game_win_deathmatch", 500);

		pszGamemodeData.iKillsRequiredToPerk = pkvParseData->GetInt("perk_kills_required", 50);
		pszGamemodeData.iZombieCreditsRequiredToRage = pkvParseData->GetInt("rage_credits_required", 10);
		pszGamemodeData.iZombieKillsRequiredToRage = pkvParseData->GetInt("rage_kills_required", 3);

		pszGamemodeData.iDefaultZombieCredits = pkvParseData->GetInt("zombie_credits_start", 10);
		pszGamemodeData.flAmmoResupplyTime = pkvParseData->GetFloat("ammo_resupply_time", 30.0f);

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/gamemode_shared!\n");

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/items_misc_base");
	if (pkvParseData)
	{
		for (KeyValues *sub = pkvParseData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			DataInventoryItem_Misc_t item;
			item.iItemID = (uint)atol(sub->GetName());
			item.iValue = sub->GetInt();
			pszItemMiscData.AddToTail(item);
		}

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/items_misc_base!\n");

	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/game/items_armor_base");
	if (pkvParseData)
	{
		for (KeyValues *sub = pkvParseData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			DataInventoryItem_Armor_t item;
			item.iItemID = (uint)atol(sub->GetName());
			item.iWeight = sub->GetInt("weight");
			item.iReductionPercent = sub->GetInt("reduction");
			pszItemArmorData.AddToTail(item);
		}

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/game/items_armor_base!\n");

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

	FileFindHandle_t findHandle;
	const char *pFilename = NULL;

#ifdef CLIENT_DLL
	pkvParseData = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/soundsets/default");
	if (pkvParseData)
	{
		for (KeyValues *sub = pkvParseData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
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

		pkvParseData->deleteThis();
	}
	else
		Warning("Failed to parse: data/soundsets/default!\n");

	pFilename = filesystem->FindFirstEx("data/soundsets/*.txt", "MOD", &findHandle);
	while (pFilename)
	{
		if ((strlen(pFilename) > 4) && strcmp(pFilename, "default.txt") && !filesystem->IsDirectory(pFilename, "MOD"))
		{
			char filePath[MAX_WEAPON_STRING];
			Q_snprintf(filePath, MAX_WEAPON_STRING, "data/soundsets/%s", pFilename);

			KeyValues *soundPrefixData = new KeyValues("CustomSoundSet");
			if (soundPrefixData->LoadFromFile(filesystem, filePath, "MOD"))
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
				Warning("Failed to parse custom: %s!\n", filePath);

			soundPrefixData->deleteThis();
		}

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
#endif

	pFilename = filesystem->FindFirstEx("data/characters/*.txt", "MOD", &findHandle);
	while (pFilename)
	{
		if ((strlen(pFilename) > 4) && !filesystem->IsDirectory(pFilename, "MOD"))
		{
			char filePath[MAX_WEAPON_STRING];
			Q_snprintf(filePath, MAX_WEAPON_STRING, "data/characters/%s", pFilename);

			KeyValues *survivorData = new KeyValues("CharacterInfo");
			if (survivorData->LoadFromFile(filesystem, filePath, "MOD"))
			{
				for (KeyValues *sub = survivorData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					DataPlayerItem_Survivor_Shared_t item;
					Q_strncpy(item.szSurvivorName, sub->GetName(), MAX_MAP_NAME);

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

#ifdef CLIENT_DLL
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
#endif

					pszPlayerSurvivorData.AddToTail(item);
				}
			}
			else
				Warning("Failed to parse custom: %s!\n", filePath);

			survivorData->deleteThis();
		}

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);

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
		CBaseAnimating::PrecacheScriptSound(pszItemSharedData[i].szSoundScriptSuccess);
		CBaseAnimating::PrecacheScriptSound(pszItemSharedData[i].szSoundScriptFailure);
		CBaseAnimating::PrecacheModel(pszItemSharedData[i].szModelPath);
	}

	for (int i = 0; i < pszPlayerSurvivorData.Count(); i++)
	{
		CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanHandsPath);
		CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanModelPath);
		CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanBodyPath);

		CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szZombieHandsPath);
		CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szZombieModelPath);
		CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szZombieBodyPath);

		// Check if there's any gibs:

		// Humans
		if (strlen(pszPlayerSurvivorData[i].szHumanGibHead) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanGibHead);

		if (strlen(pszPlayerSurvivorData[i].szHumanGibArmLeft) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanGibArmLeft);

		if (strlen(pszPlayerSurvivorData[i].szHumanGibArmRight) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanGibArmRight);

		if (strlen(pszPlayerSurvivorData[i].szHumanGibLegLeft) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanGibLegLeft);

		if (strlen(pszPlayerSurvivorData[i].szHumanGibLegRight) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szHumanGibLegRight);

		// Zombies:
		if (strlen(pszPlayerSurvivorData[i].szDeceasedGibHead) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szDeceasedGibHead);

		if (strlen(pszPlayerSurvivorData[i].szDeceasedGibArmLeft) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szDeceasedGibArmLeft);

		if (strlen(pszPlayerSurvivorData[i].szDeceasedGibArmRight) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szDeceasedGibArmRight);

		if (strlen(pszPlayerSurvivorData[i].szDeceasedGibLegLeft) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szDeceasedGibLegLeft);

		if (strlen(pszPlayerSurvivorData[i].szDeceasedGibLegRight) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerSurvivorData[i].szDeceasedGibLegRight);
	}

	for (int i = 0; i < pszPlayerPowerupData.Count(); i++)
	{
		if (strlen(pszPlayerPowerupData[i].pchModelPath) > 0)
			CBaseAnimating::PrecacheModel(pszPlayerPowerupData[i].pchModelPath);

		if (strlen(pszPlayerPowerupData[i].pchActivationSoundScript) > 0)
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

	return true;
}

// Player Data:
DataPlayerItem_Shared_t CGameDefinitionsShared::GetPlayerSharedData(void) const
{
	return pszPlayerSharedData;
}

DataPlayerItem_Player_Shared_t CGameDefinitionsShared::GetPlayerGameModeData(int iTeam) const
{
	Assert(pszPlayerData.Count() > 0);

	for (int i = 0; i < pszPlayerData.Count(); i++)
	{
		if (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == pszPlayerData[i].iGameMode) && (iTeam == pszPlayerData[i].iTeam))
			return pszPlayerData[i];
	}

	return pszPlayerData[0];
}

DataPlayerItem_MiscSkillInfo_t CGameDefinitionsShared::GetPlayerMiscSkillData(void) const
{
	return pszPlayerMiscSkillData;
}

DataPlayerItem_Humans_Skills_t CGameDefinitionsShared::GetPlayerHumanSkillData(void) const
{
	return pszHumanSkillData;
}

DataPlayerItem_Zombies_Skills_t CGameDefinitionsShared::GetPlayerZombieSkillData(void) const
{
	return pszZombieSkillData;
}

DataPlayerItem_ZombieRageMode_t CGameDefinitionsShared::GetPlayerZombieRageData(void) const
{
	return pszZombieRageModeData;
}

DataPlayerItem_Survivor_Shared_t CGameDefinitionsShared::GetSurvivorDataForIndex(int index) const
{
	if (index >= 0 && index < pszPlayerSurvivorData.Count())
		return pszPlayerSurvivorData[index];

	return pszPlayerSurvivorData[0];
}

DataPlayerItem_Player_PowerupItem_t CGameDefinitionsShared::GetPlayerPowerupData(const char *powerupName) const
{
	int index = GetIndexForPowerup(powerupName);
	if (index != -1)
		return pszPlayerPowerupData[index];

	return pszPlayerPowerupData[0];
}

DataPlayerItem_Player_PowerupItem_t CGameDefinitionsShared::GetPlayerPowerupData(int powerupFlag) const
{
	for (int i = 0; i < pszPlayerPowerupData.Count(); ++i)
	{
		if (pszPlayerPowerupData[i].iFlag == powerupFlag)
			return pszPlayerPowerupData[i];
	}

	return pszPlayerPowerupData[0];
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

	return 0;
}

int CGameDefinitionsShared::GetIndexForSurvivor(const char *name)
{
	for (int i = 0; i < pszPlayerSurvivorData.Count(); ++i)
	{
		if (!strcmp(pszPlayerSurvivorData[i].szSurvivorName, name))
			return i;
	}

	return -1;
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

const char *CGameDefinitionsShared::GetPlayerSurvivorModel(const char *name, int iTeam)
{
	int index = -1;
	for (int i = 0; i < pszPlayerSurvivorData.Count(); ++i)
	{
		if (!strcmp(pszPlayerSurvivorData[i].szSurvivorName, name))
		{
			index = i;
			break;
		}
	}

	if (index != -1)
	{
		if (iTeam == TEAM_HUMANS)
			return pszPlayerSurvivorData[index].szHumanModelPath;
		else if (iTeam == TEAM_DECEASED)
			return pszPlayerSurvivorData[index].szZombieModelPath;
	}

	return "";
}

const char *CGameDefinitionsShared::GetPlayerHandModel(const char *model)
{
	for (int i = 0; i < pszPlayerSurvivorData.Count(); ++i)
	{
		if (!strcmp(pszPlayerSurvivorData[i].szHumanModelPath, model))
			return pszPlayerSurvivorData[i].szHumanHandsPath;
		else if (!strcmp(pszPlayerSurvivorData[i].szZombieModelPath, model))
			return pszPlayerSurvivorData[i].szZombieHandsPath;
	}

	return "";
}

const char *CGameDefinitionsShared::GetPlayerBodyModel(const char *model)
{
	for (int i = 0; i < pszPlayerSurvivorData.Count(); ++i)
	{
		if (!strcmp(pszPlayerSurvivorData[i].szHumanModelPath, model))
			return pszPlayerSurvivorData[i].szHumanBodyPath;
		else if (!strcmp(pszPlayerSurvivorData[i].szZombieModelPath, model))
			return pszPlayerSurvivorData[i].szZombieBodyPath;
	}

	return "";
}

const char *CGameDefinitionsShared::GetPlayerGibForModel(const char *gib, const char *model)
{
	for (int i = 0; i < pszPlayerSurvivorData.Count(); ++i)
	{
		if (!strcmp(pszPlayerSurvivorData[i].szHumanModelPath, model))
		{
			if (!strcmp(gib, "head"))
				return pszPlayerSurvivorData[i].szHumanGibHead;
			else if (!strcmp(gib, "arms_left"))
				return pszPlayerSurvivorData[i].szHumanGibArmLeft;
			else if (!strcmp(gib, "arms_right"))
				return pszPlayerSurvivorData[i].szHumanGibArmRight;
			else if (!strcmp(gib, "legs_left"))
				return pszPlayerSurvivorData[i].szHumanGibLegLeft;
			else if (!strcmp(gib, "legs_right"))
				return pszPlayerSurvivorData[i].szHumanGibLegRight;
		}
		else if (!strcmp(pszPlayerSurvivorData[i].szZombieModelPath, model))
		{
			if (!strcmp(gib, "head"))
				return pszPlayerSurvivorData[i].szDeceasedGibHead;
			else if (!strcmp(gib, "arms_left"))
				return pszPlayerSurvivorData[i].szDeceasedGibArmLeft;
			else if (!strcmp(gib, "arms_right"))
				return pszPlayerSurvivorData[i].szDeceasedGibArmRight;
			else if (!strcmp(gib, "legs_left"))
				return pszPlayerSurvivorData[i].szDeceasedGibLegLeft;
			else if (!strcmp(gib, "legs_right"))
				return pszPlayerSurvivorData[i].szDeceasedGibLegRight;
		}
	}

	return "";
}

bool CGameDefinitionsShared::DoesPlayerHaveGibForLimb(const char *model, int gibID)
{
	for (int i = 0; i < pszPlayerSurvivorData.Count(); ++i)
	{
		if (!strcmp(pszPlayerSurvivorData[i].szHumanModelPath, model))
		{
			if ((strlen(pszPlayerSurvivorData[i].szHumanGibHead) > 0) && (gibID == GIB_NO_HEAD))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szHumanGibArmLeft) > 0) && (gibID == GIB_NO_ARM_LEFT))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szHumanGibArmRight) > 0) && (gibID == GIB_NO_ARM_RIGHT))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szHumanGibLegLeft) > 0) && (gibID == GIB_NO_LEG_LEFT))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szHumanGibLegRight) > 0) && (gibID == GIB_NO_LEG_RIGHT))
				return true;
		}
		else if (!strcmp(pszPlayerSurvivorData[i].szZombieModelPath, model))
		{
			if ((strlen(pszPlayerSurvivorData[i].szDeceasedGibHead) > 0) && (gibID == GIB_NO_HEAD))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szDeceasedGibArmLeft) > 0) && (gibID == GIB_NO_ARM_LEFT))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szDeceasedGibArmRight) > 0) && (gibID == GIB_NO_ARM_RIGHT))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szDeceasedGibLegLeft) > 0) && (gibID == GIB_NO_LEG_LEFT))
				return true;

			if ((strlen(pszPlayerSurvivorData[i].szDeceasedGibLegRight) > 0) && (gibID == GIB_NO_LEG_RIGHT))
				return true;
		}
	}

	return false;
}

float CGameDefinitionsShared::GetPlayerFirearmDamageScale(const char *weapon, int entityType, int team)
{
	float flScale = 1.0f;

	if (!HL2MPRules())
		return flScale;

	int currentGamemode = HL2MPRules()->GetCurrentGamemode();

	for (int i = 0; i < pszPlayerWeaponData.Count(); i++)
	{
		if (!strcmp(pszPlayerWeaponData[i].szWeaponClass, weapon) && (entityType >= 0) && (entityType < NUM_DAMAGE_SCALES) && (pszPlayerWeaponData[i].iTeam == team) && (pszPlayerWeaponData[i].iGameMode == currentGamemode))
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
		if (!strcmp(pszPlayerLimbData[i].szLimb, limb) && (pszPlayerLimbData[i].iTeam == team) && (pszPlayerLimbData[i].iGameMode == currentGamemode))
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
			item.iLevelReq = sub->GetInt("LevelReq");
			item.iType = sub->GetInt("Type");
			item.iSubType = sub->GetInt("SubType");
			item.iWeight = sub->GetInt("Weight");
			item.iRarity = sub->GetInt("Rarity");
			item.iSkin = 0;
			item.bIsMapItem = bIsMapItem;

			Q_strncpy(item.szSoundScriptSuccess, sub->GetString("PickupSound", "ItemShared.Pickup"), 32);
			Q_strncpy(item.szSoundScriptFailure, sub->GetString("DenySound", "ItemShared.Deny"), 32);

			KeyValues *pkvModel = sub->FindKey("model");
			if (pkvModel)
			{
				Q_strncpy(item.szModelPath, pkvModel->GetString("modelname"), MAX_WEAPON_STRING);
				item.iSkin = pkvModel->GetInt("skin");
			}

			const char *pszObjIconTexture = sub->GetString("ObjectiveIconTexture");

			item.bGlobalGlow = false;
			item.bEnableObjectiveIcon = (strlen(pszObjIconTexture) > 0);
			item.bAutoConsume = sub->GetBool("AutoConsume");
			Q_strncpy(item.szObjectiveIconTexture, pszObjIconTexture, MAX_WEAPON_STRING);
			item.clGlowColor = Color(255, 255, 255, 255);

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

#ifdef CLIENT_DLL
			const char *hudIconPath = sub->GetString("HUDIconTexture");
			int hudTextureID = -1;
			if (strlen(hudIconPath) > 0)
			{
				hudTextureID = vgui::surface()->CreateNewTextureID();
				vgui::surface()->DrawSetTextureFile(hudTextureID, hudIconPath, true, false);
			}
			item.iHUDTextureID = hudTextureID;
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

int CGameDefinitionsShared::GetInventorySharedDataValue(const char *name, uint itemID, bool bIsMapItem)
{
	int index = GetInventoryItemIndex(itemID, bIsMapItem);
	if (index != -1)
	{
		if (!strcmp(name, "Type"))
			return pszItemSharedData[index].iType;
		else if (!strcmp(name, "SubType"))
			return pszItemSharedData[index].iSubType;
		else if (!strcmp(name, "Rarity"))
			return pszItemSharedData[index].iRarity;
		else if (!strcmp(name, "LevelReq"))
			return pszItemSharedData[index].iLevelReq;
		else if (!strcmp(name, "Weight"))
			return pszItemSharedData[index].iWeight;
	}

	return 0;
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
	int index = GetInventoryItemIndex(itemID, bIsMapItem);
	return (index != -1);
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

// Sound Data
#ifdef CLIENT_DLL
const char *CGameDefinitionsShared::GetSoundPrefix(int iType, int index, const char *survivor)
{
	const char *szDefault = "";
	for (int i = 0; i < pszSoundPrefixesData.Count(); ++i)
	{
		if (pszSoundPrefixesData[i].iType != iType)
			continue;

		// Save the default soundset script in case we can't find the desired one. Fallback to this one if necessary.
		if (strlen(szDefault) <= 0)
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
		if (survivorLink && (strlen(survivorLink) > 0))
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
		char friendlyName[64];
		pList->GetComboBox()->GetItemText(list, friendlyName, 64);

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

		if (strlen(characterDefaultSet) <= 0)
			characterDefaultSet = pszSoundPrefixesData[i].szScriptName;

		if (!strcmp(script, pszSoundPrefixesData[i].szScriptName))
			return pszSoundPrefixesData[i].szScriptName;
	}

	if (strlen(characterDefaultSet) > 0)
		return characterDefaultSet;

	if (iType == BB2_SoundTypes::TYPE_PLAYER)
		return "Pantsman";

	return "Default";
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

int CGameDefinitionsShared::GetExplosiveDataIndex(int type)
{
	for (int i = 0; i < pszExplosionData.Count(); i++)
	{
		if (pszExplosionData[i].iType == type)
			return i;
	}

	return -1;
}

const char *CGameDefinitionsShared::GetExplosionParticle(int type)
{
	int index = GetExplosiveDataIndex(type);
	if (index == -1)
		return "";

	return pszExplosionData[index].szParticle;
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
	case TEAM_DECEASED_PERK_INCREASED_DAMAGE:
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

achievementStatItem_t GAME_STAT_AND_ACHIEVEMENT_DATA[CURRENT_ACHIEVEMENT_NUMBER] =
{
	{ "ACH_TUTORIAL_COMPLETE", "", 0, ACHIEVEMENT_TYPE_MAP, 0, false },

	// Level
	{ "ACH_LEVEL_5", "BBX_ST_LEVEL", 5, ACHIEVEMENT_TYPE_DEFAULT, 25, false },
	{ "ACH_LEVEL_50", "BBX_ST_LEVEL", 50, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_LEVEL_100", "BBX_ST_LEVEL", 100, ACHIEVEMENT_TYPE_DEFAULT, 1000, false },
	{ "ACH_LEVEL_150", "BBX_ST_LEVEL", 150, ACHIEVEMENT_TYPE_DEFAULT, 2500, false },
	{ "ACH_LEVEL_350", "BBX_ST_LEVEL", 350, ACHIEVEMENT_TYPE_DEFAULT, 5000, false },
	{ "ACH_LEVEL_500", "BBX_ST_LEVEL", 500, ACHIEVEMENT_TYPE_DEFAULT, 0, false },

	// Objectives / Sweetness
	{ "ACH_SURVIVOR_CAPTURE_BRIEFCASE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 50, false },
	{ "ACH_SURVIVOR_KILL_FRED", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 100, false },
	{ "ACH_ZOMBIE_KILL_HUMAN", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 50, false },
	{ "ACH_ZOMBIE_FIRST_BLOOD", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 25, false },
	{ "ACH_SKILL_PERK_ROCKET", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 25, false },

	// Zombie Demolishing
	{ "ACH_SURVIVOR_KILL_25_ZOMBIES", "BBX_KI_ZOMBIES", 25, ACHIEVEMENT_TYPE_DEFAULT, 50, false },
	{ "ACH_SURVIVOR_KILL_100_ZOMBIES", "BBX_KI_ZOMBIES", 100, ACHIEVEMENT_TYPE_DEFAULT, 200, false },
	{ "ACH_SURVIVOR_KILL_1000_ZOMBIES", "BBX_KI_ZOMBIES", 1000, ACHIEVEMENT_TYPE_DEFAULT, 1000, false },
	{ "ACH_SURVIVOR_KILL_20000_ZOMBIES", "BBX_KI_ZOMBIES", 20000, ACHIEVEMENT_TYPE_DEFAULT, 10000, false },

	// Luck
	{ "ACH_GM_SURVIVAL_PROPANE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 70, false },
	{ "ACH_WEAPON_GRENADE_FAIL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 25, false },

	// Pistols & Revolvers
	{ "ACH_WEP_BERETTA", "BBX_KI_BERETTA", 150, ACHIEVEMENT_TYPE_DEFAULT, 250, false },
	{ "ACH_WEP_DEAGLE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_1911", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_SAUER", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_HK45", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_GLOCK", "BBX_KI_GLOCK17", 200, ACHIEVEMENT_TYPE_DEFAULT, 300, false },
	{ "ACH_WEP_USP", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_357", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_DUAL", "BBX_KI_AKIMBO", 500, ACHIEVEMENT_TYPE_DEFAULT, 750, false },

	// SMGs
	{ "ACH_WEP_UZI", "BBX_KI_UZI", 300, ACHIEVEMENT_TYPE_DEFAULT, 285, false },
	{ "ACH_WEP_HKMP5", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_HKSMG2", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_HKMP7", "BBX_KI_HKMP7", 300, ACHIEVEMENT_TYPE_DEFAULT, 250, false },
	{ "ACH_WEP_MAC11", "BBX_KI_MAC11", 300, ACHIEVEMENT_TYPE_DEFAULT, 350, false },

	// Rifles
	{ "ACH_WEP_FAMAS", "BBX_KI_FAMAS", 240, ACHIEVEMENT_TYPE_DEFAULT, 150, false },
	{ "ACH_WEP_AK74", "BBX_KI_AK74", 300, ACHIEVEMENT_TYPE_DEFAULT, 150, false },
	{ "ACH_WEP_G36C", "BBX_KI_G36C", 300, ACHIEVEMENT_TYPE_DEFAULT, 200, false },
	{ "ACH_WEP_M4", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },

	// Shotguns
	{ "ACH_WEP_870", "BBX_KI_870", 210, ACHIEVEMENT_TYPE_DEFAULT, 150, false },
	{ "ACH_WEP_DOUBLEBARREL", "BBX_KI_SAWOFF", 200, ACHIEVEMENT_TYPE_DEFAULT, 200, false },
	{ "ACH_WEP_WINCHESTER", "BBX_KI_TRAPPER", 240, ACHIEVEMENT_TYPE_DEFAULT, 400, false },

	// Special & Snipers
	{ "ACH_WEP_MINIGUN", "BBX_KI_MINIGUN", 12000, ACHIEVEMENT_TYPE_DEFAULT, 3000, false },
	{ "ACH_WEP_50CAL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_700", "BBX_KI_REM700", 150, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_WEP_CROSSBOW", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_STONER", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },

	// Explosives
	{ "ACH_WEP_EXPLOSIVE", "BBX_KI_EXPLOSIVES", 200, ACHIEVEMENT_TYPE_DEFAULT, 450, false },

	// Melee
	{ "ACH_WEAPON_AXE", "BBX_KI_FIREAXE", 50, ACHIEVEMENT_TYPE_DEFAULT, 300, false },
	{ "ACH_WEAPON_KNIFE", "BBX_KI_M9PHROBIS", 35, ACHIEVEMENT_TYPE_DEFAULT, 150, false },
	{ "ACH_WEP_BASEBALLBAT", "BBX_KI_BASEBALLBAT", 80, ACHIEVEMENT_TYPE_DEFAULT, 200, false },
	{ "ACH_WEP_HATCHET", "BBX_KI_HATCHET", 120, ACHIEVEMENT_TYPE_DEFAULT, 240, false },
	{ "ACH_WEP_SLEDGE", "BBX_KI_SLEDGEHAMMER", 280, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_WEP_MACHETE", "BBX_KI_MACHETE", 280, ACHIEVEMENT_TYPE_DEFAULT, 300, false },
	{ "ACH_WEP_FISTS", "BBX_KI_FISTS", 100, ACHIEVEMENT_TYPE_DEFAULT, 400, false },

	// Special End Game:
	{ "ACH_GM_ARENA_WIN", "", 0, ACHIEVEMENT_TYPE_MAP, 250, false },
	{ "ACH_ENDGAME_NOFIREARMS", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 1000, false },
	{ "ACH_OBJ_NOFIREARMS", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 1000, false },

	// Maps:
	{ "ACH_MAP_LASTSTAND", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_TERMOIL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_ROOFTOP", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_COLOSSEUM", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_BARRACKS_ARENA", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_CARGO", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_DEVILSCRYPT", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_SWAMPTROUBLE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 500, false },
	{ "ACH_MAP_SALVAGE", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 2000, false },

	// Hidden:
	{ "ACH_SECRET_WATCHYOURSTEP", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true },
	{ "ACH_SECRET_TFO", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true },
	{ "ACH_AREA_MACHETE", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true },
	{ "ACH_AREA_TEA", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true },
	{ "ACH_SECRET_TURTLE", "", 0, ACHIEVEMENT_TYPE_MAP, 0, true },
	{ "ACH_SECRET_GIVEALL", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "ACH_WEP_BRICK", "", 0, ACHIEVEMENT_TYPE_DEFAULT, 150, true },

	// Stats Only + hidden (no achievs) - used for leaderboards...
	{ "", "BBX_ST_KILLS", 9999999, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
	{ "", "BBX_ST_DEATHS", 9999999, ACHIEVEMENT_TYPE_DEFAULT, 0, true },
};

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

DataPenetrationItem_t *GetPenetrationDataForMaterial(unsigned short material)
{
	for (int i = 0; i < PENETRATION_DATA_SIZE; i++)
	{
		if (PENETRATION_DATA_LIST[i].material == material)
			return &PENETRATION_DATA_LIST[i];
	}

	return NULL;
}
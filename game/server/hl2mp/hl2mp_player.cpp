//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: BB2 Player
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "game.h"
#include "gamerules.h"
#include "hl2mp_player_shared.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "hl2mp_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "npcevent.h"
#include "eventqueue.h"
#include "tier0/vprof.h"
#include "datacache/imdlcache.h"
#include "bone_setup.h"
#include "npc_BaseZombie.h"
#include "filesystem.h"
#include "ammodef.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "spawn_point_base.h"
#include "ilagcompensationmanager.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "objective_icon.h"
#include "particle_parse.h"
#include "world.h"
#include "viewport_panel_names.h"
#include "BaseKeyPadEntity.h"
#include "items.h"
#include "te_player_gib.h"
#include "random_extended.h"

#include "tier0/memdbgon.h"

static CLogicPlayerEquipper *m_pPlayerEquipper = NULL;

//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Logic Player Equipper - Allows the mapper to equip anyone with certain weapons...
//
//========================================================================================//

class CLogicPlayerEquipper : public CLogicalEntity
{
public:
	DECLARE_CLASS(CLogicPlayerEquipper, CLogicalEntity);
	DECLARE_DATADESC();

	CLogicPlayerEquipper();
	virtual ~CLogicPlayerEquipper();

	void Spawn(void);
	void EquipPlayer(CHL2MP_Player *pPlayer);
	bool ShouldEquipWeapon(const char *weaponName);

private:

	string_t pchWeaponLinks[5];
};

BEGIN_DATADESC(CLogicPlayerEquipper)
DEFINE_KEYFIELD(pchWeaponLinks[0], FIELD_STRING, "Weapon1"),
DEFINE_KEYFIELD(pchWeaponLinks[1], FIELD_STRING, "Weapon2"),
DEFINE_KEYFIELD(pchWeaponLinks[2], FIELD_STRING, "Weapon3"),
DEFINE_KEYFIELD(pchWeaponLinks[3], FIELD_STRING, "Weapon4"),
DEFINE_KEYFIELD(pchWeaponLinks[4], FIELD_STRING, "Weapon5"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(logic_player_equip, CLogicPlayerEquipper);

CLogicPlayerEquipper::CLogicPlayerEquipper()
{
	for (int i = 0; i < _ARRAYSIZE(pchWeaponLinks); i++)
	{
		pchWeaponLinks[i] = NULL_STRING;
	}

	m_pPlayerEquipper = this;
}

CLogicPlayerEquipper::~CLogicPlayerEquipper()
{
	m_pPlayerEquipper = NULL;
}

void CLogicPlayerEquipper::Spawn()
{
	BaseClass::Spawn();

	bool bIsValid = false;
	for (int i = 0; i < _ARRAYSIZE(pchWeaponLinks); i++)
	{
		if (pchWeaponLinks[i] != NULL_STRING)
		{
			bIsValid = true;
			break;
		}
	}

	if (!bIsValid)
	{
		Warning("Invalid logic_player_equip '%s' with no weapon links!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}
}

void CLogicPlayerEquipper::EquipPlayer(CHL2MP_Player *pPlayer)
{
	if (!pPlayer)
		return;

	for (int i = 0; i < _ARRAYSIZE(pchWeaponLinks); i++)
	{
		if (pchWeaponLinks[i] != NULL_STRING)
			pPlayer->GiveItem(STRING(pchWeaponLinks[i]), true);
	}

	// Switch to the best available wep:
	CBaseCombatWeapon *pWantedWeapon = pPlayer->GetBestWeapon();
	if (pWantedWeapon != NULL)
		pPlayer->Weapon_Switch(pWantedWeapon, true);
}

bool CLogicPlayerEquipper::ShouldEquipWeapon(const char *weaponName)
{
	for (int i = 0; i < _ARRAYSIZE(pchWeaponLinks); i++)
	{
		if (pchWeaponLinks[i] == NULL_STRING)
			continue;

		if (!strcmp(STRING(pchWeaponLinks[i]), weaponName))
			return true;
	}

	return false;
}

// END

#define HL2MP_COMMAND_MAX_RATE 0.3
#define VOICE_COMMAND_EMIT_DELAY 1.5f
#define HIGH_PING_CHECK_TIME 30.0f

extern void Host_Say(edict_t *pEdict, const char *message, bool teamonly, int chatCmd);

LINK_ENTITY_TO_CLASS(player, CHL2MP_Player);
LINK_ENTITY_TO_CLASS(info_player_human, CBaseSpawnPoint);
LINK_ENTITY_TO_CLASS(info_player_zombie, CBaseSpawnPoint);
LINK_ENTITY_TO_CLASS(info_start_camera, CPointEntity);

extern void SendProxy_Origin(const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);

void* SendProxy_SendNonLocalDataTable(const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID)
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient(objectID - 1);
	return (void *)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER(SendProxy_SendNonLocalDataTable);

BEGIN_SEND_TABLE_NOBASE(CHL2MP_Player, DT_HL2MPLocalPlayerExclusive)
// send a hi-res origin to the local player for use in prediction
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
SendPropFloat(SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE(CHL2MP_Player, DT_HL2MPNonLocalPlayerExclusive)
// send a lo-res origin to other players
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
SendPropFloat(SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f),
SendPropAngle(SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CHL2MP_Player, DT_HL2MP_Player)
SendPropDataTable(SENDINFO_DT(m_BB2Local), &REFERENCE_SEND_TABLE(DT_BB2Local), SendProxy_SendLocalDataTable),

SendPropExclude("DT_BaseAnimating", "m_flPoseParameter"),
SendPropExclude("DT_BaseAnimating", "m_flPlaybackRate"),
SendPropExclude("DT_BaseAnimating", "m_nSequence"),
SendPropExclude("DT_BaseEntity", "m_angRotation"),
SendPropExclude("DT_BaseAnimatingOverlay", "overlay_vars"),

SendPropExclude("DT_BaseEntity", "m_vecOrigin"),

// playeranimstate and clientside animation takes care of these on the client
SendPropExclude("DT_ServerAnimationData", "m_flCycle"),
SendPropExclude("DT_AnimTimeMustBeFirst", "m_flAnimTime"),

SendPropExclude("DT_BaseFlex", "m_flexWeight"),
SendPropExclude("DT_BaseFlex", "m_blinktoggle"),
SendPropExclude("DT_BaseFlex", "m_viewtarget"),

// BB2 Warn
SendPropExclude("DT_BaseFlex", "m_vecLean"),
SendPropExclude("DT_BaseFlex", "m_vecShift"),

// Data that only gets sent to the local player.
SendPropDataTable("hl2mplocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2MPLocalPlayerExclusive), SendProxy_SendLocalDataTable),
// Data that gets sent to all other players
SendPropDataTable("hl2mpnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2MPNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable),

SendPropInt(SENDINFO(m_iSpawnInterpCounter), 4),
SendPropInt(SENDINFO(m_nPerkFlags), PERK_MAX_BITS, SPROP_UNSIGNED),
SendPropBool(SENDINFO(m_bIsInSlide)),

// Client-Sided Player Model Logic:
SendPropArray3(SENDINFO_ARRAY3(m_iCustomizationChoices), SendPropInt(SENDINFO_ARRAY(m_iCustomizationChoices), 5, SPROP_UNSIGNED)),
SendPropString(SENDINFO(m_szModelChoice)),
SendPropInt(SENDINFO(m_iModelIncrementor), 11, SPROP_UNSIGNED),
END_SEND_TABLE()

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CHL2MP_Player::CHL2MP_Player()
{
	m_PlayerAnimState = CreateHL2MPPlayerAnimState(this);
	m_achievStats = new CPlayerAchievStats(this);
	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_iTotalScore = 0;
	m_iTotalDeaths = 0;
	m_iRoundScore = 0;
	m_iRoundDeaths = 0;
	m_iZombKills = 0;
	m_nGroupID = 0;
	m_iZombDeaths = 0;
	m_iNumPerkKills = 0;
	m_iSelectedTeam = 0;
	m_iSkill_Level = 0;
	m_nPerkFlags = 0;
	m_nMaterialOverlayFlags = 0;
	m_bIsInSlide = false;

	m_iSpawnInterpCounter = 0;

	m_bIsInfected = false;
	m_bEnterObserver = false;
	m_bHasReadProfileData = false;
	m_bWantsToDeployAsHuman = false;
	m_bHasFullySpawned = false;
	m_bHasTriedToLoadStats = false;
	m_bTriedToJoinGame = false;
	m_bPlayerUsedFirearm = false;
	m_bEnableFlashlighOnSwitch = false;
	m_iAdminLevel = 0;

	m_flUpdateTime = 0.0f;
	m_flNextResupplyTime = 0.0f;
	m_flLastInfectionTwitchTime = 0.0f;
	m_flLastTimeRanCommand = 0.0f;
	m_flZombieRageTime = m_flZombieAttackTime = m_flZombieDamageThresholdDepletion = 0.0f;

	m_flAmmoRequestTime = m_flLastTimeSharedAmmo = 0.0f;
	m_iAmmoRequestID = 0;

	m_iTotalPing = 0;
	m_iTimesCheckedPing = 0;
	m_flLastTimeCheckedPing = 0.0f;
	m_flTimeUntilEndCheckPing = 0.0f;
	m_bFinishedPingCheck = false;

	m_hLastKiller = NULL;

	m_ullCachedSteamID = 0;

	// Client-Sided Player Model Logic:
	for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
		m_iCustomizationChoices.Set(i, 0);
	Q_strncpy(m_szModelChoice.GetForModify(), "", MAX_MAP_NAME);
	m_iModelIncrementor = 0;

	BaseClass::ChangeTeam(0);
}

CHL2MP_Player::~CHL2MP_Player()
{
	if ((engine->IsDedicatedServer() == false) && (m_iSkill_Level > 0))
		HandleLocalProfile(true);

	pszWeaponPenaltyList.Purge();
	m_PlayerAnimState->Release();
	delete m_achievStats;
	m_achievStats = NULL;
	m_hLastKiller = NULL;
}

void CHL2MP_Player::Precache(void)
{
	BaseClass::Precache();

	PrecacheModel("sprites/glow01.vmt");

	PrecacheModel(DEFAULT_PLAYER_MODEL(TEAM_HUMANS));
	PrecacheModel(DEFAULT_PLAYER_MODEL(TEAM_DECEASED));

	// Extra handy stuff...
	PrecacheScriptSound("BaseEnemyHumanoid.Die");
	PrecacheScriptSound("ItemShared.Pickup");
	PrecacheScriptSound("ItemShared.Deny");
	PrecacheScriptSound("Music.Round.Start");
	PrecacheScriptSound("Player.ArmorImpact");

	PrecacheScriptSound("SkillActivated.BlazingAmmo");
	PrecacheScriptSound("SkillActivated.FrostAmmo");
	PrecacheScriptSound("SkillActivated.MagRefill");
	PrecacheScriptSound("SkillActivated.MeleeBleed");
	PrecacheScriptSound("SkillActivated.MeleeStun");
	PrecacheScriptSound("SkillActivated.BulletPenetration");
	PrecacheScriptSound("SkillActivated.LifeLeech");

	PrecacheScriptSound("KeyPad.Unlock");
	PrecacheScriptSound("KeyPad.Fail");

	PrecacheParticleSystem("bb2_levelup_effect");
	PrecacheParticleSystem("bb2_perk_activate");
}

void CHL2MP_Player::HandleFirstTimeConnection(bool bForceDefault)
{
	if (m_bHasReadProfileData && !bForceDefault)
		return;

	// Load / TRY to load stored profile.
	if (!bForceDefault)
	{
		if (AchievementManager::CanLoadSteamStats(this) || HandleLocalProfile())
			return;
	}

	// We only init this if we don't init the skills / global stats.
	int iLevel = (GameBaseServer()->IsTutorialModeEnabled() ? MAX_PLAYER_LEVEL : clamp(GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iLevel, 1, MAX_PLAYER_LEVEL));
	m_iSkill_Level = iLevel;
	m_BB2Local.m_iSkill_Talents = GameBaseShared()->GetSharedGameDetails()->CalculatePointsForLevel(iLevel);
	m_BB2Local.m_iSkill_XPLeft = (GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iXPIncreasePerLevel * iLevel);
	OnLateStatsLoadEnterGame();
	m_bHasTriedToLoadStats = true;
}

void CHL2MP_Player::OnLateStatsLoadEnterGame(void)
{
	m_ullCachedSteamID = (m_ullCachedSteamID > 0) ? m_ullCachedSteamID : ((unsigned long long)GetSteamIDAsUInt64());

	if (!GameBaseServer()->IsTutorialModeEnabled())
	{
		// Verify Skills! Check if the player somehow managed to get more points than allowed!
		if (HL2MPRules()->CanUseSkills())
		{
			int pointsTotal = m_BB2Local.m_iSkill_Talents.Get();
			for (int i = 0; i < PLAYER_SKILL_ZOMBIE_HEALTH; i++)
				pointsTotal += m_BB2Local.m_iPlayerSkills.Get(i);

			if (pointsTotal > MAX_PLAYER_TALENTS)
			{
				m_BB2Local.m_iSkill_Talents = GameBaseShared()->GetSharedGameDetails()->CalculatePointsForLevel(GetPlayerLevel());
				for (int i = 0; i < PLAYER_SKILL_ZOMBIE_HEALTH; i++)
					m_BB2Local.m_iPlayerSkills.Set(i, 0);
			}
		}

		switch (HL2MPRules()->GetCurrentGamemode())
		{

		case MODE_ARENA:
		{
			if (!HL2MPRules()->m_bRoundStarted)
				HandleCommand_JoinTeam(TEAM_HUMANS);
			break;
		}

		case MODE_OBJECTIVE:
		{
			bool bAllowedToJoinLate = (bb2_allow_latejoin.GetBool() && !HL2MPRules()->DidClientDisconnectRecently(m_ullCachedSteamID));
			if (bAllowedToJoinLate || GameBaseServer()->IsStoryMode() || !HL2MPRules()->m_bRoundStarted)
			{
				m_bWantsToDeployAsHuman = true;
				HandleCommand_JoinTeam(TEAM_HUMANS);
			}
			else
				HandleCommand_JoinTeam(TEAM_DECEASED);
			break;
		}

		case MODE_DEATHMATCH:
		{
			if (HL2MPRules()->m_bRoundStarted)
				m_BB2Local.m_flPlayerRespawnTime = HL2MPRules()->GetPlayerRespawnTime(this);
			else
				HandleCommand_JoinTeam(TEAM_HUMANS);
			break;
		}

		case MODE_ELIMINATION:
		{
			HandleCommand_JoinTeam(GetSelectedTeam());
			break;
		}

		}
	}

	ForceRespawn();
}

void CHL2MP_Player::PickDefaultSpawnTeam(int iForceTeam)
{
	SetConnected(PlayerConnected);
	m_flTimeUntilEndCheckPing = gpGlobals->curtime + HIGH_PING_CHECK_TIME;
	if (HL2MPRules()->m_bRoundStarted)
		m_flLastTimeRanCommand = gpGlobals->curtime;

	if (GetTeamNumber() <= TEAM_UNASSIGNED)
		GameBaseServer()->NewPlayerConnection(this);

	if (iForceTeam > 0)
	{
		m_bHasFullySpawned = m_bHasTriedToLoadStats = true;
		SetSelectedTeam(iForceTeam);
		HandleCommand_JoinTeam(iForceTeam, true);
		ForceRespawn();
		return;
	}

	if (GetTeamNumber() <= TEAM_UNASSIGNED)
	{
		if (GameBaseServer()->IsTutorialModeEnabled())
		{
			m_bHasFullySpawned = m_bHasTriedToLoadStats = true;
			m_BB2Local.m_iZombieCredits = 100;
			HandleCommand_JoinTeam(TEAM_HUMANS);
			HandleFirstTimeConnection();
		}
		else
		{
			Spawn();
			HandleCommand_JoinTeam(TEAM_SPECTATOR);

			const ConVar *hostname = cvar->FindVar("hostname");
			const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

			KeyValues *data = new KeyValues("data");
			data->SetString("title", title);		 // info panel title
			data->SetString("type", "1");			// show userdata from stringtable entry
			data->SetInt("cmd", 1);			       // Joingame cmd on closing.
			data->SetString("msg", "motd");		  // use this stringtable entry
			data->SetBool("unload", true);

			ShowViewPortPanel(PANEL_INFO, true, data);

			data->deleteThis();
		}
	}
}

bool CHL2MP_Player::SaveGlobalStats(void)
{
	if (!AchievementManager::IsGlobalStatsAllowed() || !m_bHasReadProfileData || IsBot())
		return false;

	CSteamID pSteamClient;
	if (!GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %s\n", GetPlayerName());
		return false;
	}

	// BB2 SKILL TREE - Base
	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, "BBX_ST_XP_CURRENT", m_BB2Local.m_iSkill_XPCurrent);
	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, "BBX_ST_XP_LEFT", m_BB2Local.m_iSkill_XPLeft);
	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, "BBX_ST_LEVEL", GetPlayerLevel());
	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, "BBX_ST_TALENTS", m_BB2Local.m_iSkill_Talents); // Can be spent on the skills panel tree!
	steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, "BBX_ST_ZM_POINTS", m_BB2Local.m_iZombieCredits);

	for (int i = 0; i < _ARRAYSIZE(pszGameSkills); i++)
		steamgameserverapicontext->SteamGameServerStats()->SetUserStat(pSteamClient, pszGameSkills[i], GetSkillValue(i));

	steamgameserverapicontext->SteamGameServerStats()->StoreUserStats(pSteamClient);
	return true;
}

void CHL2MP_Player::OnReceivedSteamStats(GSStatsReceived_t *pCallback, bool bIOFailure)
{
	if (m_bHasTriedToLoadStats || m_bHasReadProfileData)
		return;

	Warning("Received and loaded stats for user %s, result %i\n", GetPlayerName(), pCallback->m_eResult);
	m_bHasTriedToLoadStats = true;

	if (!AchievementManager::IsGlobalStatsAllowed() || bIOFailure || (pCallback->m_eResult != k_EResultOK))
	{
		if (HandleLocalProfile())
			return;

		HandleFirstTimeConnection(true);
		return;
	}

	int iXPCurrent = 0, iXPLeft = 65, iLevel = 1, iTalents = 0, iZombieCredits = 0;

	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pCallback->m_steamIDUser, "BBX_ST_XP_CURRENT", &iXPCurrent);
	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pCallback->m_steamIDUser, "BBX_ST_XP_LEFT", &iXPLeft);
	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pCallback->m_steamIDUser, "BBX_ST_LEVEL", &iLevel);
	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pCallback->m_steamIDUser, "BBX_ST_TALENTS", &iTalents);
	steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pCallback->m_steamIDUser, "BBX_ST_ZM_POINTS", &iZombieCredits);

	m_BB2Local.m_iSkill_XPCurrent = iXPCurrent;
	m_BB2Local.m_iSkill_XPLeft = iXPLeft;
	SetPlayerLevel(iLevel);
	m_BB2Local.m_iSkill_Talents = iTalents; // Can be spent on the skills panel tree!
	m_BB2Local.m_iZombieCredits = iZombieCredits;

	for (int i = 0; i < _ARRAYSIZE(pszGameSkills); i++)
	{
		int iSkillValue = 0;
		steamgameserverapicontext->SteamGameServerStats()->GetUserStat(pCallback->m_steamIDUser, pszGameSkills[i], &iSkillValue);
		SetSkillValue(i, iSkillValue);
	}

	m_bHasReadProfileData = true;
	OnLateStatsLoadEnterGame();
}

// ID the player.
Class_T CHL2MP_Player::Classify(void)
{
	Class_T base = BaseClass::Classify();
	if (base != CLASS_NONE)
		return base;

	if (GetTeamNumber() == TEAM_DECEASED)
		return CLASS_PLAYER_ZOMB;
	else if (m_bIsInfected)
		return CLASS_PLAYER_INFECTED;
	else if (GetTeamNumber() == TEAM_HUMANS)
		return CLASS_PLAYER;

	return CLASS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2MP_Player::Spawn(void)
{
	BaseClass::Spawn();

	m_BB2Local.m_bPlayerJumped = false;

	// If infection started and we died in the middle of it then remove it when we respawn:
	m_bIsInfected = false;
	m_BB2Local.m_flInfectionTimer = 0.0f;
	m_flLastInfectionTwitchTime = 0.0f;

	// Perk Reset
	m_BB2Local.m_flPerkTimer = 0.0f;
	m_iNumPerkKills = 0;
	m_nPerkFlags = 0;
	m_BB2Local.m_bCanActivatePerk = false;
	m_BB2Local.m_flZombieRageThresholdDamage = 0.0f;
	m_flZombieRageTime = m_flZombieAttackTime = m_flZombieDamageThresholdDepletion = 0.0f;

	// Misc
	m_flNextResupplyTime = 0.0f;
	m_nMaterialOverlayFlags = 0;
	m_bEnableFlashlighOnSwitch = false;

	// Ammo Sharing
	m_flAmmoRequestTime = m_flLastTimeSharedAmmo = 0.0f;
	m_iAmmoRequestID = 0;

	ResetSlideVars();
	OnSetGibHealth();

	if (!IsObserver())
	{
		SetPlayerClass(GetTeamNumber());

		pl.deadflag = false;
		RemoveSolidFlags(FSOLID_NOT_SOLID);

		RemoveEffects(EF_NODRAW);

		RefreshSpeed();

		AddMaterialOverlayFlag(MAT_OVERLAY_SPAWNPROTECTION);
	}

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;

	AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

	m_impactEnergyScale = HL2MPPLAYER_PHYSDAMAGE_SCALE;

	if (HL2MPRules()->IsGameoverOrScoresVisible())
		AddFlag(FL_FROZEN);
	else
		RemoveFlag(FL_FROZEN);

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater(false);

	if (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION))
		SetGlowMode(GLOW_MODE_TEAMMATE);
	else
		SetGlowMode(GLOW_MODE_NONE);

	pszWeaponPenaltyList.Purge();

	ExecuteClientEffect(PLAYER_EFFECT_FULLCHECK, 1);

	DoAnimationEvent(PLAYERANIMEVENT_SPAWN, 0, true);

	m_achievStats->OnSpawned();
}

// Perform reasonable updates at a cheap interval:
void CHL2MP_Player::PerformPlayerUpdate(void)
{
	// Save Stats
	SaveGlobalStats();

	if (HL2MPRules()->CanUseSkills() && IsAlive())
	{
		// Team Bonus:
		int iPlayersFound = 0;
		// We glow our teammates if we're a human and our friends are far away but in range and out of sight:
		// We glow our enemies if we're a zombie.
		if (GetTeamNumber() >= TEAM_HUMANS)
		{
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pOther = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				if (pOther == NULL)
					continue;

				if ((pOther->entindex() == this->entindex()) ||
					!pOther->IsAlive() ||
					(pOther->GetTeamNumber() < TEAM_HUMANS) ||
					(this->GetTeamNumber() != pOther->GetTeamNumber()))
					continue;

				// Check for team bonus
				if ((GetLocalOrigin().DistTo(pOther->GetLocalOrigin()) <= MAX_TEAMMATE_DISTANCE) && this->FVisible(pOther, MASK_VISIBLE))
					iPlayersFound++;
			}
		}

		m_BB2Local.m_iPerkTeamBonus = iPlayersFound;

		if (IsHuman())
		{
			m_BB2Local.m_bCanActivatePerk = (!GetPerkFlags() && (m_iNumPerkKills >= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->iKillsRequiredToPerk)
				&& ((GetSkillValue(PLAYER_SKILL_HUMAN_REALITY_PHASE) > 0) || (GetSkillValue(PLAYER_SKILL_HUMAN_BLOOD_RAGE) > 0) || (GetSkillValue(PLAYER_SKILL_HUMAN_GUNSLINGER) > 0)));
		}
		else if (IsZombie())
		{
			m_BB2Local.m_bCanActivatePerk = (!GetPerkFlags() && (m_BB2Local.m_flZombieRageThresholdDamage >= GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flRequiredDamageThreshold));
		}
	}
	else
	{
		m_BB2Local.m_iPerkTeamBonus = 0;
	}
}

void CHL2MP_Player::DispatchDamageText(CBaseEntity *pVictim, int damage)
{
	if (!pVictim || (damage == 0))
		return;

	Vector absPos = pVictim->GetAbsOrigin();

	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();

	UserMessageBegin(filter, "DamageTextInfo");
	WRITE_FLOAT(absPos.x);
	WRITE_FLOAT(absPos.y);
	WRITE_FLOAT(absPos.z);
	WRITE_SHORT(damage);
	MessageEnd();

	if (!IsPerkFlagActive(PERK_ZOMBIE_RAGE) && pVictim->IsPlayer() && !pVictim->IsZombie(true) && IsZombie(true) && (damage < 0))
	{
		m_flZombieAttackTime = gpGlobals->curtime;

		float maxThreshold = GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flRequiredDamageThreshold;
		if (m_BB2Local.m_flZombieRageThresholdDamage < maxThreshold)
			m_BB2Local.m_flZombieRageThresholdDamage += abs(damage);

		m_BB2Local.m_flZombieRageThresholdDamage = clamp(m_BB2Local.m_flZombieRageThresholdDamage, 0.0f, maxThreshold);

		CheckCanRage();
	}
}

void CHL2MP_Player::PickupObject(CBaseEntity *pObject, bool bLimitMassAndSize)
{
	BaseClass::PickupObject(pObject, bLimitMassAndSize);
}

void CHL2MP_Player::SetPlayerModel(int overrideTeam)
{
	int teamNum = GetTeamNumber();
	if (overrideTeam != -1)
		teamNum = overrideTeam;

	const char *survivorChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice");
	if (survivorChoice && survivorChoice[0])
		Q_strncpy(m_szModelChoice.GetForModify(), survivorChoice, MAX_MAP_NAME);
	else
		Q_strncpy(m_szModelChoice.GetForModify(), "survivor1", MAX_MAP_NAME);

	SetModel(DEFAULT_PLAYER_MODEL(teamNum));
	SetupCustomization();

	// Soundset Logic:
	const char *survivorLink = survivorChoice;
	if ((survivorLink == NULL) || !survivorLink[0])
		survivorLink = "survivor1";

	const char *soundsetPrefix = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), ((GetTeamNumber() == TEAM_HUMANS) ? "bb2_sound_player_human" : "bb2_sound_player_deceased"));
	if ((soundsetPrefix == NULL) || !soundsetPrefix[0])
		soundsetPrefix = ((GetTeamNumber() == TEAM_HUMANS) ? "Pantsman" : "Default");

	Q_strncpy(pchSoundsetSurvivorLink, survivorLink, 64);
	Q_strncpy(pchSoundsetPrefix, soundsetPrefix, 64);
	m_bSoundsetGender = true; // Obsolete, set "gender" "X" <- in character mdl script.

	m_iModelIncrementor++; // Let clients register a change, easily...
}

void CHL2MP_Player::SetupCustomization(void)
{
	m_nBody = m_nSkin = 0;
	for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
		m_iCustomizationChoices.Set(i, 0);

	const char *skinChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice_skin");
	const char *headChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice_extra_head");
	const char *bodyChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice_extra_body");
	const char *rightLegChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice_extra_leg_right");
	const char *leftLegChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice_extra_leg_left");

	if ((skinChoice != NULL) && (headChoice != NULL) && (bodyChoice != NULL) && (rightLegChoice != NULL) && (leftLegChoice != NULL))
	{
		m_nSkin = atoi(skinChoice);

		int bodygroupAccessoryValues[PLAYER_ACCESSORY_MAX] =
		{
			atoi(headChoice),
			atoi(bodyChoice),
			atoi(leftLegChoice),
			atoi(rightLegChoice),
		};

		for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
			m_iCustomizationChoices.Set(i, bodygroupAccessoryValues[i]);
	}
}

bool CHL2MP_Player::Weapon_Switch(CBaseCombatWeapon *pWeapon, bool bWantDraw)
{
	MDLCACHE_CRITICAL_SECTION();

	bool bRet = BaseClass::Weapon_Switch(pWeapon, bWantDraw);
	if (bRet == true)
	{
		if (bWantDraw && m_bEnableFlashlighOnSwitch)
		{
			m_bEnableFlashlighOnSwitch = false;
			FlashlightTurnOn();
		}
		else
		{
			RemoveEffects(EF_DIMLIGHT);
			if (IsAlive())
				EmitSound("HL2Player.FlashlightOff");
		}
	}

	return bRet;
}

void CHL2MP_Player::PreThink(void)
{
	BaseClass::PreThink();
	State_PreThink();

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;
}

// Give us armor or remove!
void CHL2MP_Player::ApplyArmor(float flArmorDurability, int iType)
{
	m_ArmorValue = ((int)flArmorDurability);

	if (flArmorDurability <= 0)
	{
		m_ArmorValue = 0;
		m_BB2Local.m_iActiveArmorType = 0;
		GameBaseShared()->ComputePlayerWeight(this);
		return;
	}

	m_BB2Local.m_iActiveArmorType = iType;
	GameBaseShared()->ComputePlayerWeight(this);
}

void CHL2MP_Player::OnZombieInfectionComplete(void)
{
	// Throw out the weapons.
	DropAllWeapons();
	ResetSlideVars();

	// Change to zombie!
	ChangeTeam(TEAM_DECEASED, true);
	SetPlayerClass(TEAM_DECEASED);
	ExecuteClientEffect(PLAYER_EFFECT_BECOME_ZOMBIE, 1);

	m_BB2Local.m_flCarryWeight = 0.0f;
	m_BB2Local.m_flPerkTimer = 0.0f;
	m_iNumPerkKills = 0;
	m_nPerkFlags = 0;
	m_BB2Local.m_bCanActivatePerk = false;
	m_bEnableFlashlighOnSwitch = false;

	RefreshSpeed();
}

void CHL2MP_Player::OnAffectedBySkill(const CTakeDamageInfo &info)
{
	int skillFlag = info.GetSkillFlags();
	if (!skillFlag || !(skillFlag & SKILL_FLAG_BLAZINGAMMO) || (info.GetInflictor() == NULL) || IsMaterialOverlayFlagActive(MAT_OVERLAY_SPAWNPROTECTION) || !IsAlive() || IsObserver())
		return;

	CHL2MP_Player *pAttacker = ToHL2MPPlayer(info.GetAttacker());
	if (!pAttacker)
		return;

	if (IsAffectedBySkillFlag(skillFlag) || !FClassnameIs(info.GetInflictor(), "weapon_flamethrower"))
		return;

	AddMaterialOverlayFlag(MAT_OVERLAY_BURNING);

	playerSkillAffectionItem_t item;
	item.flag = SKILL_FLAG_BLAZINGAMMO;
	item.overlayFlag = MAT_OVERLAY_BURNING;
	item.damage = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flPlayerBurnDamage;
	item.duration = (gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flPlayerBurnDuration);
	item.nextTimeToTakeDamage = 0.0f;
	item.timeToTakeDamage = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flPlayerBurnFrequency;
	item.misc = info.GetForcedWeaponID();
	item.m_pAttacker = pAttacker;

	m_pActiveSkillEffects.AddToTail(item);

	OnSkillFlagState(SKILL_FLAG_BLAZINGAMMO, true);
}

int CHL2MP_Player::GetSkillAffectionDamageType(int skillFlag)
{
	if (skillFlag & SKILL_FLAG_BLAZINGAMMO)
		return DMG_BURN;

	return DMG_CLUB;
}

bool CHL2MP_Player::DoHighPingCheck(void)
{
	if (m_bFinishedPingCheck || IsBot())
		return false;

	if (gpGlobals->curtime > m_flLastTimeCheckedPing)
	{
		m_flLastTimeCheckedPing = gpGlobals->curtime + 1.0f; // Check ping every sec.
		m_iTimesCheckedPing++;
		int ping, packetloss;
		UTIL_GetPlayerConnectionInfo(entindex(), ping, packetloss);
		m_iTotalPing += ping;
	}

	if (gpGlobals->curtime > m_flTimeUntilEndCheckPing)
	{
		m_bFinishedPingCheck = true;
		int averagePing = (m_iTotalPing / m_iTimesCheckedPing);
		if (averagePing > bb2_high_ping_limit.GetInt())
		{
			char pchKickCmd[128];
			Q_snprintf(pchKickCmd, 128, "kickid %i Too high ping!\n", GetUserID());
			engine->ServerCommand(pchKickCmd);
			engine->ServerExecute();
			return true;
		}
	}

	return false;
}

void CHL2MP_Player::PostThink(void)
{
	BaseClass::PostThink();

	if (bb2_enable_high_ping_kicker.GetBool() && engine->IsDedicatedServer() && DoHighPingCheck())
		return;

	if (HL2MPRules() && HL2MPRules()->IsGameoverOrScoresVisible())
		m_flLastTimeRanCommand = gpGlobals->curtime;

	if (!IsBot() && bb2_enable_afk_kicker.GetBool() && engine->IsDedicatedServer() && !((GetTeamNumber() == TEAM_SPECTATOR) && HasLoadedStats()) && (m_flLastTimeRanCommand > 0.0f))
	{
		float flTimeSinceLastCMD = (gpGlobals->curtime - m_flLastTimeRanCommand);
		if (flTimeSinceLastCMD >= bb2_afk_kick_time.GetFloat())
		{
			char pchKickCmd[128];
			Q_snprintf(pchKickCmd, 128, "kickid %i AFK for too long!\n", GetUserID());
			engine->ServerCommand(pchKickCmd);
			engine->ServerExecute();
			return;
		}
	}

	CheckSkillAffections();

	if ((m_flSpawnProtection > 0) && (gpGlobals->curtime > m_flSpawnProtection))
		RemoveSpawnProtection();

	// Make sure we're never under 5:
	// m_iSkill_XPLeft == required xp until level. If 0 = broken, if 1 = too easy...
	if (m_BB2Local.m_iSkill_XPLeft < 5)
		m_BB2Local.m_iSkill_XPLeft = 5;

	// Never let the carry weight become negative!
	if (m_BB2Local.m_flCarryWeight < 0)
		m_BB2Local.m_flCarryWeight = 0.0f;

	if (IsSliding())
	{
		SetCollisionBounds(VEC_SLIDE_TRACE_MIN, VEC_SLIDE_TRACE_MAX);
	}
	else if (GetFlags() & FL_DUCKING)
	{
		SetCollisionBounds(VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX);
	}

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles(angles);

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();
	m_PlayerAnimState->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);

	// Every X sec we update costly player functions:
	if (m_flUpdateTime <= gpGlobals->curtime)
	{
		m_flUpdateTime = gpGlobals->curtime + 2.0f;
		PerformPlayerUpdate();
	}

	switch (CTeam::GetActivePerk(GetTeamNumber()))
	{

	case TEAM_HUMAN_PERK_UNLIMITED_AMMO:
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if (pWeapon)
		{
			if ((pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL) && !pWeapon->IsMeleeWeapon() && pWeapon->UsesClipsForAmmo())
			{
				pWeapon->m_iClip = pWeapon->GetMaxClip();
				pWeapon->AffectedByPlayerSkill(PLAYER_SKILL_HUMAN_GUNSLINGER);
			}
		}
		break;
	}

	}

	if (IsZombie())
	{
		if (IsAlive())
		{
			if (IsPerkFlagActive(PERK_ZOMBIE_RAGE))
			{
				if (m_flZombieRageTime < gpGlobals->curtime)
					LeaveRageMode();
			}
			else
			{
				float timeSinceAttack = gpGlobals->curtime - m_flZombieAttackTime;
				if ((m_BB2Local.m_flZombieRageThresholdDamage > 0.0f) && (timeSinceAttack >= GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flTimeUntilBarDepletes))
				{
					m_flZombieDamageThresholdDepletion += gpGlobals->frametime * GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flDepletionRate;
					if (m_flZombieDamageThresholdDepletion >= 1.0f)
					{
						m_BB2Local.m_flZombieRageThresholdDamage -= m_flZombieDamageThresholdDepletion;
						m_flZombieDamageThresholdDepletion = 0.0f;
						if (m_BB2Local.m_flZombieRageThresholdDamage < 0)
							m_BB2Local.m_flZombieRageThresholdDamage = 0.0f;
					}
				}
			}
		}

		return;
	}

	if (IsAlive())
	{
		if (GetPerkFlags())
		{
			if (m_BB2Local.m_flPerkTimer < gpGlobals->curtime)
			{
				m_BB2Local.m_flPerkTimer = 0.0f;
				ResetPerksAndPowerups();
				RefreshSpeed();

				if (IsPerkFlagActive(PERK_HUMAN_REALITYPHASE))
					CheckIsPlayerStuck();

				ClearPerkFlags();
				GameBaseShared()->ComputePlayerWeight(this);
			}

			if (IsPerkFlagActive(PERK_HUMAN_GUNSLINGER))
			{
				CBaseCombatWeapon *pWeapon = GetActiveWeapon();
				if (pWeapon)
				{
					if ((pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL) && !pWeapon->IsMeleeWeapon() && pWeapon->UsesClipsForAmmo())
					{
						pWeapon->m_iClip = pWeapon->GetMaxClip();
						pWeapon->AffectedByPlayerSkill(PLAYER_SKILL_HUMAN_GUNSLINGER);
					}
				}
			}
		}

		for (int i = (pszWeaponPenaltyList.Count() - 1); i >= 0; i--)
		{
			if (pszWeaponPenaltyList[i].m_flPenaltyTime < gpGlobals->curtime)
				pszWeaponPenaltyList.Remove(i);
		}
	}

	// Infection Stuff
	if (m_bIsInfected)
	{
		if (m_BB2Local.m_flInfectionTimer <= gpGlobals->curtime)
		{
			m_bIsInfected = false;
			m_BB2Local.m_flInfectionTimer = 0.0f;
			OnZombieInfectionComplete();
		}
	}
}

void CHL2MP_Player::DropAllWeapons(void)
{
	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
		Weapon_DropSlot(i);
}

void CHL2MP_Player::PlayerDeathThink()
{
	if (!IsObserver())
	{
		BaseClass::PlayerDeathThink();
	}
}

void CHL2MP_Player::FireBullets(const FireBulletsInfo_t &info)
{
	bool bNoSkills = info.m_bIgnoreSkills;
	if (bNoSkills)
	{
		BaseClass::FireBullets(info);
		return;
	}

	lagcompensation->BuildLagCompList(this, LAGCOMP_BULLET);

	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>(GetActiveWeapon());
	if (pWeapon)
	{
		int m_iDefDamage = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
		int m_iTeamBonusDamage = (m_BB2Local.m_iPerkTeamBonus - 5);
		int m_iNewDamage = (m_iDefDamage + GetSkillWeaponDamage((float)m_iDefDamage, pWeapon->GetWpnData().m_flSkillDamageFactor, pWeapon->GetWeaponType()));

		if (m_iTeamBonusDamage > 0)
			m_iNewDamage += ((m_iNewDamage / 100) * (m_iTeamBonusDamage * GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iTeamBonusDamageIncrease));

		float flPierceDMG = 0.0f;
		if (IsPerkFlagActive(PERK_HUMAN_GUNSLINGER))
			flPierceDMG = ((float)m_iNewDamage / 100.0f) * (GetSkillValue(PLAYER_SKILL_HUMAN_GUNSLINGER, TEAM_HUMANS));

		if (IsPerkFlagActive(PERK_POWERUP_CRITICAL))
		{
			const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(PERK_POWERUP_CRITICAL);
			if (data)
				flPierceDMG = (((float)m_iNewDamage / 100.0f) * data->flExtraFactor);
		}

		modinfo.m_iPlayerDamage += (m_iNewDamage + flPierceDMG);
		modinfo.m_flDamage += (m_iNewDamage + flPierceDMG);
		
		//Msg( "Killing with %s\nDamage %i\n", pWeapon->GetClassname(), modinfo.m_iPlayerDamage );
	}

	int iSkillFlags = 0;
	double perc = 0.0;

	if (GetSkillValue(PLAYER_SKILL_HUMAN_BLAZING_AMMO) > 0)
	{
		perc = (GetSkillValue(PLAYER_SKILL_HUMAN_BLAZING_AMMO, TEAM_HUMANS) / PERCENT_BASE);
		if (TryTheLuck(perc))
			iSkillFlags |= SKILL_FLAG_BLAZINGAMMO;
	}

	if (GetSkillValue(PLAYER_SKILL_HUMAN_COLDSNAP) > 0)
	{
		perc = (GetSkillValue(PLAYER_SKILL_HUMAN_COLDSNAP, TEAM_HUMANS) / PERCENT_BASE);
		if (TryTheLuck(perc))
			iSkillFlags |= SKILL_FLAG_COLDSNAP;
	}

	if (GetSkillValue(PLAYER_SKILL_HUMAN_EMPOWERED_BULLETS) > 0)
	{
		perc = (GetSkillValue(PLAYER_SKILL_HUMAN_EMPOWERED_BULLETS, TEAM_HUMANS) / PERCENT_BASE);
		if (TryTheLuck(perc))
			iSkillFlags |= SKILL_FLAG_EMPOWERED_BULLETS;
	}

	modinfo.m_nPlayerSkillFlags = iSkillFlags;

	NoteWeaponFired();

	BaseClass::FireBullets(modinfo);

	m_bPlayerUsedFirearm = true;

	lagcompensation->ClearLagCompList();
}

void CHL2MP_Player::NoteWeaponFired(void)
{
	Assert(m_pCurrentCommand);
	if (m_pCurrentCommand)
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

bool CHL2MP_Player::WantsLagCompensationOnEntity(const CBaseEntity *pEntity, const CUserCmd *pCmd) const
{
	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if (pActiveWeapon)
	{
		if (pActiveWeapon->m_iMeleeAttackType.Get() > 0)
		{
			// Only compensate ents within a reasonable distance.
			float distance = GetLocalOrigin().DistTo(pEntity->GetLocalOrigin());
			if (distance > MAX_MELEE_LAGCOMP_DIST)
				return false;
		}

		if (!pActiveWeapon->WantsLagCompensation(pEntity))
			return false;
	}

	return BaseClass::WantsLagCompensationOnEntity(pEntity, pCmd);
}

void CHL2MP_Player::Weapon_Equip(CBaseCombatWeapon *pWeapon)
{
	BaseClass::Weapon_Equip(pWeapon);
	GameBaseShared()->ComputePlayerWeight(this);
}

CBaseCombatWeapon *CHL2MP_Player::GetBestWeapon()
{
	int iWeight = 0;
	CBaseCombatWeapon *pWantedWeapon = NULL;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pUserWeapon = GetWeapon(i);
		if (!pUserWeapon)
			continue;

		if (pUserWeapon->GetWeight() > iWeight)
		{
			iWeight = pUserWeapon->GetWeight();
			pWantedWeapon = pUserWeapon;
		}
	}

	return pWantedWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CHL2MP_Player::BumpWeapon(CBaseCombatWeapon *pWeapon)
{	
	// Can I have this weapon type?
	if (!IsAllowedToPickupWeapons() || (GetTeamNumber() >= TEAM_DECEASED) || !IsAlive())
		return false;

	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();
	if (pOwner || !Weapon_CanSwitchTo(pWeapon))
		return false;

	bool bHasWeapon = false;
	CBaseCombatWeapon *pIdenticalWep = Weapon_GetSlot(pWeapon->GetSlot());
	if (pIdenticalWep)
	{
		if (FClassnameIs(pWeapon, pIdenticalWep->GetClassname()))
			bHasWeapon = true;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if (!pWeapon->FVisible(this, MASK_SOLID) && !(GetFlags() & FL_NOTARGET))
		return false;

	if ((m_iSkill_Level < pWeapon->GetWpnData().m_iLevelReq) && HL2MPRules()->CanUseSkills())
	{
		char pchArg1[16];
		Q_snprintf(pchArg1, 16, "%i", pWeapon->GetWpnData().m_iLevelReq);
		GameBaseServer()->SendToolTip("#TOOLTIP_WEAPON_DENY_LEVEL", GAME_TIP_DEFAULT, entindex(), pchArg1);
		return false;
	}

	// Have we picked up this very weapon a moment ago?
	if (HL2MPRules()->CanUseSkills() && (pWeapon->HasSpawnFlags(SF_NORESPAWN) == false) && !GameBaseServer()->IsTutorialModeEnabled())
	{
		int index = -1;
		for (int i = 0; i < pszWeaponPenaltyList.Count(); i++)
		{
			if (!strcmp(pszWeaponPenaltyList[i].szWeaponClassName, pWeapon->GetClassname()))
			{
				index = i;
				break;
			}
		}

		if (index == -1)
		{
			WeaponPickupItem_t item;
			Q_strncpy(item.szWeaponClassName, pWeapon->GetClassname(), MAX_WEAPON_STRING);
			item.m_flPenaltyTime = gpGlobals->curtime + pWeapon->GetWpnData().m_flPickupPenalty;
			pszWeaponPenaltyList.AddToTail(item);
		}
		else
		{
			int timeLeft = pszWeaponPenaltyList[index].m_flPenaltyTime - gpGlobals->curtime;
			if (timeLeft < 0)
				timeLeft = 0;

			char pchArg1[16];
			Q_snprintf(pchArg1, 16, "%d:%02d", (timeLeft / 60), (timeLeft % 60));
			GameBaseServer()->SendToolTip("#TOOLTIP_WEAPON_DENY_WAIT", GAME_TIP_DEFAULT, entindex(), pchArg1);
			return false;
		}
	}

	if (bHasWeapon)
	{
		// Consume it if possible!
		if (pWeapon->CanPickupWeaponAsAmmo() && (pIdenticalWep->GiveAmmo(pWeapon->GetDefaultClip()) > 0))
		{
			pWeapon->CheckRespawn();
			UTIL_Remove(pWeapon);
			return true;
		}
		return false;
	}

	Weapon_DropSlot(pWeapon->GetSlot());
	pWeapon->CheckRespawn();
	pWeapon->AddSolidFlags(FSOLID_NOT_SOLID);
	pWeapon->AddEffects(EF_NODRAW);
	Weapon_Equip(pWeapon);

	CBaseCombatWeapon *pActiveWep = GetActiveWeapon();
	if (pActiveWep)
	{
		bool bCantDeploy = (pActiveWep->m_bInReload || (pActiveWep->m_flNextBashAttack > gpGlobals->curtime) || !pActiveWep->CanDeploy());
		if (!bCantDeploy)
			Weapon_Switch(pWeapon);
	}
	else
		Weapon_Switch(pWeapon, true);

	return true;
}

// Gives the appropriate item:
bool CHL2MP_Player::GiveItem(const char *szItemName, bool bDoLevelCheck)
{
	if (bDoLevelCheck && HL2MPRules()->CanUseSkills())
	{
		WEAPON_FILE_INFO_HANDLE weaponHandle = LookupWeaponInfoSlot(szItemName);
		if (weaponHandle != GetInvalidWeaponInfoHandle())
		{
			FileWeaponInfo_t *pWeaponInfo = GetFileWeaponInfoFromHandle(weaponHandle);
			Assert(pWeaponInfo != NULL);
			int levelRequired = pWeaponInfo->m_iLevelReq;
			if (m_iSkill_Level < levelRequired)
			{
				char pchArg1[16];
				Q_snprintf(pchArg1, 16, "%i", levelRequired);
				GameBaseServer()->SendToolTip("#TOOLTIP_WEAPON_DENY_LEVEL", GAME_TIP_DEFAULT, entindex(), pchArg1);
				return false;
			}
		}
	}

	EHANDLE pent;
	pent = CreateEntityByName(szItemName);
	if (pent == NULL)
		return false;

	pent->SetLocalOrigin(GetLocalOrigin());
	pent->AddSpawnFlags(SF_NORESPAWN);

	DispatchSpawn(pent);

	if (pent != NULL && !(pent->IsMarkedForDeletion()))
		pent->Touch(this);

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*> ((CBaseEntity*)pent);
	if (pWeapon)
	{
		Weapon_DropSlot(pWeapon->GetSlot());
		pWeapon->CheckRespawn();
		pWeapon->AddSolidFlags(FSOLID_NOT_SOLID);
		pWeapon->AddEffects(EF_NODRAW);
		Weapon_Equip(pWeapon);

		CBaseCombatWeapon *pActiveWep = GetActiveWeapon();
		if (pActiveWep)
		{
			bool bCantDeploy = (pActiveWep->m_bInReload || (pActiveWep->m_flNextBashAttack > gpGlobals->curtime) || !pActiveWep->CanDeploy());
			if (!bCantDeploy)
				Weapon_Switch(pWeapon);
		}
		else
			Weapon_Switch(pWeapon, true);
	}

	return true;
}

bool CHL2MP_Player::PerformLevelUp(int iXP)
{
	int iXPToGive = iXP;

	if ((iXPToGive >= m_BB2Local.m_iSkill_XPLeft) && (m_iSkill_Level < MAX_PLAYER_LEVEL))
	{
		iXPToGive -= m_BB2Local.m_iSkill_XPLeft;

		// We increase xp needed for next level for the client.
		m_BB2Local.m_iSkill_XPLeft += GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iXPIncreasePerLevel;

		// Add Level.
		m_iSkill_Level++;
		// Reset kill counter so we start over again. IEX: Now you need 10 kills, if we no reset you just need 5! Which is no change at all.
		m_BB2Local.m_iSkill_XPCurrent = 0;

		// Add talent point(s).
		int pointsSpent = 0;
		for (int i = 0; i < PLAYER_SKILL_ZOMBIE_HEALTH; i++)
			pointsSpent += m_BB2Local.m_iPlayerSkills.Get(i);

		int pointsBefore = m_BB2Local.m_iSkill_Talents.Get();
		int pointsAfter = (GameBaseShared()->GetSharedGameDetails()->CalculatePointsForLevel(m_iSkill_Level) - pointsSpent);
		m_BB2Local.m_iSkill_Talents.Set(pointsAfter);

		if (!IsBot())
		{
			if (pointsAfter != pointsBefore)
			{
				// Congrats!
				char pchArg[32]; pchArg[0] = 0;
				Q_snprintf(pchArg, 32, "%i", pointsAfter);
				GameBaseServer()->SendToolTip("#TOOLTIP_LEVELUP2", GAME_TIP_DEFAULT, this->entindex(), pchArg);
			}

			{
				// Friendly messages:	
				char pchArg[32]; pchArg[0] = 0;
				Q_snprintf(pchArg, 32, "%i", m_iSkill_Level);
				GameBaseServer()->SendToolTip("#TOOLTIP_LEVELUP1", GAME_TIP_DEFAULT, this->entindex(), pchArg);
			}

			// Particle fanciness...
			DispatchParticleEffect("bb2_levelup_effect", PATTACH_ROOTBONE_FOLLOW, this, -1, true);
		}

		if (iXPToGive > 0)
			PerformLevelUp(iXPToGive);

		return true;
	}

	m_BB2Local.m_iSkill_XPCurrent = iXPToGive;
	return false;
}

#define PERCENT_TO_TAUNT 0.075f // X%

bool CHL2MP_Player::CanLevelUp(int iXP, CBaseEntity *pVictim)
{
	// Don't give XP to people who've not spawned!
	if (!HasFullySpawned())
		return false;

	if (!HL2MPRules()->CanUseSkills())
	{
		if ((pVictim != NULL) && IsAlive() && (iXP > 1))
		{
			if (TryTheLuck(PERCENT_TO_TAUNT))
				HL2MPRules()->EmitSoundToClient(this, "Taunt", BB2_SoundTypes::TYPE_PLAYER, GetSoundsetGender());
		}

		return false;
	}

	if ((pVictim != NULL) && m_bIsInfected && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE))
		return false;

	if (m_iSkill_Level < MAX_PLAYER_LEVEL)
	{
		int iExperienceToGive = iXP;
		if ((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) && HL2MPRules()->IsGamemodeFlagActive(GM_FLAG_ARENA_HARDMODE))
			iExperienceToGive = ceil(((float)iXP) * GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flArenaHardModeXPMultiplier);		

		if (pVictim && (iXP > 1))
			iExperienceToGive += (m_BB2Local.m_iPerkTeamBonus * GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iTeamBonusXPIncrease);

		// While this one goes for leveling... Because we also want to reset it or else we will actually only need 5 kills per level. 
		m_BB2Local.m_iSkill_XPCurrent += iExperienceToGive;
	}

	// Enable Perks: (% chance stuff)
	if ((pVictim != NULL) && IsAlive() && (iXP > 1))
	{
		if (TryTheLuck(PERCENT_TO_TAUNT))
			HL2MPRules()->EmitSoundToClient(this, "Taunt", BB2_SoundTypes::TYPE_PLAYER, GetSoundsetGender());

		double perc = 0.0;

		if (GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH) > 0)
		{
			perc = (GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH, TEAM_HUMANS) / PERCENT_BASE);
			if (TryTheLuck(perc))
			{
				float m_flEnemyHealth = pVictim->GetMaxHealth();
				float m_flHealthToTake = ((m_flEnemyHealth / 100.0f) * (GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH, TEAM_HUMANS)));
				TakeHealth(m_flHealthToTake, DMG_GENERIC);
				DispatchDamageText(pVictim, (int)m_flHealthToTake);
				PlaySkillSoundCue(SKILL_SOUND_CUE_LIFE_LEECH);
			}
		}

		if (GetSkillValue(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL) > 0)
		{
			perc = (GetSkillValue(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL, TEAM_HUMANS) / PERCENT_BASE);
			if (TryTheLuck(perc))
			{
				CBaseCombatWeapon *pMyWeapon = GetActiveWeapon();
				if (pMyWeapon)
				{
					if ((pMyWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL) && !pMyWeapon->IsMeleeWeapon() && pMyWeapon->UsesClipsForAmmo())
					{
						pMyWeapon->m_iClip = pMyWeapon->GetMaxClip();
						pMyWeapon->AffectedByPlayerSkill(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL);
						PlaySkillSoundCue(SKILL_SOUND_CUE_AMMO_REFILL);
					}
				}
			}
		}
	}

	// Disable leveling if we're at max level.
	if (m_iSkill_Level >= MAX_PLAYER_LEVEL)
		return false;

	// We try to level up:
	IGNORE_PREDICTION_SUPPRESSION;
	return PerformLevelUp(m_BB2Local.m_iSkill_XPCurrent);
}

bool CHL2MP_Player::ActivatePerk(int skill)
{
	if (HL2MPRules()->IsGameoverOrScoresVisible() || !IsHuman() || !IsAlive() || (GetSkillValue(skill) <= 0) || (GetPerkFlags() != 0) || (m_iNumPerkKills < GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->iKillsRequiredToPerk) || (!HL2MPRules()->CanUseSkills()))
		return false;

	m_iNumPerkKills = 0;
	m_BB2Local.m_bCanActivatePerk = false;
	SetHealth(GetMaxHealth()); // Give full HP on activation!

	switch (skill)
	{
	case PLAYER_SKILL_HUMAN_REALITY_PHASE:
	{
		SetPlayerSpeed(GetSkillCombination(GetSkillValue("Speed", PLAYER_SKILL_HUMAN_SPEED, TEAM_HUMANS), GetSkillValue(PLAYER_SKILL_HUMAN_REALITY_PHASE, TEAM_HUMANS)));
		m_BB2Local.m_flPerkTimer = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->flPerkTime;
		RefreshSpeed();
		AddPerkFlag(PERK_HUMAN_REALITYPHASE);
		SetCollisionGroup(COLLISION_GROUP_PLAYER_REALITY_PHASE);
		AchievementManager::WriteToAchievement(this, "ACH_SKILL_PERK_ROCKET");
		break;
	}

	case PLAYER_SKILL_HUMAN_GUNSLINGER:
	{
		m_BB2Local.m_flPerkTimer = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->flPerkTime;
		AddPerkFlag(PERK_HUMAN_GUNSLINGER);
		break;
	}

	case PLAYER_SKILL_HUMAN_BLOOD_RAGE:
	{
		m_BB2Local.m_flPerkTimer = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->flPerkTime;
		AddPerkFlag(PERK_HUMAN_BLOODRAGE);
		break;
	}
	}

	IGNORE_PREDICTION_SUPPRESSION;
	DispatchParticleEffect("bb2_perk_activate", PATTACH_ROOTBONE_FOLLOW, this, -1, true);
	return true;
}

bool CHL2MP_Player::EnterRageMode(bool bForce) // Zombie 'Perk' thing. 
{
	if (!IsZombie() || !IsAlive() || (GetPerkFlags() != 0) || HL2MPRules()->IsGameoverOrScoresVisible())
		return false;

	if (!bForce)
	{
		if (!HL2MPRules()->CanUseSkills() || (m_BB2Local.m_flZombieRageThresholdDamage < GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flRequiredDamageThreshold))
			return false;
	}

	m_BB2Local.m_bCanActivatePerk = false;
	m_BB2Local.m_flZombieRageThresholdDamage = 0.0f;
	m_flZombieRageTime = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flDuration;

	float flHealth = ceil(GetSkillValue("Health", PLAYER_SKILL_ZOMBIE_HEALTH, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flHealth);
	SetHealth((int)flHealth);
	SetMaxHealth((int)flHealth);
	SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_ZOMBIE_SPEED, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flSpeed);
	SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_ZOMBIE_LEAP, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flLeap);
	SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_ZOMBIE_JUMP, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flJump);
	SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_ZOMBIE_HEALTH_REGEN, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flHealthRegen);

	RefreshSpeed();

	AddPerkFlag(PERK_ZOMBIE_RAGE);
	IGNORE_PREDICTION_SUPPRESSION;
	DispatchParticleEffect("bb2_perk_activate", PATTACH_ROOTBONE_FOLLOW, this, -1, true);
	return true;
}

void CHL2MP_Player::LeaveRageMode(void)
{
	if (!IsZombie() || !IsAlive() || !IsPerkFlagActive(PERK_ZOMBIE_RAGE))
		return;

	m_BB2Local.m_bCanActivatePerk = false;
	m_BB2Local.m_flZombieRageThresholdDamage = 0.0f;
	m_flZombieRageTime = m_flZombieAttackTime = m_flZombieDamageThresholdDepletion = 0.0f;
	m_nPerkFlags &= ~PERK_ZOMBIE_RAGE;

	float flHealth = ceil(GetSkillValue("Health", PLAYER_SKILL_ZOMBIE_HEALTH, TEAM_DECEASED));
	SetMaxHealth((int)flHealth);
	SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_ZOMBIE_SPEED, TEAM_DECEASED));
	SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_ZOMBIE_LEAP, TEAM_DECEASED));
	SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_ZOMBIE_JUMP, TEAM_DECEASED));
	SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_ZOMBIE_HEALTH_REGEN, TEAM_DECEASED));

	if (GetMaxHealth() < GetHealth())
		SetHealth(GetMaxHealth());

	RefreshSpeed();
}

// If we're active in some external mode, ex rage mode / perk and use the skill tree then we might want to sum extra values to the skill values, 
// ex speed during rage mode for zombies, using the skill tree before would reset your speed to the base skill speed, not taking the 'rage' speed into account, compensate for it here...
float CHL2MP_Player::GetExtraPerkData(int type) 
{
	if (IsPerkFlagActive(PERK_ZOMBIE_RAGE))
	{
		switch (type)
		{
		case PLAYER_SKILL_ZOMBIE_HEALTH:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flHealth;

		case PLAYER_SKILL_ZOMBIE_SPEED:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flSpeed;

		case PLAYER_SKILL_ZOMBIE_JUMP:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flJump;

		case PLAYER_SKILL_ZOMBIE_LEAP:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flLeap;

		case PLAYER_SKILL_ZOMBIE_HEALTH_REGEN:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flHealthRegen;
		}
	}

	return 0.0f;
}

bool CHL2MP_Player::CanEnablePowerup(int powerupFlag, float duration)
{
	if (GetPerkFlags() || !HL2MPRules()->IsPowerupsAllowed())
		return false;

	const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(powerupFlag);
	if (!data)
		return false;

	AddPerkFlag(powerupFlag);
	m_BB2Local.m_flPerkTimer = gpGlobals->curtime + duration;

	int currHP = GetHealth();
	float healthIncrease = ((float)GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->iHealth / 100.0f) * data->flHealth;
	float speedIncrease = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flSpeed / 100.0f) * data->flSpeed;
	float jumpIncrease = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flJumpHeight / 100.0f) * data->flJumpHeight;
	float leapIncrease = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flLeapLength / 100.0f) * data->flLeapLength;
	float healthRegenRate = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flHealthRegenerationRate / 100.0f) * data->flHealthRegenerationRate;
	if (healthRegenRate <= 0)
		healthRegenRate = data->flHealthRegenerationRate;

	SetHealth(healthIncrease + currHP);
	SetMaxHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->iHealth + healthIncrease);
	SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flSpeed + speedIncrease);
	SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flLeapLength + leapIncrease);
	SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flJumpHeight + jumpIncrease);
	SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flHealthRegenerationRate + healthRegenRate);
	m_fTimeLastHurt = 0.0f;
	m_flHealthRegenWaitTime = 0.2f;

	if (GetHealth() > GetMaxHealth())
		SetHealth(GetMaxHealth());

	RefreshSpeed();
	GameBaseShared()->ComputePlayerWeight(this);

	return true;
}

void CHL2MP_Player::ResetPerksAndPowerups(void)
{
	m_flHealthRegenWaitTime = 4.0f;

	if (HL2MPRules()->CanUseSkills())
	{
		SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_HUMAN_SPEED, TEAM_HUMANS));
	}
	else
	{
		float currHealth = (float)GetHealth();
		float currMaxHealth = (float)GetMaxHealth();

		m_fTimeLastHurt = gpGlobals->curtime;
		SetMaxHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->iHealth);
		SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flSpeed);
		SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flLeapLength);
		SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flJumpHeight);
		SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->flHealthRegenerationRate);

		if (GetHealth() > GetMaxHealth())
			SetHealth(GetMaxHealth());

		if (IsPerkFlagActive(PERK_POWERUP_PAINKILLER))
		{
			float percentLost = (currHealth / currMaxHealth);
			float newHealth = ((float)GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->iHealth * percentLost);

			if (newHealth <= 0.0f)
				SetHealth(1);
			else
				SetHealth((int)newHealth);
		}		
	}

	if (!m_bIsInfected)
		SetCollisionGroup(COLLISION_GROUP_PLAYER);
}

void CHL2MP_Player::RemovePowerups(void)
{
	if (!HL2MPRules()->IsPowerupsAllowed() || !GetPerkFlags())
		return;

	float durationLeft = (m_BB2Local.m_flPerkTimer - gpGlobals->curtime);
	if (durationLeft <= 0)
		return;

	const char *powerup = NULL;
	if (IsPerkFlagActive(PERK_POWERUP_CRITICAL))
		powerup = "Critical";
	else if (IsPerkFlagActive(PERK_POWERUP_CHEETAH))
		powerup = "Cheetah";
	else if (IsPerkFlagActive(PERK_POWERUP_NANITES))
		powerup = "Nanites";
	else if (IsPerkFlagActive(PERK_POWERUP_PAINKILLER))
		powerup = "Painkiller";
	else if (IsPerkFlagActive(PERK_POWERUP_PREDATOR))
		powerup = "Predator";

	if (powerup == NULL)
		return;

	CItem *pPowerupEnt = (CItem*)CreateEntityByName("item_powerup");
	if (pPowerupEnt)
	{
		Vector vecAbsOrigin = GetAbsOrigin();
		Vector vecStartPos = vecAbsOrigin + Vector(0, 0, 30);
		Vector vecEndPos = vecAbsOrigin;
		Vector vecDir = (vecEndPos - vecStartPos);
		VectorNormalize(vecDir);

		trace_t tr;
		CTraceFilterNoNPCsOrPlayer trFilter(this, GetCollisionGroup());
		UTIL_TraceHull(vecStartPos, vecEndPos + (vecDir * MAX_TRACE_LENGTH), GetPlayerMins(), GetPlayerMaxs(), MASK_SHOT_HULL, &trFilter, &tr);

		pPowerupEnt->SetParam(powerup);
		pPowerupEnt->Spawn();
		pPowerupEnt->SetParam(durationLeft);

		Vector endPoint = tr.endpos;
		const model_t *pModel = modelinfo->GetModel(pPowerupEnt->GetModelIndex());
		if (pModel)
		{
			Vector mins, maxs;
			modelinfo->GetModelBounds(pModel, mins, maxs);
			endPoint.z += maxs.z;
		}
		pPowerupEnt->SetLocalOrigin(endPoint);
	}
}

void CHL2MP_Player::ChangeTeam(int iTeam, bool bInfection)
{
	int iOldTeam = GetTeamNumber();
	bool bKill = false;
	if (iTeam != iOldTeam && (iOldTeam >= TEAM_HUMANS))
		bKill = true;

	// When we leave spec mode we need to refresh this value to make sure we don't get kicked if we didn't move while spectating.
	if ((iOldTeam == TEAM_SPECTATOR) && HasLoadedStats() && (iTeam >= TEAM_HUMANS) && (m_flLastTimeRanCommand > 0.0f))
		m_flLastTimeRanCommand = gpGlobals->curtime;

	DropAllWeapons();

	ExecuteClientEffect(PLAYER_EFFECT_ZOMBIE_FLASHLIGHT, 0);
	FlashlightTurnOff();

	BaseClass::ChangeTeam(iTeam);

	SetPlayerModel();

	bool bResetFade = false;
	if (iTeam == TEAM_SPECTATOR)
	{
		RemoveAllItems();
		State_Transition(STATE_OBSERVER_MODE);
		bResetFade = true;
	}

	if ((bKill == true) && !bInfection)
		CommitSuicide();

	if (bResetFade)
	{
		color32 nothing = { 0, 0, 0, 255 };
		UTIL_ScreenFade(this, nothing, 0, 0, FFADE_IN | FFADE_PURGE);
	}
}

bool CHL2MP_Player::HandleCommand_JoinTeam(int team, bool bInfection)
{
	if (!GetGlobalTeam(team) || team == 0)
	{
		Warning("HandleCommand_JoinTeam( %d ) - invalid team index.\nFORCING SPEC MODE\n", team);
		HandleCommand_JoinTeam(TEAM_SPECTATOR);
		return false;
	}

	if (team == TEAM_SPECTATOR)
	{
		if (!bInfection)
		{
			if (GetTeamNumber() != TEAM_UNASSIGNED && !IsDead())
			{
				m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work

				CommitSuicide();

				// add 1 to frags to balance out the 1 subtracted for killing yourself
				IncrementFragCount(1);
			}
		}

		ChangeTeam(TEAM_SPECTATOR, bInfection);

		return true;
	}
	else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}

	// Switch their actual team...
	ChangeTeam(team, bInfection);

	return true;
}

bool CHL2MP_Player::ClientCommand(const CCommand &args)
{
	if (FStrEq(args[0], "spectate"))
	{
		if (sv_cheats->GetBool())
		{
			if (ShouldRunRateLimitedCommand(args))
			{
				// instantly join spectators
				HandleCommand_JoinTeam(TEAM_SPECTATOR);
			}
		}

		return true;
	}

	else if (FStrEq(args[0], "deploy_human"))
	{
		bool bIsTutorial = GameBaseServer()->IsTutorialModeEnabled();

		if (ShouldRunRateLimitedCommand(args) && (sv_cheats->GetBool() || bIsTutorial))
		{
			m_bWantsToDeployAsHuman = true;
			HandleCommand_JoinTeam(TEAM_HUMANS, bIsTutorial);
			ForceRespawn();
		}

		return true;
	}

	else if (FStrEq(args[0], "deploy_zombie"))
	{
		bool bIsTutorial = GameBaseServer()->IsTutorialModeEnabled();

		if (ShouldRunRateLimitedCommand(args) && (sv_cheats->GetBool() || bIsTutorial))
		{
			m_bWantsToDeployAsHuman = true;
			HandleCommand_JoinTeam(TEAM_DECEASED, bIsTutorial);
			ForceRespawn();
		}

		return true;
	}

	// BB2 Stuff

	else if (FStrEq(args[0], "player_vote_yes"))
	{
		if (HL2MPRules() && ShouldRunRateLimitedCommand(args))
			HL2MPRules()->PlayerVote(this, true);
		return true;
	}

	else if (FStrEq(args[0], "player_vote_no"))
	{
		if (HL2MPRules() && ShouldRunRateLimitedCommand(args))
			HL2MPRules()->PlayerVote(this, false);
		return true;
	}

	else if (FStrEq(args[0], "joingame"))
	{
		if (m_bTriedToJoinGame)
			return true;

		m_bTriedToJoinGame = true; // Only run this cmd once!

		// BB2 Warn : Hack - Give the zombie plrs snacks...		
		m_BB2Local.m_iZombieCredits = GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->iDefaultZombieCredits;

		if (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)
		{
			ShowViewPortPanel("team", true);
			return true;
		}

		HandleFirstTimeConnection();
		return true;
	}

	else if (FStrEq(args[0], "giveall"))
	{
		if (ShouldRunRateLimitedCommand(args))
			AchievementManager::WriteToAchievement(this, "ACH_SECRET_GIVEALL");
		return true;
	}

	else if (FStrEq(args[0], "jointeam"))
	{
		if ((args.ArgC() != 2) || (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION) || !ShouldRunRateLimitedCommand(args))
			return true;

		int iTeam = (atoi(args[1]) == 1) ? TEAM_DECEASED : TEAM_HUMANS;
		if (iTeam == m_iSelectedTeam) // You're already on this team...
		{
			GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_CHANGE_FAIL1", GAME_TIP_DEFAULT, entindex());
			ShowViewPortPanel("team", false);
			return true;
		}

		if (m_BB2Local.m_flPlayerRespawnTime > gpGlobals->curtime)
		{
			float flTimeToWait = m_BB2Local.m_flPlayerRespawnTime - gpGlobals->curtime;

			char pszTimer[16];
			Q_snprintf(pszTimer, 16, "%i", (int)flTimeToWait);
			GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_CHANGE_FAIL2", GAME_TIP_DEFAULT, entindex(), pszTimer);
			ShowViewPortPanel("team", false);
			return true;
		}

		int teamOverride = HL2MPRules()->GetNewTeam(iTeam);
		if (teamOverride != iTeam)
		{
			GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_CHANGE_FAIL3", GAME_TIP_DEFAULT, entindex());
			if (m_iSelectedTeam != TEAM_UNASSIGNED)
				ShowViewPortPanel("team", false);

			return true;
		}

		ShowViewPortPanel("team", false);
		SetSelectedTeam(iTeam);
		m_bWantsToDeployAsHuman = true;		

		if (m_bHasReadProfileData || m_bHasFullySpawned || m_bHasTriedToLoadStats)
		{
			HandleCommand_JoinTeam(iTeam);
			return true;
		}

		HandleFirstTimeConnection();
		return true;
	}

	else if (FStrEq(args[0], "bb_voice_command"))
	{
		if ((args.ArgC() != 2) || !IsHuman() || !IsAlive() || !HL2MPRules()->IsTeamplay() || !ShouldRunRateLimitedCommand(args))
			return true;

		if ((LastTimePlayerTalked() + VOICE_COMMAND_EMIT_DELAY) < gpGlobals->curtime)
		{
			NotePlayerTalked();

			int iCommand = atoi(args[1]);
			if (iCommand >= 0 && iCommand < VOICE_COMMAND_EXIT)
			{
				char pszVoiceMessage[128];
				Q_strncpy(pszVoiceMessage, GetVoiceCommandChatMessage(iCommand), 128);
				Host_Say(edict(), pszVoiceMessage, true, CHAT_CMD_VOICE);
				HL2MPRules()->EmitSoundToClient(this, GetVoiceCommandString(iCommand), BB2_SoundTypes::TYPE_PLAYER, GetSoundsetGender());
				OnVoiceCommand(iCommand);
			}
		}

		return true;
	}

	else if (FStrEq(args[0], "bb_inventory_item_drop"))
	{
		if ((args.ArgC() != 3) || !ShouldRunRateLimitedCommand(args))
			return true;

		uint iID = (uint)atol(args[1]);
		bool bMapItem = (atoi(args[2]) >= 1);

		GameBaseShared()->RemoveInventoryItem(entindex(), GetAbsOrigin(), (bMapItem ? 1 : 0), iID);
		m_flLastTimeRanCommand = gpGlobals->curtime;
		return true;
	}

	else if (FStrEq(args[0], "bb_inventory_item_use"))
	{
		if ((args.ArgC() != 3) || !ShouldRunRateLimitedCommand(args))
			return true;

		uint iID = (uint)atol(args[1]);
		bool bMapItem = (atoi(args[2]) >= 1);

		GameBaseShared()->UseInventoryItem(entindex(), iID, bMapItem);
		m_flLastTimeRanCommand = gpGlobals->curtime;
		return true;
	}

	else if (FStrEq(args[0], "bb2_keypad_unlock"))
	{
		if ((args.ArgC() != 3) || !ShouldRunRateLimitedCommand(args))
			return true;

		CBaseKeyPadEntity::UseKeyPadCode(this, UTIL_EntityByIndex(atoi(args[1])), args[2]);
		m_flLastTimeRanCommand = gpGlobals->curtime;
		return true;
	}

	else if (FStrEq(args[0], "bb2_skill_progress_human"))
	{
		if ((args.ArgC() != 3) || !HL2MPRules()->CanUseSkills() || !ShouldRunRateLimitedCommand(args))
			return true;

		bool m_bCanApplyChanges = ((GetTeamNumber() == TEAM_HUMANS) && IsAlive());

		int iSkillType = atoi(args[1]);
		bool bShouldAdd = (atoi(args[2]) >= 1);
		int iSkillValue = GetSkillValue(iSkillType);

		if ((bShouldAdd && ((m_BB2Local.m_iSkill_Talents <= 0) || (iSkillValue >= 10))) || (!bShouldAdd && (iSkillValue <= 0)))
			return true;

		// Other Special checks
		if ((iSkillType == PLAYER_SKILL_HUMAN_HEALTH) && (m_iHealth < 50) && m_bCanApplyChanges)
			return true;

		if (bShouldAdd)
		{
			m_BB2Local.m_iSkill_Talents--;
			SetSkillValue(iSkillType);
		}
		else
		{
			m_BB2Local.m_iSkill_Talents++;
			SetSkillValue(iSkillType, true);
		}

		if (m_bCanApplyChanges)
		{
			switch (iSkillType)
			{

			case PLAYER_SKILL_HUMAN_HEALTH:
			{
				int value = (bShouldAdd ? ceil(GetSkillValue(iSkillType, TEAM_HUMANS, true)) : floor(GetSkillValue(iSkillType, TEAM_HUMANS, true)));
				float flHealth = ceil(GetSkillValue("Health", PLAYER_SKILL_HUMAN_HEALTH, TEAM_HUMANS));
				SetMaxHealth((int)flHealth);
				m_iHealth += bShouldAdd ? value : -value;
				m_iHealth = clamp(m_iHealth, 1, GetMaxHealth());
				break;
			}
			case PLAYER_SKILL_HUMAN_SPEED:
			{
				SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_HUMAN_SPEED, TEAM_HUMANS));
				RefreshSpeed();
				break;
			}
			case PLAYER_SKILL_HUMAN_ACROBATICS:
			{
				SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_HUMAN_ACROBATICS, TEAM_HUMANS, PLAYER_SKILL_HUMAN_LEAP));
				SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_HUMAN_ACROBATICS, TEAM_HUMANS, PLAYER_SKILL_HUMAN_JUMP));
				break;
			}
			case PLAYER_SKILL_HUMAN_LIGHTWEIGHT:
			{
				GameBaseShared()->ComputePlayerWeight(this);
				break;
			}
			case PLAYER_SKILL_HUMAN_HEALTHREGEN:
			{
				SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_HUMAN_HEALTHREGEN, TEAM_HUMANS));
				break;
			}

			}
		}

		m_flLastTimeRanCommand = gpGlobals->curtime;
		return true;
	}

	else if (FStrEq(args[0], "bb2_skill_progress_zombie"))
	{
		if ((args.ArgC() != 3) || !HL2MPRules()->CanUseSkills() || !ShouldRunRateLimitedCommand(args))
			return true;

		bool m_bCanApplyChanges = ((GetTeamNumber() == TEAM_DECEASED) && IsAlive());

		int iSkillType = atoi(args[1]);
		bool bShouldAdd = (atoi(args[2]) >= 1);		
		int iSkillValue = GetSkillValue(iSkillType);

		if ((bShouldAdd && ((m_BB2Local.m_iZombieCredits <= 0) || (iSkillValue >= 10))) || (!bShouldAdd && (iSkillValue <= 0)))
			return true;

		// Other Special checks
		if ((iSkillType == PLAYER_SKILL_ZOMBIE_HEALTH) && (m_iHealth < 50) && m_bCanApplyChanges)
			return true;

		if (bShouldAdd)
		{
			m_BB2Local.m_iZombieCredits--;
			SetSkillValue(iSkillType);
		}
		else
		{
			m_BB2Local.m_iZombieCredits++;
			SetSkillValue(iSkillType, true);
		}

		if (m_bCanApplyChanges)
		{
			float extraData = GetExtraPerkData(iSkillType);

			switch (iSkillType)
			{

			case PLAYER_SKILL_ZOMBIE_HEALTH:
			{
				int value = (bShouldAdd ? ceil(GetSkillValue(iSkillType, TEAM_DECEASED, true)) : floor(GetSkillValue(iSkillType, TEAM_DECEASED, true)));
				float flHealth = ceil(GetSkillValue("Health", PLAYER_SKILL_ZOMBIE_HEALTH, TEAM_DECEASED) + extraData);
				SetMaxHealth((int)flHealth);
				m_iHealth += bShouldAdd ? value : -value;
				m_iHealth = clamp(m_iHealth, 1, GetMaxHealth());
				break;
			}
			case PLAYER_SKILL_ZOMBIE_SPEED:
			{
				SetPlayerSpeed(GetSkillValue("Speed", iSkillType, TEAM_DECEASED) + extraData);
				RefreshSpeed();
				break;
			}
			case PLAYER_SKILL_ZOMBIE_JUMP:
			{
				SetJumpHeight(GetSkillValue("Jump", iSkillType, TEAM_DECEASED) + extraData);
				break;
			}
			case PLAYER_SKILL_ZOMBIE_LEAP:
			{
				SetLeapLength(GetSkillValue("Leap", iSkillType, TEAM_DECEASED) + extraData);
				break;
			}
			case PLAYER_SKILL_ZOMBIE_HEALTH_REGEN:
			{
				SetHealthRegenAmount(GetSkillValue("HealthRegen", iSkillType, TEAM_DECEASED) + extraData);
				break;
			}

			}
		}

		m_flLastTimeRanCommand = gpGlobals->curtime;
		return true;
	}

	return BaseClass::ClientCommand(args);
}

void CHL2MP_Player::CheatImpulseCommands(int iImpulse)
{
	switch (iImpulse)
	{
	case 100:
	{
		if (IsHuman())
		{
			if (FlashlightIsOn())
				FlashlightTurnOff();
			else
				FlashlightTurnOn();
		}
		else
		{
			if (IsAlive() && (GetTeamNumber() == TEAM_DECEASED) && m_flZombieVisionLockTime < gpGlobals->curtime)
			{
				ExecuteClientEffect(PLAYER_EFFECT_ZOMBIE_FLASHLIGHT, !m_bZombieVisionState);

				if (!GameBaseServer()->IsTutorialModeEnabled())
					HL2MPRules()->EmitSoundToClient(this, "Vision", BB2_SoundTypes::TYPE_DECEASED, GetSoundsetGender());
			}
		}

		break;
	}

	default:
		BaseClass::CheatImpulseCommands(iImpulse);
	}
}

void CHL2MP_Player::OnVoiceCommand(int cmd)
{
	if (cmd == VOICE_COMMAND_OUTOFAMMO)
	{
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if (pWeapon && !pWeapon->IsMeleeWeapon() && (pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL))
		{
			m_flAmmoRequestTime = gpGlobals->curtime;
			m_iAmmoRequestID = pWeapon->GetAmmoTypeID();
		}
	}
}

bool CHL2MP_Player::ShouldRunRateLimitedCommand(const CCommand &args)
{
	int i = m_RateLimitLastCommandTimes.Find(args[0]);
	if (i == m_RateLimitLastCommandTimes.InvalidIndex())
	{
		m_RateLimitLastCommandTimes.Insert(args[0], gpGlobals->curtime);
		return true;
	}
	else if ((gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HL2MP_COMMAND_MAX_RATE)
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

void CHL2MP_Player::CreateViewModel(void)
{
	if (GetViewModel())
		return;

	CPredictedViewModel *vm = (CPredictedViewModel *)CreateEntityByName("predicted_viewmodel");
	if (vm)
	{
		vm->SetAbsOrigin(GetAbsOrigin());
		vm->SetOwner(this);
		DispatchSpawn(vm);
		vm->FollowEntity(this, false);
		m_hViewModel.Set(vm);
	}
}

bool CHL2MP_Player::BecomeRagdollOnClient(const Vector &force)
{
	return true;
}

void CHL2MP_Player::CreateRagdollEntity(void)
{
	Vector vecNewVelocity = g_vecAttackDir * -1;
	vecNewVelocity.x += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity.y += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity.z += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity *= 900.0f;

	TE_PlayerGibRagdoll(entindex(), GetGibFlags(), CLIENT_RAGDOLL, WorldSpaceCenter(), GetAbsOrigin(), vecNewVelocity, GetAbsAngles());
}

int CHL2MP_Player::AllowEntityToBeGibbed(void)
{
	if (IsObserver() || (GetTeamNumber() <= TEAM_SPECTATOR) || (m_iHealth > 0))
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2MP_Player::FlashlightIsOn(void)
{
	return IsEffectActive(EF_DIMLIGHT);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOn(void)
{
	if (IsAlive() && (GetTeamNumber() == TEAM_HUMANS) && GetActiveWeapon())
	{
		AddEffects(EF_DIMLIGHT);
		EmitSound("HL2Player.FlashlightOn");
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOff(void)
{
	RemoveEffects(EF_DIMLIGHT);
	m_bEnableFlashlighOnSwitch = false;
	if (IsAlive())
	{
		EmitSound("HL2Player.FlashlightOff");
	}
}

void CHL2MP_Player::RemoveSpawnProtection(void)
{
	if (m_flSpawnProtection <= 0)
		return;

	m_flSpawnProtection = 0.0f;
	RemoveMaterialOverlayFlag(MAT_OVERLAY_SPAWNPROTECTION);
}

void CHL2MP_Player::Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity)
{
	BaseClass::Weapon_Drop(pWeapon, pvecTarget, pVelocity);
	GameBaseShared()->ComputePlayerWeight(this);
}

void CHL2MP_Player::Event_Killed(const CTakeDamageInfo &info)
{
	// Drop all our items:
	GameBaseShared()->RemoveInventoryItem(entindex(), GetAbsOrigin());

	ResetSlideVars();

	// Drop our weps, give snacks to the living...:
	DropAllWeapons(); 

	m_iTotalDeaths++;
	m_iRoundDeaths++;

	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce(m_vecTotalBulletForce);

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	// Ragdoll only if not spectating! BB2
	if (GetTeamNumber() >= TEAM_HUMANS)
	{
		CreateRagdollEntity();
		RemovePowerups();
	}

	BaseClass::Event_Killed(subinfo);

	CBaseEntity *pAttacker = info.GetAttacker();
	if (pAttacker)
	{
		if ((HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE) && !GameBaseServer()->IsStoryMode())
		{
			if (IsZombie())
			{
				float flCreditsToLose = ceil((GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flZombieCreditsPercentToLose / 100.0f) * ((float)m_BB2Local.m_iZombieCredits));
				if (!IsPerkFlagActive(PERK_ZOMBIE_RAGE) && !m_BB2Local.m_bCanRespawnAsHuman && (flCreditsToLose > 0.0f))
				{
					m_BB2Local.m_iZombieCredits -= MIN(((int)flCreditsToLose), m_BB2Local.m_iZombieCredits.Get());

					char pchArg1[16];
					Q_snprintf(pchArg1, 16, "%i", ((int)flCreditsToLose));
					GameBaseServer()->SendToolTip("#TOOLTIP_ZOMBIE_DEATH", GAME_TIP_DEFAULT, this->entindex(), pchArg1);
				}

				if (pAttacker != this)
				{
					m_iZombDeaths++;
					CheckCanRespawnAsHuman();
				}
			}

			if (!m_bIsInfected && IsHuman())
			{
				if (pAttacker->IsZombie() && bb2_classic_zombie_noteamchange.GetBool())
					m_bWantsToDeployAsHuman = true;
				else if (pAttacker->IsHuman(true) && pAttacker->IsNPC())
					m_bWantsToDeployAsHuman = true;
				else if ((pAttacker->IsHumanBoss() || pAttacker->IsZombieBoss()) && pAttacker->IsNPC())
					m_bWantsToDeployAsHuman = true;
			}
		}
		else if (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)
		{
			CTeam *pEnemyTeam = GetGlobalTeam(pAttacker->GetTeamNumber());
			if ((pAttacker != this) && pEnemyTeam)
			{
				if (IsHuman() && pAttacker->IsZombie())
					pEnemyTeam->AddScore(bb2_elimination_score_zombies.GetInt(), this);
				else if (IsZombie() && pAttacker->IsHuman())
					pEnemyTeam->AddScore(bb2_elimination_score_humans.GetInt(), this);
			}
		}

		if (pAttacker->IsPlayer() && (pAttacker != this))
			AchievementManager::WriteToStatPvP(ToHL2MPPlayer(pAttacker), "PVP_KILLS");
	}

	ExecuteClientEffect(PLAYER_EFFECT_ZOMBIE_FLASHLIGHT, 0);
	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects(EF_NODRAW);	// still draw player body

	color32 darkred = { 53, 0, 0, 90 };
	UTIL_ScreenFade(this, darkred, 1.0f, 5.0f, FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT);

	AchievementManager::WriteToStat(this, "BBX_ST_DEATHS");
	AchievementManager::WriteToStatPvP(this, "PVP_DEATHS");

	ExecuteClientEffect(PLAYER_EFFECT_DEATH, 1);

	m_achievStats->OnDeath(subinfo);
}

int CHL2MP_Player::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	// Spawn Protection:
	if (gpGlobals->curtime < m_flSpawnProtection)
		return 0;

	CTakeDamageInfo damageCopy = inputInfo;
	CBaseEntity *pAttacker = damageCopy.GetAttacker();
	if (pAttacker)
	{
		float flDamageScale = GameBaseServer()->GetDamageScaleForEntity(pAttacker, this, damageCopy.GetDamageType(), damageCopy.GetDamageCustom());
		damageCopy.ScaleDamage(flDamageScale);
	}

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pAttacker);
	if (pPlayer && pPlayer->IsHuman() && IsZombie() && (CTeam::GetActivePerk(GetTeamNumber()) == TEAM_DECEASED_PERK_INCREASED_STRENGTH))
	{
		CBaseCombatWeapon *pWeaponUsed = pPlayer->GetActiveWeapon();
		if (pWeaponUsed && !pWeaponUsed->IsMeleeWeapon())
		{
			float damageTaken = damageCopy.GetDamage();
			float health = (GetMaxHealth() - GetHealth());
			TakeHealth(MIN(damageTaken, health), DMG_GENERIC);
			PlaySkillSoundCue(SKILL_SOUND_CUE_LIFE_LEECH);
			return 0;
		}
	}

	m_vecTotalBulletForce += damageCopy.GetDamageForce();

	if (pAttacker != this)
		m_achievStats->OnTookDamage(damageCopy);

	return BaseClass::OnTakeDamage(damageCopy);
}

void CHL2MP_Player::DeathSound(const CTakeDamageInfo &info)
{
	const char *soundname = "Death";
	if (IsPlayerUnderwater())
		soundname = "DrownDying";

	HL2MPRules()->EmitSoundToClient(this, soundname, GetSoundType(), GetSoundsetGender());
}

void CHL2MP_Player::MeleeSwingSound(bool bBash)
{
	const char *soundname = "MeleeSwing";
	if (bBash)
		soundname = "BashMeleeSwing";

	HL2MPRules()->EmitSoundToClient(this, soundname, GetSoundType(), GetSoundsetGender());
}

void CHL2MP_Player::HandlePainSound(int iMajor, int iDamageTypeBits)
{
	HL2MPRules()->EmitSoundToClient(this, "ImpactPain", GetSoundType(), GetSoundsetGender());
}

CBaseEntity* CHL2MP_Player::EntSelectSpawnPoint(void)
{
	CUtlVector<CBaseEntity*> pSpawnPointsValid;
	CUtlVector<CBaseEntity*> pSpawnPointsInValid;
	CUtlVector<CBaseEntity*> pSpawnPointsLastResort;

	CBaseEntity *pFinalSpawnPoint = NULL;

	bool bUseCameraSpawn = true;
	const char *pSpawnpointName = "info_start_camera";

	if (GetTeamNumber() == TEAM_HUMANS)
	{
		bUseCameraSpawn = false;
		pSpawnpointName = "info_player_human";
	}
	else if (GetTeamNumber() == TEAM_DECEASED)
	{
		bUseCameraSpawn = false;
		pSpawnpointName = "info_player_zombie";
	}

	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, pSpawnpointName);
	while (pEntity)
	{
		if (bUseCameraSpawn == false)
		{
			CBaseSpawnPoint *pPoint = dynamic_cast<CBaseSpawnPoint*> (pEntity);
			if (pPoint)
			{
				if (pPoint->IsEnabled())
				{
					if (g_pGameRules->IsSpawnPointValid(pEntity, this))
						pSpawnPointsValid.AddToTail(pEntity);
					else
						pSpawnPointsInValid.AddToTail(pEntity);
				}

				if (pPoint->IsMaster())
					pSpawnPointsLastResort.AddToTail(pEntity);
			}
		}
		else
			pSpawnPointsValid.AddToTail(pEntity);

		pEntity = gEntList.FindEntityByClassname(pEntity, pSpawnpointName);
	}

	int iValidSpawns = pSpawnPointsValid.Count();
	int iInvalidSpawns = pSpawnPointsInValid.Count();
	int iLastResortSpawns = pSpawnPointsLastResort.Count();

	if ((iValidSpawns <= 0) && (iInvalidSpawns <= 0))
	{
		if (iLastResortSpawns > 0)
			pFinalSpawnPoint = pSpawnPointsLastResort[random->RandomInt(0, (iLastResortSpawns - 1))];
		else
		{
			Warning("No available spawn point!\nForcing (0,0,0)!\n");
			pFinalSpawnPoint = GetWorldEntity();
		}
	}
	else if (iValidSpawns > 0)
		pFinalSpawnPoint = pSpawnPointsValid[random->RandomInt(0, (iValidSpawns - 1))];
	else
		pFinalSpawnPoint = pSpawnPointsInValid[random->RandomInt(0, (iInvalidSpawns - 1))];

	m_flSpawnProtection = gpGlobals->curtime + bb2_spawn_protection.GetFloat();
	m_flZombieVisionLockTime = gpGlobals->curtime + 0.5f;

	pSpawnPointsValid.RemoveAll();
	pSpawnPointsInValid.RemoveAll();
	pSpawnPointsLastResort.RemoveAll();

	// Allow dyn. respawns in story mode.
	if (GameBaseServer()->IsStoryMode() && HL2MPRules()->m_bRoundStarted && !HL2MPRules()->IsGameoverOrScoresVisible()
		&& (GetTeamNumber() == TEAM_HUMANS) && pFinalSpawnPoint && bb2_story_dynamic_respawn.GetBool())
	{
		CBasePlayer *pDistantPlayer = UTIL_GetMostDistantPlayer(this, pFinalSpawnPoint->GetLocalOrigin());
		if (pDistantPlayer)
			return pDistantPlayer;
	}

	return pFinalSpawnPoint;
}

void CHL2MP_Player::Reset()
{
	//ResetDeathCount();
	//ResetFragCount();

	m_iZombKills = 0;
	m_iZombDeaths = 0;

	m_iRoundScore = 0;
	m_iRoundDeaths = 0;

	m_BB2Local.m_bHasPlayerEscaped = false;
	m_BB2Local.m_bCanRespawnAsHuman = false;
}

void CHL2MP_Player::ResetSlideVars()
{
	m_BB2Local.m_bSliding = false;
	m_BB2Local.m_bStandToSlide = false;
	m_BB2Local.m_flSlideTime = 0.0f;
	m_BB2Local.m_flSlideKickCooldownEnd = 0.0f;
	m_BB2Local.m_flSlideKickCooldownStart = 0.0f;
	m_bIsInSlide = false;
}

void CHL2MP_Player::CheckCanRespawnAsHuman()
{
	if ((HL2MPRules() && !HL2MPRules()->CanUseSkills()) || GameBaseServer()->IsTutorialModeEnabled())
		return;

	int killsRequired = bb2_zombie_kills_required.GetInt();
	int deathMercy = bb2_allow_mercy.GetInt();

	if ((killsRequired && (m_iZombKills >= killsRequired)) || (deathMercy && (m_iZombDeaths >= deathMercy)))
		m_BB2Local.m_bCanRespawnAsHuman = true;
}

void CHL2MP_Player::CheckCanRage()
{
	if ((HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)) || !GameBaseShared()->GetSharedGameDetails() || GetPerkFlags())
		return;

	if (m_BB2Local.m_flZombieRageThresholdDamage >= GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flRequiredDamageThreshold)
		EnterRageMode(true);
}

void CHL2MP_Player::CheckChatText(char *p, int bufsize)
{
	//Look for escape sequences and replace

	char *buf = new char[bufsize];
	int pos = 0;

	// Parse say text for escape sequences
	for (char *pSrc = p; pSrc != NULL && *pSrc != 0 && pos < bufsize - 1; pSrc++)
	{
		// copy each char across
		buf[pos] = *pSrc;
		pos++;
	}

	buf[pos] = '\0';

	// copy buf back into p
	Q_strncpy(p, buf, bufsize);

	delete[] buf;
}

void CHL2MP_Player::State_Transition(HL2MPPlayerState newState)
{
	State_Leave();
	State_Enter(newState);
}


void CHL2MP_Player::State_Enter(HL2MPPlayerState newState)
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo(newState);

	// Initialize the new state.
	if (m_pCurStateInfo && m_pCurStateInfo->pfnEnterState)
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CHL2MP_Player::State_Leave()
{
	if (m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState)
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}

void CHL2MP_Player::State_PreThink()
{
	if (m_pCurStateInfo && m_pCurStateInfo->pfnPreThink)
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}

CHL2MPPlayerStateInfo *CHL2MP_Player::State_LookupInfo(HL2MPPlayerState state)
{
	// This table MUST match the 
	static CHL2MPPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE, "STATE_ACTIVE", &CHL2MP_Player::State_Enter_ACTIVE, NULL, &CHL2MP_Player::State_PreThink_ACTIVE },
		{ STATE_OBSERVER_MODE, "STATE_OBSERVER_MODE", &CHL2MP_Player::State_Enter_OBSERVER_MODE, NULL, &CHL2MP_Player::State_PreThink_OBSERVER_MODE }
	};

	for (int i = 0; i < ARRAYSIZE(playerStateInfos); i++)
	{
		if (playerStateInfos[i].m_iPlayerState == state)
			return &playerStateInfos[i];
	}

	return NULL;
}

bool CHL2MP_Player::StartObserverMode(int mode)
{
	//we only want to go into observer mode if the player asked to, not on a death timeout
	if (m_bEnterObserver)
	{
		ResetSlideVars();
		VPhysicsDestroyObject();
		return BaseClass::StartObserverMode(mode);
	}

	return false;
}

void CHL2MP_Player::StopObserverMode()
{
	m_bEnterObserver = false;
	BaseClass::StopObserverMode();
}

void CHL2MP_Player::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;
	if (IsNetClient())
	{
		const char *pIdealMode = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "cl_spec_mode");
		if (pIdealMode)
		{
			observerMode = atoi(pIdealMode);
			if (observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING)
			{
				observerMode = m_iObserverLastMode;
			}
		}
	}
	m_bEnterObserver = true;
	StartObserverMode(observerMode);
}

void CHL2MP_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert(m_takedamage == DAMAGE_NO);
	Assert(IsSolidFlagSet(FSOLID_NOT_SOLID));
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert(m_lifeState == LIFE_DEAD);
	Assert(pl.deadflag);
}


void CHL2MP_Player::State_Enter_ACTIVE()
{
	SetMoveType(MOVETYPE_WALK);

	// md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
	// is false, then they'll be spectating but blocking live players from moving.
	// RemoveSolidFlags( FSOLID_NOT_SOLID );

	m_Local.m_iHideHUD = 0;
}

void CHL2MP_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death
}

void CHL2MP_Player::CheckShouldEnableFlashlightOnSwitch(void)
{
	if (!IsAlive())
		return;

	m_bEnableFlashlighOnSwitch = (FlashlightIsOn() >= 1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2MP_Player::CanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
	// can always hear the console unless we're ignoring all chat
	if (!pPlayer)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: multiplayer does not do autoaiming.
//-----------------------------------------------------------------------------
Vector CHL2MP_Player::GetAutoaimVector(void)
{
	Vector forward;
	AngleVectors(EyeAngles() + m_Local.m_vecPunchAngle, &forward);
	return forward;
}

// BB2 SKILL SETUP, THIS IS HOW WE FIGURE OUT WHAT SPEED OFFSET(S) WE SHOULD HAVE AT ALL TIMES!!!

float CHL2MP_Player::GetTeamPerkValue(float flOriginalValue)
{
	switch (CTeam::GetActivePerk(GetTeamNumber()))
	{
	case TEAM_DECEASED_PERK_INCREASED_STRENGTH:
		return (flOriginalValue * 2.0f);
	}
	return 0.0f;
}

void CHL2MP_Player::SetSkillValue(int skillType, bool bDecrement)
{
	if ((skillType < 0) || (skillType >= PLAYER_SKILL_END))
		return;

	int iCurrentValue = GetSkillValue(skillType);

	if (bDecrement)
		iCurrentValue--;
	else
		iCurrentValue++;

	iCurrentValue = clamp(iCurrentValue, 0, 10);
	m_BB2Local.m_iPlayerSkills.Set(skillType, iCurrentValue);
}

void CHL2MP_Player::SetSkillValue(int skillType, int value)
{
	if ((skillType < 0) || (skillType >= PLAYER_SKILL_END))
		return;

	value = clamp(value, 0, 10);
	m_BB2Local.m_iPlayerSkills.Set(skillType, value);
}

float CHL2MP_Player::GetSkillWeaponDamage(float flDefaultDamage, float dmgFactor, int weaponType)
{
	float flExtraDamagePercent = 0.0f;
	switch (weaponType)
	{
	case WEAPON_TYPE_SMG:
	case WEAPON_TYPE_REVOLVER:
	case WEAPON_TYPE_PISTOL:
		flExtraDamagePercent = dmgFactor * (float)GetSkillValue(PLAYER_SKILL_HUMAN_PISTOL_MASTER);
		break;
	case WEAPON_TYPE_RIFLE:
		flExtraDamagePercent = dmgFactor * (float)GetSkillValue(PLAYER_SKILL_HUMAN_RIFLE_MASTER);
		break;
	case WEAPON_TYPE_SHOTGUN:
		flExtraDamagePercent = dmgFactor * (float)GetSkillValue(PLAYER_SKILL_HUMAN_SHOTGUN_MASTER);
		break;
	case WEAPON_TYPE_SNIPER:
		flExtraDamagePercent = dmgFactor * (float)GetSkillValue(PLAYER_SKILL_HUMAN_SNIPER_MASTER);
		break;
	}

	return ((flDefaultDamage / 100.0f) * flExtraDamagePercent);
}

// Reset All zombie related skills.
void CHL2MP_Player::ResetZombieSkills(void)
{
	m_iZombKills = 0;
	m_iZombDeaths = 0;
}

void CHL2MP_Player::RefreshSpeed(void)
{
	StartWalking();
	StopWalking();

	float flSpeed = GetPlayerSpeed();
	if (IsWalking())
		flSpeed /= 2.0f;

	SetMaxSpeed(flSpeed);
}

bool CHL2MP_Player::HandleLocalProfile(bool bSave)
{
	if (IsBot() || (bSave != m_bHasReadProfileData) || (GameBaseServer()->CanStoreSkills() != PROFILE_LOCAL))
		return false;

	unsigned long long steamID = (m_ullCachedSteamID > 0) ? m_ullCachedSteamID : ((unsigned long long)GetSteamIDAsUInt64());

	char pszFilePath[128];
	Q_snprintf(pszFilePath, 128, "data/local/%llu.txt", steamID);

	// Save your local profile:
	if (bSave)
	{
		FileHandle_t localProfile = g_pFullFileSystem->Open(pszFilePath, "w");
		if (localProfile != FILESYSTEM_INVALID_HANDLE)
		{
			char pszFileContent[2048];
			Q_snprintf(pszFileContent, 2048,
				"\"Profile\"\n"
				"{\n"

				"    \"Level\" \"%i\"\n"
				"    \"XPCurrent\" \"%i\"\n"
				"    \"XPLeft\" \"%i\"\n"
				"    \"Talents\" \"%i\"\n"
				"    \"ZombiePoints\" \"%i\"\n"

				"    \"Speed\" \"%i\"\n"
				"    \"Acrobatics\" \"%i\"\n"
				"    \"Slide\" \"%i\"\n"
				"    \"SniperMaster\" \"%i\"\n"
				"    \"EnhancedReflexes\" \"%i\"\n"
				"    \"MeleeSpeed\" \"%i\"\n"
				"    \"Lightweight\" \"%i\"\n"
				"    \"Weightless\" \"%i\"\n"
				"    \"HealthRegen\" \"%i\"\n"
				"    \"RealityPhase\" \"%i\"\n"

				"    \"Health\" \"%i\"\n"
				"    \"Impenetrable\" \"%i\"\n"
				"    \"Painkiller\" \"%i\"\n"
				"    \"LifeLeech\" \"%i\"\n"
				"    \"PowerKick\" \"%i\"\n"
				"    \"Bleed\" \"%i\"\n"
				"    \"CripplingBlow\" \"%i\"\n"
				"    \"ArmorMaster\" \"%i\"\n"
				"    \"MeleeMaster\" \"%i\"\n"
				"    \"BloodRage\" \"%i\"\n"

				"    \"RifleMaster\" \"%i\"\n"
				"    \"ShotgunMaster\" \"%i\"\n"
				"    \"PistolMaster\" \"%i\"\n"
				"    \"Resourceful\" \"%i\"\n"
				"    \"BlazingAmmo\" \"%i\"\n"
				"    \"Coldsnap\" \"%i\"\n"
				"    \"ShoutAndSpray\" \"%i\"\n"
				"    \"EmpoweredBullets\" \"%i\"\n"
				"    \"MagazineRefill\" \"%i\"\n"
				"    \"Gunslinger\" \"%i\"\n"

				"    \"ZombieHealth\" \"%i\"\n"
				"    \"ZombieDamage\" \"%i\"\n"
				"    \"ZombieDamageReduction\" \"%i\"\n"
				"    \"ZombieSpeed\" \"%i\"\n"
				"    \"ZombieJump\" \"%i\"\n"
				"    \"ZombieLeap\" \"%i\"\n"
				"    \"ZombieDeath\" \"%i\"\n"
				"    \"ZombieLifeLeech\" \"%i\"\n"
				"    \"ZombieHealthRegen\" \"%i\"\n"
				"    \"ZombieMassInvasion\" \"%i\"\n"

				"}\n",

				m_iSkill_Level,
				m_BB2Local.m_iSkill_XPCurrent.Get(),
				m_BB2Local.m_iSkill_XPLeft.Get(),
				m_BB2Local.m_iSkill_Talents.Get(),
				m_BB2Local.m_iZombieCredits.Get(),

				GetSkillValue(PLAYER_SKILL_HUMAN_SPEED),
				GetSkillValue(PLAYER_SKILL_HUMAN_ACROBATICS),
				GetSkillValue(PLAYER_SKILL_HUMAN_SLIDE),
				GetSkillValue(PLAYER_SKILL_HUMAN_SNIPER_MASTER),
				GetSkillValue(PLAYER_SKILL_HUMAN_ENHANCED_REFLEXES),
				GetSkillValue(PLAYER_SKILL_HUMAN_MELEE_SPEED),
				GetSkillValue(PLAYER_SKILL_HUMAN_LIGHTWEIGHT),
				GetSkillValue(PLAYER_SKILL_HUMAN_WEIGHTLESS),
				GetSkillValue(PLAYER_SKILL_HUMAN_HEALTHREGEN),
				GetSkillValue(PLAYER_SKILL_HUMAN_REALITY_PHASE),

				GetSkillValue(PLAYER_SKILL_HUMAN_HEALTH),
				GetSkillValue(PLAYER_SKILL_HUMAN_IMPENETRABLE),
				GetSkillValue(PLAYER_SKILL_HUMAN_PAINKILLER),
				GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH),
				GetSkillValue(PLAYER_SKILL_HUMAN_POWER_KICK),
				GetSkillValue(PLAYER_SKILL_HUMAN_BLEED),
				GetSkillValue(PLAYER_SKILL_HUMAN_CRIPPLING_BLOW),
				GetSkillValue(PLAYER_SKILL_HUMAN_ARMOR_MASTER),
				GetSkillValue(PLAYER_SKILL_HUMAN_MELEE_MASTER),
				GetSkillValue(PLAYER_SKILL_HUMAN_BLOOD_RAGE),

				GetSkillValue(PLAYER_SKILL_HUMAN_RIFLE_MASTER),
				GetSkillValue(PLAYER_SKILL_HUMAN_SHOTGUN_MASTER),
				GetSkillValue(PLAYER_SKILL_HUMAN_PISTOL_MASTER),
				GetSkillValue(PLAYER_SKILL_HUMAN_RESOURCEFUL),
				GetSkillValue(PLAYER_SKILL_HUMAN_BLAZING_AMMO),
				GetSkillValue(PLAYER_SKILL_HUMAN_COLDSNAP),
				GetSkillValue(PLAYER_SKILL_HUMAN_SHOUT_AND_SPRAY),
				GetSkillValue(PLAYER_SKILL_HUMAN_EMPOWERED_BULLETS),
				GetSkillValue(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL),
				GetSkillValue(PLAYER_SKILL_HUMAN_GUNSLINGER),

				GetSkillValue(PLAYER_SKILL_ZOMBIE_HEALTH),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_DAMAGE),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_DAMAGE_REDUCTION),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_SPEED),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_JUMP),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_LEAP),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_DEATH),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_LIFE_LEECH),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_HEALTH_REGEN),
				GetSkillValue(PLAYER_SKILL_ZOMBIE_MASS_INVASION)
				);

			g_pFullFileSystem->Write(&pszFileContent, strlen(pszFileContent), localProfile);
			g_pFullFileSystem->Close(localProfile);
			return true;
		}

		return false;
	}

	// Load your local profile:
	m_bHasReadProfileData = true;
	m_bHasTriedToLoadStats = true;
	m_ullCachedSteamID = steamID;

	if (!filesystem->FileExists(pszFilePath, "MOD"))
		return false;

	bool bCouldParse = false;

	KeyValues *pkvProfile = new KeyValues("ProfileData");
	if (pkvProfile->LoadFromFile(filesystem, pszFilePath, "MOD"))
	{
		bCouldParse = true;

		// Load Base
		m_iSkill_Level = pkvProfile->GetInt("Level", 1);
		m_BB2Local.m_iSkill_XPCurrent = pkvProfile->GetInt("XPCurrent");
		m_BB2Local.m_iSkill_XPLeft = pkvProfile->GetInt("XPLeft", (GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iXPIncreasePerLevel * m_iSkill_Level));
		m_BB2Local.m_iSkill_Talents = pkvProfile->GetInt("Talents");
		m_BB2Local.m_iZombieCredits = pkvProfile->GetInt("ZombiePoints");

		int indexIter = 0; // Skip the 5 first kvs!
		int skillIndex = 0;
		for (KeyValues *sub = pkvProfile->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			indexIter++;
			if (indexIter > 5)
			{
				SetSkillValue(skillIndex, sub->GetInt());
				skillIndex++;
			}
		}
	}

	pkvProfile->deleteThis();

	if (bCouldParse)
		OnLateStatsLoadEnterGame();

	return bCouldParse;
}

void CHL2MP_Player::SetZombie()
{
	SetCollisionGroup(COLLISION_GROUP_PLAYER_ZOMBIE);

	if (IsAlive())
	{
		SetPlayerModel(TEAM_DECEASED);
		ApplyArmor(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED)->iArmor, GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED)->iArmorType);

		float flHealth = ceil(GetSkillValue("Health", PLAYER_SKILL_ZOMBIE_HEALTH, TEAM_DECEASED));
		SetHealth((int)flHealth);
		SetMaxHealth((int)flHealth);
		SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_ZOMBIE_SPEED, TEAM_DECEASED));
		SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_ZOMBIE_LEAP, TEAM_DECEASED));
		SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_ZOMBIE_JUMP, TEAM_DECEASED));
		SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_ZOMBIE_HEALTH_REGEN, TEAM_DECEASED));

		GiveItem("weapon_zombhands");

		RefreshSpeed();

		if ((m_BB2Local.m_iZombieCredits >= 5) && !GameBaseServer()->IsTutorialModeEnabled() && HL2MPRules()->CanUseSkills())
			GameBaseServer()->SendToolTip("#TOOLTIP_SPEND_TALENTS_HINT", GAME_TIP_DEFAULT, this->entindex());
	}
}

void CHL2MP_Player::SetHuman()
{
	SetCollisionGroup(COLLISION_GROUP_PLAYER);
	ResetZombieSkills();

	if (IsAlive())
	{
		m_BB2Local.m_bHasPlayerEscaped = false;
		m_BB2Local.m_bCanRespawnAsHuman = false;

		SetPlayerModel(TEAM_HUMANS);
		ApplyArmor(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->iArmor, GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS)->iArmorType);

		float flHealth = ceil(GetSkillValue("Health", PLAYER_SKILL_HUMAN_HEALTH, TEAM_HUMANS));
		SetHealth((int)flHealth);
		SetMaxHealth((int)flHealth);
		SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_HUMAN_SPEED, TEAM_HUMANS));
		SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_HUMAN_ACROBATICS, TEAM_HUMANS, PLAYER_SKILL_HUMAN_LEAP));
		SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_HUMAN_ACROBATICS, TEAM_HUMANS, PLAYER_SKILL_HUMAN_JUMP));
		SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_HUMAN_HEALTHREGEN, TEAM_HUMANS));

		GiveItem("weapon_hands");

		RefreshSpeed();

		if (m_pPlayerEquipper)
			m_pPlayerEquipper->EquipPlayer(this);

		if ((m_BB2Local.m_iSkill_Talents >= 5) && !GameBaseServer()->IsTutorialModeEnabled() && HL2MPRules()->CanUseSkills())
			GameBaseServer()->SendToolTip("#TOOLTIP_SPEND_TALENTS_HINT", GAME_TIP_DEFAULT, this->entindex());

		// Give a random low-end weapon in this mode:
		if (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)
		{
			const char *pchRandomFirearm[] =
			{
				"weapon_glock17",
				"weapon_beretta",
			};

			const char *pchRandomMelee[] =
			{
				"weapon_m9_bayonet",
				"weapon_hatchet",
			};

			GiveItem(pchRandomMelee[random->RandomInt(0, (_ARRAYSIZE(pchRandomMelee) - 1))]);
			GiveItem(pchRandomFirearm[random->RandomInt(0, (_ARRAYSIZE(pchRandomFirearm) - 1))]);

			// Switch to the best available wep:
			CBaseCombatWeapon *pWantedWeapon = GetBestWeapon();
			if (pWantedWeapon != NULL)
				Weapon_Switch(pWantedWeapon, true);
		}

		if (GameBaseShared()->GetPlayerLoadoutHandler())
			GameBaseShared()->GetPlayerLoadoutHandler()->LoadDataForPlayer(this);
	}
}

void CHL2MP_Player::SetPlayerClass(int iTeam)
{
	// Reset weight value:
	m_BB2Local.m_flCarryWeight = 0.0f;
	m_flHealthRegenWaitTime = 4.0f;

	// Decide if we should set our client to be a zombie or not.
	// Default, in case it really fucks up we just throw our client over to the human side, no questions asked.
	switch (iTeam)
	{
	case TEAM_HUMANS:
		SetHuman();
		break;
	case TEAM_DECEASED:
		SetZombie();
		break;
	}

	m_BB2Local.m_iPerkTeamBonus = 0;

	// Shared Handlers:
	m_bWantsToDeployAsHuman = false;
	m_BB2Local.m_flPlayerRespawnTime = 0.0f;

	if (iTeam >= TEAM_HUMANS)
		m_bHasFullySpawned = true;

	if (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)
		SetSelectedTeam(iTeam);

	m_iDMKills = 0;
	m_flDMTimeSinceLastKill = 0.0f;
}

bool CHL2MP_Player::IsValidObserverTarget(CBaseEntity * target)
{
	bool ret = BaseClass::IsValidObserverTarget(target);

	CHL2MP_Player *player = ToHL2MPPlayer(target);
	if (!player)
		return ret;

	// In elimination we can't spectate non team members.
	if ((HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION) && GetSelectedTeam() != player->GetSelectedTeam())
		return false;

	return ret;
}

bool CHL2MP_Player::IsWeaponEquippedByDefault(const char *weaponName)
{
	if (m_pPlayerEquipper && m_pPlayerEquipper->ShouldEquipWeapon(weaponName))
		return true;

	if (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION))
	{
		if (
			!strcmp(weaponName, "weapon_glock17") ||
			!strcmp(weaponName, "weapon_beretta") ||
			!strcmp(weaponName, "weapon_m9_bayonet") ||
			!strcmp(weaponName, "weapon_hatchet")
			)
			return true;
	}

	return false;
}

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //
class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEPlayerAnimEvent, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEPlayerAnimEvent(const char *name) : CBaseTempEntity(name)
	{
	}

	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
	CNetworkVar(float, m_flData);
};

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlayerAnimEvent, DT_TEPlayerAnimEvent)
SendPropEHandle(SENDINFO(m_hPlayer)),
SendPropInt(SENDINFO(m_iEvent), Q_log2(PLAYERANIMEVENT_COUNT) + 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_nData), 32),
SendPropFloat(SENDINFO(m_flData))
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent("PlayerAnimEvent");

void TE_PlayerAnimEvent(CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData, bool bSkipPrediction, float flData)
{
	CPVSFilter filter((const Vector&)pPlayer->EyePosition());

	if (bSkipPrediction == false)
		filter.UsePredictionRules();

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.m_flData = flData;
	g_TEPlayerAnimEvent.Create(filter, 0.0f);
}

void CHL2MP_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData, bool bSkipPrediction, float flData)
{
	// Disable spawn prot. if we fire in elimination or deathmatch mode!
	bool bShouldDisable = (
		event == PLAYERANIMEVENT_ATTACK_PRIMARY ||
		event == PLAYERANIMEVENT_ATTACK_SECONDARY ||
		event == PLAYERANIMEVENT_JUMP ||
		event == PLAYERANIMEVENT_BASH ||
		event == PLAYERANIMEVENT_KICK ||
		event == PLAYERANIMEVENT_SLIDE
		);

	if (bShouldDisable && HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA))
		RemoveSpawnProtection();

	if (nData > 0)
		flData = GetPlaybackRateForAnimEvent(event, nData);

	m_PlayerAnimState->DoAnimationEvent(event, nData, flData);
	TE_PlayerAnimEvent(this, event, nData, bSkipPrediction, flData);	// Send to any clients who can see this guy.
}

//-----------------------------------------------------------------------------
// Purpose: Override setup bones so that is uses the render angles from
//			the HL2MP animation state to setup the hitboxes.
//-----------------------------------------------------------------------------
void CHL2MP_Player::SetupBones(matrix3x4_t *pBoneToWorld, int boneMask)
{
	VPROF_BUDGET("CHL2MP_Player::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM);

	// Get the studio header.
	Assert(GetModelPtr());
	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return;

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	// Adjust hit boxes based on IK driven offset.
	Vector adjOrigin = GetAbsOrigin() + Vector(0, 0, m_flEstIkOffset);

	// FIXME: pass this into Studio_BuildMatrices to skip transforms
	CBoneBitList boneComputed;
	if (m_pIk)
	{
		m_iIKCounter++;
		m_pIk->Init(pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask);
		GetSkeleton(pStudioHdr, pos, q, boneMask);

		m_pIk->UpdateTargets(pos, q, pBoneToWorld, boneComputed);
		CalculateIKLocks(gpGlobals->curtime);
		m_pIk->SolveDependencies(pos, q, pBoneToWorld, boneComputed);
	}
	else
	{
		GetSkeleton(pStudioHdr, pos, q, boneMask);
	}

	CBaseAnimating *pParent = dynamic_cast< CBaseAnimating* >(GetMoveParent());
	if (pParent)
	{
		// We're doing bone merging, so do special stuff here.
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if (pParentCache)
		{
			BuildMatricesWithBoneMerge(
				pStudioHdr,
				m_PlayerAnimState->GetRenderAngles(),
				adjOrigin,
				pos,
				q,
				pBoneToWorld,
				pParent,
				pParentCache);

			return;
		}
	}

	Studio_BuildMatrices(
		pStudioHdr,
		m_PlayerAnimState->GetRenderAngles(),
		adjOrigin,
		pos,
		q,
		-1,
		GetModelScale(), // Scaling
		pBoneToWorld,
		boneMask);
}

CON_COMMAND(classic_rejoin_zombie, "Re-join the game as a zombie!")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if ((HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) || GameBaseServer()->IsStoryMode() || HL2MPRules()->IsGameoverOrScoresVisible())
		return;

	if (pClient->IsBot() || (pClient->GetTeamNumber() != TEAM_SPECTATOR) || !pClient->HasPlayerEscaped())
		return;

	pClient->m_bWantsToDeployAsHuman = false;
	pClient->HandleCommand_JoinTeam(TEAM_DECEASED, true);
	pClient->ForceRespawn();

	const char *randomFun[] =
	{
		"#TOOLTIP_ESCAPE_JOIN_ZOMBIE1",
		"#TOOLTIP_ESCAPE_JOIN_ZOMBIE2",
		"#TOOLTIP_ESCAPE_JOIN_ZOMBIE3",
	};

	GameBaseServer()->SendToolTip(randomFun[random->RandomInt(0, (_ARRAYSIZE(randomFun) - 1))], GAME_TIP_DEFAULT, pClient->entindex());
}

CON_COMMAND(classic_respawn_ashuman, "Respawn as a human if possible!")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if ((HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) || GameBaseServer()->IsStoryMode() || GameBaseServer()->IsTutorialModeEnabled() || HL2MPRules()->IsGameoverOrScoresVisible())
		return;

	if (pClient->IsBot() || (pClient->GetTeamNumber() != TEAM_DECEASED) || pClient->HasPlayerEscaped())
		return;

	if (!pClient->m_BB2Local.m_bCanRespawnAsHuman)
		return;

	pClient->m_BB2Local.m_bCanRespawnAsHuman = false;
	pClient->m_iZombKills = 0;
	pClient->m_iZombDeaths = 0;

	pClient->m_bWantsToDeployAsHuman = true;
	pClient->HandleCommand_JoinTeam(TEAM_HUMANS, true);
	pClient->ForceRespawn();

	const char *randomFun[] =
	{
		"#TOOLTIP_RESPAWN_HUMAN1",
		"#TOOLTIP_RESPAWN_HUMAN2",
		"#TOOLTIP_RESPAWN_HUMAN3",
	};

	GameBaseServer()->SendToolTip(randomFun[random->RandomInt(0, (_ARRAYSIZE(randomFun) - 1))], GAME_TIP_DEFAULT, pClient->entindex());
}

CON_COMMAND(skill_tree, "Player Skill Tree")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || !HL2MPRules()->CanUseSkills())
		return;

	pClient->ShowViewPortPanel("skills", true);
}

CON_COMMAND(zombie_tree, "Zombie Skill Tree")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || !HL2MPRules()->CanUseSkills() || (HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) || GameBaseServer()->IsStoryMode())
		return;

	pClient->ShowViewPortPanel("zombie", true);
}

CON_COMMAND(team_menu, "Team Selection Menu")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION))
		return;

	pClient->ShowViewPortPanel("team", true);
}

CON_COMMAND(voice_menu, "Voice Command Menu")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || !pClient->IsHuman() || !pClient->IsAlive() || !HL2MPRules()->IsTeamplay())
		return;

	pClient->ShowViewPortPanel("voicewheel", true);
}

CON_COMMAND(activate_perk_agility, "Activate Agility Perk")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	pClient->ActivatePerk(PLAYER_SKILL_HUMAN_REALITY_PHASE);
}

CON_COMMAND(activate_perk_strength, "Activate Strength Perk")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	pClient->ActivatePerk(PLAYER_SKILL_HUMAN_BLOOD_RAGE);
}

CON_COMMAND(activate_perk_proficiency, "Activate Proficiency Perk")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	pClient->ActivatePerk(PLAYER_SKILL_HUMAN_GUNSLINGER);
}

CON_COMMAND(activate_zombie_rage, "Activate Zombie Rage Perk")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	pClient->EnterRageMode();
}

CON_COMMAND(holster_weapon, "Holster your weapon.")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || !pClient->IsHuman())
		return;

	CBaseCombatWeapon *pWantedWeapon = pClient->Weapon_OwnsThisType("weapon_hands");
	if (!pWantedWeapon)
		return;

	CBaseCombatWeapon *pActiveWeapon = pClient->GetActiveWeapon();
	if (pActiveWeapon == pWantedWeapon)
		return;

	if (!pActiveWeapon->CanHolster())
		return;

	pClient->Weapon_Switch(pWantedWeapon);
}

CON_COMMAND(bb2_reset_local_stats, "Reset local stats on any server which allows this, reverts you back to lvl 1, normally.")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pPlayer || ((pPlayer->LastTimePlayerTalked() + 1.0f) >= gpGlobals->curtime))
		return;

	pPlayer->NotePlayerTalked();

	if (GameBaseServer()->CanStoreSkills() != PROFILE_LOCAL)
	{
		ClientPrint(pPlayer, HUD_PRINTCONSOLE, "This command can only be used on servers which allow local stats!\n");
		return;
	}

	// BB2 SKILL TREE - Base

	int iLevel = clamp(GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iLevel, 1, MAX_PLAYER_LEVEL);

	pPlayer->SetPlayerLevel(iLevel);
	pPlayer->m_BB2Local.m_iSkill_Talents = GameBaseShared()->GetSharedGameDetails()->CalculatePointsForLevel(iLevel);
	pPlayer->m_BB2Local.m_iSkill_XPLeft = (GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iXPIncreasePerLevel * iLevel);
	pPlayer->m_BB2Local.m_iSkill_XPCurrent = 0;
	pPlayer->m_BB2Local.m_iZombieCredits = 0;

	for (int i = 0; i < MAX_SKILL_ARRAY; i++)
		pPlayer->m_BB2Local.m_iPlayerSkills.Set(i, 0);

	ClientPrint(pPlayer, HUD_PRINTCONSOLE, "You've reset all your skills!\n");
}

CON_COMMAND_F(bb2_set_fps, "Set fps of all animations. (weapons)", FCVAR_CHEAT)
{
	if (args.ArgC() != 2)
		return;

	int fps = atoi(args[1]);

	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	CBaseViewModel *pvm = pClient->GetViewModel();
	if (pvm)
	{
		CStudioHdr *pStudioHdr = pvm->GetModelPtr();
		if (pStudioHdr)
		{
			int seq = pvm->GetSequence();
			int sequences = pStudioHdr->GetNumSeq();

			for (int i = 0; i < sequences; i++)
			{
				mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc(i);
				mstudioanimdesc_t &animdesc = pStudioHdr->pAnimdesc(pStudioHdr->iRelativeAnim(i, seqdesc.anim(0, 0)));
				animdesc.fps = fps;
			}		

			pvm->ResetSequenceInfo();
			pvm->ResetSequence(seq);
		}
	}

	pClient->m_flNextAttack = gpGlobals->curtime;

	CBaseCombatWeapon *pWeapon = pClient->GetActiveWeapon();
	if (pWeapon)
	{
		pWeapon->ResetSequenceInfo();
		pWeapon->m_flNextPrimaryAttack = gpGlobals->curtime;
		pWeapon->m_flNextSecondaryAttack = gpGlobals->curtime;
	}
}
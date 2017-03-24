//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: BB2 Player
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "globalstate.h"
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
#include "gamestats.h"
#include "tier0/vprof.h"
#include "bone_setup.h"
#include "npc_BaseZombie.h"
#include "filesystem.h"
#include "ammodef.h"
#include "bb2_prop_button.h"
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

#include "tier0/memdbgon.h"

CLogicPlayerEquipper *m_pPlayerEquipper = NULL;

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

// Data that only gets sent to the local player.
SendPropDataTable("hl2mplocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2MPLocalPlayerExclusive), SendProxy_SendLocalDataTable),
// Data that gets sent to all other players
SendPropDataTable("hl2mpnonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_HL2MPNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable),

SendPropInt(SENDINFO(m_iSpawnInterpCounter), 4),
SendPropInt(SENDINFO(m_nPerkFlags), PERK_MAX_BITS, SPROP_UNSIGNED),
END_SEND_TABLE()

BEGIN_DATADESC(CHL2MP_Player)
DEFINE_EMBEDDED(m_BB2Local),
END_DATADESC()

const char *g_ppszRandomHumanModels[] =
{
	"models/characters/player/marine.mdl",
};

const char *g_ppszRandomZombieModels[] =
{
	"models/characters/player/marine_zombie.mdl",
};

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CHL2MP_Player::CHL2MP_Player()
{
	m_PlayerAnimState = CreateHL2MPPlayerAnimState(this);
	UseClientSideAnimation();

	m_angEyeAngles.Init();

	m_iLastWeaponFireUsercmd = 0;

	m_iTotalScore = 0;
	m_iTotalDeaths = 0;
	m_iRoundScore = 0;
	m_iRoundDeaths = 0;
	m_iZombKills = 0;
	m_iZombCurrentKills = 0;
	m_nGroupID = 0;
	m_iZombDeaths = 0;
	m_iNumPerkKills = 0;
	m_iSelectedTeam = 0;
	m_iSkill_Level = 0;
	m_nPerkFlags = 0;
	m_nMaterialOverlayFlags = 0;

	m_iSpawnInterpCounter = 0;

	m_bIsInfected = false;
	m_bEnterObserver = false;
	m_bHasReadProfileData = false;
	m_bWantsToDeployAsHuman = false;
	m_bHasFullySpawned = false;
	m_bHasJoinedGame = false;
	m_bHasTriedToLoadStats = false;
	m_bIsServerAdmin = false;
	m_bPlayerUsedFirearm = false;

	m_flUpdateTime = 0.0f;
	m_flNextResupplyTime = 0.0f;
	m_flLastInfectionTwitchTime = 0.0f;
	m_flLastTimeRanCommand = 0.0f;

	m_iTotalPing = 0;
	m_iTimesCheckedPing = 0;
	m_flLastTimeCheckedPing = 0.0f;
	m_flTimeUntilEndCheckPing = 0.0f;
	m_bFinishedPingCheck = false;

	m_hLastKiller = NULL;

	BaseClass::ChangeTeam(0);
}

CHL2MP_Player::~CHL2MP_Player(void)
{
	m_PlayerAnimState->Release();
}

void CHL2MP_Player::UpdateOnRemove(void)
{
	BaseClass::UpdateOnRemove();
}

void CHL2MP_Player::Precache(void)
{
	BaseClass::Precache();

	PrecacheModel("sprites/glow01.vmt");

	//Precache Zombie models
	int nHeads = ARRAYSIZE(g_ppszRandomZombieModels);
	int i;

	for (i = 0; i < nHeads; ++i)
		PrecacheModel(g_ppszRandomZombieModels[i]);

	//Precache Human Models
	nHeads = ARRAYSIZE(g_ppszRandomHumanModels);

	for (i = 0; i < nHeads; ++i)
		PrecacheModel(g_ppszRandomHumanModels[i]);

	// Extra handy stuff...
	PrecacheScriptSound("BaseEnemyHumanoid.Die");
	PrecacheScriptSound("ItemShared.Pickup");
	PrecacheScriptSound("ItemShared.Deny");
	PrecacheScriptSound("Music.Round.Start");
	PrecacheScriptSound("Player.ArmorImpact");

	PrecacheParticleSystem("bb2_levelup_effect");
	PrecacheParticleSystem("bb2_perk_activate");
}

void CHL2MP_Player::HandleFirstTimeConnection(bool bForceDefault)
{
	if (m_bHasReadProfileData && !bForceDefault)
		return;

	bool bShouldLoadDefaultStats = true;

	// Load / TRY to load stored profile.
	if (!bForceDefault)
	{
		if (GameBaseShared()->GetAchievementManager()->LoadGlobalStats(this) || HandleLocalProfile())
			bShouldLoadDefaultStats = false;
	}

	// We only init this if we don't init the skills / global stats.
	if (bShouldLoadDefaultStats)
	{
		int iLevel = GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().iLevel;
		if (iLevel < 1)
			iLevel = 1;

		if (GameBaseServer()->IsTutorialModeEnabled())
			iLevel = 100;

		m_iSkill_Level = iLevel;
		m_BB2Local.m_iSkill_Talents = ((iLevel > 100) ? 100 : (iLevel - 1));
		m_BB2Local.m_iSkill_XPLeft = (GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().iXPIncreasePerLevel * iLevel);

		OnLateStatsLoadEnterGame();
		m_bHasTriedToLoadStats = true;
	}
}

void CHL2MP_Player::OnLateStatsLoadEnterGame(void)
{
	if (!GameBaseServer()->IsTutorialModeEnabled())
	{
		if (HL2MPRules()->GetCurrentGamemode() == MODE_ARENA)
		{
			if (!HL2MPRules()->m_bRoundStarted)
				HandleCommand_JoinTeam(TEAM_HUMANS);
		}
		else if (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE)
		{
			if (bb2_allow_latejoin.GetBool() || GameBaseServer()->IsStoryMode() || !HL2MPRules()->m_bRoundStarted)
			{
				m_bWantsToDeployAsHuman = true;
				HandleCommand_JoinTeam(TEAM_HUMANS);
			}
			else
				HandleCommand_JoinTeam(TEAM_DECEASED);
		}
	}

	ForceRespawn();
}

void CHL2MP_Player::PickDefaultSpawnTeam(int iForceTeam)
{
	m_flTimeUntilEndCheckPing = gpGlobals->curtime + HIGH_PING_CHECK_TIME;
	if (HL2MPRules()->m_bRoundStarted)
		m_flLastTimeRanCommand = gpGlobals->curtime;

	if (GetTeamNumber() <= TEAM_UNASSIGNED)
		GameBaseServer()->NewPlayerConnection(this);

	if (iForceTeam > 0)
	{
		m_bHasFullySpawned = m_bHasJoinedGame = m_bHasTriedToLoadStats = true;
		SetSelectedTeam(iForceTeam);
		HandleCommand_JoinTeam(iForceTeam, true);
		ForceRespawn();
		return;
	}

	if (GetTeamNumber() == 0)
	{
		if (GameBaseServer()->IsTutorialModeEnabled())
		{
			m_bHasFullySpawned = m_bHasJoinedGame = m_bHasTriedToLoadStats = true;
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

bool CHL2MP_Player::LoadGlobalStats(void)
{
	if (m_bHasReadProfileData || IsBot())
		return false;

	CSteamID pSteamClient;
	if (!GetSteamID(&pSteamClient))
	{
		Warning("Unable to get SteamID for user %s\n", GetPlayerName());
		return false;
	}

	SteamAPICall_t apiCall = steamgameserverapicontext->SteamGameServerStats()->RequestUserStats(pSteamClient);
	m_SteamCallResultRequestPlayerStats.Set(apiCall, this, &CHL2MP_Player::OnReceiveStatsForPlayer);
	return true;
}

bool CHL2MP_Player::SaveGlobalStatsForPlayer(void)
{
	if (!m_bHasReadProfileData || IsBot())
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

void CHL2MP_Player::OnReceiveStatsForPlayer(GSStatsReceived_t *pCallback, bool bIOFailure)
{
	if (m_bHasReadProfileData)
		return;

	m_bHasTriedToLoadStats = true;

	bool bFailed = (bIOFailure || (pCallback->m_eResult != k_EResultOK));
	if (bFailed)
	{
		HandleFirstTimeConnection(true);
		Warning("Failed to load stats for user %s, result %i\n", GetPlayerName(), pCallback->m_eResult);
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
	Warning("Received and loaded stats for user %s, result %i\n", GetPlayerName(), pCallback->m_eResult);
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
// Purpose: Called the first time the player's created
//-----------------------------------------------------------------------------
void CHL2MP_Player::InitialSpawn(void)
{
	BaseClass::InitialSpawn();
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

	// Misc
	m_flNextResupplyTime = 0.0f;
	m_nMaterialOverlayFlags = 0;

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

	if (HL2MPRules()->IsIntermission() || HL2MPRules()->m_bShouldShowScores)
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

	DoAnimationEvent(PLAYERANIMEVENT_SPAWN);
}

// Perform reasonable updates at a cheap interval:
void CHL2MP_Player::PerformPlayerUpdate(void)
{
	// Save Stats
	GameBaseShared()->GetAchievementManager()->SaveGlobalStats(this);

	if (HL2MPRules()->CanUseSkills())
	{
		// Team Bonus:
		int iPlayersFound = 0;
		// We glow our teammates if we're a human and our friends are far away but in range and out of sight:
		// We glow our enemies if we're a zombie.
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CHL2MP_Player *pPlr = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
			if (pPlr)
			{
				if ((pPlr->entindex() == this->entindex()))
					continue;

				// Check for team bonus
				bool bCanCheckPlayer = (IsAlive() && pPlr->IsAlive() && (pPlr->GetTeamNumber() >= TEAM_HUMANS) && (GetTeamNumber() >= TEAM_HUMANS) && (GetTeamNumber() == pPlr->GetTeamNumber()));
				if (bCanCheckPlayer)
				{
					if (GetAbsOrigin().DistTo(pPlr->GetAbsOrigin()) <= MAX_TEAMMATE_DISTANCE)
					{
						if (pPlr->FVisible(this, MASK_VISIBLE))
							iPlayersFound++;
					}
				}
			}
		}

		m_BB2Local.m_iPerkTeamBonus = iPlayersFound;

		if (IsAlive())
		{
			if (IsHuman())
			{
				m_BB2Local.m_bCanActivatePerk = (!GetPerkFlags() && (m_iNumPerkKills >= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iKillsRequiredToPerk)
					&& ((GetSkillValue(PLAYER_SKILL_HUMAN_REALITY_PHASE) > 0) || (GetSkillValue(PLAYER_SKILL_HUMAN_BLOOD_RAGE) > 0) || (GetSkillValue(PLAYER_SKILL_HUMAN_GUNSLINGER) > 0)));
			}
			else if (IsZombie())
			{
				m_BB2Local.m_bCanActivatePerk = (!GetPerkFlags() && (m_BB2Local.m_iZombieCredits >= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iZombieCreditsRequiredToRage));
			}
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
}

void CHL2MP_Player::PickupObject(CBaseEntity *pObject, bool bLimitMassAndSize)
{
	//Weapon_SwitchBySlot( 3 ); // CHANGE TO HANDS!
	BaseClass::PickupObject(pObject, bLimitMassAndSize);
}

void CHL2MP_Player::SetPlayerModel(int overrideTeam)
{
	const char *szModelName = g_ppszRandomHumanModels[0];
	int teamNum = GetTeamNumber();
	if (overrideTeam != -1)
		teamNum = overrideTeam;

	const char *survivorChoice = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "bb2_survivor_choice");
	if (survivorChoice != NULL && (strlen(survivorChoice) > 0))
		szModelName = GameBaseShared()->GetSharedGameDetails()->GetPlayerSurvivorModel(survivorChoice, teamNum);
	else
		szModelName = (teamNum == TEAM_DECEASED ? g_ppszRandomZombieModels[0] : g_ppszRandomHumanModels[0]);

	if (strlen(szModelName) <= 0)
		szModelName = (teamNum == TEAM_DECEASED ? g_ppszRandomZombieModels[0] : g_ppszRandomHumanModels[0]);
	else if (!engine->IsModelPrecached(szModelName))
	{
		int tryPrecache = PrecacheModel(szModelName);
		if (tryPrecache == -1)
			szModelName = (teamNum == TEAM_DECEASED ? g_ppszRandomZombieModels[0] : g_ppszRandomHumanModels[0]);
	}

	SetModel(szModelName);

	SetupCustomization();

	// Soundset Logic:
	const char *survivorLink = survivorChoice;
	if ((survivorLink == NULL) || (strlen(survivorLink) <= 0))
		survivorLink = "survivor1";

	const char *soundsetPrefix = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), ((GetTeamNumber() == TEAM_HUMANS) ? "bb2_sound_player_human" : "bb2_sound_player_deceased"));
	if ((soundsetPrefix == NULL) || (strlen(soundsetPrefix) <= 0))
		soundsetPrefix = ((GetTeamNumber() == TEAM_HUMANS) ? "Pantsman" : "Default");

	Q_strncpy(pchSoundsetSurvivorLink, survivorLink, 64);
	Q_strncpy(pchSoundsetPrefix, soundsetPrefix, 64);
	m_bSoundsetGender = ((Q_stristr(STRING(GetModelName()), "female")) ? false : true);
}

void CHL2MP_Player::SetupCustomization(void)
{
	m_nBody = 0;
	m_nSkin = 0;

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
		{
			int accessoryGroup = FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[i]);
			if (accessoryGroup == -1)
				continue;

			SetBodygroup(accessoryGroup, bodygroupAccessoryValues[i]);
		}
	}
}

bool CHL2MP_Player::Weapon_Switch(CBaseCombatWeapon *pWeapon, bool bWantDraw, int viewmodelindex)
{
	bool bRet = BaseClass::Weapon_Switch(pWeapon, bWantDraw, viewmodelindex);

	if (bRet == true)
	{
		FlashlightTurnOff();
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
	m_ArmorValue = (int)flArmorDurability;

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

	RefreshSpeed();
}

void CHL2MP_Player::DoHighPingCheck(void)
{
	if (m_bFinishedPingCheck || IsBot())
		return;

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
		}
	}
}

void CHL2MP_Player::PostThink(void)
{
	BaseClass::PostThink();

	if (bb2_enable_high_ping_kicker.GetBool() && engine->IsDedicatedServer())
		DoHighPingCheck();

	if (!IsBot() && bb2_enable_afk_kicker.GetBool() && engine->IsDedicatedServer() && !((GetTeamNumber() == TEAM_SPECTATOR) && m_bHasJoinedGame) && (m_flLastTimeRanCommand > 0.0f))
	{
		float flTimeSinceLastCMD = (gpGlobals->curtime - m_flLastTimeRanCommand);
		if (flTimeSinceLastCMD >= bb2_afk_kick_time.GetFloat())
		{
			char pchKickCmd[128];
			Q_snprintf(pchKickCmd, 128, "kickid %i AFK for too long!\n", GetUserID());
			engine->ServerCommand(pchKickCmd);
			engine->ServerExecute();
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

	CTeam *pMyTeam = GetGlobalTeam(GetTeamNumber());
	if (pMyTeam)
	{
		switch (pMyTeam->GetActivePerk())
		{
		case TEAM_HUMAN_PERK_UNLIMITED_AMMO:
		{
			CBaseCombatWeapon *pWeapon = GetActiveWeapon();
			if (pWeapon)
			{
				if ((pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL) && !pWeapon->IsMeleeWeapon())
				{
					bool bAffectedBySkill = false;

					if (pWeapon->UsesClipsForAmmo1())
					{
						pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
						bAffectedBySkill = true;
					}

					if (pWeapon->UsesClipsForAmmo2())
					{
						pWeapon->m_iClip2 = pWeapon->GetMaxClip2();
						bAffectedBySkill = true;
					}

					if (bAffectedBySkill)
						pWeapon->AffectedByPlayerSkill(PLAYER_SKILL_HUMAN_GUNSLINGER);
				}
			}
			break;
		}
		}
	}

	if (IsZombie())
		return;

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
			}

			if (IsPerkFlagActive(PERK_HUMAN_GUNSLINGER))
			{
				CBaseCombatWeapon *pWeapon = GetActiveWeapon();
				if (pWeapon)
				{
					if ((pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL) && !pWeapon->IsMeleeWeapon())
					{
						bool bAffectedBySkill = false;

						if (pWeapon->UsesClipsForAmmo1())
						{
							pWeapon->m_iClip1 = pWeapon->GetMaxClip1();
							bAffectedBySkill = true;
						}

						if (pWeapon->UsesClipsForAmmo2())
						{
							pWeapon->m_iClip2 = pWeapon->GetMaxClip2();
							bAffectedBySkill = true;
						}

						if (bAffectedBySkill)
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
	for (int i = 0; i < 4; i++)
	{
		if (Weapon_GetSlot(i) != NULL)
			Weapon_DropSlot(i);
	}
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

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation(this, this->GetCurrentCommand());

	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>(GetActiveWeapon());
	if (pWeapon)
	{
		int m_iDefDamage = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
		int m_iTeamBonusDamage = (m_BB2Local.m_iPerkTeamBonus - 5);
		int m_iNewDamage = (m_iDefDamage + GetSkillWeaponDamage((float)m_iDefDamage, pWeapon->GetWpnData().m_flSkillDamageFactor, pWeapon->GetWeaponType()));

		if (m_iTeamBonusDamage > 0)
			m_iNewDamage += ((m_iNewDamage / 100) * (m_iTeamBonusDamage * GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().iTeamBonusDamageIncrease));

		float flPierceDMG = 0.0f;
		if (IsPerkFlagActive(PERK_HUMAN_GUNSLINGER))
			flPierceDMG = ((float)m_iNewDamage / 100.0f) * (GetSkillValue(PLAYER_SKILL_HUMAN_GUNSLINGER, TEAM_HUMANS));

		if (IsPerkFlagActive(PERK_POWERUP_CRITICAL))
		{
			DataPlayerItem_Player_PowerupItem_t data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(PERK_POWERUP_CRITICAL);
			flPierceDMG = (((float)m_iNewDamage / 100.0f) * data.flExtraFactor);
		}

		modinfo.m_iPlayerDamage += (m_iNewDamage + flPierceDMG);
		modinfo.m_flDamage += (m_iNewDamage + flPierceDMG);
		
		//Msg( "Killing with %s\nDamage %i\n", pWeapon->GetClassname(), modinfo.m_iPlayerDamage );
	}

	int iSkillFlags = 0;

	if (GetSkillValue(PLAYER_SKILL_HUMAN_BLAZING_AMMO) > 0)
	{
		int iPercChance = (int)GetSkillValue(PLAYER_SKILL_HUMAN_BLAZING_AMMO, TEAM_HUMANS);
		if (random->RandomInt(0, 100) <= iPercChance)
		{
			iSkillFlags |= SKILL_FLAG_BLAZINGAMMO;
		}
	}

	if (GetSkillValue(PLAYER_SKILL_HUMAN_COLDSNAP) > 0)
	{
		int iPercChance = (int)GetSkillValue(PLAYER_SKILL_HUMAN_COLDSNAP, TEAM_HUMANS);
		if (random->RandomInt(0, 100) <= iPercChance)
			iSkillFlags |= SKILL_FLAG_COLDSNAP;
	}

	if (GetSkillValue(PLAYER_SKILL_HUMAN_EMPOWERED_BULLETS) > 0)
	{
		int iPercChance = (int)GetSkillValue(PLAYER_SKILL_HUMAN_EMPOWERED_BULLETS, TEAM_HUMANS);
		if (random->RandomInt(0, 100) <= iPercChance)
			iSkillFlags |= SKILL_FLAG_EMPOWERED_BULLETS;
	}

	modinfo.m_nPlayerSkillFlags = iSkillFlags;

	NoteWeaponFired();

	BaseClass::FireBullets(modinfo);

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation(this);

	m_bPlayerUsedFirearm = true;
}

void CHL2MP_Player::NoteWeaponFired(void)
{
	Assert(m_pCurrentCommand);
	if (m_pCurrentCommand)
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

bool CHL2MP_Player::WantsLagCompensationOnEntity(const CBaseEntity *pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const
{
	bool bCheckAttackButton = true;

	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if (pActiveWeapon)
	{
		if (pActiveWeapon->m_iMeleeAttackType > 0)
		{
			bCheckAttackButton = false;

			// Only compensate NPCs within a reasonable distance.
			float maxRange = pActiveWeapon->GetRange() * 8.0f;
			float distance = GetAbsOrigin().DistTo(pEntity->GetAbsOrigin());
			if (distance > maxRange)
				return false;
		}
	}

	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if (bCheckAttackButton && !(pCmd->buttons & IN_ATTACK) && !(pCmd->buttons & IN_ATTACK2) && !(pCmd->buttons & IN_ATTACK3) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5))
		return false;

	return BaseClass::WantsLagCompensationOnEntity(pEntity, pCmd, pEntityTransmitBits);
}

void CHL2MP_Player::Weapon_Equip(CBaseCombatWeapon *pWeapon)
{
	BaseClass::Weapon_Equip(pWeapon);

	GameBaseShared()->ComputePlayerWeight(this);
}

bool CHL2MP_Player::EquipAmmoFromWeapon(CBaseCombatWeapon *pWeapon)
{
	if (!pWeapon)
		return false;

	if (!pWeapon->CanPickupWeaponAsAmmo())
		return false;

	int primaryAmmo = GiveAmmo(pWeapon->GetDefaultClip1(), pWeapon->m_iPrimaryAmmoType, false);
	int secondaryAmmo = GiveAmmo(pWeapon->GetDefaultClip2(), pWeapon->m_iSecondaryAmmoType, false);

	if (primaryAmmo != 0 || secondaryAmmo != 0)
		return true;

	return false;
}

CBaseCombatWeapon *CHL2MP_Player::GetBestWeapon()
{
	int iWeight = 0;
	CBaseCombatWeapon *pWantedWeapon = NULL;
	for (int i = 0; i < 4; i++)
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
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if (!IsAllowedToPickupWeapons() || (GetTeamNumber() >= TEAM_DECEASED) || !IsAlive())
		return false;

	if (pOwner || !Weapon_CanUse(pWeapon) || !Weapon_CanSwitchTo(pWeapon))
		return false;

	bool bHasWeapon = false;
	for (int i = 0; i < 4; i++)
	{
		CBaseCombatWeapon *pMyWeapon = Weapon_GetSlot(i);
		if (pMyWeapon)
		{
			if (FClassnameIs(pWeapon, pMyWeapon->GetClassname()))
			{
				bHasWeapon = true;
				break;
			}
		}
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if (!pWeapon->FVisible(this, MASK_SOLID) && !(GetFlags() & FL_NOTARGET))
		return false;

	if ((m_iSkill_Level < pWeapon->GetWpnData().m_iLevelReq) && HL2MPRules()->CanUseSkills())
	{
		char pchArg1[16];
		Q_snprintf(pchArg1, 16, "%i", pWeapon->GetWpnData().m_iLevelReq);
		GameBaseServer()->SendToolTip("#TOOLTIP_WEAPON_DENY_LEVEL", 0, entindex(), pchArg1);
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
			GameBaseServer()->SendToolTip("#TOOLTIP_WEAPON_DENY_WAIT", 0, entindex(), pchArg1);
			return false;
		}
	}

	if (bHasWeapon)
	{
		if (EquipAmmoFromWeapon(pWeapon))
		{
			pWeapon->CheckRespawn();
			UTIL_Remove(pWeapon);
			return true;
		}
		else
			return false;
	}

	Weapon_DropSlot(pWeapon->GetSlot());
	pWeapon->CheckRespawn();
	pWeapon->AddSolidFlags(FSOLID_NOT_SOLID);
	pWeapon->AddEffects(EF_NODRAW);
	Weapon_Equip(pWeapon);

	if (IsInAVehicle())
		pWeapon->FullHolster();
	else
	{
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

// Gives the appropriate item:
bool CHL2MP_Player::GiveItem(const char *szItemName, bool bDoLevelCheck)
{
	if (bDoLevelCheck && HL2MPRules()->CanUseSkills())
	{
		WEAPON_FILE_INFO_HANDLE weaponHandle = LookupWeaponInfoSlot(szItemName);
		if (weaponHandle != GetInvalidWeaponInfoHandle())
		{
			FileWeaponInfo_t *pWeaponInfo = GetFileWeaponInfoFromHandle(weaponHandle);
			int levelRequired = pWeaponInfo->m_iLevelReq;
			if (m_iSkill_Level < levelRequired)
			{
				char pchArg1[16];
				Q_snprintf(pchArg1, 16, "%i", levelRequired);
				GameBaseServer()->SendToolTip("#TOOLTIP_WEAPON_DENY_LEVEL", 0, entindex(), pchArg1);
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
		m_BB2Local.m_iSkill_XPLeft += GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().iXPIncreasePerLevel;

		// You only receive talent points until level 100.
		if (MAX_LEVEL_FOR_TALENTS > m_iSkill_Level)
		{
			// Add a talent point.
			m_BB2Local.m_iSkill_Talents++;

			// Congrats!
			char pchArg1[16];
			Q_snprintf(pchArg1, 16, "%i", m_BB2Local.m_iSkill_Talents);
			GameBaseServer()->SendToolTip("#TOOLTIP_LEVELUP2", 0, this->entindex(), pchArg1);
		}

		// Add Level.
		m_iSkill_Level++;
		// Reset kill counter so we start over again. IEX: Now you need 10 kills, if we no reset you just need 5! Which is no change at all.
		m_BB2Local.m_iSkill_XPCurrent = 0;

		if (!IsBot())
		{
			// Friendly messages:
			char pchArg1[16];
			Q_snprintf(pchArg1, 16, "%i", m_iSkill_Level);
			GameBaseServer()->SendToolTip("#TOOLTIP_LEVELUP1", 0, this->entindex(), pchArg1);
			DispatchParticleEffect("bb2_levelup_effect", PATTACH_ROOTBONE_FOLLOW, this, -1, true);
		}

		if (iXPToGive > 0)
			PerformLevelUp(iXPToGive);			

		return true;
	}

	m_BB2Local.m_iSkill_XPCurrent = iXPToGive;
	return false;
}

bool CHL2MP_Player::CanLevelUp(int iXP, CBaseEntity *pVictim)
{
	// Don't give XP to people who've not spawned!
	if (!HasFullySpawned())
		return false;

	if (!HL2MPRules()->CanUseSkills())
	{
		if ((pVictim != NULL) && IsAlive() && (iXP > 1))
		{
			if (random->RandomInt(0, 8) == 4)
				HL2MPRules()->EmitSoundToClient(this, "Taunt", BB2_SoundTypes::TYPE_PLAYER, GetSoundsetGender());
		}

		return false;
	}

	if (pVictim != NULL && m_bIsInfected && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE))
		return false;

	if (m_iSkill_Level < MAX_PLAYER_LEVEL)
	{
		int iExperienceToGive = iXP;
		if (pVictim && (iXP > 1))
			iExperienceToGive += (m_BB2Local.m_iPerkTeamBonus * GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().iTeamBonusXPIncrease);

		// While this one goes for leveling... Because we also want to reset it or else we will actually only need 5 kills per level. 
		m_BB2Local.m_iSkill_XPCurrent += iExperienceToGive;
	}

	// Enable Perks: (% chance stuff)
	if ((pVictim != NULL) && IsAlive() && (iXP > 1))
	{
		if (random->RandomInt(0, 8) == 4)
			HL2MPRules()->EmitSoundToClient(this, "Taunt", BB2_SoundTypes::TYPE_PLAYER, GetSoundsetGender());

		int iPercentChance = 0;
		if (GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH) > 0)
		{
			iPercentChance = (int)GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH, TEAM_HUMANS);
			if (random->RandomInt(0, 100) <= iPercentChance)
			{
				float m_flEnemyHealth = pVictim->GetMaxHealth();
				float m_flHealthToTake = ((m_flEnemyHealth / 100.0f) * (GetSkillValue(PLAYER_SKILL_HUMAN_LIFE_LEECH, TEAM_HUMANS)));
				TakeHealth(m_flHealthToTake, DMG_GENERIC);
				DispatchDamageText(pVictim, (int)m_flHealthToTake);
			}
		}

		if (GetSkillValue(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL) > 0)
		{
			iPercentChance = (int)GetSkillValue(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL, TEAM_HUMANS);
			if (random->RandomInt(0, 100) <= iPercentChance)
			{
				CBaseCombatWeapon *pMyWeapon = GetActiveWeapon();
				if (pMyWeapon)
				{
					if ((pMyWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL) && !pMyWeapon->IsMeleeWeapon())
					{
						bool bAffectedBySkill = false;

						if (pMyWeapon->UsesClipsForAmmo1())
						{
							pMyWeapon->m_iClip1 = pMyWeapon->GetMaxClip1();
							bAffectedBySkill = true;
						}

						if (pMyWeapon->UsesClipsForAmmo2())
						{
							pMyWeapon->m_iClip2 = pMyWeapon->GetMaxClip2();
							bAffectedBySkill = true;
						}

						if (bAffectedBySkill)
							pMyWeapon->AffectedByPlayerSkill(PLAYER_SKILL_HUMAN_MAGAZINE_REFILL);
					}
				}
			}
		}
	}

	// Disable leveling if we're at max level.
	if (m_iSkill_Level >= MAX_PLAYER_LEVEL)
		return false;

	// We try to level up:
	return PerformLevelUp(m_BB2Local.m_iSkill_XPCurrent);
}

bool CHL2MP_Player::ActivatePerk(int skill)
{
	if (!IsHuman() || !IsAlive() || (GetSkillValue(skill) <= 0) || (GetPerkFlags() != 0) || (m_iNumPerkKills < GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iKillsRequiredToPerk) || (!HL2MPRules()->CanUseSkills()))
		return false;

	m_iNumPerkKills = 0;
	m_BB2Local.m_bCanActivatePerk = false;

	switch (skill)
	{
	case PLAYER_SKILL_HUMAN_REALITY_PHASE:
	{
		SetPlayerSpeed(GetSkillCombination(GetSkillValue("Speed", PLAYER_SKILL_HUMAN_SPEED, TEAM_HUMANS), GetSkillValue(PLAYER_SKILL_HUMAN_REALITY_PHASE, TEAM_HUMANS)));
		m_BB2Local.m_flPerkTimer = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().flPerkTime;
		RefreshSpeed();
		AddPerkFlag(PERK_HUMAN_REALITYPHASE);
		SetCollisionGroup(COLLISION_GROUP_PLAYER_REALITY_PHASE);
		GameBaseShared()->GetAchievementManager()->WriteToAchievement(this, "ACH_SKILL_PERK_ROCKET");
		break;
	}

	case PLAYER_SKILL_HUMAN_GUNSLINGER:
	{
		m_BB2Local.m_flPerkTimer = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().flPerkTime;
		AddPerkFlag(PERK_HUMAN_GUNSLINGER);
		break;
	}

	case PLAYER_SKILL_HUMAN_BLOOD_RAGE:
	{
		m_BB2Local.m_flPerkTimer = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData().flPerkTime;
		AddPerkFlag(PERK_HUMAN_BLOODRAGE);
		break;
	}
	}

	DispatchParticleEffect("bb2_perk_activate", PATTACH_ROOTBONE_FOLLOW, this, -1, true);
	return true;
}

bool CHL2MP_Player::EnterRageMode(bool bForce) // Zombie 'Perk' thing. (lasts until death)
{
	if (!IsZombie() || !IsAlive() || (GetPerkFlags() != 0))
		return false;

	if (!bForce)
	{
		if (!HL2MPRules()->CanUseSkills() || (m_BB2Local.m_iZombieCredits < GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iZombieCreditsRequiredToRage))
			return false;

		m_BB2Local.m_iZombieCredits -= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iZombieCreditsRequiredToRage;
	}

	m_BB2Local.m_bCanActivatePerk = false;

	if (HL2MPRules()->CanUseSkills())
	{
		int health = round(GetSkillValue("Health", PLAYER_SKILL_ZOMBIE_HEALTH, TEAM_DECEASED)) + (int)GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flHealth;
		SetHealth(health);
		SetMaxHealth(health);
		SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_ZOMBIE_SPEED, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flSpeed);
		SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_ZOMBIE_LEAP, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flLeap);
		SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_ZOMBIE_JUMP, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flJump);
		SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_ZOMBIE_HEALTH_REGEN, TEAM_DECEASED) + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flHealthRegen);
	}
	else
	{
		int health = GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).iHealth + (int)GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flHealth;
		SetHealth(health);
		SetMaxHealth(health);
		SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flSpeed + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flSpeed);
		SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flLeapLength + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flLeap);
		SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flJumpHeight + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flJump);
		SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flHealthRegenerationRate + GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flHealthRegen);
	}

	RefreshSpeed();

	AddPerkFlag(PERK_ZOMBIE_RAGE);
	DispatchParticleEffect("bb2_perk_activate", PATTACH_ROOTBONE_FOLLOW, this, -1, true);
	return true;
}

// If we're active in some external mode, ex rage mode / perk and use the skill tree then we might want to sum extra values to the skill values, 
// ex speed during rage mode for zombies, using the skill tree before would reset your speed to the base skill speed, not taking the 'rage' speed into account, compensate for it here...
float CHL2MP_Player::GetExtraPerkData(int type) 
{
	if (IsPerkFlagActive(PERK_ZOMBIE_RAGE))
	{
		switch (type)
		{
		case PLAYER_SKILL_ZOMBIE_SPEED:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flSpeed;

		case PLAYER_SKILL_ZOMBIE_JUMP:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flJump;

		case PLAYER_SKILL_ZOMBIE_LEAP:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flLeap;

		case PLAYER_SKILL_ZOMBIE_HEALTH_REGEN:
			return GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flHealthRegen;
		}
	}

	return 0.0f;
}

bool CHL2MP_Player::CanEnablePowerup(int powerupFlag, float duration)
{
	if (GetPerkFlags() || !HL2MPRules()->IsPowerupsAllowed())
		return false;

	DataPlayerItem_Player_PowerupItem_t data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(powerupFlag);
	AddPerkFlag(powerupFlag);
	m_BB2Local.m_flPerkTimer = gpGlobals->curtime + duration;

	int currHP = GetHealth();
	float healthIncrease = ((float)GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iHealth / 100.0f) * data.flHealth;
	float speedIncrease = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flSpeed / 100.0f) * data.flSpeed;
	float jumpIncrease = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flJumpHeight / 100.0f) * data.flJumpHeight;
	float leapIncrease = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flLeapLength / 100.0f) * data.flLeapLength;
	float healthRegenRate = (GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flHealthRegenerationRate / 100.0f) * data.flHealthRegenerationRate;
	if (healthRegenRate <= 0)
		healthRegenRate = data.flHealthRegenerationRate;

	SetHealth(healthIncrease + currHP);
	SetMaxHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iHealth + healthIncrease);
	SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flSpeed + speedIncrease);
	SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flLeapLength + leapIncrease);
	SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flJumpHeight + jumpIncrease);
	SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flHealthRegenerationRate + healthRegenRate);
	m_fTimeLastHurt = 0.0f;
	m_flHealthRegenWaitTime = 0.2f;

	if (GetHealth() > GetMaxHealth())
		SetHealth(GetMaxHealth());

	RefreshSpeed();
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
		SetMaxHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iHealth);
		SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flSpeed);
		SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flLeapLength);
		SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flJumpHeight);
		SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flHealthRegenerationRate);

		if (GetHealth() > GetMaxHealth())
			SetHealth(GetMaxHealth());

		if (IsPerkFlagActive(PERK_POWERUP_PAINKILLER))
		{
			float percentLost = (currHealth / currMaxHealth);
			float newHealth = ((float)GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iHealth * percentLost);

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

		pPowerupEnt->SetLocalOrigin(tr.endpos);
		pPowerupEnt->SetParam(powerup);
		pPowerupEnt->Spawn();
		pPowerupEnt->SetParam(durationLeft);
	}
}

void CHL2MP_Player::ChangeTeam(int iTeam, bool bInfection)
{
	int iOldTeam = GetTeamNumber();
	bool bKill = false;
	if (iTeam != iOldTeam && (iOldTeam >= TEAM_HUMANS))
		bKill = true;

	// When we leave spec mode we need to refresh this value to make sure we don't get kicked if we didn't move while spectating.
	if ((iOldTeam == TEAM_SPECTATOR) && m_bHasJoinedGame && (iTeam >= TEAM_HUMANS) && (m_flLastTimeRanCommand > 0.0f))
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

		if (sv_cheats->GetBool() || bIsTutorial)
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

		if (sv_cheats->GetBool() || bIsTutorial)
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
		if (HL2MPRules())
			HL2MPRules()->PlayerVote(this, true);
		return true;
	}

	else if (FStrEq(args[0], "player_vote_no"))
	{
		if (HL2MPRules())
			HL2MPRules()->PlayerVote(this, false);
		return true;
	}

	else if (FStrEq(args[0], "joingame"))
	{
		if (m_bHasJoinedGame)
			return true;

		m_bHasJoinedGame = true;

		// BB2 Warn : Hack - Give the zombie plrs snacks...		
		m_BB2Local.m_iZombieCredits = GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iDefaultZombieCredits;

		if (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)
		{
			ShowViewPortPanel("team", true);
			return true;
		}
		else if (HL2MPRules()->GetCurrentGamemode() == MODE_DEATHMATCH)
		{
			if (HL2MPRules()->m_bRoundStarted)
				m_BB2Local.m_flPlayerRespawnTime = HL2MPRules()->GetPlayerRespawnTime(this);
			else
				HandleCommand_JoinTeam(TEAM_HUMANS);
		}

		HandleFirstTimeConnection();
		return true;
	}

	else if (FStrEq(args[0], "giveall"))
	{
		GameBaseShared()->GetAchievementManager()->WriteToAchievement(this, "ACH_SECRET_GIVEALL");
		return true;
	}

	else if (FStrEq(args[0], "jointeam"))
	{
		if (args.ArgC() != 2)
			return true;

		if (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)
			return true;

		int iTeam = (atoi(args[1]) == 1) ? TEAM_DECEASED : TEAM_HUMANS;
		if (iTeam == m_iSelectedTeam) // You're already on this team...
		{
			GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_CHANGE_FAIL1", 0, entindex());
			ShowViewPortPanel("team", false);
			return true;
		}

		if (m_BB2Local.m_flPlayerRespawnTime > gpGlobals->curtime)
		{
			float flTimeToWait = m_BB2Local.m_flPlayerRespawnTime - gpGlobals->curtime;

			char pszTimer[16];
			Q_snprintf(pszTimer, 16, "%i", (int)flTimeToWait);
			GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_CHANGE_FAIL2", 0, entindex(), pszTimer);
			ShowViewPortPanel("team", false);
			return true;
		}

		int teamOverride = HL2MPRules()->GetNewTeam(iTeam);
		if (teamOverride != iTeam)
		{
			GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_CHANGE_FAIL3", 0, entindex());
			if (m_iSelectedTeam != TEAM_UNASSIGNED)
				ShowViewPortPanel("team", false);

			return true;
		}

		ShowViewPortPanel("team", false);
		SetSelectedTeam(iTeam);
		m_bWantsToDeployAsHuman = true;
		HandleCommand_JoinTeam(iTeam);

		if (m_bHasReadProfileData || m_bHasFullySpawned || m_bHasTriedToLoadStats)
		{
			//ForceRespawn();
			return true;
		}

		HandleFirstTimeConnection();
		return true;
	}

	else if (FStrEq(args[0], "bb_voice_command"))
	{
		if (args.ArgC() != 2)
			return true;

		if (!IsHuman() || !IsAlive() || !HL2MPRules()->IsTeamplay())
			return true;

		if ((LastTimePlayerTalked() + VOICE_COMMAND_EMIT_DELAY) < gpGlobals->curtime)
		{
			NotePlayerTalked();

			int iCommand = atoi(args[1]);
			for (int i = VOICE_COMMAND_AGREE; i < VOICE_COMMAND_EXIT; i++)
			{
				if (i == iCommand)
				{
					char pszVoiceMessage[128];
					Q_snprintf(pszVoiceMessage, 128, "%s", GetVoiceCommandChatMessage(i));
					Host_Say(edict(), pszVoiceMessage, true, CHAT_CMD_VOICE);
					HL2MPRules()->EmitSoundToClient(this, GetVoiceCommandString(i), BB2_SoundTypes::TYPE_PLAYER, GetSoundsetGender());
					break;
				}
			}
		}

		return true;
	}

	else if (FStrEq(args[0], "bb_inventory_item_drop"))
	{
		if (args.ArgC() != 3)
			return true;

		uint iID = (uint)atol(args[1]);
		bool bMapItem = (atoi(args[2]) >= 1);

		GameBaseShared()->RemoveInventoryItem(entindex(), GetAbsOrigin(), (bMapItem ? 1 : 0), iID);
		return true;
	}

	else if (FStrEq(args[0], "bb_inventory_item_use"))
	{
		if (args.ArgC() != 3)
			return true;

		uint iID = (uint)atol(args[1]);
		bool bMapItem = (atoi(args[2]) >= 1);

		GameBaseShared()->UseInventoryItem(entindex(), iID, bMapItem);
		return true;
	}

	else if (FStrEq(args[0], "bb2_keypad_unlock_output"))
	{
		if (args.ArgC() != 2)
			return true;

		if (GetTeamNumber() != TEAM_HUMANS)
			return true;

		CBaseEntity *pEnt = UTIL_EntityByIndex(atoi(args[1]));
		if (pEnt)
		{
			CBB2Button *pBtn = dynamic_cast<CBB2Button*> (pEnt);
			if (pBtn)
			{
				float distToButton = GetAbsOrigin().DistTo(pBtn->GetAbsOrigin());
				if (distToButton < 125.0f)
					pBtn->FireKeyPadOutput(this);
			}
		}

		return true;
	}

	else if (FStrEq(args[0], "bb2_skill_progress_human"))
	{
		if ((args.ArgC() != 3) || !HL2MPRules()->CanUseSkills())
			return true;

		bool m_bCanApplyChanges = ((GetTeamNumber() == TEAM_HUMANS) && IsAlive());

		int iSkillType = atoi(args[1]);
		bool bShouldAdd = (atoi(args[2]) >= 1);

		int iDataValue = (int)GetSkillValue(iSkillType, TEAM_HUMANS, true);
		int iSkillValue = GetSkillValue(iSkillType);

		if (bShouldAdd && ((m_BB2Local.m_iSkill_Talents <= 0) || (iSkillValue >= 10)))
			return true;
		else if (!bShouldAdd && (iSkillValue <= 0))
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
				if (bShouldAdd)
				{
					m_iHealth += iDataValue;
					m_iMaxHealth += iDataValue;
				}
				else
				{
					m_iHealth -= iDataValue;
					m_iMaxHealth -= iDataValue;
				}
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

		CSingleUserRecipientFilter filter(this);
		filter.MakeReliable();
		UserMessageBegin(filter, "RunBuyCommand");
		WRITE_BYTE(1);
		MessageEnd();

		return true;
	}

	else if (FStrEq(args[0], "bb2_skill_progress_zombie"))
	{
		if ((args.ArgC() != 3) || !HL2MPRules()->CanUseSkills())
			return true;

		bool m_bCanApplyChanges = ((GetTeamNumber() == TEAM_DECEASED) && IsAlive());

		int iSkillType = atoi(args[1]);
		bool bShouldAdd = (atoi(args[2]) >= 1);

		int iDataValue = (int)GetSkillValue(iSkillType, TEAM_DECEASED, true);
		int iSkillValue = GetSkillValue(iSkillType);

		if (bShouldAdd && ((m_BB2Local.m_iZombieCredits <= 0) || (iSkillValue >= 10)))
			return true;
		else if (!bShouldAdd && (iSkillValue <= 0))
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
				if (bShouldAdd)
				{
					m_iHealth += iDataValue;
					m_iMaxHealth += iDataValue;
				}
				else
				{
					m_iHealth -= iDataValue;
					m_iMaxHealth -= iDataValue;
				}
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

		CSingleUserRecipientFilter filter(this);
		filter.MakeReliable();
		UserMessageBegin(filter, "RunBuyCommand");
		WRITE_BYTE(1);
		MessageEnd();

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

// Switch to the given weapon bucket/slot.
// Slot 0 = Melee
// Slot 1 = Special
// Slot 2 = Secondary Wep
// Slot 3 = Primary Wep
// Slot 7 = HANDS
bool CHL2MP_Player::Weapon_SwitchBySlot(int iSlot, int viewmodelindex)
{
	CBaseCombatWeapon *pWeapon = Weapon_GetSlot(iSlot);
	if (pWeapon != NULL)
	{
		if (GetActiveWeapon())
			Weapon_Switch(pWeapon, false, viewmodelindex);
		else
			Weapon_Switch(pWeapon, true, viewmodelindex);

		return true;
	}

	return false;
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

void CHL2MP_Player::CreateViewModel(int index /*=0*/)
{
	Assert(index >= 0 && index < MAX_VIEWMODELS);

	if (GetViewModel(index))
		return;

	CPredictedViewModel *vm = (CPredictedViewModel *)CreateEntityByName("predicted_viewmodel");
	if (vm)
	{
		vm->SetAbsOrigin(GetAbsOrigin());
		vm->SetOwner(this);
		vm->SetIndex(index);
		DispatchSpawn(vm);
		vm->FollowEntity(this, false);
		m_hViewModel.Set(index, vm);
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
	vecNewVelocity *= 900;

	CPASFilter filter(WorldSpaceCenter());
	te->ClientSideGib(filter, -1, GetModelIndex(), m_nBody, m_nSkin, GetAbsOrigin(), GetAbsAngles(), vecNewVelocity, GetGibFlags(), 0, CLIENT_GIB_RAGDOLL, entindex());
}

int CHL2MP_Player::AllowEntityToBeGibbed(void)
{
	if (IsObserver() || (GetTeamNumber() <= TEAM_SPECTATOR) || (m_iHealth > 0))
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

void CHL2MP_Player::OnGibbedGroup(int hitgroup, bool bExploded)
{
	if (bExploded)
	{
		for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
		{
			int accessoryGroup = FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[i]);
			if (accessoryGroup == -1)
				continue;

			SetBodygroup(accessoryGroup, 0);
		}

		return;
	}

	int iAccessoryGroup = -1;
	if (hitgroup == HITGROUP_HEAD)
		iAccessoryGroup = 0;
	else if (hitgroup == HITGROUP_LEFTLEG)
		iAccessoryGroup = 2;
	else if (hitgroup == HITGROUP_RIGHTLEG)
		iAccessoryGroup = 3;

	if (iAccessoryGroup == -1 || (iAccessoryGroup < 0) || (iAccessoryGroup >= PLAYER_ACCESSORY_MAX))
		return;

	int accessoryGroup = FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[iAccessoryGroup]);
	if (accessoryGroup == -1)
		return;

	SetBodygroup(accessoryGroup, 0);
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
			if ((pAttacker != this) && IsZombie())
			{
				m_iZombDeaths++;
				CheckCanRespawnAsHuman();
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
					pEnemyTeam->AddScore(bb2_elimination_score_zombies.GetInt());
				else if (IsZombie() && pAttacker->IsHuman())
					pEnemyTeam->AddScore(bb2_elimination_score_humans.GetInt());
			}
		}
	}

	ExecuteClientEffect(PLAYER_EFFECT_ZOMBIE_FLASHLIGHT, 0);
	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects(EF_NODRAW);	// still draw player body

	color32 darkred = { 53, 0, 0, 90 };
	UTIL_ScreenFade(this, darkred, 1.0f, 5.0f, FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT);

	GameBaseShared()->GetAchievementManager()->WriteToStat(this, "BBX_ST_DEATHS");

	ExecuteClientEffect(PLAYER_EFFECT_DEATH, 1);
}

int CHL2MP_Player::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	CTakeDamageInfo damageCopy = inputInfo;

	// Spawn Protection:
	if (gpGlobals->curtime < m_flSpawnProtection)
		return 0;

	CBaseEntity *pAttacker = damageCopy.GetAttacker();
	if (pAttacker)
	{
		float flDamageScale = GameBaseServer()->GetDamageScaleForEntity(pAttacker, this);
		damageCopy.ScaleDamage(flDamageScale);
	}

	m_vecTotalBulletForce += damageCopy.GetDamageForce();

	gamestats->Event_PlayerDamage(this, damageCopy);

	int ret = BaseClass::OnTakeDamage(damageCopy);
	if (ret)
	{
		if (IsHuman() && m_BB2Local.m_iActiveArmorType && ((LastHitGroup() == HITGROUP_CHEST) || (LastHitGroup() == HITGROUP_GENERIC) || (LastHitGroup() == HITGROUP_STOMACH)))
			EmitSound("Player.ArmorImpact");
	}

	return ret;
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
	CBaseEntity *pFinalSpawnPoint = NULL;
	const char *pSpawnpointName = "info_start_camera";
	int iAvailablePoints = 0;
	if (GetTeamNumber() == TEAM_HUMANS)
		pSpawnpointName = "info_player_human";
	else if (GetTeamNumber() == TEAM_DECEASED)
		pSpawnpointName = "info_player_zombie";

	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, pSpawnpointName);
	while (pEntity)
	{
		if (strcmp(pSpawnpointName, "info_start_camera"))
		{
			CBaseSpawnPoint *pPoint = dynamic_cast<CBaseSpawnPoint*> (pEntity);
			if (pPoint)
			{
				if (pPoint->IsEnabled())
				{
					if (g_pGameRules->IsSpawnPointValid(pEntity, this))
						iAvailablePoints++;
				}
			}
		}
		else
			iAvailablePoints++;

		pEntity = gEntList.FindEntityByClassname(pEntity, pSpawnpointName);
	}

	int iChoosenSpawn = random->RandomInt(1, iAvailablePoints);
	iAvailablePoints = 0;

	pEntity = gEntList.FindEntityByClassname(NULL, pSpawnpointName);
	while (pEntity)
	{
		if (strcmp(pSpawnpointName, "info_start_camera"))
		{
			CBaseSpawnPoint *pPoint = dynamic_cast<CBaseSpawnPoint*> (pEntity);
			if (pPoint)
			{
				if (pPoint->IsEnabled())
				{
					if (g_pGameRules->IsSpawnPointValid(pEntity, this))
					{
						iAvailablePoints++;
						if (iChoosenSpawn == iAvailablePoints)
						{
							pFinalSpawnPoint = pEntity;
							break;
						}
					}
				}
			}
		}
		else
		{
			iAvailablePoints++;
			if (iChoosenSpawn == iAvailablePoints)
			{
				pFinalSpawnPoint = pEntity;
				break;
			}
		}

		pEntity = gEntList.FindEntityByClassname(pEntity, pSpawnpointName);
	}

	iAvailablePoints = 0;

	if (!pFinalSpawnPoint)
	{
		pEntity = gEntList.FindEntityByClassname(NULL, pSpawnpointName);
		while (pEntity)
		{
			if (strcmp(pSpawnpointName, "info_start_camera"))
			{
				CBaseSpawnPoint *pPoint = dynamic_cast<CBaseSpawnPoint*> (pEntity);
				if (pPoint)
				{
					if (pPoint->IsEnabled())
					{
						iAvailablePoints++;
						if (iChoosenSpawn == iAvailablePoints)
						{
							pFinalSpawnPoint = pEntity;
							break;
						}
					}
				}
			}

			pEntity = gEntList.FindEntityByClassname(pEntity, pSpawnpointName);
		}

		if (!pFinalSpawnPoint)
		{
			Warning("No available spawn point!\nForcing 0,0,0!\n");
			pFinalSpawnPoint = GetWorldEntity();
		}
	}

	m_flSpawnProtection = gpGlobals->curtime + bb2_spawn_protection.GetFloat();
	m_flZombieVisionLockTime = gpGlobals->curtime + 0.5f;

	//// We use 'dynamic' spawns in story mode:
	//if (GameBaseServer()->IsStoryMode() && (GetTeamNumber() >= TEAM_HUMANS) && pFinalSpawnPoint)
	//{
	//	CBasePlayer *pDistantPlayer = UTIL_GetMostDistantPlayer(this, pFinalSpawnPoint->GetAbsOrigin());
	//	if (pDistantPlayer)
	//		return pDistantPlayer;
	//}

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
}

void CHL2MP_Player::CheckCanRespawnAsHuman()
{
	if ((HL2MPRules() && !HL2MPRules()->CanUseSkills()) || GameBaseServer()->IsTutorialModeEnabled())
		return;

	if ((m_iZombKills >= 3) || (bb2_allow_mercy.GetInt() && (m_iZombDeaths >= bb2_allow_mercy.GetInt())))
		m_BB2Local.m_bCanRespawnAsHuman = true;
}

void CHL2MP_Player::CheckCanRage()
{
	if ((HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)) || !GameBaseShared()->GetSharedGameDetails() || GetPerkFlags())
		return;

	m_iZombCurrentKills++;
	if (m_iZombCurrentKills >= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData().iZombieKillsRequiredToRage)
	{
		m_iZombCurrentKills = 0;
		EnterRageMode(true);
	}
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

	const char *pReadyCheck = p;

	HL2MPRules()->CheckChatForReadySignal(this, pReadyCheck);
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

void CHL2MP_Player::HandleAnimEvent(animevent_t *pEvent)
{
	BaseClass::HandleAnimEvent(pEvent);
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
Vector CHL2MP_Player::GetAutoaimVector(float flScale)
{
	//No Autoaim
	Vector forward;
	AngleVectors(EyeAngles() + m_Local.m_vecPunchAngle, &forward);
	return forward;
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void CHL2MP_Player::SetAnimation(PLAYER_ANIM playerAnim)
{
	return;
}

// BB2 SKILL SETUP, THIS IS HOW WE FIGURE OUT WHAT SPEED OFFSET(S) WE SHOULD HAVE AT ALL TIMES!!!
float CHL2MP_Player::GetTeamPerkValue(float flOriginalValue)
{
	float flNewValue = flOriginalValue;
	CTeam *pMyTeam = GetGlobalTeam(GetTeamNumber());
	if (pMyTeam)
	{
		switch (pMyTeam->GetActivePerk())
		{
		case TEAM_DECEASED_PERK_INCREASED_DAMAGE:
		{
			flNewValue *= 2;
			return flNewValue;
		}
		}
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

	if (iCurrentValue < 0)
		iCurrentValue = 0;
	else if (iCurrentValue > 10)
		iCurrentValue = 10;

	m_BB2Local.m_iPlayerSkills.Set(skillType, iCurrentValue);
}

void CHL2MP_Player::SetSkillValue(int skillType, int value)
{
	if ((skillType < 0) || (skillType >= PLAYER_SKILL_END))
		return;

	m_BB2Local.m_iPlayerSkills.Set(skillType, value);
}

int CHL2MP_Player::GetSkillValue(int index)
{
	if ((index < 0) || (index >= PLAYER_SKILL_END))
		return 0;

	return m_BB2Local.m_iPlayerSkills.Get(index);
}

float CHL2MP_Player::GetSkillValue(int skillType, int team, bool bDataValueOnly, int dataSubType)
{
	if (bDataValueOnly)
		return GameBaseShared()->GetSharedGameDetails()->GetPlayerSkillValue(skillType, team, dataSubType);

	return (GameBaseShared()->GetSharedGameDetails()->GetPlayerSkillValue(skillType, team, dataSubType) * (float)GetSkillValue(skillType));
}

float CHL2MP_Player::GetSkillValue(const char *pszType, int skillType, int team, int dataSubType)
{
	float flDefaultValue = GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedValue(pszType, team);
	if (flDefaultValue <= 0.0f)
		return (GetSkillValue(skillType, team, false, dataSubType));

	float flReturn = (flDefaultValue + ((flDefaultValue / 100) * ((float)GetSkillValue(skillType) * GameBaseShared()->GetSharedGameDetails()->GetPlayerSkillValue(skillType, team, dataSubType))));
	return flReturn;
}

float CHL2MP_Player::GetSkillCombination(int skillDefault, int skillExtra)
{
	float flDefault = (float)skillDefault;
	return (flDefault + ((flDefault / 100) * skillExtra));
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

	float flMultiplier = ((flDefaultDamage / 100) * flExtraDamagePercent);
	return flMultiplier;
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
		SetMaxSpeed((flSpeed / 2.0f));
	else
		SetMaxSpeed(flSpeed);

	// Update Anim State
	m_PlayerAnimState->SetRunSpeed(flSpeed);
	m_PlayerAnimState->SetWalkSpeed((flSpeed / 2.0f));
}

bool CHL2MP_Player::HandleLocalProfile(bool bSave)
{
	if (GameBaseServer()->CanStoreSkills() != PROFILE_LOCAL)
		return false;

	char pszFileName[64];
	Q_snprintf(pszFileName, 64, "%llu", (unsigned long long)GetSteamIDAsUInt64());

	char pszFilePath[80];
	Q_snprintf(pszFilePath, 80, "data/local/%s.txt", pszFileName);

	// Save your local profile:
	if (bSave)
	{
		if (m_bHasReadProfileData)
		{
			FileHandle_t localProfile = g_pFullFileSystem->Open(pszFilePath, "w");
			if (localProfile != FILESYSTEM_INVALID_HANDLE)
			{
				char pszFileContent[1024];
				Q_snprintf(pszFileContent, 1024,
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
					m_BB2Local.m_iSkill_XPCurrent,
					m_BB2Local.m_iSkill_XPLeft,
					m_BB2Local.m_iSkill_Talents,
					m_BB2Local.m_iZombieCredits,

					GetSkillValue(0),
					GetSkillValue(1),
					GetSkillValue(2),
					GetSkillValue(3),
					GetSkillValue(4),
					GetSkillValue(5),
					GetSkillValue(6),
					GetSkillValue(7),
					GetSkillValue(8),
					GetSkillValue(9),

					GetSkillValue(10),
					GetSkillValue(11),
					GetSkillValue(12),
					GetSkillValue(13),
					GetSkillValue(14),
					GetSkillValue(15),
					GetSkillValue(16),
					GetSkillValue(17),
					GetSkillValue(18),
					GetSkillValue(19),

					GetSkillValue(20),
					GetSkillValue(21),
					GetSkillValue(22),
					GetSkillValue(23),
					GetSkillValue(24),
					GetSkillValue(25),
					GetSkillValue(26),
					GetSkillValue(27),
					GetSkillValue(28),
					GetSkillValue(29),

					GetSkillValue(30),
					GetSkillValue(31),
					GetSkillValue(32),
					GetSkillValue(33),
					GetSkillValue(34),
					GetSkillValue(35),
					GetSkillValue(36),
					GetSkillValue(37),
					GetSkillValue(38),
					GetSkillValue(39)
					);

				g_pFullFileSystem->Write(&pszFileContent, strlen(pszFileContent), localProfile);
				g_pFullFileSystem->Close(localProfile);
				return true;
			}
		}

		return false;
	}
	
	// Load your local profile:
	m_bHasReadProfileData = true;
	m_bHasTriedToLoadStats = true;

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
		m_BB2Local.m_iSkill_XPLeft = pkvProfile->GetInt("XPLeft", 5);
		m_BB2Local.m_iSkill_Talents = pkvProfile->GetInt("Talents");
		m_BB2Local.m_iZombieCredits = pkvProfile->GetInt("ZombiePoints");

		int indexIter = 0;
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
		ApplyArmor(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).iArmor, GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).iArmorType);

		if (HL2MPRules()->CanUseSkills())
		{
			int health = round(GetSkillValue("Health", PLAYER_SKILL_ZOMBIE_HEALTH, TEAM_DECEASED));
			SetHealth(health);
			SetMaxHealth(health);
			SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_ZOMBIE_SPEED, TEAM_DECEASED));
			SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_ZOMBIE_LEAP, TEAM_DECEASED));
			SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_ZOMBIE_JUMP, TEAM_DECEASED));
			SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_ZOMBIE_HEALTH_REGEN, TEAM_DECEASED));
		}
		else
		{
			SetHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).iHealth);
			SetMaxHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).iHealth);
			SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flSpeed);
			SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flLeapLength);
			SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flJumpHeight);
			SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_DECEASED).flHealthRegenerationRate);
		}

		GiveItem("weapon_zombhands");

		RefreshSpeed();

		if ((m_BB2Local.m_iZombieCredits >= 5) && !GameBaseServer()->IsTutorialModeEnabled() && HL2MPRules()->CanUseSkills())
			GameBaseServer()->SendToolTip("#TOOLTIP_SPEND_TALENTS_HINT", 0, this->entindex());
	}
}

void CHL2MP_Player::SetHuman()
{
	SetCollisionGroup(COLLISION_GROUP_PLAYER);

	if (IsAlive())
	{
		m_BB2Local.m_bHasPlayerEscaped = false;
		m_BB2Local.m_bCanRespawnAsHuman = false;

		SetPlayerModel(TEAM_HUMANS);
		ApplyArmor(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iArmor, GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iArmorType);

		if (HL2MPRules()->CanUseSkills())
		{
			int health = round(GetSkillValue("Health", PLAYER_SKILL_HUMAN_HEALTH, TEAM_HUMANS));
			SetHealth(health);
			SetMaxHealth(health);
			SetPlayerSpeed(GetSkillValue("Speed", PLAYER_SKILL_HUMAN_SPEED, TEAM_HUMANS));
			SetLeapLength(GetSkillValue("Leap", PLAYER_SKILL_HUMAN_ACROBATICS, TEAM_HUMANS, PLAYER_SKILL_HUMAN_LEAP));
			SetJumpHeight(GetSkillValue("Jump", PLAYER_SKILL_HUMAN_ACROBATICS, TEAM_HUMANS, PLAYER_SKILL_HUMAN_JUMP));
			SetHealthRegenAmount(GetSkillValue("HealthRegen", PLAYER_SKILL_HUMAN_HEALTHREGEN, TEAM_HUMANS));
		}
		else // Load non-skill affected values in DM mode...
		{
			SetHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iHealth);
			SetMaxHealth(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).iHealth);
			SetPlayerSpeed(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flSpeed);
			SetLeapLength(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flLeapLength);
			SetJumpHeight(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flJumpHeight);
			SetHealthRegenAmount(GameBaseShared()->GetSharedGameDetails()->GetPlayerGameModeData(TEAM_HUMANS).flHealthRegenerationRate);
		}

		GiveItem("weapon_hands");

		RefreshSpeed();

		if (m_pPlayerEquipper)
			m_pPlayerEquipper->EquipPlayer(this);

		if ((m_BB2Local.m_iSkill_Talents >= 5) && !GameBaseServer()->IsTutorialModeEnabled() && HL2MPRules()->CanUseSkills())
			GameBaseServer()->SendToolTip("#TOOLTIP_SPEND_TALENTS_HINT", 0, this->entindex());

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
	m_BB2Local.m_flCarryWeight = 0;
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

	m_iZombCurrentKills = 0;
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
};

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlayerAnimEvent, DT_TEPlayerAnimEvent)
SendPropEHandle(SENDINFO(m_hPlayer)),
SendPropInt(SENDINFO(m_iEvent), Q_log2(PLAYERANIMEVENT_COUNT) + 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_nData), 32)
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent("PlayerAnimEvent");

void TE_PlayerAnimEvent(CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData)
{
	CPVSFilter filter((const Vector&)pPlayer->EyePosition());

	//Tony; use prediction rules.
	filter.UsePredictionRules();

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create(filter, 0);
}

void CHL2MP_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
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

	if (bShouldDisable && ((HL2MPRules()->GetCurrentGamemode() == MODE_DEATHMATCH) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)))
		RemoveSpawnProtection();

	m_PlayerAnimState->DoAnimationEvent(event, nData);
	TE_PlayerAnimEvent(this, event, nData);	// Send to any clients who can see this guy.
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

	if ((HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) || GameBaseServer()->IsStoryMode())
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

	GameBaseServer()->SendToolTip(randomFun[random->RandomInt(0, (_ARRAYSIZE(randomFun) - 1))], 0, pClient->entindex());
}

CON_COMMAND(classic_respawn_ashuman, "Respawn as a human if possible!")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if ((HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) || GameBaseServer()->IsStoryMode() || GameBaseServer()->IsTutorialModeEnabled())
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

	GameBaseServer()->SendToolTip(randomFun[random->RandomInt(0, (_ARRAYSIZE(randomFun) - 1))], 0, pClient->entindex());
}

CON_COMMAND(skill_tree, "Player Skill Tree")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (!HL2MPRules()->CanUseSkills())
		return;

	pClient->ShowViewPortPanel("skills", true);
}

CON_COMMAND(zombie_tree, "Zombie Skill Tree")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (!HL2MPRules()->CanUseSkills())
		return;

	if ((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) || GameBaseServer()->IsStoryMode())
		return;

	pClient->ShowViewPortPanel("zombie", true);
}

CON_COMMAND(team_menu, "Team Selection Menu")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)
		return;

	pClient->ShowViewPortPanel("team", true);
}

CON_COMMAND(voice_menu, "Voice Command Menu")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (!pClient->IsHuman() || !pClient->IsAlive() || !HL2MPRules()->IsTeamplay())
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
	if (!pClient)
		return;

	if (!pClient->IsHuman())
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

CON_COMMAND_F(bb2_get_brush_bounds, "Get brush bounds of any brush in front of you.", FCVAR_CHEAT)
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	Vector forward;
	trace_t tr;

	pClient->EyeVectors(&forward);
	Vector start = pClient->EyePosition();
	UTIL_TraceLine(start, start + forward * MAX_COORD_RANGE, MASK_SOLID, pClient, COLLISION_GROUP_NONE, &tr);

	CBaseEntity *pEnt = tr.m_pEnt;
	if (!pEnt)
		return;

	Msg("Entity: %s\nBounds: %f %f %f\n", pEnt->GetClassname(), pEnt->CollisionProp()->OBBSize().x, pEnt->CollisionProp()->OBBSize().y, pEnt->CollisionProp()->OBBSize().z);
}
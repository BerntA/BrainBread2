//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2MP_PLAYER_H
#define HL2MP_PLAYER_H
#pragma once

class CHL2MP_Player;
class CLogicPlayerEquipper;

#include "basemultiplayerplayer.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "hl2mp_playeranimstate.h"
#include "hl2mp_player_shared.h"
#include "hl2mp_gamerules.h"
#include "utldict.h"
#include "skills_shareddefs.h"
#include "bb2_playerlocaldata.h"

struct WeaponPickupItem_t
{
	char szWeaponClassName[MAX_WEAPON_STRING];
	float m_flPenaltyTime;
};

//=============================================================================
// >> HL2MP_Player
//=============================================================================
class CHL2MPPlayerStateInfo
{
public:
	HL2MPPlayerState m_iPlayerState;
	const char *m_pStateName;

	void (CHL2MP_Player::*pfnEnterState)();	// Init and deinit the state.
	void (CHL2MP_Player::*pfnLeaveState)();

	void (CHL2MP_Player::*pfnPreThink)();	// Do a PreThink() in this state.
};

class CHL2MP_Player : public CHL2_Player
{
	enum
	{
		Human = 0,
		Zombie,
		Default,
	};

public:

	// BB2 Class Sys.
	void SetPlayerClass(int iTeam);
	void SetZombie();
	void SetHuman();
	void ResetZombieSkills(void);
	void RefreshSpeed(void);
	bool HandleLocalProfile(bool bSave = false);

public:
	DECLARE_CLASS(CHL2MP_Player, CHL2_Player);

	CHL2MP_Player();
	~CHL2MP_Player(void);

	static CHL2MP_Player *CreatePlayer(const char *className, edict_t *ed)
	{
		CHL2MP_Player::s_PlayerEdict = ed;
		return (CHL2MP_Player*)CreateEntityByName(className);
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// This passes the event to the client's and server's CHL2MPPlayerAnimState.
	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);
	void SetupBones(matrix3x4_t *pBoneToWorld, int boneMask);

	// BB2 Weapon stuff
	bool Weapon_SwitchBySlot(int iSlot, int viewmodelindex = 0);
	void DropAllWeapons(void);

	// Zombie Stored Kills Purpose: Since every zombie needs to be an individual we store a kill amount to respawn the zombie as a human after the death with the X score!
	int m_iZombKills;
	// To make use of bb2_allow_mercy we store the death count, if it exceeds the mercy value we allow you to spawn as a human because the humans are too OP.
	int m_iZombDeaths;

	void SetRespawnTime(float flTime) { m_BB2Local.m_flPlayerRespawnTime = flTime; }
	float GetRespawnTime(void) { return m_BB2Local.m_flPlayerRespawnTime; }
	bool HasFullySpawned(void) { return m_bHasFullySpawned; }
	bool HasJoinedGame(void) { return m_bHasJoinedGame; }
	bool HasLoadedStats(void) { return m_bHasTriedToLoadStats; }

	float GetTeamPerkValue(float flOriginalValue);

	void SetSkillValue(int skillType, bool bDecrement = false);
	void SetSkillValue(int skillType, int value);
	int GetSkillValue(int index);
	float GetSkillValue(int skillType, int team, bool bDataValueOnly = false, int dataSubType = -1);
	float GetSkillValue(const char *pszType, int skillType, int team = 2, int dataSubType = -1);
	float GetSkillCombination(int skillDefault, int skillExtra);
	float GetSkillWeaponDamage(float flDefaultDamage, float dmgFactor, int weaponType);

	// JoinTeam Fix
	bool m_bWantsToDeployAsHuman;

	// Profile System Helper:
	bool m_bHasReadProfileData;
	bool m_bHasTriedToLoadStats;

	// The total weight of items and weapons combined.
	void ApplyArmor(float flArmorDurability, int iType = 0);

	// Skill Perk 
	int m_iNumPerkKills;

	// DM - Announcer
	int m_iDMKills;
	float m_flDMTimeSinceLastKill;
	EHANDLE m_hLastKiller;

	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void PostThink(void);
	virtual void PreThink(void);
	virtual void PlayerDeathThink(void);
	virtual bool HandleCommand_JoinTeam(int team, bool bInfection = false);
	virtual bool ClientCommand(const CCommand &args);
	virtual void CreateViewModel(int viewmodelindex = 0);
	virtual bool BecomeRagdollOnClient(const Vector &force);
	virtual void Event_Killed(const CTakeDamageInfo &info);
	virtual int OnTakeDamage(const CTakeDamageInfo &inputInfo);
	virtual void InitialSpawn(void);
	virtual bool IsValidObserverTarget(CBaseEntity * target); 
	virtual void OnZombieInfectionComplete(void);

	void OnUpdateInfected(void);
	void SharedPostThinkHL2MP(void);
	bool IsSliding(void) const;

	// Movement:
	virtual float GetPlayerSpeed();
	virtual float GetLeapLength();
	virtual float GetJumpHeight();

	virtual const Vector GetPlayerMins(void) const;
	virtual const Vector GetPlayerMaxs(void) const;

	void SetPlayerSpeed(float flSpeed) { m_BB2Local.m_flPlayerSpeed = flSpeed; }
	void SetLeapLength(float flLength) { m_BB2Local.m_flLeapLength = flLength; }
	void SetJumpHeight(float flHeight) { m_BB2Local.m_flJumpHeight = flHeight; }

	bool HasPlayerEscaped(void) { return m_BB2Local.m_bHasPlayerEscaped; }
	void SetPlayerEscaped(bool value) { m_BB2Local.m_bHasPlayerEscaped = value; }

	virtual Class_T Classify(void);

	virtual bool WantsLagCompensationOnEntity(const CBaseEntity *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const;

	virtual void FireBullets(const FireBulletsInfo_t &info);
	virtual bool Weapon_Switch(CBaseCombatWeapon *pWeapon, bool bWantDraw = false, int viewmodelindex = 0);
	virtual bool BumpWeapon(CBaseCombatWeapon *pWeapon);
	virtual bool EquipAmmoFromWeapon(CBaseCombatWeapon *pWeapon);
	virtual CBaseCombatWeapon *GetBestWeapon();
	virtual void Weapon_Equip(CBaseCombatWeapon *pWeapon);
	virtual void ChangeTeam(int iTeam, bool bInfection = false);
	virtual void PickupObject(CBaseEntity *pObject, bool bLimitMassAndSize);
	virtual void PlayStepSound(Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force);
	virtual void Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL);
	virtual void UpdateOnRemove(void);
	virtual void DeathSound(const CTakeDamageInfo &info);
	virtual void MeleeSwingSound(bool bBash = false);
	virtual CBaseEntity* EntSelectSpawnPoint(void);

	int FlashlightIsOn(void);
	void FlashlightTurnOn(void);
	void FlashlightTurnOff(void);
	void RemoveSpawnProtection(void);

	// Perform various updates at a reasonable interval.
	void PerformPlayerUpdate(void);
	void DispatchDamageText(CBaseEntity *pVictim, int damage);

	bool GiveItem(const char *szItemName, bool bDoLevelCheck = false);
	bool CanLevelUp(int iXP, CBaseEntity *pVictim);
	bool PerformLevelUp(int iXP);
	bool ActivatePerk(int skill);
	bool EnterRageMode(bool bForce = false);
	float GetExtraPerkData(int type);

	Vector GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL);
	virtual Vector GetAutoaimVector(float flDelta);

	void CheatImpulseCommands(int iImpulse);
	void CreateRagdollEntity(void);

	void NoteWeaponFired(void);

	void SetAnimation(PLAYER_ANIM playerAnim);
	void SetPlayerModel(int overrideTeam = -1);
	void SetupCustomization(void);

	void  HandleFirstTimeConnection(bool bForceDefault = false);
	void  OnLateStatsLoadEnterGame(void);
	void  PickDefaultSpawnTeam(int iForceTeam = 0);

	bool LoadGlobalStats(void);
	bool SaveGlobalStatsForPlayer(void);

	int AllowEntityToBeGibbed(void);
	void OnGibbedGroup(int hitgroup, bool bExploded);

	void Reset();
	void ResetSlideVars();

	void CheckCanRespawnAsHuman();
	void CheckCanRage();

	void CheckChatText(char *p, int bufsize);

	void State_Transition(HL2MPPlayerState newState);
	void State_Enter(HL2MPPlayerState newState);
	void State_Leave();
	void State_PreThink();
	CHL2MPPlayerStateInfo *State_LookupInfo(HL2MPPlayerState state);

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	virtual bool StartObserverMode(int mode);
	virtual void StopObserverMode(void);

	bool CanEnablePowerup(int powerupFlag, float duration);
	void ResetPerksAndPowerups(void);
	void RemovePowerups(void);

	Vector m_vecTotalBulletForce;	//Accumulator for bullet force in a single frame

	virtual bool	CanHearAndReadChatFrom(CBasePlayer *pPlayer);

	CNetworkVarEmbedded(CBB2PlayerLocalData, m_BB2Local);

	// Player Shared Info
	int GetSelectedTeam(void) { return m_iSelectedTeam; }
	void SetSelectedTeam(int value) { m_iSelectedTeam = value; }

	bool IsGroupIDFlagActive(int nFlag) { return (m_nGroupID & nFlag) != 0; }
	void AddGroupIDFlag(int nFlag) { m_nGroupID |= nFlag; }
	int GetGroupIDFlags(void) { return m_nGroupID; }

	bool IsPerkFlagActive(int nFlag) { return (m_nPerkFlags & nFlag) != 0; }
	void AddPerkFlag(int nFlag) { m_nPerkFlags |= nFlag; }
	int GetPerkFlags(void) { return m_nPerkFlags; }
	void ClearPerkFlags(void) { m_nPerkFlags = 0; }

	void SetPlayerSlideState(bool value) { m_bIsInSlide = value; }

	bool IsPlayerInfected(void) { return m_bIsInfected; }
	void SetPlayerInfected(bool value) { m_bIsInfected = value; }

	int GetPlayerLevel(void) { return m_iSkill_Level; }
	void SetPlayerLevel(int level) { m_iSkill_Level = level; }

	int GetTotalScore(void){ return m_iTotalScore; }
	int GetTotalDeaths(void) { return m_iTotalDeaths; }

	int GetRoundScore(void){ return m_iRoundScore; }
	int GetRoundDeaths(void) { return m_iRoundDeaths; }

	void IncrementTotalScore(void) { m_iTotalScore++; }
	void IncrementRoundScore(void) { m_iRoundScore++; }

	void IncrementTotalDeaths(void) { m_iTotalDeaths++; }
	void IncrementRoundDeaths(void) { m_iRoundDeaths++; }

	virtual void HandleAnimEvent(animevent_t *pEvent);

	void SetAdminStatus(bool value) { m_bIsServerAdmin = value; }
	bool IsAdminOnServer(void) { return m_bIsServerAdmin; }

	bool HasPlayerUsedFirearm(void) { return m_bPlayerUsedFirearm; }

	void AddAssociatedAmmoEnt(CBaseEntity *pEnt);
	void CleanupAssociatedAmmoEntities(void);

	void OnDroppedAmmoNow(void) { m_flLastTimeDroppedAmmo = gpGlobals->curtime; }
	float GetLastTimeDroppedAmmo(void) { return m_flLastTimeDroppedAmmo; }

	void CheckShouldEnableFlashlightOnSwitch(void);

	static bool IsWeaponEquippedByDefault(const char *weaponName);

private:

	// Player Shared Info
	int m_iSkill_Level;
	int m_nGroupID;
	int m_iTotalScore;
	int m_iTotalDeaths;
	int m_iRoundScore;
	int m_iRoundDeaths;
	int m_iSelectedTeam;
	bool m_bIsInfected;
	float m_flLastInfectionTwitchTime;

	CHL2MPPlayerAnimState *m_PlayerAnimState;
	CNetworkQAngle(m_angEyeAngles);

	int m_iLastWeaponFireUsercmd;
	int m_iModelType;
	CNetworkVar(int, m_iSpawnInterpCounter);
	CNetworkVar(int, m_nPerkFlags);
	CNetworkVar(bool, m_bIsInSlide);

	float m_flSpawnProtection;
	float m_flZombieVisionLockTime;
	float m_flUpdateTime;
	float m_flLastTimeDroppedAmmo;
	bool m_bHasFullySpawned;
	bool m_bHasJoinedGame;
	bool m_bEnableFlashlighOnSwitch;

	bool m_bIsServerAdmin;

	// Achievements:
	bool m_bPlayerUsedFirearm;

	// Weapon Pickup Fixes
	CUtlVector<WeaponPickupItem_t> pszWeaponPenaltyList;
	CUtlVector<EHANDLE> m_pAssociatedAmmoEntities;

	HL2MPPlayerState m_iPlayerState;
	CHL2MPPlayerStateInfo *m_pCurStateInfo;

	bool ShouldRunRateLimitedCommand(const CCommand &args);

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float, int>	m_RateLimitLastCommandTimes;

	bool m_bEnterObserver;

	// High Ping Kicker:
	void DoHighPingCheck(void);
	int m_iTotalPing;
	int m_iTimesCheckedPing;
	float m_flLastTimeCheckedPing;
	float m_flTimeUntilEndCheckPing;
	bool m_bFinishedPingCheck;

	// Zombie
	// Monitor amount of kills since spawn, allow auto rage when X kills have been reached in certain gamemodes.
	int m_iZombCurrentKills;

protected:
	virtual void HandlePainSound(int iMajor, int iDamageTypeBits);
	virtual void DoPlayerKick(void);

	void OnReceiveStatsForPlayer(GSStatsReceived_t *pCallback, bool bIOFailure);
	CCallResult<CHL2MP_Player, GSStatsReceived_t> m_SteamCallResultRequestPlayerStats;
};

inline CHL2MP_Player *ToHL2MPPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<CHL2MP_Player*>(pEntity);
}

#endif //HL2MP_PLAYER_H

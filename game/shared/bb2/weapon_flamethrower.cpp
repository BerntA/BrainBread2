//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Flamethrower - Oh yeaaah!
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "GameBase_Shared.h"

#ifdef CLIENT_DLL
#include "c_bb2_player_shared.h"
#include "input.h"
#else
#include "ilagcompensationmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponFlamethrower C_WeaponFlamethrower
#endif

enum FlamethrowerFXFlags
{
	FLAMETHROWER_RENDER_TINY_FLAME = 0x01,
	FLAMETHROWER_RENDER_LARGE_FLAME = 0x02,
};

// MIN VALUES ARE 0.0F!

#define POSEPARAM_FIRE_SEQUENCE "blend_attack"
#define POSEPARAM_FIRE_SEQUENCE_MAX 1.0f

#define POSEPARAM_GAUGE_SEQUENCE "blend_pressure"
#define POSEPARAM_GAUGE_SEQUENCE_MAX 2.0f

class CWeaponFlamethrower : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponFlamethrower, CBaseHL2MPCombatWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponFlamethrower();

	int GetMinBurst() { return 1; }
	int GetMaxBurst() { return 1; }
	float GetMinRestTime() { return 0; }
	float GetMaxRestTime() { return 0; }

	int GetUniqueWeaponID() { return WEAPON_ID_FLAMETHROWER; }
	int GetWeaponType(void) { return WEAPON_TYPE_SPECIAL; }
	float GetFireRate(void) { return GetWpnData().m_flFireRate; }
	float GetRange(void) { return ((float)GetWpnData().m_iRangeMax); }

	bool Reload(void);
	void PrimaryAttack(CBasePlayer *pOwner, CBaseViewModel *pVM, float fraction);
	void PrimaryAttack(void) { }
	void SecondaryAttack(void) { }
	void ItemPostFrame(void);

	void StartHolsterSequence();
	bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void Drop(const Vector &vecVelocity);
	void UpdateOnRemove(void);

	const char *GetAmmoEntityLink(void) { return "ammo_cannister"; }

#ifdef CLIENT_DLL
	void ClientThink(void);
	void OnDataChanged(DataUpdateType_t updateType);
#endif

protected:
	float GetChargeFraction(void) 
	{
		return (clamp(((gpGlobals->curtime - m_flAttackedTime) / m_flGrowTime), 0.0f, (m_flGrowTime / GetWpnData().m_flWeaponChargeTime)));
	}

private:
	CWeaponFlamethrower(const CWeaponFlamethrower &);

	CNetworkVar(int, m_nEffectState);
	CNetworkVar(float, m_flAttackedTime);
	CNetworkVar(float, m_flGrowTime); 
	CNetworkVar(float, m_flLastChargeFraction);

	int m_iPoseParamFire, m_iPoseParamGauge;
	float m_flSoundTimer;

#ifdef CLIENT_DLL
	void CreateEffects(void);
	void CleanupEffects(bool bNoFade = false);
	void RemoveParticleEffect(CNewParticleEffect *effect, bool bNoFade = false);

	CNewParticleEffect *m_pParticleFireSmall;
	CNewParticleEffect *m_pParticleFireLarge;

	int m_nClientEffectFlags;
#else
	float GetActualDamage(void);
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponFlamethrower, DT_WeaponFlamethrower)

BEGIN_NETWORK_TABLE(CWeaponFlamethrower, DT_WeaponFlamethrower)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_nEffectState)),
RecvPropTime(RECVINFO(m_flAttackedTime)),
RecvPropTime(RECVINFO(m_flGrowTime)),
RecvPropTime(RECVINFO(m_flLastChargeFraction)),
#else
SendPropInt(SENDINFO(m_nEffectState), 2, SPROP_UNSIGNED),
SendPropTime(SENDINFO(m_flAttackedTime)),
SendPropTime(SENDINFO(m_flGrowTime)),
SendPropTime(SENDINFO(m_flLastChargeFraction)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponFlamethrower)
DEFINE_PRED_FIELD(m_nEffectState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD_TOL(m_flAttackedTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
DEFINE_PRED_FIELD_TOL(m_flGrowTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
DEFINE_PRED_FIELD_TOL(m_flLastChargeFraction, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_flamethrower, CWeaponFlamethrower);
PRECACHE_WEAPON_REGISTER(weapon_flamethrower);

acttable_t CWeaponFlamethrower::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_MINIGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_MINIGUN, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_MINIGUN, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_MINIGUN, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_MINIGUN, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_MINIGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_MINIGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MINIGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MINIGUN, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_MINIGUN, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_MINIGUN, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_MINIGUN, false },

#ifndef CLIENT_DLL
	// HL2
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_MINIGUN, true },

	{ ACT_IDLE, ACT_IDLE_MINIGUN, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY, ACT_IDLE_MINIGUN, true },		// FIXME: hook to AR2 unique
	{ ACT_WALK, ACT_WALK_MINIGUN, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_MINIGUN, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_MINIGUN, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_MINIGUN, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_MINIGUN, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_MINIGUN, false },
	{ ACT_WALK_AGITATED, ACT_WALK_MINIGUN, false },//always aims

	{ ACT_RUN_RELAXED, ACT_WALK_MINIGUN, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_WALK_MINIGUN, false },
	{ ACT_RUN_AGITATED, ACT_WALK_MINIGUN, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_MINIGUN, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_MINIGUN, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_MINIGUN, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_MINIGUN, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_MINIGUN, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_MINIGUN, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_WALK_MINIGUN, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_WALK_MINIGUN, false },
	{ ACT_RUN_AIM_AGITATED, ACT_WALK_MINIGUN, false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_MINIGUN, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_MINIGUN, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_MINIGUN, true },
	{ ACT_RUN, ACT_WALK_MINIGUN, true },
	{ ACT_RUN_AIM, ACT_WALK_MINIGUN, true },
	{ ACT_RUN_CROUCH, ACT_WALK_CROUCH_MINIGUN, true },
	{ ACT_RUN_CROUCH_AIM, ACT_WALK_CROUCH_MINIGUN, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_MINIGUN, false },
	{ ACT_COVER_LOW, ACT_COVER_MINIGUN, false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW, ACT_IDLE_MINIGUN, false },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_MINIGUN, true },		// FIXME: hook to AR2 unique
#endif
};

IMPLEMENT_ACTTABLE(CWeaponFlamethrower);

CWeaponFlamethrower::CWeaponFlamethrower()
{
	m_nEffectState = 0;
	m_flGrowTime = m_flAttackedTime = m_flLastChargeFraction = 0.0f;
	m_flSoundTimer = 0.0f;
	m_iPoseParamFire = m_iPoseParamGauge = -1;

#ifdef CLIENT_DLL
	m_nClientEffectFlags = 0;
	m_pParticleFireSmall = m_pParticleFireLarge = NULL;
#endif
}

bool CWeaponFlamethrower::Reload(void)
{
	return false;
}

void CWeaponFlamethrower::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	CBaseViewModel *pVM = pOwner->GetViewModel();
	if (!pVM)
		return;

	WeaponIdle();

	if (!(m_nEffectState & FLAMETHROWER_RENDER_TINY_FLAME) && (gpGlobals->curtime >= m_flNextPrimaryAttack)) // Enable the tiny flame! play snd!
	{
		m_nEffectState |= FLAMETHROWER_RENDER_TINY_FLAME;
		WeaponSound(SPECIAL1);
	}

	bool bCanFire = !(m_iClip1 <= 0 || (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false));

	if ((!(pOwner->m_nButtons & IN_ATTACK) || !bCanFire) && (m_nEffectState & FLAMETHROWER_RENDER_LARGE_FLAME))
	{
		m_nEffectState &= ~FLAMETHROWER_RENDER_LARGE_FLAME; 
		m_flAttackedTime = gpGlobals->curtime;
		m_flGrowTime = GetWpnData().m_flWeaponChargeTime * ((pVM->GetPoseParameter(m_iPoseParamFire) / POSEPARAM_FIRE_SEQUENCE_MAX));
		m_flNextSecondaryAttack = gpGlobals->curtime + m_flGrowTime;
		StopWeaponSound(SINGLE);
		WeaponSound(SPECIAL3);
	}

	if ((pOwner->m_nButtons & IN_ATTACK) && (gpGlobals->curtime >= m_flNextPrimaryAttack))
	{
		if (bCanFire == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
			return;
		}

		if (!(m_nEffectState & FLAMETHROWER_RENDER_LARGE_FLAME))
		{
			m_flAttackedTime = m_flNextSecondaryAttack = gpGlobals->curtime;
			m_nEffectState |= FLAMETHROWER_RENDER_LARGE_FLAME;
			m_iPoseParamFire = pVM->LookupPoseParameter(POSEPARAM_FIRE_SEQUENCE);
			m_iPoseParamGauge = pVM->LookupPoseParameter(POSEPARAM_GAUGE_SEQUENCE);

			float currentFireBlend = pVM->GetPoseParameter(m_iPoseParamFire);
			m_flGrowTime = GetWpnData().m_flWeaponChargeTime * (1.0f - (currentFireBlend / POSEPARAM_FIRE_SEQUENCE_MAX));
			m_flLastChargeFraction = (currentFireBlend / POSEPARAM_FIRE_SEQUENCE_MAX);
			m_flSoundTimer = gpGlobals->curtime + 0.8f;

			StopWeaponSound(SINGLE);
			WeaponSound(SPECIAL2);
		}

		PrimaryAttack(pOwner, pVM, (m_flLastChargeFraction + GetChargeFraction()));
		return;
	}

	if ((m_flNextSecondaryAttack >= gpGlobals->curtime) && ((m_nEffectState & FLAMETHROWER_RENDER_LARGE_FLAME) == false))
	{
		float fraction = (m_flGrowTime / GetWpnData().m_flWeaponChargeTime) - GetChargeFraction();
		
		if (m_iPoseParamFire >= 0)
			pVM->SetPoseParameter(m_iPoseParamFire, fraction * POSEPARAM_FIRE_SEQUENCE_MAX);

		if (m_iPoseParamGauge >= 0)
			pVM->SetPoseParameter(m_iPoseParamGauge, fraction * POSEPARAM_GAUGE_SEQUENCE_MAX);
	}
}

void CWeaponFlamethrower::StartHolsterSequence()
{
	StopWeaponSound(SINGLE);
	WeaponSound(SPECIAL1);
	m_nEffectState = 0;
	BaseClass::StartHolsterSequence();
}

bool CWeaponFlamethrower::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_nEffectState = 0;
	return BaseClass::Holster(pSwitchingTo);
}

void CWeaponFlamethrower::Drop(const Vector &vecVelocity)
{
	StopWeaponSound(SINGLE);
	m_nEffectState = 0;
	BaseClass::Drop(vecVelocity);
}

void CWeaponFlamethrower::UpdateOnRemove(void)
{
	m_nEffectState = 0;

#ifdef CLIENT_DLL
	CleanupEffects(true);
#endif

	BaseClass::UpdateOnRemove();
}

void CWeaponFlamethrower::PrimaryAttack(CBasePlayer *pOwner, CBaseViewModel *pVM, float fraction) 
{
	if (pOwner == NULL || pVM == NULL)
		return;

	if (m_flSoundTimer <= gpGlobals->curtime)
	{
		m_flSoundTimer = gpGlobals->curtime + 1.5f;
		WeaponSound(SINGLE);
	}

	if (m_iPoseParamFire >= 0)
		pVM->SetPoseParameter(m_iPoseParamFire, fraction * POSEPARAM_FIRE_SEQUENCE_MAX);

	if (m_iPoseParamGauge >= 0)
		pVM->SetPoseParameter(m_iPoseParamGauge, fraction * POSEPARAM_GAUGE_SEQUENCE_MAX);

	if ((gpGlobals->curtime >= m_flNextSecondaryAttack) && (fraction > 0.0f)) // Attack frequently!
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + MAX(GetFireRate(), 0.1f);

#ifndef CLIENT_DLL
		Vector vecStart = pOwner->Weapon_ShootPosition();
		Vector vecForward = pOwner->GetAutoaimVector(1.0f), vecHull = Vector(5, 5, 5);
		VectorNormalize(vecForward);

		trace_t traceHit;
		lagcompensation->TraceRealtime(pOwner,
			vecStart,
			(vecStart + vecForward * (GetRange() * fraction)),
			-vecHull,
			vecHull,
			NULL, &traceHit, (GetRange() * fraction), false, true
			);
		CBaseEntity *pHitEnt = traceHit.m_pEnt;
		if (pHitEnt)
		{
			float flProperDamage = GameBaseShared()->GetDropOffDamage(vecStart, traceHit.endpos, GetActualDamage(), GetWpnData().m_flDropOffDistance);

			CTakeDamageInfo info(this, pOwner, flProperDamage, DMG_BURN);
			info.SetSkillFlags(SKILL_FLAG_BLAZINGAMMO);
			info.SetForcedWeaponID(WEAPON_ID_FLAMETHROWER);
			CalculateMeleeDamageForce(&info, vecForward, traceHit.endpos);
			pHitEnt->DispatchTraceAttack(info, vecForward, &traceHit);
			ApplyMultiDamage();
			TraceAttackToTriggers(info, traceHit.startpos, traceHit.endpos, vecForward);
		}
#endif

		float ammoToLose = MAX(round(GetWpnData().m_flDepletionFactor * fraction), 1.0f);
		m_iClip1 -= ((int)ammoToLose);
		if (m_iClip1 < 0)
			m_iClip1 = 0;
	}
}

#ifdef CLIENT_DLL
void C_WeaponFlamethrower::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if (updateType == DATA_UPDATE_CREATED)
		SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_WeaponFlamethrower::ClientThink(void)
{
	if (m_nClientEffectFlags != m_nEffectState)
	{
		m_nClientEffectFlags = m_nEffectState;

		if (m_pParticleFireLarge)
		{
			RemoveParticleEffect(m_pParticleFireLarge);
			m_pParticleFireLarge = NULL;
		}
	}

	C_BaseCombatCharacter *pOwner = GetOwner();
	if (IsEffectActive(EF_NODRAW) || IsDormant() || (pOwner == NULL) || (ShouldDraw() == false) || (m_nEffectState <= 0))
	{
		CleanupEffects();
		return;
	}

	CreateEffects();

	// Will this be too much?? Spawn regular DLights when firing.
	if (m_pParticleFireLarge && (pOwner == (C_BaseCombatCharacter*)BB2PlayerGlobals->GetCurrentViewModelOwner()))
		ProcessMuzzleFlashEvent();
}

void C_WeaponFlamethrower::CreateEffects(void)
{
	if ((m_pParticleFireSmall != NULL) && (m_pParticleFireLarge != NULL))
		return;

	C_BaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return;

	C_BasePlayer *pPlayer = ToBasePlayer(pOwner);

	CParticleProperty *pProp = ParticleProp();
	if (pPlayer && (g_bShouldRenderLocalPlayerExternally == false) && (input->CAM_IsThirdPerson() == false) && (pPlayer == (C_BasePlayer*)BB2PlayerGlobals->GetCurrentViewModelOwner()))
	{
		C_BaseViewModel *pvm = pPlayer->GetViewModel();
		if (pvm)
			pProp = pvm->ParticleProp();
	}

	if (pProp == NULL)
		return;

	if ((m_pParticleFireSmall == NULL) && (m_nClientEffectFlags & FLAMETHROWER_RENDER_TINY_FLAME))
		m_pParticleFireSmall = pProp->Create("flame_tiny", PATTACH_POINT_FOLLOW, "flame");

	if ((m_pParticleFireLarge == NULL) && (m_nClientEffectFlags & FLAMETHROWER_RENDER_LARGE_FLAME))
		m_pParticleFireLarge = pProp->Create("weapon_flame_fire_1", PATTACH_POINT_FOLLOW, "muzzle");
}

void C_WeaponFlamethrower::CleanupEffects(bool bNoFade)
{
	RemoveParticleEffect(m_pParticleFireSmall, bNoFade);
	RemoveParticleEffect(m_pParticleFireLarge, bNoFade);
	m_pParticleFireSmall = m_pParticleFireLarge = NULL;
}

void C_WeaponFlamethrower::RemoveParticleEffect(CNewParticleEffect *effect, bool bNoFade)
{
	if (effect == NULL)
		return;

	if (bNoFade)
		::ParticleMgr()->RemoveEffect(effect);
	else
		effect->StopEmission(false, false, true);
}
#else
float CWeaponFlamethrower::GetActualDamage(void)
{
	float baseDmg = ((float)GetHL2MPWpnData().m_iPlayerDamage);

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (pPlayer)
	{
		int m_iTeamBonusDamage = (pPlayer->m_BB2Local.m_iPerkTeamBonus - 5);
		if (m_iTeamBonusDamage > 0)
			baseDmg += ((baseDmg / 100.0f) * (m_iTeamBonusDamage * GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iTeamBonusDamageIncrease));

		if (pPlayer->IsPerkFlagActive(PERK_POWERUP_CRITICAL))
		{
			const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(PERK_POWERUP_CRITICAL);
			if (data)
				baseDmg += ((baseDmg / 100.0f) * data->flExtraFactor);
		}
	}

	return baseDmg;
}
#endif
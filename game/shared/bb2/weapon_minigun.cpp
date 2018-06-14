//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Minigun - Special Powner
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponMinigun C_WeaponMinigun
#endif

enum MinigunWeaponStates_t
{
	MINIGUN_STATE_IDLE = 0,
	MINIGUN_STATE_PREPARE,
	MINIGUN_STATE_SPIN_IDLE,
};

#define MINIGUN_AMMO_DEATHMATCH 200

class CWeaponMinigun : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CWeaponMinigun, CHL2MPMachineGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponMinigun();

	int GetOverloadCapacity() { return 25; }

	int GetMinBurst() { return 1; }
	int GetMaxBurst() { return 1; }
	float GetMinRestTime() { return 0; }
	float GetMaxRestTime() { return 0; }

	int GetWeaponType(void) { return WEAPON_TYPE_SPECIAL; }
	float GetFireRate(void) { return GetWpnData().m_flFireRate; }

	int GetMaxClip1(void) const;
	int GetDefaultClip1(void) const;

	bool Reload(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void ItemPostFrame(void);
	void OnWeaponOverload(void);

	bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void Drop(const Vector &vecVelocity);

	const WeaponProficiencyInfo_t *GetProficiencyValues();	

private:
	CWeaponMinigun(const CWeaponMinigun &);

	CNetworkVar(int, m_iWeaponState);
	CNetworkVar(float, m_flSoonestAttackTime);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMinigun, DT_WeaponMinigun)

BEGIN_NETWORK_TABLE(CWeaponMinigun, DT_WeaponMinigun)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iWeaponState)),
RecvPropTime(RECVINFO(m_flSoonestAttackTime)),
#else
SendPropInt(SENDINFO(m_iWeaponState), 2, SPROP_UNSIGNED),
SendPropTime(SENDINFO(m_flSoonestAttackTime)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMinigun)
DEFINE_PRED_FIELD(m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD_TOL(m_flSoonestAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_minigun, CWeaponMinigun);
PRECACHE_WEAPON_REGISTER(weapon_minigun);

acttable_t CWeaponMinigun::m_acttable[] =
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

IMPLEMENT_ACTTABLE(CWeaponMinigun);

CWeaponMinigun::CWeaponMinigun()
{
	m_iWeaponState = MINIGUN_STATE_IDLE;
	m_flSoonestAttackTime = 0.0f;
}

bool CWeaponMinigun::Reload(void)
{
	return false;
}

int CWeaponMinigun::GetMaxClip1(void) const
{
	if (HL2MPRules() && ((HL2MPRules()->GetCurrentGamemode() == MODE_DEATHMATCH) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)))
		return MINIGUN_AMMO_DEATHMATCH;

	return BaseClass::GetMaxClip1();
}

int CWeaponMinigun::GetDefaultClip1(void) const
{
	if (HL2MPRules() && ((HL2MPRules()->GetCurrentGamemode() == MODE_DEATHMATCH) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)))
		return MINIGUN_AMMO_DEATHMATCH;

	return BaseClass::GetDefaultClip1();
}

void CWeaponMinigun::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	DoWeaponFX();
	UpdateAutoFire();
	m_fFireDuration = (pOwner->m_nButtons & IN_ATTACK) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;

	if (m_iWeaponState != MINIGUN_STATE_IDLE)
	{
		if (m_flSoonestAttackTime < gpGlobals->curtime)
		{
			if ((!(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2)) || m_iClip1 <= 0)
			{
				StopWeaponSound(SPECIAL3);
				SendWeaponAnim(ACT_VM_SPIN_TO_IDLE);
				m_flSoonestAttackTime = gpGlobals->curtime + GetViewModelSequenceDuration();
				m_iWeaponState = MINIGUN_STATE_IDLE;
				WeaponSound(SPECIAL2);
				return;
			}

			if ((m_iWeaponState == MINIGUN_STATE_SPIN_IDLE) && (!(pOwner->m_nButtons & IN_ATTACK)))
			{
				SendWeaponAnim(ACT_VM_SPIN_IDLE);
				m_flSoonestAttackTime = gpGlobals->curtime + GetViewModelSequenceDuration();
				WeaponSound(SPECIAL3);
			}

			if (m_iWeaponState == MINIGUN_STATE_PREPARE)
			{
				SendWeaponAnim(ACT_VM_SPIN_IDLE);
				m_flSoonestAttackTime = gpGlobals->curtime + GetViewModelSequenceDuration();
				m_iWeaponState = MINIGUN_STATE_SPIN_IDLE;
				WeaponSound(SPECIAL3);
				m_flNextPrimaryAttack = gpGlobals->curtime;
			}
		}

		if (m_iWeaponState == MINIGUN_STATE_SPIN_IDLE)
		{
			if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
			{
				if (m_iClip1 <= 0 || (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false))
				{
					WeaponSound(EMPTY);
					m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
				}
				else
				{
					if (pOwner->m_afButtonPressed & IN_ATTACK)
						m_flNextPrimaryAttack = gpGlobals->curtime;

					BaseClass::PrimaryAttack();

					if (AutoFiresFullClip())
						m_bFiringWholeClip = true;

#ifdef CLIENT_DLL
					pOwner->SetFiredWeapon(true);
#endif
				}
			}
		}
	}
	else
	{
		if (m_flSoonestAttackTime < gpGlobals->curtime)
			WeaponIdle();

		if ((pOwner->m_afButtonPressed & IN_ATTACK) || (pOwner->m_afButtonPressed & IN_ATTACK2))
		{
			if (m_iClip1 <= 0)
				WeaponSound(EMPTY);
		}

		if ((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2))
			PrimaryAttack();
	}
}

void CWeaponMinigun::OnWeaponOverload(void)
{
}

bool CWeaponMinigun::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_iWeaponState = MINIGUN_STATE_IDLE;
	m_flSoonestAttackTime = 0.0f;
	StopWeaponSound(SPECIAL3);

	return BaseClass::Holster(pSwitchingTo);
}

void CWeaponMinigun::Drop(const Vector &vecVelocity)
{
	m_iWeaponState = MINIGUN_STATE_IDLE;
	m_flSoonestAttackTime = 0.0f;
	StopWeaponSound(SPECIAL3);

	BaseClass::Drop(vecVelocity);
}

void CWeaponMinigun::PrimaryAttack(void)
{
	if ((m_iWeaponState != MINIGUN_STATE_IDLE) || (m_flSoonestAttackTime > gpGlobals->curtime) || (m_iClip1 <= 0))
		return;

	SendWeaponAnim(ACT_VM_IDLE_TO_SPIN);
	m_flSoonestAttackTime = gpGlobals->curtime + GetViewModelSequenceDuration();
	m_iWeaponState = MINIGUN_STATE_PREPARE;
	WeaponSound(SPECIAL1);
}

void CWeaponMinigun::SecondaryAttack(void)
{
}

const WeaponProficiencyInfo_t *CWeaponMinigun::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0, 0.75 },
		{ 5.00, 0.75 },
		{ 3.0, 0.85 },
		{ 5.0 / 3.0, 0.75 },
		{ 1.00, 1.0 },
	};

	COMPILE_TIME_ASSERT(ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
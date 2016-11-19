//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Hands Melee
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponHands C_WeaponHands
#endif

enum MegaPunchStates
{
	MEGA_PUNCH_STATE_NONE = 0,
	MEGA_PUNCH_STATE_CHARGE,
	MEGA_PUNCH_STATE_IDLE,
	MEGA_PUNCH_STATE_RELEASE,
};

class CWeaponHands : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponHands, CBaseHL2MPBludgeonWeapon);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponHands();
	CWeaponHands(const CWeaponHands &);

	int GetMeleeDamageType() { return DMG_CLUB; }
	int GetMeleeSkillFlags(void) { return 0; }

	void Drop(const Vector &vecVelocity);
	bool VisibleInWeaponSelection() { return false; }

	Activity GetCustomActivity(int bIsSecondary);

	void ItemPostFrame(void);
	void SecondaryAttack(void);

	CNetworkVar(int, m_iChargeState);
	CNetworkVar(float, m_flTimeCharged);
	CNetworkVar(float, m_flTimeChargeDecay);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHands, DT_WeaponHands)

BEGIN_NETWORK_TABLE(CWeaponHands, DT_WeaponHands)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iChargeState)),
RecvPropFloat(RECVINFO(m_flTimeCharged)),
RecvPropFloat(RECVINFO(m_flTimeChargeDecay)),
#else
SendPropInt(SENDINFO(m_iChargeState), 2, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_flTimeCharged)),
SendPropFloat(SENDINFO(m_flTimeChargeDecay)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponHands)
DEFINE_PRED_FIELD(m_iChargeState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flTimeCharged, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flTimeChargeDecay, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_hands, CWeaponHands);
PRECACHE_WEAPON_REGISTER(weapon_hands);

acttable_t	CWeaponHands::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_MELEE, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_MELEE, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_MELEE, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_MELEE, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_MELEE, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_MELEE, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_MELEE, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};

IMPLEMENT_ACTTABLE(CWeaponHands);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponHands::CWeaponHands(void)
{
	m_iChargeState = MEGA_PUNCH_STATE_NONE;
	m_flTimeCharged = 0.0f;
	m_flTimeChargeDecay = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHands::Drop(const Vector &vecVelocity)
{
#ifndef CLIENT_DLL
	UTIL_Remove(this);
#endif
}

Activity CWeaponHands::GetCustomActivity(int bIsSecondary)
{
	if (bIsSecondary)
		return ACT_VM_CHARGE_ATTACK;

	return ACT_VM_PRIMARYATTACK;
}

void CWeaponHands::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	switch (m_iChargeState)
	{

	case MEGA_PUNCH_STATE_CHARGE:
	{
		if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			m_iChargeState = MEGA_PUNCH_STATE_IDLE;
			SendWeaponAnim(ACT_VM_CHARGE_IDLE);
			m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
			m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
		}

		break;
	}

	case MEGA_PUNCH_STATE_IDLE:
	{
		if (!(pOwner->m_nButtons & IN_ATTACK2))
		{
			m_iChargeState = MEGA_PUNCH_STATE_RELEASE;
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			SendWeaponAnim(ACT_VM_CHARGE_IDLE);
			m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
			m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
		}

		break;
	}

	case MEGA_PUNCH_STATE_RELEASE:
	{
		m_iChargeState = MEGA_PUNCH_STATE_NONE;

		WeaponSound(SINGLE);
		SendWeaponAnim(ACT_VM_CHARGE_ATTACK);
		pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_CHARGE_ATTACK);
		m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
		m_flNextSecondaryAttack = gpGlobals->curtime + SpecialPunishTime() + GetViewModelSequenceDuration();
		m_flMeleeCooldown = gpGlobals->curtime;
		break;
	}

	}
}

void CWeaponHands::SecondaryAttack(void)
{
	if (m_iChargeState != MEGA_PUNCH_STATE_NONE)
		return;

	m_iChargeState = MEGA_PUNCH_STATE_CHARGE;

	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_CHARGE_START);
	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
}
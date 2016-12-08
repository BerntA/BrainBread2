//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Melee Weapon Chargeable Base Class.
//
//========================================================================================//

#include "cbase.h"
#include "weapon_melee_chargeable.h"
#include "hl2mp_player_shared.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum WeaponChargeStates
{
	CHARGE_STATE_NONE = 0,
	CHARGE_STATE_START,
	CHARGE_STATE_IDLE,
	CHARGE_STATE_RELEASE,
};

LINK_ENTITY_TO_CLASS(weaponmeleechargeable, CHL2MPMeleeChargeable);

IMPLEMENT_NETWORKCLASS_ALIASED(HL2MPMeleeChargeable, DT_HL2MPMeleeChargeable)

BEGIN_NETWORK_TABLE(CHL2MPMeleeChargeable, DT_HL2MPMeleeChargeable)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iChargeState)),
RecvPropFloat(RECVINFO(m_flTimeCharged)),
#else
SendPropInt(SENDINFO(m_iChargeState), 2, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_flTimeCharged)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CHL2MPMeleeChargeable)
DEFINE_PRED_FIELD(m_iChargeState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flTimeCharged, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

CHL2MPMeleeChargeable::CHL2MPMeleeChargeable()
{
	ResetStates();
}

void CHL2MPMeleeChargeable::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	switch (m_iChargeState)
	{

	case CHARGE_STATE_START:
	{
		if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			m_iChargeState = CHARGE_STATE_IDLE;
			SendWeaponAnim(ACT_VM_CHARGE_IDLE);
			m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
			m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
			m_flTimeCharged = gpGlobals->curtime;
		}

		break;
	}

	case CHARGE_STATE_IDLE:
	{
		if (!(pOwner->m_nButtons & IN_ATTACK2))
		{
			m_iChargeState = CHARGE_STATE_RELEASE;
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			SendWeaponAnim(ACT_VM_CHARGE_IDLE);
			m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
			m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
		}

		break;
	}

	case CHARGE_STATE_RELEASE:
	{
		m_iChargeState = CHARGE_STATE_NONE;

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

void CHL2MPMeleeChargeable::PrimaryAttack(void)
{
	if (m_iChargeState != CHARGE_STATE_NONE)
		return;

	BaseClass::PrimaryAttack();
}

void CHL2MPMeleeChargeable::SecondaryAttack(void)
{
	if (m_iChargeState != CHARGE_STATE_NONE)
		return;

	m_iChargeState = CHARGE_STATE_START;

	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_CHARGE_START);
	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
}

float CHL2MPMeleeChargeable::GetDamageForActivity(Activity hitActivity)
{
	float damage = BaseClass::GetDamageForActivity(hitActivity);
	bool bSpecialAttack = (hitActivity == ACT_VM_CHARGE_ATTACK);
	if (bSpecialAttack)
		damage += GetChargeDamage();

	return damage;
}

float CHL2MPMeleeChargeable::GetChargeDamage(void)
{
	return (GetSpecialAttackDamage() + (GetWpnData().m_flSpecialDamage2 * GetChargeFraction()));
}

float CHL2MPMeleeChargeable::GetChargeFraction(void)
{
	return (clamp(((gpGlobals->curtime - m_flTimeCharged) / GetMaxChargeTime()), 0.0f, 1.0f));
}

void CHL2MPMeleeChargeable::ResetStates()
{
	m_iChargeState = CHARGE_STATE_NONE;
	m_flTimeCharged = 0.0f;
}

void CHL2MPMeleeChargeable::StartHolsterSequence()
{
	ResetStates();
	BaseClass::StartHolsterSequence();
}

bool CHL2MPMeleeChargeable::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	ResetStates();
	return BaseClass::Holster(pSwitchingTo);
}

void CHL2MPMeleeChargeable::Drop(const Vector &vecVelocity)
{
	ResetStates();
	BaseClass::Drop(vecVelocity);
}

bool CHL2MPMeleeChargeable::Deploy(void)
{
	ResetStates();
	return BaseClass::Deploy();
}
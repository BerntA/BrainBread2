//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Akimbo Weapon Skeleton.
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_akimbo.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(HL2MPBaseAkimbo, DT_HL2MPBaseAkimbo)

BEGIN_NETWORK_TABLE(CHL2MPBaseAkimbo, DT_HL2MPBaseAkimbo)
#if !defined( CLIENT_DLL )
SendPropBool(SENDINFO(m_bFireNext)),
#else
RecvPropBool(RECVINFO(m_bFireNext)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CHL2MPBaseAkimbo)
DEFINE_PRED_FIELD(m_bFireNext, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

acttable_t CHL2MPBaseAkimbo::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_AKIMBO, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_AKIMBO, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_REVOLVER, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_AKIMBO, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_AKIMBO, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_AKIMBO, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_AKIMBO, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_AKIMBO, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_AKIMBO, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_AKIMBO, false },
};

IMPLEMENT_ACTTABLE(CHL2MPBaseAkimbo);

CHL2MPBaseAkimbo::CHL2MPBaseAkimbo(void)
{
	m_bReloadsSingly = m_bFiresUnderwater = false;
	m_bFireNext.Set(false);
}

void CHL2MPBaseAkimbo::AddViewKick(void)
{
	if (m_iMeleeAttackType.Get() > 0)
	{
		BaseClass::AddViewKick();
		return;
	}

	// Normal gunfire viewkick...
	CHL2MP_Player* pPlayer = ToHL2MPPlayer(GetOwner());
	if (pPlayer)
		pPlayer->ViewPunch(GetViewKickAngle());
}

void CHL2MPBaseAkimbo::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();
}

void CHL2MPBaseAkimbo::StartHolsterSequence()
{
	BaseClass::StartHolsterSequence();
}

bool CHL2MPBaseAkimbo::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

void CHL2MPBaseAkimbo::Drop(const Vector& vecVelocity)
{
	BaseClass::Drop(vecVelocity);
}

bool CHL2MPBaseAkimbo::Deploy(void)
{
	return BaseClass::Deploy();
}

void CHL2MPBaseAkimbo::PerformAttack(void)
{
	CHL2MP_Player* pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer || ((m_iClip1 <= 0) && (m_iClip2 <= 0)))
		return;

	if (m_iClip1 <= 0)
		m_bFireNext.Set(false);
	else if (m_iClip2 <= 0)
		m_bFireNext.Set(true);
	else
		m_bFireNext.Set(!m_bFireNext.Get());

	bool bPrimary = m_bFireNext.Get();
	if (bPrimary)
		m_iClip1--;
	else
		m_iClip2--;

	int shootAct = (bPrimary ? ACT_VM_SHOOT_LEFT : ACT_VM_SHOOT_RIGHT);
	if (UsesEmptyAnimation())
	{
		if (bPrimary && (m_iClip1 <= 0))
			shootAct = ACT_VM_SHOOT_LEFT_LAST;
		else if (!bPrimary && (m_iClip2 <= 0))
			shootAct = ACT_VM_SHOOT_RIGHT_LAST;
	}

	WeaponSound(SINGLE);
	SendWeaponAnim(shootAct);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector();

	FireBulletsInfo_t info(GetWpnData().m_iPellets, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, bPrimary ? m_iPrimaryAmmoType.Get() : m_iSecondaryAmmoType.Get());
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetLocalOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;
	info.m_vecSpread = pPlayer->GetAttackSpread(this);
	info.m_bPrimaryAttack = bPrimary;
	pPlayer->FireBullets(info);
	pPlayer->DoAnimationEvent((bPrimary ? PLAYERANIMEVENT_ATTACK_PRIMARY : PLAYERANIMEVENT_ATTACK_SECONDARY), shootAct);
	AddViewKick();
}

void CHL2MPBaseAkimbo::PrimaryAttack(void)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer || !(pPlayer->m_afButtonPressed & IN_ATTACK))
		return;

	m_flNextPrimaryAttack = (gpGlobals->curtime + (GetFireRate() / 2.0f));
	PerformAttack();
}

void CHL2MPBaseAkimbo::SecondaryAttack(void)
{
	// NOTHING
}
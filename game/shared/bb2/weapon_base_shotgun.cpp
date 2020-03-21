//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shotgun baseclass.
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_base_shotgun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(HL2MPBaseShotgun, DT_HL2MPBaseShotgun)

BEGIN_NETWORK_TABLE(CHL2MPBaseShotgun, DT_HL2MPBaseShotgun)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bInReload)),
RecvPropBool(RECVINFO(m_bShouldReloadEmpty)),
#else
SendPropBool(SENDINFO(m_bInReload)),
SendPropBool(SENDINFO(m_bShouldReloadEmpty)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CHL2MPBaseShotgun)
DEFINE_PRED_FIELD(m_bInReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bShouldReloadEmpty, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_baseshotgun, CHL2MPBaseShotgun);

CHL2MPBaseShotgun::CHL2MPBaseShotgun(void)
{
	m_bReloadsSingly = true;
	m_bInReload = false;
	m_bShouldReloadEmpty = false;
}

void CHL2MPBaseShotgun::AffectedByPlayerSkill(int skill)
{
	switch (skill)
	{
	case PLAYER_SKILL_HUMAN_GUNSLINGER:
	case PLAYER_SKILL_HUMAN_MAGAZINE_REFILL:
	{
		m_bInReload = false;
		m_bShouldReloadEmpty = false;
		break;
	}
	}
}

void CHL2MPBaseShotgun::FillClip(int iAmount)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		if (Clip1() < GetMaxClip1())
		{
			m_iClip1 += iAmount;
			pOwner->RemoveAmmo(iAmount, m_iPrimaryAmmoType);
		}
	}
}

void CHL2MPBaseShotgun::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CHL2MPBaseShotgun::PrimaryAttack(Activity fireActivity, WeaponSound_t sound, bool bPrimary)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	WeaponSound(sound);
	SendWeaponAnim(fireActivity);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, fireActivity);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector();

	FireBulletsInfo_t info(GetWpnData().m_iPellets, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, bPrimary);
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetLocalOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;
	pPlayer->FireBullets(info);

#ifdef BB2_AI
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2);
#endif
#endif //BB2_AI

	pPlayer->ViewPunch(GetViewKickAngle());
}

void CHL2MPBaseShotgun::ItemPostFrame(void)
{
	if (!m_bInReload)
		BaseClass::GenericBB2Animations();
}

void CHL2MPBaseShotgun::WeaponIdle(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;

	if (!m_bInReload && IsViewModelSequenceFinished() && (m_flNextBashAttack <= gpGlobals->curtime) && !(pOwner->m_nButtons & (IN_BASH | IN_ATTACK | IN_ATTACK2 | IN_RELOAD)) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		BaseClass::WeaponIdle();
}

void CHL2MPBaseShotgun::StartHolsterSequence()
{
	m_bInReload = false;
	m_bShouldReloadEmpty = false;
	BaseClass::StartHolsterSequence();
}

bool CHL2MPBaseShotgun::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_bInReload = false;
	m_bShouldReloadEmpty = false;
	return BaseClass::Holster(pSwitchingTo);
}

void CHL2MPBaseShotgun::Drop(const Vector &vecVelocity)
{
	m_bInReload = false;
	m_bShouldReloadEmpty = false;
	BaseClass::Drop(vecVelocity);
}

#ifndef CLIENT_DLL
#ifdef BB2_AI
void CHL2MPBaseShotgun::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles)
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	Assert(npc != NULL);
	WeaponSound(SINGLE_NPC);
	m_iClip1 = m_iClip1 - 1;

	if (bUseWeaponAngles)
	{
		QAngle angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);
	}

	pOperator->FireBullets(GetWpnData().m_iPellets, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
}

void CHL2MPBaseShotgun::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	m_iClip1++;
	FireNPCPrimaryAttack(pOperator, true);
}

void CHL2MPBaseShotgun::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SHOTGUN_FIRE:
		FireNPCPrimaryAttack(pOperator, false);
		break;

	default:
		CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif // BB2_AI
#endif
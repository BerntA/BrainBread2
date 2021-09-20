//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Pistol/Handgun base class. (includes burst)
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_pistol.h"
#include "hl2mp_player_shared.h"
#include "npcevent.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(basepistolweapon, CHL2MPBasePistol);

IMPLEMENT_NETWORKCLASS_ALIASED(HL2MPBasePistol, DT_HL2MPBasePistol)

BEGIN_NETWORK_TABLE(CHL2MPBasePistol, DT_HL2MPBasePistol)
#if !defined( CLIENT_DLL )
SendPropBool(SENDINFO(m_bHasFiredGun)),
#else
RecvPropBool(RECVINFO(m_bHasFiredGun)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CHL2MPBasePistol)
DEFINE_PRED_FIELD(m_bHasFiredGun, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

CHL2MPBasePistol::CHL2MPBasePistol(void)
{
	m_bHasFiredGun = false;

	m_fMinRange1 = 24;
	m_fMaxRange1 = 1500;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;

	m_bFiresUnderwater = true;
}

void CHL2MPBasePistol::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	if (m_bHasFiredGun && (!(pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_afButtonReleased & IN_ATTACK)))
		m_bHasFiredGun = false;

	BaseClass::ItemPostFrame();

	if (m_bInReload || (m_flNextBashAttack > gpGlobals->curtime))
		return;

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack < gpGlobals->curtime) && (m_iClip <= 0))
		DryFire();
}

void CHL2MPBasePistol::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CHL2MPBasePistol::PrimaryAttack(void)
{
	if (m_bIsFiringBurst)
	{
		BaseClass::PrimaryAttack();
		return;
	}

	if (m_bHasFiredGun)
		return;

	m_bHasFiredGun = true;
	CBaseCombatWeapon::PrimaryAttack();
}

void CHL2MPBasePistol::SecondaryAttack(void)
{
	if (m_iClip <= 0)
	{
		DryFire();
		return;
	}

	BaseClass::SecondaryAttack();
}

void CHL2MPBasePistol::StartHolsterSequence()
{
	m_bHasFiredGun = false;
	BaseClass::StartHolsterSequence();
}

bool CHL2MPBasePistol::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_bHasFiredGun = false;
	return BaseClass::Holster(pSwitchingTo);
}

void CHL2MPBasePistol::Drop(const Vector &vecVelocity)
{
	m_bHasFiredGun = false;
	BaseClass::Drop(vecVelocity);
}

bool CHL2MPBasePistol::Deploy(void)
{
	m_bHasFiredGun = false;
	return BaseClass::Deploy();
}

#ifndef CLIENT_DLL
void CHL2MPBasePistol::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2:
	case EVENT_WEAPON_SMG1:
	case EVENT_WEAPON_PISTOL_FIRE:
	{
		Vector vecShootOrigin, vecShootDir;
		vecShootOrigin = pOperator->Weapon_ShootPosition();

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);

		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

		CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

		WeaponSound(SINGLE_NPC);
		pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, GetAmmoTypeID(), 2);

		if (!npc->IsBoss())
			m_iClip--;

		break;
	}

	default:
		CWeaponHL2MPBase::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Sniper Base Class, includes 3x zoom levels and more.
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_sniper.h"
#include "hl2mp_player_shared.h"
#include "in_buttons.h"
#include "npcevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(basesniperweapon, CHL2MPSniperRifle);

IMPLEMENT_NETWORKCLASS_ALIASED(HL2MPSniperRifle, DT_HL2MPSniperRifle)

BEGIN_NETWORK_TABLE(CHL2MPSniperRifle, DT_HL2MPSniperRifle)
#if !defined( CLIENT_DLL )
SendPropInt(SENDINFO(m_iCurrentZoomLevel), 2, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iOldZoomLevel), 2, SPROP_UNSIGNED),
#else
RecvPropInt(RECVINFO(m_iCurrentZoomLevel)),
RecvPropInt(RECVINFO(m_iOldZoomLevel)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CHL2MPSniperRifle)
DEFINE_PRED_FIELD(m_iCurrentZoomLevel, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_iOldZoomLevel, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

CHL2MPSniperRifle::CHL2MPSniperRifle(void)
{
	m_iCurrentZoomLevel = 0;
	m_iOldZoomLevel = 0;
}

void CHL2MPSniperRifle::SetZoomLevel(int level)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	CBaseViewModel *pViewMDL = pOwner->GetViewModel();
	if (!pViewMDL)
		return;

	m_iOldZoomLevel = 0;
	int oldLevel = m_iCurrentZoomLevel;
	m_iCurrentZoomLevel = level;
	if (m_iCurrentZoomLevel)
	{
		int extraFOV = GetFOVForZoom(m_iCurrentZoomLevel);
		pOwner->SetFOV(this, pOwner->GetDefaultFOV() + extraFOV, 0.3f);

		if (ShouldHideViewmodelOnZoom() && !pViewMDL->IsEffectActive(EF_NODRAW))
			pViewMDL->AddEffects(EF_NODRAW);

		if (ShouldPlayZoomSounds())
			WeaponSound(SPECIAL1); // Zoom-In.
		return;
	}

	if (oldLevel)
	{
		pOwner->SetFOV(this, 0, 0.3f);

		if (ShouldHideViewmodelOnZoom())
			pViewMDL->RemoveEffects(EF_NODRAW);

		if (ShouldPlayZoomSounds())
			WeaponSound(SPECIAL2); // Zoom-Out.
	}
}

void CHL2MPSniperRifle::WeaponIdle(void)
{
	if (HasWeaponIdleTimeElapsed())
	{
		if ((m_iClip1 <= 0) && UsesEmptyAnimation())
			SendWeaponAnim(ACT_VM_IDLE_EMPTY);
		else
			SendWeaponAnim(ACT_VM_IDLE);
	}
}

void CHL2MPSniperRifle::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	if (gpGlobals->curtime > m_flNextPrimaryAttack)
	{
		if (m_iOldZoomLevel)
			SetZoomLevel(m_iOldZoomLevel);
		else if (pOwner->m_afButtonPressed & IN_ATTACK2)
			SecondaryAttack();
	}
}

void CHL2MPSniperRifle::PrimaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		if (pOwner->m_afButtonPressed & IN_ATTACK)
		{
			BaseClass::PrimaryAttack();
			m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
			int iZoomLevel = (m_iClip1 > 0) ? m_iCurrentZoomLevel : 0;
			SetZoomLevel(0);
			m_iOldZoomLevel = iZoomLevel;
		}
	}
}

void CHL2MPSniperRifle::SecondaryAttack(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	CBaseViewModel *pViewMDL = pOwner->GetViewModel();
	if (!pViewMDL)
		return;

	int iWantZoom = m_iCurrentZoomLevel;
	if (iWantZoom < GetMaxZoomLevel())
		iWantZoom++;
	else
		iWantZoom = 0;

	SetZoomLevel(iWantZoom);
}

void CHL2MPSniperRifle::StartHolsterSequence()
{
	SetZoomLevel(0);
	BaseClass::StartHolsterSequence();
}

bool CHL2MPSniperRifle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	SetZoomLevel(0);
	return BaseClass::Holster(pSwitchingTo);
}

void CHL2MPSniperRifle::Drop(const Vector &vecVelocity)
{
	SetZoomLevel(0);
	BaseClass::Drop(vecVelocity);
}

bool CHL2MPSniperRifle::Deploy(void)
{
	SetZoomLevel(0);
	return BaseClass::Deploy();
}

int CHL2MPSniperRifle::GetFOVForZoom(int level)
{
	int iZoomLevel = (level - 1);
	if ((iZoomLevel >= 0) && (iZoomLevel < MAX_WEAPON_ZOOM_MODES))
		return GetWpnData().m_iZoomModeFOV[iZoomLevel];

	return 0;
}

bool CHL2MPSniperRifle::Reload(void)
{
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return false;

	bool ret = BaseClass::Reload();
	if (ret)
		SetZoomLevel(0);

	return ret;
}

bool CHL2MPSniperRifle::CanPerformMeleeAttacks()
{
	if (m_iCurrentZoomLevel)
		return false;

	return true;
}

#ifdef BB2_AI
#ifndef CLIENT_DLL
void CHL2MPSniperRifle::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
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
		pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2);
		m_iClip1 = m_iClip1 - 1;

		break;
	}

	default:		
		CWeaponHL2MPBase::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif
#endif //BB2_AI
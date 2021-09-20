//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shotgun baseclass.
//
//========================================================================================//

#ifndef WEAPON_BASE_SHOTGUN_H
#define WEAPON_BASE_SHOTGUN_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#if defined( CLIENT_DLL )
#define CHL2MPBaseShotgun C_HL2MPBaseShotgun
#endif

class CHL2MPBaseShotgun : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CHL2MPBaseShotgun, CBaseHL2MPCombatWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CHL2MPBaseShotgun();

	virtual int GetMinBurst() { return 1; }
	virtual int GetMaxBurst() { return 1; }
	virtual float GetFireRate(void) { return GetWpnData().m_flFireRate; }

	virtual int GetWeaponType(void) { return WEAPON_TYPE_SHOTGUN; }
	virtual const char *GetAmmoEntityLink(void) { return "ammo_slugs"; }

	virtual void DryFire(void);
	virtual void WeaponIdle(void);
	virtual void AffectedByPlayerSkill(int skill);
	virtual void ItemPostFrame(void);
	virtual void PrimaryAttack(Activity fireActivity, WeaponSound_t sound, bool bPrimary = true);

	virtual void FillClip(int iAmount);

	virtual void StartHolsterSequence();
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual void Drop(const Vector &vecVelocity);

#ifndef CLIENT_DLL
	virtual int CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	virtual void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	virtual void Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary);
	virtual void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, bool bUseWeaponAngles);
#endif

	virtual const char		*GetAmmoTypeName(void) { return "Buckshot"; }
	virtual int				GetAmmoMaxCarry(void) { return 32; }

protected:
	CNetworkVar(bool, m_bInReload);
	CNetworkVar(bool, m_bShouldReloadEmpty);

private:
	CHL2MPBaseShotgun(const CHL2MPBaseShotgun &);
};

#endif // WEAPON_BASE_SHOTGUN_H
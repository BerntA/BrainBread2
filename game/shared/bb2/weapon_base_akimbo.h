//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Akimbo Weapon Skeleton.
//
//========================================================================================//

#ifndef WEAPON_BASE_AKIMBO_H
#define WEAPON_BASE_AKIMBO_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#if defined( CLIENT_DLL )
#define CHL2MPBaseAkimbo C_HL2MPBaseAkimbo
#endif

class CHL2MPBaseAkimbo : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CHL2MPBaseAkimbo, CBaseHL2MPCombatWeapon);
	DECLARE_ACTTABLE();

	CHL2MPBaseAkimbo();

	virtual void AddViewKick(void);
	virtual int	GetTracerAttachment(void);
	virtual void ItemPostFrame(void);
	virtual void PerformAttack(bool bPrimary);
	virtual void PrimaryAttack(void) { PerformAttack(true); }
	virtual void SecondaryAttack(void) { PerformAttack(false); }

	virtual void StartHolsterSequence();
	virtual bool Holster(CBaseCombatWeapon* pSwitchingTo = NULL);
	virtual void Drop(const Vector& vecVelocity);
	virtual bool Deploy(void);

	virtual int GetOverloadCapacity() { return 5; }
	virtual bool UsesEmptyAnimation() { return true; }
	virtual int GetWeaponType(void) { return WEAPON_TYPE_PISTOL; }
	virtual const char* GetAmmoEntityLink(void) { return "ammo_pistol"; }

	virtual bool IsAkimboWeapon(void) { return true; }
	virtual const char* GetMuzzleflashAttachment(bool bPrimaryAttack, bool bThirdperson)
	{
		if (bPrimaryAttack)
			return "muzzle_left";

		return "muzzle";
	}

private:
	CHL2MPBaseAkimbo(const CHL2MPBaseAkimbo&);
};

#endif // WEAPON_BASE_AKIMBO_H
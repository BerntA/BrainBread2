//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Pistol/Handgun base class. (includes burst)
//
//========================================================================================//

#ifndef WEAPON_BASE_PISTOL_H
#define WEAPON_BASE_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbase_machinegun.h"

#if defined( CLIENT_DLL )
#define CHL2MPBasePistol C_HL2MPBasePistol
#endif

class CHL2MPBasePistol : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CHL2MPBasePistol, CHL2MPMachineGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CHL2MPBasePistol();

	virtual bool UsesEmptyAnimation() { return true; }
	virtual int GetOverloadCapacity() { return 4; }
	virtual int GetWeaponType(void) { return WEAPON_TYPE_PISTOL; }
	virtual const char *GetAmmoEntityLink(void) { return "ammo_pistol"; }

	virtual void ItemPostFrame(void);
	virtual void DryFire(void);

	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);

	virtual void StartHolsterSequence();
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual void Drop(const Vector &vecVelocity);
	virtual bool Deploy(void);

#ifdef BB2_AI
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	virtual void Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary) { return; }
#endif
#endif //BB2_AI

	virtual const char		*GetAmmoTypeName(void) { return "Pistol"; }

protected:
	CNetworkVar(bool, m_bHasFiredGun);

private:
	CHL2MPBasePistol(const CHL2MPBasePistol &);
};

#endif // WEAPON_BASE_PISTOL_H
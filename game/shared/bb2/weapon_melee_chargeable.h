//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Melee Weapon Chargeable Base Class.
//
//========================================================================================//

#ifndef WEAPON_BASE_MELEE_CHARGEABLE_H
#define WEAPON_BASE_MELEE_CHARGEABLE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbasebasebludgeon.h"

#if defined( CLIENT_DLL )
#define CHL2MPMeleeChargeable C_HL2MPMeleeChargeable
#endif

class CHL2MPMeleeChargeable : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CHL2MPMeleeChargeable, CBaseHL2MPBludgeonWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CHL2MPMeleeChargeable();

	virtual void ItemPostFrame(void);
	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);

	virtual float GetDamageForActivity(Activity hitActivity);
	virtual float GetChargeDamage(void);
	virtual float GetMaxChargeTime(void) { return GetWpnData().m_flWeaponChargeTime; }
	virtual float GetChargeFraction(void);
	virtual bool IsChargingWeapon(void) { return (m_iChargeState == 2); }

	virtual void ResetStates();
	virtual void StartHolsterSequence();
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual void Drop(const Vector &vecVelocity);
	virtual bool Deploy(void);

protected:
	CNetworkVar(int, m_iChargeState);
	CNetworkVar(float, m_flTimeCharged);

private:
	CHL2MPMeleeChargeable(const CHL2MPMeleeChargeable &);
};

#endif // WEAPON_BASE_MELEE_CHARGEABLE_H
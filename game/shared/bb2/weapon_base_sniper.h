//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Sniper Base Class, includes 3x zoom levels and more.
//
//========================================================================================//

#ifndef WEAPON_BASE_SNIPER_H
#define WEAPON_BASE_SNIPER_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbase.h"

#if defined( CLIENT_DLL )
#define CHL2MPSniperRifle C_HL2MPSniperRifle
#endif

class CHL2MPSniperRifle : public CWeaponHL2MPBase
{
public:
	DECLARE_CLASS(CHL2MPSniperRifle, CWeaponHL2MPBase);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CHL2MPSniperRifle();

	virtual int GetZoomLevel(void) { return m_iCurrentZoomLevel; }
	virtual void SetZoomLevel(int level);
	virtual int GetMaxZoomLevel(void) { return 3; }
	virtual bool ShouldDrawCrosshair(void) { return (GetZoomLevel() <= 0); }
	virtual int GetOverloadCapacity() { return 2; }
	virtual int GetWeaponType(void) { return WEAPON_TYPE_SNIPER; }
	virtual const char *GetAmmoEntityLink(void) { return "ammo_sniper"; }
	virtual void WeaponIdle(void);
	virtual void ItemPostFrame(void);

	virtual void PrimaryAttack(void);
	virtual void SecondaryAttack(void);

	virtual void StartHolsterSequence();
	virtual bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual void Drop(const Vector &vecVelocity);
	virtual bool Deploy(void);

	virtual int GetFOVForZoom(int level);

	virtual bool Reload(void);
	virtual bool CanPerformMeleeAttacks();
	virtual bool ShouldPlayZoomSounds() { return true; }
	virtual bool ShouldHideViewmodelOnZoom() { return true; }

	int	GetMinBurst() { return 1; }
	int	GetMaxBurst() { return 1; }

protected:
	CNetworkVar(int, m_iCurrentZoomLevel);
	CNetworkVar(int, m_iOldZoomLevel);

private:
	CHL2MPSniperRifle(const CHL2MPSniperRifle &);
};

#endif // WEAPON_BASE_SNIPER_H
//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base HL2MP Combat Weapon
//
//==============================================================================================//

#ifndef WEAPON_BASEHL2MPCOMBATWEAPON_SHARED_H
#define WEAPON_BASEHL2MPCOMBATWEAPON_SHARED_H

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
#define CBaseHL2MPCombatWeapon C_BaseHL2MPCombatWeapon
#endif

class CBaseHL2MPCombatWeapon : public CWeaponHL2MPBase
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
	DECLARE_CLASS(CBaseHL2MPCombatWeapon, CWeaponHL2MPBase);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseHL2MPCombatWeapon();

private:
	CBaseHL2MPCombatWeapon(const CBaseHL2MPCombatWeapon &);
};

#endif // WEAPON_BASEHL2MPCOMBATWEAPON_SHARED_H

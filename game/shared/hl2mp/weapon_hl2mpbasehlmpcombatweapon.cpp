//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base HL2MP Combat Weapon
//
//==============================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(basehl2mpcombatweapon, CBaseHL2MPCombatWeapon);

IMPLEMENT_NETWORKCLASS_ALIASED(BaseHL2MPCombatWeapon, DT_BaseHL2MPCombatWeapon)

BEGIN_NETWORK_TABLE(CBaseHL2MPCombatWeapon, DT_BaseHL2MPCombatWeapon)
#if !defined( CLIENT_DLL )
#else
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CBaseHL2MPCombatWeapon)
END_DATADESC()

#endif

BEGIN_PREDICTION_DATA(CBaseHL2MPCombatWeapon)
END_PREDICTION_DATA()

CBaseHL2MPCombatWeapon::CBaseHL2MPCombatWeapon(void)
{
}
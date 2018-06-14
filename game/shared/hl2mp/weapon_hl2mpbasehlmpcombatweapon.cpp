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

LINK_ENTITY_TO_CLASS( basehl2mpcombatweapon, CBaseHL2MPCombatWeapon );

IMPLEMENT_NETWORKCLASS_ALIASED( BaseHL2MPCombatWeapon , DT_BaseHL2MPCombatWeapon )

BEGIN_NETWORK_TABLE( CBaseHL2MPCombatWeapon , DT_BaseHL2MPCombatWeapon )
#if !defined( CLIENT_DLL )
#else
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )

#include "globalstate.h"

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBaseHL2MPCombatWeapon )
END_DATADESC()

#endif

BEGIN_PREDICTION_DATA( CBaseHL2MPCombatWeapon )
END_PREDICTION_DATA()

CBaseHL2MPCombatWeapon::CBaseHL2MPCombatWeapon( void )
{
}

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
Vector CBaseHL2MPCombatWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
	return BaseClass::GetBulletSpread( proficiency );
}

//-----------------------------------------------------------------------------
float CBaseHL2MPCombatWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
	return BaseClass::GetSpreadBias( proficiency );
}
//-----------------------------------------------------------------------------

const WeaponProficiencyInfo_t *CBaseHL2MPCombatWeapon::GetProficiencyValues()
{
	return NULL;
}

#else

//-----------------------------------------------------------------------------
Vector CBaseHL2MPCombatWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
	Vector baseSpread = BaseClass::GetBulletSpread( proficiency );

	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	float flModifier = (pProficiencyValues)[ proficiency ].spreadscale;
	return ( baseSpread * flModifier );
}

//-----------------------------------------------------------------------------
float CBaseHL2MPCombatWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	return (pProficiencyValues)[ proficiency ].bias;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CBaseHL2MPCombatWeapon::GetProficiencyValues()
{
	return GetDefaultProficiencyValues();
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CBaseHL2MPCombatWeapon::GetDefaultProficiencyValues()
{
	// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
	static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
	{
		{ 2.50, 1.0	},
		{ 2.00, 1.0	},
		{ 1.50, 1.0	},
		{ 1.25, 1.0 },
		{ 1.00, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return g_BaseWeaponProficiencyTable;
}

#endif
//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Glock-17 Akimbo Handgun
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_akimbo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponGlock17Akimbo C_WeaponGlock17Akimbo
#endif

class CWeaponGlock17Akimbo : public CHL2MPBaseAkimbo
{
public:
	DECLARE_CLASS(CWeaponGlock17Akimbo, CHL2MPBaseAkimbo);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponGlock17Akimbo(void);
	int GetUniqueWeaponID() { return WEAPON_ID_GLOCK17_AKIMBO; }

private:
	CWeaponGlock17Akimbo(const CWeaponGlock17Akimbo &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGlock17Akimbo, DT_WeaponGlock17Akimbo)

BEGIN_NETWORK_TABLE(CWeaponGlock17Akimbo, DT_WeaponGlock17Akimbo)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGlock17Akimbo)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_akimbo_glock17, CWeaponGlock17Akimbo);
PRECACHE_WEAPON_REGISTER(weapon_akimbo_glock17);

CWeaponGlock17Akimbo::CWeaponGlock17Akimbo(void)
{
}
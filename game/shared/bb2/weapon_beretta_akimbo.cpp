//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Beretta M9 Akimbo Handguns
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_akimbo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponBerettaAkimbo C_WeaponBerettaAkimbo
#endif

class CWeaponBerettaAkimbo : public CHL2MPBaseAkimbo
{
public:
	DECLARE_CLASS(CWeaponBerettaAkimbo, CHL2MPBaseAkimbo);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponBerettaAkimbo(void);
	int GetUniqueWeaponID() { return WEAPON_ID_BERETTA_AKIMBO; }	

private:
	CWeaponBerettaAkimbo(const CWeaponBerettaAkimbo &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponBerettaAkimbo, DT_WeaponBerettaAkimbo)

BEGIN_NETWORK_TABLE(CWeaponBerettaAkimbo, DT_WeaponBerettaAkimbo)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponBerettaAkimbo)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_akimbo_beretta, CWeaponBerettaAkimbo);
PRECACHE_WEAPON_REGISTER(weapon_akimbo_beretta);

CWeaponBerettaAkimbo::CWeaponBerettaAkimbo(void)
{
}
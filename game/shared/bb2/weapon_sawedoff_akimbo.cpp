//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Sawed-Off Akimbo Shotguns, hOt damn!!!
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_akimbo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponSawedOffAkimbo C_WeaponSawedOffAkimbo
#endif

class CWeaponSawedOffAkimbo : public CHL2MPBaseAkimbo
{
public:
	DECLARE_CLASS(CWeaponSawedOffAkimbo, CHL2MPBaseAkimbo);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponSawedOffAkimbo(void);

	int GetWeaponType(void) { return WEAPON_TYPE_SHOTGUN; }
	int GetUniqueWeaponID() { return WEAPON_ID_SAWEDOFF_AKIMBO; }
	int GetOverloadCapacity() { return 2; }
	int GetMinBurst() { return 1; }
	int GetMaxBurst() { return 1; }
	bool UsesEmptyAnimation() { return false; }

	const char		*GetAmmoTypeName(void) { return "Buckshot"; }
	int				GetAmmoMaxCarry(void) { return 32; }

	float GetFireRate(void) 
	{ 
		return ((GetViewModelSequenceDuration() / 2.0f) + GetWpnData().m_flFireRate);
	}

	const char* GetAmmoEntityLink(void) { return "ammo_slugs"; }

private:
	CWeaponSawedOffAkimbo(const CWeaponSawedOffAkimbo&);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSawedOffAkimbo, DT_WeaponSawedOffAkimbo)

BEGIN_NETWORK_TABLE(CWeaponSawedOffAkimbo, DT_WeaponSawedOffAkimbo)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSawedOffAkimbo)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_akimbo_sawedoff, CWeaponSawedOffAkimbo);
PRECACHE_WEAPON_REGISTER(weapon_akimbo_sawedoff);

CWeaponSawedOffAkimbo::CWeaponSawedOffAkimbo(void)
{
}
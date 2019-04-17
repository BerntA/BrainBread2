//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: MP412 REX Akimbo - Revolver
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_akimbo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponREXAkimbo C_WeaponREXAkimbo
#endif

class CWeaponREXAkimbo : public CHL2MPBaseAkimbo
{
public:
	DECLARE_CLASS(CWeaponREXAkimbo, CHL2MPBaseAkimbo);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponREXAkimbo(void);
	
	void AddViewKick(void);
	bool UsesEmptyAnimation() { return false; }
	int GetOverloadCapacity() { return 3; }
	int GetUniqueWeaponID() { return WEAPON_ID_REXMP412_AKIMBO; }
	int GetWeaponType(void) { return WEAPON_TYPE_REVOLVER; }
	const char *GetAmmoEntityLink(void) { return "ammo_revolver"; }
	const char *GetMuzzleflashAttachment(bool bPrimaryAttack)
	{
		if (bPrimaryAttack)
			return "left_muzzle";

		return "right_muzzle";
	}

private:
	CWeaponREXAkimbo(const CWeaponREXAkimbo &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponREXAkimbo, DT_WeaponREXAkimbo)

BEGIN_NETWORK_TABLE(CWeaponREXAkimbo, DT_WeaponREXAkimbo)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponREXAkimbo)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_akimbo_rex, CWeaponREXAkimbo);
PRECACHE_WEAPON_REGISTER(weapon_akimbo_rex);

CWeaponREXAkimbo::CWeaponREXAkimbo(void)
{
}

void CWeaponREXAkimbo::AddViewKick(void)
{
	if (m_iMeleeAttackType.Get() > 0)
	{
		CBaseHL2MPCombatWeapon::AddViewKick();
		return;
	}

	// Normal gunfire viewkick...
	CHL2MP_Player* pPlayer = ToHL2MPPlayer(GetOwner());
	if (pPlayer)
	{
		QAngle angles = pPlayer->GetLocalAngles();

		angles.x += random->RandomInt(-1, 1);
		angles.y += random->RandomInt(-1, 1);
		angles.z = 0;
#ifndef CLIENT_DLL
		pPlayer->SnapEyeAngles(angles);
#endif

		m_iShotsFired++;
		pPlayer->ViewPunch(QAngle(-5, random->RandomFloat(-1, 1), 0));
	}
}
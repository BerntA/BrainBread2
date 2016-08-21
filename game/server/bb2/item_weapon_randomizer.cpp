//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Randomize weapon spawns.
//
//========================================================================================//

#include "cbase.h"
#include "spawnable_entity_base.h"
#include "inventory_item.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "objective_icon.h"
#include "basecombatweapon_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar bb2_weapon_randomizer_refresh_time("bb2_weapon_randomizer_refresh_time", "120", FCVAR_REPLICATED, "For how long can random weapons spawned by the item_weapon_randomizer stay before being refreshed? (seconds)", true, 30.0f, false, 0.0f);

struct weaponInfoItem
{
	const char *classname;
	int chance;
	bool melee;
};

weaponInfoItem pszWeapons[] =
{
	{ "weapon_glock17", 65, false },
	{ "weapon_rex", 35, false },
	{ "weapon_akimbo_rex", 20, false },
	{ "weapon_mac11", 60, false },
	{ "weapon_mp7", 45, false },
	{ "weapon_ak47", 100, false },
	{ "weapon_famas", 50, false },
	{ "weapon_remington700", 15, false },
	{ "weapon_minigun", 4, false },
	{ "weapon_sawedoff", 25, false },
	{ "weapon_remington", 40, false },
	{ "weapon_m9_bayonet", 100, true },
	{ "weapon_fireaxe", 50, true },
	{ "weapon_machete", 30, true },
	{ "weapon_sledgehammer", 25, true },
	{ "weapon_baseballbat", 45, true },
	{ "weapon_brick", 60, true },
	{ "weapon_hatchet", 65, true },
};

const char *GetRandomWeapon(int type)
{
	int chance = random->RandomInt(0, 100);
	CUtlVector<int> weaponIndexes;
	for (int i = 0; i < _ARRAYSIZE(pszWeapons); i++)
	{
		if ((pszWeapons[i].melee && (type == 1)) || (!pszWeapons[i].melee && (type == 2)))
			continue;

		if (pszWeapons[i].chance < chance)
			continue;

		weaponIndexes.AddToTail(i);
	}

	if (!weaponIndexes.Count())
		return "";

	return (pszWeapons[weaponIndexes[random->RandomInt(0, (weaponIndexes.Count() - 1))]].classname);
}

class CItemWeaponRandomizer : public CSpawnableEntity
{
public:
	DECLARE_CLASS(CItemWeaponRandomizer, CSpawnableEntity);
	DECLARE_DATADESC();

	CItemWeaponRandomizer();

protected:
	bool ShouldRespawnEntity(CBaseEntity *pActiveEntity);
	CBaseEntity *SpawnNewEntity(void);

private:
	int m_iItemType;
	bool m_bShouldRefresh;
	float m_flTimeSinceLastSpawn;
};

BEGIN_DATADESC(CItemWeaponRandomizer)
DEFINE_KEYFIELD(m_iItemType, FIELD_INTEGER, "WeaponType"),
DEFINE_KEYFIELD(m_bShouldRefresh, FIELD_BOOLEAN, "ShouldRefresh"),
DEFINE_FIELD(m_flTimeSinceLastSpawn, FIELD_FLOAT),
END_DATADESC()

LINK_ENTITY_TO_CLASS(item_weapon_randomizer, CItemWeaponRandomizer);

CItemWeaponRandomizer::CItemWeaponRandomizer()
{
	m_iItemType = 0;
	m_bShouldRefresh = true;
	m_flTimeSinceLastSpawn = 0.0f;
}

bool CItemWeaponRandomizer::ShouldRespawnEntity(CBaseEntity *pActiveEntity)
{
	bool ret = BaseClass::ShouldRespawnEntity(pActiveEntity);
	if (pActiveEntity)
	{

		CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*> (pActiveEntity);
		if (pWeapon && pWeapon->GetOwner())
			ret = true;
		else
		{
			float timeElapsedSinceLastSpawn = gpGlobals->curtime - m_flTimeSinceLastSpawn;
			if ((timeElapsedSinceLastSpawn >= bb2_weapon_randomizer_refresh_time.GetFloat()) && m_bShouldRefresh)
			{
				ret = true;

				CBaseEntity *pEntity = m_pEntityToSpawn.Get();
				if (pEntity)
				{
					UTIL_Remove(pEntity);
					m_pEntityToSpawn = NULL;
				}
			}
		}
	}

	return ret;
}

CBaseEntity *CItemWeaponRandomizer::SpawnNewEntity(void)
{
	const char *pszClassname = GetRandomWeapon(m_iItemType);
	if (strlen(pszClassname) <= 0)
		return NULL;

	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)CreateEntityByName(pszClassname);
	if (pWeapon)
	{
		Vector vecOrigin = GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

		pWeapon->SetAbsOrigin(vecOrigin);
		pWeapon->SetAbsAngles(QAngle(0, 0, 0));
		pWeapon->Spawn();
		pWeapon->m_bSuppressRespawn = true;
		pWeapon->EnableRotationEffect();
		UTIL_DropToFloor(pWeapon, MASK_SOLID_BRUSHONLY, this);
	}

	m_flTimeSinceLastSpawn = gpGlobals->curtime;
	return pWeapon;
}

//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Randomize explosive weapon spawns.
//
//========================================================================================//

const char *pszExplosiveClassnames[] =
{
	"weapon_frag",
	"weapon_propane",
};

class CItemExplosiveRandomizer : public CSpawnableEntity
{
public:
	DECLARE_CLASS(CItemExplosiveRandomizer, CSpawnableEntity);
	DECLARE_DATADESC();

	CItemExplosiveRandomizer();

protected:
	bool ShouldRespawnEntity(CBaseEntity *pActiveEntity);
	CBaseEntity *SpawnNewEntity(void);
};

BEGIN_DATADESC(CItemExplosiveRandomizer)
END_DATADESC()

LINK_ENTITY_TO_CLASS(item_explosive_randomizer, CItemExplosiveRandomizer);

CItemExplosiveRandomizer::CItemExplosiveRandomizer()
{
}

bool CItemExplosiveRandomizer::ShouldRespawnEntity(CBaseEntity *pActiveEntity)
{
	bool ret = BaseClass::ShouldRespawnEntity(pActiveEntity);
	if (pActiveEntity)
	{
		CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*> (pActiveEntity);
		if (pWeapon && pWeapon->GetOwner())
			ret = true;
	}

	return ret;
}

CBaseEntity *CItemExplosiveRandomizer::SpawnNewEntity(void)
{
	const char *pszClassname = pszExplosiveClassnames[random->RandomInt(0, (_ARRAYSIZE(pszExplosiveClassnames) - 1))];
	if (strlen(pszClassname) <= 0)
		return NULL;

	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)CreateEntityByName(pszClassname);
	if (pWeapon)
	{
		Vector vecOrigin = GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

		pWeapon->SetAbsOrigin(vecOrigin);
		pWeapon->SetAbsAngles(QAngle(0, 0, 0));
		pWeapon->Spawn();
		pWeapon->m_bSuppressRespawn = true;
		pWeapon->EnableRotationEffect();
		UTIL_DropToFloor(pWeapon, MASK_SOLID_BRUSHONLY, this);
	}

	return pWeapon;
}
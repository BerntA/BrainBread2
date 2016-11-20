//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Ammunition Clip Defs 
//
//========================================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "baseanimating.h"
#include "items.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "hl2mp_gamerules.h"
#include "player.h"
#include "ammodef.h"
#include "npcevent.h"
#include "eventlist.h"

int GetAmmoCountMultiplier(int wepType)
{
	if (wepType == WEAPON_TYPE_SHOTGUN)
		return 3;

	return 2;
}

bool CanReplenishAmmo(CBasePlayer *pPlayer, const char *linkedWeapons[], int size, bool bSecondaryType = false, bool bSuppressSound = false)
{
	if (!pPlayer)
		return false;

	bool bReceived = false;
	for (int i = 0; i < 4; i++)
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if (!pWeapon)
			continue;

		bool bFoundWeapon = false;
		for (int iWep = 0; iWep < size; iWep++)
		{
			if (FClassnameIs(pWeapon, linkedWeapons[iWep]))
			{
				bFoundWeapon = true;
				break;
			}
		}

		if (!bFoundWeapon)
			continue;

		int wepType = pWeapon->GetWeaponType();
		int ammoCount = 0;
		if (bSecondaryType)
		{
			ammoCount = (pWeapon->GetMaxClip2() * GetAmmoCountMultiplier(wepType));
			if (pPlayer->GiveAmmo(ammoCount, pWeapon->GetSecondaryAmmoType(), bSuppressSound))
				bReceived = true;
		}
		else
		{
			ammoCount = (pWeapon->GetMaxClip1() * GetAmmoCountMultiplier(wepType));
			if (pPlayer->GiveAmmo(ammoCount, pWeapon->GetPrimaryAmmoType(), bSuppressSound))
				bReceived = true;
		}
	}

	return bReceived;
}

class CAmmoPistol : public CItem
{
public:
	DECLARE_CLASS(CAmmoPistol, CItem);

	CAmmoPistol()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_pistol, CAmmoPistol);
PRECACHE_REGISTER(ammo_pistol);

void CAmmoPistol::Spawn(void)
{
	Precache();
	SetModel("models/weapons/ammo/pistol_ammo.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoPistol::Precache(void)
{
	PrecacheModel("models/weapons/ammo/pistol_ammo.mdl");
}

void CAmmoPistol::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_beretta", "weapon_glock17" };
	if (CanReplenishAmmo(pPlayer, weapons, 2))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}

class CAmmoRifle : public CItem
{
public:
	DECLARE_CLASS(CAmmoRifle, CItem);

	CAmmoRifle()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_rifle, CAmmoRifle);
PRECACHE_REGISTER(ammo_rifle);

void CAmmoRifle::Spawn(void)
{
	Precache();
	SetModel("models/items/combine_rifle_cartridge01.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoRifle::Precache(void)
{
	PrecacheModel("models/items/combine_rifle_cartridge01.mdl");
}

void CAmmoRifle::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_ak47", "weapon_famas" };
	if (CanReplenishAmmo(pPlayer, weapons, 2))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}

class CAmmoSlugs : public CItem
{
public:
	DECLARE_CLASS(CAmmoSlugs, CItem);

	CAmmoSlugs()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_slugs, CAmmoSlugs);
PRECACHE_REGISTER(ammo_slugs);

void CAmmoSlugs::Spawn(void)
{
	Precache();
	SetModel("models/items/boxbuckshot.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoSlugs::Precache(void)
{
	PrecacheModel("models/items/boxbuckshot.mdl");
}

void CAmmoSlugs::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_sawedoff", "weapon_remington" };
	if (CanReplenishAmmo(pPlayer, weapons, 2))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}

class CAmmoRevolver : public CItem
{
public:
	DECLARE_CLASS(CAmmoRevolver, CItem);

	CAmmoRevolver()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_revolver, CAmmoRevolver);
PRECACHE_REGISTER(ammo_revolver);

void CAmmoRevolver::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_speedloader.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoRevolver::Precache(void)
{
	PrecacheModel("models/items/ammo_speedloader.mdl");
}

void CAmmoRevolver::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_rex", "weapon_akimbo_rex" };
	if (CanReplenishAmmo(pPlayer, weapons, 2))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}

class CAmmoSMG : public CItem
{
public:
	DECLARE_CLASS(CAmmoSMG, CItem);

	CAmmoSMG()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_smg, CAmmoSMG);
PRECACHE_REGISTER(ammo_smg);

void CAmmoSMG::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_smg.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoSMG::Precache(void)
{
	PrecacheModel("models/items/ammo_smg.mdl");
}

void CAmmoSMG::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_mac11", "weapon_mp7" };
	if (CanReplenishAmmo(pPlayer, weapons, 2))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}

class CAmmoSniper : public CItem
{
public:
	DECLARE_CLASS(CAmmoSniper, CItem);

	CAmmoSniper()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_sniper, CAmmoSniper);
PRECACHE_REGISTER(ammo_sniper);

void CAmmoSniper::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_700.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoSniper::Precache(void)
{
	PrecacheModel("models/items/ammo_700.mdl");
}

void CAmmoSniper::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_remington700" };
	if (CanReplenishAmmo(pPlayer, weapons, 1))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}

class CAmmoTrapper : public CItem
{
public:
	DECLARE_CLASS(CAmmoTrapper, CItem);

	CAmmoTrapper()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(ammo_trapper, CAmmoTrapper);
PRECACHE_REGISTER(ammo_trapper);

void CAmmoTrapper::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_winchester.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CAmmoTrapper::Precache(void)
{
	PrecacheModel("models/items/ammo_winchester.mdl");
}

void CAmmoTrapper::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (!CanPickup())
		return;

	const char *weapons[] = { "weapon_winchester1894" };
	if (CanReplenishAmmo(pPlayer, weapons, 1))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
}
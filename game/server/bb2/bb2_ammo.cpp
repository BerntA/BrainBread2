//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
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

bool CanReplenishAmmo(const char *ammoClassname, CBasePlayer *pPlayer, int amountOverride, bool bSecondaryType = false, bool bSuppressSound = false)
{
	if (!pPlayer)
		return false;

	bool bReceived = false;
	for (int i = 0; i < 4; i++)
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if (!pWeapon)
			continue;

		if (strcmp(pWeapon->GetAmmoEntityLink(), ammoClassname))
			continue;

		int wepType = pWeapon->GetWeaponType();
		int ammoCount = 0;
		if (bSecondaryType)
		{
			ammoCount = ((amountOverride > 0) ? amountOverride : (pWeapon->GetMaxClip2() * GetAmmoCountMultiplier(wepType)));
			if (pPlayer->GiveAmmo(ammoCount, pWeapon->GetSecondaryAmmoType(), bSuppressSound))
				bReceived = true;
		}
		else
		{
			ammoCount = ((amountOverride > 0) ? amountOverride : (pWeapon->GetMaxClip1() * GetAmmoCountMultiplier(wepType)));
			if (pPlayer->GiveAmmo(ammoCount, pWeapon->GetPrimaryAmmoType(), bSuppressSound))
				bReceived = true;
		}
	}

	return bReceived;
}

class CAmmoItemBase : public CItem
{
public:
	DECLARE_CLASS(CAmmoItemBase, CItem);

	CAmmoItemBase()
	{
		color32 col32 = { 135, 206, 250, 240 };
		m_GlowColor = col32;
		m_iAmmoAmountOverride = 0;
	}

	virtual void Spawn(void);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void SetAmmoOverrideAmount(int amount) { m_iAmmoAmountOverride = amount; }

protected:
	int m_iAmmoAmountOverride;
};

void CAmmoItemBase::Spawn(void)
{
	BaseClass::Spawn();

	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
}

void CAmmoItemBase::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (pActivator->IsZombie())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);
	if (!pPlayer)
		return;

	if (!CanPickup())
		return;

	if (CanReplenishAmmo(GetClassname(), pPlayer, m_iAmmoAmountOverride))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
			Respawn();
		else
			UTIL_Remove(this);
	}
}

#define AMMO_BASE_CLASS CAmmoItemBase

class CAmmoPistol : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoPistol, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_pistol, CAmmoPistol);
PRECACHE_REGISTER(ammo_pistol);

void CAmmoPistol::Spawn(void)
{
	Precache();
	SetModel("models/weapons/ammo/pistol_ammo.mdl");
	BaseClass::Spawn();
}

void CAmmoPistol::Precache(void)
{
	PrecacheModel("models/weapons/ammo/pistol_ammo.mdl");
}

class CAmmoRifle : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoRifle, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_rifle, CAmmoRifle);
PRECACHE_REGISTER(ammo_rifle);

void CAmmoRifle::Spawn(void)
{
	Precache();
	SetModel("models/items/combine_rifle_cartridge01.mdl");
	BaseClass::Spawn();
}

void CAmmoRifle::Precache(void)
{
	PrecacheModel("models/items/combine_rifle_cartridge01.mdl");
}

class CAmmoSlugs : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoSlugs, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_slugs, CAmmoSlugs);
PRECACHE_REGISTER(ammo_slugs);

void CAmmoSlugs::Spawn(void)
{
	Precache();
	SetModel("models/items/boxbuckshot.mdl");
	BaseClass::Spawn();
}

void CAmmoSlugs::Precache(void)
{
	PrecacheModel("models/items/boxbuckshot.mdl");
}

class CAmmoRevolver : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoRevolver, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_revolver, CAmmoRevolver);
PRECACHE_REGISTER(ammo_revolver);

void CAmmoRevolver::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_speedloader.mdl");
	BaseClass::Spawn();
}

void CAmmoRevolver::Precache(void)
{
	PrecacheModel("models/items/ammo_speedloader.mdl");
}

class CAmmoSMG : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoSMG, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_smg, CAmmoSMG);
PRECACHE_REGISTER(ammo_smg);

void CAmmoSMG::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_smg.mdl");
	BaseClass::Spawn();
}

void CAmmoSMG::Precache(void)
{
	PrecacheModel("models/items/ammo_smg.mdl");
}

class CAmmoSniper : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoSniper, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_sniper, CAmmoSniper);
PRECACHE_REGISTER(ammo_sniper);

void CAmmoSniper::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_700.mdl");
	BaseClass::Spawn();
}

void CAmmoSniper::Precache(void)
{
	PrecacheModel("models/items/ammo_700.mdl");
}

class CAmmoTrapper : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoTrapper, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(ammo_trapper, CAmmoTrapper);
PRECACHE_REGISTER(ammo_trapper);

void CAmmoTrapper::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_winchester.mdl");
	BaseClass::Spawn();
}

void CAmmoTrapper::Precache(void)
{
	PrecacheModel("models/items/ammo_winchester.mdl");
}

CON_COMMAND(drop_ammo, "Drop ammo, give ammo to your teammates.")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pPlayer)
		return;

	if (!pPlayer->IsHuman() || !pPlayer->IsAlive())
		return;

	CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if (!pWeapon)
		return;

	if (pWeapon->IsMeleeWeapon() || !pWeapon->UsesClipsForAmmo1() || !pWeapon->UsesPrimaryAmmo())
		return;

	int ammoCount = pPlayer->GetAmmoCount(pWeapon->m_iPrimaryAmmoType);
	if (ammoCount <= 0)
		return;

	const char *classNew = pWeapon->GetAmmoEntityLink();
	if (strlen(classNew) <= 0)
		return;

	int ammoForItem = pWeapon->GetMaxClip1();
	if (ammoForItem > ammoCount)
		ammoForItem = ammoCount;

	pPlayer->RemoveAmmo(ammoForItem, pWeapon->m_iPrimaryAmmoType);
	CAmmoItemBase *pEntity = (CAmmoItemBase*)CreateEntityByName(classNew);
	if (pEntity)
	{
		Vector vecAbsOrigin = pPlayer->GetAbsOrigin();
		int iCollisionGroup = pPlayer ? pPlayer->GetCollisionGroup() : COLLISION_GROUP_WEAPON;
		Vector vecStartPos = vecAbsOrigin + Vector(0, 0, 20);
		Vector vecEndPos = vecAbsOrigin;
		Vector vecDir = (vecEndPos - vecStartPos);
		VectorNormalize(vecDir);

		trace_t tr;
		CTraceFilterNoNPCsOrPlayer trFilter(pPlayer, iCollisionGroup);
		UTIL_TraceLine(vecStartPos, vecEndPos + (vecDir * MAX_TRACE_LENGTH), MASK_SHOT, &trFilter, &tr);

		pEntity->Spawn();
		pEntity->AddSpawnFlags(SF_NORESPAWN);
		pEntity->SetAmmoOverrideAmount(ammoForItem);

		Vector endPoint = tr.endpos;
		const model_t *pModel = modelinfo->GetModel(pEntity->GetModelIndex());
		if (pModel)
		{
			Vector mins, maxs;
			modelinfo->GetModelBounds(pModel, mins, maxs);
			endPoint.z += maxs.z;
		}
		pEntity->SetLocalOrigin(endPoint);

		pPlayer->AddAssociatedAmmoEnt(pEntity);
	}
}
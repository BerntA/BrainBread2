//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Ammunition Clip Defs 
//
//========================================================================================//

#include "cbase.h"
#include "items.h"
#include "hl2_player.h"
#include "hl2mp_gamerules.h"
#include "ammodef.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"

int GetAmmoCountMultiplier(int wepType, int wepId)
{
	if (wepId == WEAPON_ID_SAWEDOFF)
		return 8;
	else if (wepType == WEAPON_TYPE_SHOTGUN)
		return 3;

	return 2;
}

// Replenish ammo for non special weapons:
bool CanReplenishAmmo(const char *ammoClassname, CBasePlayer *pPlayer, int amountOverride, bool bSuppressSound = false)
{
	if (!pPlayer)
		return false;

	bool bReceived = false;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if (!pWeapon || pWeapon->IsMeleeWeapon() || (pWeapon->GetAmmoTypeID() == -1) || strcmp(pWeapon->GetAmmoEntityLink(), ammoClassname))
			continue;

		int ammoCount = ((amountOverride > 0) ? amountOverride : (pWeapon->GetMaxClip() * GetAmmoCountMultiplier(pWeapon->GetWeaponType(), pWeapon->GetUniqueWeaponID())));
		if (pWeapon->GiveAmmo(ammoCount, bSuppressSound))
			bReceived = true;
	}

	return bReceived;
}

// Replenish ammo for special weapons:
bool CanReplenishAmmo(const char *ammoClassname, CBasePlayer *pPlayer, bool bSuppressSound = false)
{
	if (!pPlayer)
		return false;

	bool bReceived = false;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if (!pWeapon)
			continue;

		if (pWeapon->IsMeleeWeapon() || (pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL))
			continue;

		int nAmmoIndex = pWeapon->GetAmmoTypeID();
		if (nAmmoIndex == -1)
			continue;

		if (strcmp(pWeapon->GetAmmoEntityLink(), ammoClassname))
			continue;

		CSingleUserRecipientFilter user(pPlayer);
		user.MakeReliable();

		if (pWeapon->m_iClip >= pWeapon->GetMaxClip())
		{
			UserMessageBegin(user, "AmmoDenied");
			WRITE_SHORT(nAmmoIndex);
			MessageEnd();
			continue;
		}
		else
		{
			if (bSuppressSound == false)
				pPlayer->EmitSound("BaseCombatCharacter.AmmoPickup");

			UserMessageBegin(user, "ItemPickup");
			WRITE_STRING("AMMO");
			WRITE_SHORT(nAmmoIndex);
			MessageEnd();

			bReceived = true;
		}

		pWeapon->m_iClip = pWeapon->GetMaxClip();
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
	virtual bool CanReplenishAmmo(CBasePlayer *pPlayer);

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
	if (!pActivator || !pActivator->IsPlayer() || pActivator->IsZombie() || !CanPickup())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);
	if (!pPlayer)
		return;

	if (CanReplenishAmmo(pPlayer))
	{
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
			Respawn();
		else
			UTIL_Remove(this);
	}
}

bool CAmmoItemBase::CanReplenishAmmo(CBasePlayer *pPlayer)
{
	return (::CanReplenishAmmo(GetClassname(), pPlayer, m_iAmmoAmountOverride));
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

class CAmmoCannister : public AMMO_BASE_CLASS
{
public:
	DECLARE_CLASS(CAmmoCannister, AMMO_BASE_CLASS);

	void Spawn(void);
	void Precache(void);
	bool CanReplenishAmmo(CBasePlayer *pPlayer) { return (::CanReplenishAmmo(GetClassname(), pPlayer)); }
};

LINK_ENTITY_TO_CLASS(ammo_cannister, CAmmoCannister);
PRECACHE_REGISTER(ammo_cannister);

void CAmmoCannister::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_flamethrower.mdl");
	BaseClass::Spawn();
}

void CAmmoCannister::Precache(void)
{
	PrecacheModel("models/items/ammo_flamethrower.mdl");
}

#define AMMO_DROP_WAIT_TIME 1.0f

CON_COMMAND(drop_ammo, "Drop ammo, give ammo to your teammates.")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pPlayer || !pPlayer->IsHuman() || !pPlayer->IsAlive() || !HL2MPRules()->IsTeamplay() || HL2MPRules()->IsGameoverOrScoresVisible())
		return;

	CBaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if (!pWeapon || pWeapon->IsMeleeWeapon() || !pWeapon->UsesClipsForAmmo() || (pWeapon->GetAmmoTypeID() == -1) || (pWeapon->GetWeaponType() == WEAPON_TYPE_SPECIAL))
		return;

	int ammoCount = pWeapon->GetAmmoCount();
	if (ammoCount <= 0)
		return;

	const char *classNew = pWeapon->GetAmmoEntityLink();
	if (!classNew || !classNew[0])
		return;

	int ammoForItem = MIN(pWeapon->GetMaxClip(), ammoCount);
	float timeSinceLastDrop = gpGlobals->curtime - pPlayer->GetLastTimeDroppedAmmo();
	if (timeSinceLastDrop < AMMO_DROP_WAIT_TIME)
	{
		char pchTime[16];
		Q_snprintf(pchTime, 16, "%.1f", fabs((AMMO_DROP_WAIT_TIME - timeSinceLastDrop)));
		GameBaseServer()->SendToolTip("#TOOLTIP_AMMO_DROP_DENY", GAME_TIP_WARNING, pPlayer->entindex(), pchTime);
		return;
	}

	pPlayer->OnDroppedAmmoNow();
	pWeapon->RemoveAmmo(ammoForItem);
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
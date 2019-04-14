//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Propane Tank! 
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "GameBase_Shared.h"

#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#else
#include "te_effect_dispatch.h"
#endif

#include "basegrenade_shared.h"
#include "effect_dispatch_data.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PROPANE_GRIEF_XP 50
#define PROPANE_THROW_OFFSET_FORWARD 5.0f
#define PROPANE_THROW_OFFSET_RIGHT 5.0f
#define RETHROW_DELAY	0.5

#ifdef CLIENT_DLL
#define CWeaponPropane C_WeaponPropane
#endif

#ifndef CLIENT_DLL
ConVar sk_weapon_propane_lockedtime("sk_weapon_propane_lockedtime", "2", FCVAR_GAMEDLL);

class CPropaneExplosive : public CBaseGrenade
{
	DECLARE_CLASS(CPropaneExplosive, CBaseGrenade);

public:

	CPropaneExplosive();
	~CPropaneExplosive();

	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void RemoveProp(void);
	virtual void SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity);
	virtual bool CreateVPhysics();
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	OnTakeDamage(const CTakeDamageInfo &inputInfo);
	virtual void SetProperties(CBaseEntity *pThrower);
	virtual bool CanEntityHurtUs(CBaseEntity *pEntity);

private:
	bool m_bExploded;
	float m_flLockedTime;

	EHANDLE m_pThrowOwner;
};

LINK_ENTITY_TO_CLASS(prop_propane_explosive, CPropaneExplosive);

CPropaneExplosive::CPropaneExplosive()
{
	m_bExploded = false;
	m_flLockedTime = 0.0f;
	m_pThrowOwner = NULL;
}

CPropaneExplosive::~CPropaneExplosive()
{
}

void CPropaneExplosive::SetProperties(CBaseEntity *pThrower)
{
	m_flLockedTime = gpGlobals->curtime + sk_weapon_propane_lockedtime.GetFloat();
	m_pThrowOwner = pThrower;
}

bool CPropaneExplosive::CanEntityHurtUs(CBaseEntity *pEntity)
{
	if (!pEntity)
		return false;

	if (m_flLockedTime > gpGlobals->curtime)
	{
		CBaseEntity *pThrower = m_pThrowOwner.Get();
		if (!pThrower)
			return true;

		if (pThrower != pEntity)
			return false;
	}

	return true;
}

int CPropaneExplosive::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	if (m_bExploded)
		return 0;

	CBaseEntity *pAttacker = inputInfo.GetAttacker();
	CBaseEntity *pInflictor = inputInfo.GetInflictor();
	if (!pAttacker || !pInflictor)
		return 0;

	if (!CanEntityHurtUs(pAttacker))
		return 0;

	m_bExploded = true;

	// If we've been damaged by another propane entity, kill us instantly! (prevent overload)
	if (FClassnameIs(pInflictor, "prop_propane_explosive") || FClassnameIs(pAttacker, "prop_propane_explosive"))
	{
		UTIL_Remove(this);
		return 1;
	}

	// If someone else destroys the throwers propane tank they will receive some 'you suck' kind of griefing XP.
	CHL2MP_Player *pThrower = ToHL2MPPlayer(m_pThrowOwner.Get());
	if (pThrower && (pThrower != pAttacker))
		pThrower->CanLevelUp(PROPANE_GRIEF_XP, NULL);

	SetThrower(ToBaseCombatCharacter(pAttacker));
	CSoundEnt::InsertSound(SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this);
	Detonate();
	return 1;
}

void CPropaneExplosive::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	BaseClass::Use(pActivator, pCaller, useType, value);

	if (!pActivator->IsHuman())
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pActivator);
	if (!pPlayer)
		return;

	CBaseCombatWeapon *pPropane = pPlayer->Weapon_OwnsThisType("weapon_propane");
	if (pPropane)
	{
		int iAmmoType = GetAmmoDef()->Index("Propane");
		if (iAmmoType == -1)
			return;

		pPlayer->GiveAmmo(1, iAmmoType, false);
		UTIL_Remove(this);
		return;
	}

	// No propane? Create propane then.
	if (pPlayer->GiveItem("weapon_propane", true))
		UTIL_Remove(this);
}

void CPropaneExplosive::SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if (pPhysicsObject)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}
}

bool CPropaneExplosive::CreateVPhysics()
{
	VPhysicsDestroyObject();
	VPhysicsInitNormal(SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false);
	return true;
}

void CPropaneExplosive::Precache(void)
{
	PrecacheModel("models/weapons/explosives/propane/w_propane.mdl");
}

void CPropaneExplosive::RemoveProp(void)
{
	float flDuration = (HL2MPRules()->IsFastPacedGameplay() ? 35.0f : 15.0f);
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(gpGlobals->curtime + flDuration);
}

void CPropaneExplosive::Spawn(void)
{
	Precache();

	BaseClass::Spawn();

	SetSolid(SOLID_BBOX);

	SetModel("models/weapons/explosives/propane/w_propane.mdl");

	CreateVPhysics();

	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	m_takedamage = DAMAGE_EVENTS_ONLY;
	m_iHealth = 1;
	SetBlocksLOS(false);

	const model_t *pModel = modelinfo->GetModel(this->GetModelIndex());
	if (pModel)
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds(pModel, mins, maxs);
		this->SetCollisionBounds(mins, maxs);
	}

	CollisionProp()->UseTriggerBounds(true, 10.0f);

	const DataExplosiveItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetExplosiveDataForType(EXPLOSIVE_TYPE_PROPANE);
	if (data)
	{
		m_flDamage = data->flPlayerDamage;
		m_DmgRadius = data->flRadius;
	}

	color32 col32 = { 70, 130, 180, 255 };
	m_GlowColor = col32;
	SetGlowMode(GLOW_MODE_RADIUS);
}
#endif

//-----------------------------------------------------------------------------
// Propane Gas Wep...
//-----------------------------------------------------------------------------
class CWeaponPropane : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponPropane, CBaseHL2MPCombatWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponPropane();

	void	Precache(void);
	void	PrimaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter *pOwner);
	void	ItemPostFrame(void);
	void    Drop(const Vector &vecVelocity);

	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	bool	Reload(void);

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

	void	ThrowGrenade(CBasePlayer *pPlayer);

	bool CanPickupWeaponAsAmmo() { return true; }
	bool CanPerformMeleeAttacks() { return false; }

	int GetUniqueWeaponID() { return WEAPON_ID_PROPANE; }

private:
	CWeaponPropane(const CWeaponPropane &);

#ifndef CLIENT_DLL
	bool m_bRemoveWeapon;
#endif
};

acttable_t	CWeaponPropane::m_acttable[] =
{
#ifdef BB2_AI
{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },

{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_PROPANE, false },
{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_PROPANE, false },

{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
{ ACT_MP_WALK, ACT_HL2MP_WALK_PROPANE, false },
{ ACT_MP_RUN, ACT_HL2MP_RUN_PROPANE, false },
{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_PROPANE, false },

{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_PROPANE, false },
{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_PROPANE, false },

{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_PROPANE, false },
{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_PROPANE, false },

{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_PROPANE, false },
{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_PROPANE, false },

{ ACT_MP_JUMP, ACT_HL2MP_JUMP_PROPANE, false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE(CWeaponPropane);

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPropane, DT_WeaponPropane)

BEGIN_NETWORK_TABLE(CWeaponPropane, DT_WeaponPropane)
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponPropane)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_propane, CWeaponPropane);
PRECACHE_WEAPON_REGISTER(weapon_propane);

CWeaponPropane::CWeaponPropane(void) : CBaseHL2MPCombatWeapon()
{
#ifndef CLIENT_DLL
	m_bRemoveWeapon = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropane::Precache(void)
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther("prop_propane_explosive");
#endif
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponPropane::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	bool fThrewGrenade = false;

	switch (pEvent->event)
	{
	case EVENT_WEAPON_THROW:
	{
		ThrowGrenade(pPlayer);
		DecrementAmmo(pPlayer);
		fThrewGrenade = true;
		HL2MPRules()->EmitSoundToClient(pPlayer, "ThrowPropane", pPlayer->GetSoundType(), pPlayer->GetSoundsetGender());
		break;
	}

	case EVENT_WEAPON_SEQUENCE_FINISHED:
	{
		m_bRemoveWeapon = true;
		pPlayer->Weapon_DropSlot(GetSlot()); // throw this weapon away!
		break;
	}

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}

	if (fThrewGrenade)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponPropane::Deploy(void)
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPropane::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPropane::Reload(void)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropane::PrimaryAttack(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());;
	if (!pPlayer)
		return;

	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	SendWeaponAnim(ACT_VM_THROW);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_THROW);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponPropane::DecrementAmmo(CBaseCombatCharacter *pOwner)
{
	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPropane::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();
}

void CWeaponPropane::Drop(const Vector &vecVelocity)
{
#ifndef CLIENT_DLL
	if (m_bRemoveWeapon)
	{
		UTIL_Remove(this);
		return;
	}
#endif

	BaseClass::Drop(vecVelocity);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponPropane::ThrowGrenade(CBasePlayer *pPlayer)
{
#ifndef CLIENT_DLL
	Vector vForward, vRight;
	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = pPlayer->EyePosition() + (vForward * PROPANE_THROW_OFFSET_FORWARD) + (vRight * PROPANE_THROW_OFFSET_RIGHT);
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vForward * 1400;

	CPropaneExplosive *pPropane = (CPropaneExplosive*)CBaseEntity::Create("prop_propane_explosive", vecSrc, vec3_angle);
	if (pPropane)
	{
		pPropane->SetVelocity(vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0));

		if (pPlayer && pPlayer->m_lifeState != LIFE_ALIVE)
		{
			pPlayer->GetVelocity(&vecThrow, NULL);

			IPhysicsObject *pPhysicsObject = pPropane->VPhysicsGetObject();
			if (pPhysicsObject)
			{
				pPhysicsObject->SetVelocity(&vecThrow, NULL);
			}
		}

		pPropane->SetHealth(1);
		pPropane->SetMaxHealth(1);
		pPropane->m_iExplosiveType = EXPLOSIVE_TYPE_PROPANE;
		pPropane->RemoveProp();
		pPropane->SetProperties(pPlayer);
	}
#endif

	WeaponSound(SINGLE);
}
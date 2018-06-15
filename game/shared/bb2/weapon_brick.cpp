//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Brick Melee 
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#include "props.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponBrick C_WeaponBrick
#endif

ConVar sk_weapon_brick_throwforce("sk_weapon_brick_throwforce", "6000", FCVAR_REPLICATED);
ConVar sk_weapon_brick_throwdistance("sk_weapon_brick_throwdistance", "250", FCVAR_REPLICATED);

#define BRICK_OFFSET_FORWARD 6.0f
#define BRICK_OFFSET_RIGHT 6.0f

#ifndef CLIENT_DLL
class CPropBrick : public CPhysicsProp
{
public:
	DECLARE_CLASS(CPropBrick, CPhysicsProp);

	CPropBrick();
	~CPropBrick();

	virtual void Precache(void);
	virtual void Spawn(void);
	virtual void SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity);
	virtual bool CreateVPhysics();
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void SetProperties(CBasePlayer *pThrower, float damage);

	void BrickTouch(CBaseEntity *pOther);
	unsigned int PhysicsSolidMaskForEntity() const;

private:
	float m_flDamage;
	EHANDLE m_pThrower;
	Vector vecThrownFrom;
	Vector vecThrowVelocity;
};

LINK_ENTITY_TO_CLASS(prop_thrown_brick, CPropBrick);

CPropBrick::CPropBrick()
{
	m_flDamage = 0.0f;
	m_pThrower = NULL;
	vecThrownFrom = Vector(0, 0, 0);
}

CPropBrick::~CPropBrick()
{
}

void CPropBrick::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator->IsHuman())
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pActivator);
	if (!pPlayer)
		return;

	CBaseCombatWeapon *pBrick = pPlayer->Weapon_OwnsThisType("weapon_brick");
	if (pBrick)
		return;

	if (pPlayer->GiveItem("weapon_brick", true))
		UTIL_Remove(this);
}

void CPropBrick::SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if (pPhysicsObject)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}

	vecThrowVelocity = velocity;
}

bool CPropBrick::CreateVPhysics()
{
	VPhysicsDestroyObject();
	VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);
	return true;
}

unsigned int CPropBrick::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

void CPropBrick::Precache(void)
{
	PrecacheModel("models/weapons/melee/brick/w_brick.mdl");
}

void CPropBrick::Spawn(void)
{
	Precache();

	SetSolid(SOLID_VPHYSICS);

	SetModel("models/weapons/melee/brick/w_brick.mdl");

	CreateVPhysics();

	m_takedamage = DAMAGE_EVENTS_ONLY;
	m_iHealth = 1;
	SetBlocksLOS(false);

	color32 col32 = { 70, 130, 180, 255 };
	m_GlowColor = col32;
	SetGlowMode(GLOW_MODE_RADIUS);

	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(gpGlobals->curtime + 6.0f);

	SetTouch(&CPropBrick::BrickTouch);
}

void CPropBrick::BrickTouch(CBaseEntity *pOther)
{
	if (!pOther)
		return;

	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
		return;

	// Never damage our owner!
	CBaseEntity *pThrower = m_pThrower.Get();
	if (pThrower == pOther || (pThrower == NULL))
		return;

	float damage = m_flDamage;

	// Too far away? Add dmg drop off.
	float distanceFromStartArea = (vecThrownFrom - GetAbsOrigin()).Length();
	if (distanceFromStartArea > sk_weapon_brick_throwdistance.GetFloat())
		damage = (damage * (sk_weapon_brick_throwdistance.GetFloat() / distanceFromStartArea));

	if (pOther->m_takedamage != DAMAGE_NO)
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = vecThrowVelocity;

		ClearMultiDamage();
		VectorNormalize(vecNormalizedVel);

		CTakeDamageInfo	dmgInfo(this, pThrower, damage, DMG_CLUB);
		CalculateMeleeDamageForce(&dmgInfo, vecNormalizedVel, tr.endpos, 0.7f);
		dmgInfo.SetDamagePosition(tr.endpos);
		pOther->DispatchTraceAttack(dmgInfo, vecNormalizedVel, &tr);

		ApplyMultiDamage();

		// keep going through the glass.
		if (pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS)
			return;
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if (pOther->GetMoveType() == MOVETYPE_NONE && !(tr.surface.flags & SURF_SKY))
			UTIL_ImpactTrace(&tr, DMG_CLUB);
		else
		{
			// Put a mark unless we've hit the sky
			if ((tr.surface.flags & SURF_SKY) == false)
			{
				UTIL_ImpactTrace(&tr, DMG_CLUB);
			}
		}
	}

	SetTouch(NULL);
}

void CPropBrick::SetProperties(CBasePlayer *pThrower, float damage)
{
	if (!pThrower)
		return;

	m_pThrower = pThrower;
	m_flDamage = damage;
	vecThrownFrom = pThrower->GetAbsOrigin();
	//SetOwnerEntity(pThrower);
}
#endif

class CWeaponBrick : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponBrick, CBaseHL2MPBludgeonWeapon);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponBrick();
	CWeaponBrick(const CWeaponBrick &);

	void SecondaryAttack();
	void Drop(const Vector &vecVelocity);
	int GetMeleeDamageType() { return DMG_CLUB; }

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	bool m_bThrewWeapon;
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponBrick, DT_WeaponBrick)

BEGIN_NETWORK_TABLE(CWeaponBrick, DT_WeaponBrick)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponBrick)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_brick, CWeaponBrick);
PRECACHE_WEAPON_REGISTER(weapon_brick);

acttable_t	CWeaponBrick::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_MELEE_1HANDED, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_MELEE_1HANDED, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_MELEE_1HANDED, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_MELEE_1HANDED, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_MELEE_1HANDED, false },
	{ ACT_MP_RUN, ACT_HL2MP_RUN_MELEE_1HANDED, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_MELEE_1HANDED, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE_1HANDED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE_1HANDED, false },

	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_MELEE_1HANDED, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_MELEE_1HANDED, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_MELEE_1HANDED, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },

	{ ACT_MP_ATTACK_STAND_GRENADE, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
};

IMPLEMENT_ACTTABLE(CWeaponBrick);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponBrick::CWeaponBrick(void)
{
#ifndef CLIENT_DLL
	m_bThrewWeapon = false;
#endif
}

void CWeaponBrick::SecondaryAttack()
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	SendWeaponAnim(ACT_VM_THROW);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_SECONDARY, ACT_VM_THROW);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
}

void CWeaponBrick::Drop(const Vector &vecVelocity)
{
#ifndef CLIENT_DLL
	if (m_bThrewWeapon)
	{
		UTIL_Remove(this);
		return;
	}
#endif

	BaseClass::Drop(vecVelocity);
}

#ifndef CLIENT_DLL
void CWeaponBrick::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{

	case EVENT_WEAPON_THROW:
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
		Assert(pPlayer != NULL);

		Vector vecEye = pPlayer->EyePosition();
		Vector vForward, vRight;

		pPlayer->EyeVectors(&vForward, &vRight, NULL);
		vecEye += (vForward * BRICK_OFFSET_FORWARD) + (vRight * BRICK_OFFSET_RIGHT);
		vForward[2] += 0.1f;

		Vector vecThrow;
		pPlayer->GetVelocity(&vecThrow, NULL);
		vecThrow += vForward * sk_weapon_brick_throwforce.GetFloat();

		CPropBrick *pBrick = (CPropBrick*)CBaseEntity::Create("prop_thrown_brick", vecEye, vec3_angle);
		if (pBrick)
		{
			pBrick->SetProperties(pPlayer, GetSpecialDamage());
			pBrick->SetVelocity(vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0));

			if (pPlayer && pPlayer->m_lifeState != LIFE_ALIVE)
			{
				pPlayer->GetVelocity(&vecThrow, NULL);

				IPhysicsObject *pPhysicsObject = pBrick->VPhysicsGetObject();
				if (pPhysicsObject)
				{
					pPhysicsObject->SetVelocity(&vecThrow, NULL);
				}
			}
		}

		m_bThrewWeapon = true;
		pPlayer->Weapon_DropSlot(GetSlot()); // throw this weapon away!
		break;
	}

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;

	}
}
#endif
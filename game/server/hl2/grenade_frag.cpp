//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FRAG_GRENADE_WARN_TIME 1.5f
#define GRENADE_MODEL "models/weapons/explosives/frag/w_frag_thrown.mdl"

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

class CGrenadeFrag : public CBaseGrenade
{
	DECLARE_CLASS(CGrenadeFrag, CBaseGrenade);

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	~CGrenadeFrag(void);

public:
	void	Spawn(void);
	void	Precache(void);
	bool	CreateVPhysics(void);
	void	SetTimer(float detonateDelay, float warnDelay);
	void	SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity);
	int		OnTakeDamage(const CTakeDamageInfo &inputInfo);
	void	DelayThink();
	void	VPhysicsUpdate(IPhysicsObject *pPhysics);
	void	OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);
	void	InputSetTimer(inputdata_t &inputdata);
	void	ExplodeOnTouch(void) { AddSolidFlags(FSOLID_TRIGGER); SetCollisionGroup(COLLISION_GROUP_PROJECTILE); SetTouch(&CBaseGrenade::ExplodeTouch); m_flDetonateTime = (gpGlobals->curtime + MAX_COORD_FLOAT); }

protected:
	bool	m_inSolid;
};

LINK_ENTITY_TO_CLASS( npc_grenade_frag, CGrenadeFrag );

BEGIN_DATADESC( CGrenadeFrag )

	// Function Pointers
	DEFINE_THINKFUNC( DelayThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTimer", InputSetTimer ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrenadeFrag::~CGrenadeFrag( void )
{
}

void CGrenadeFrag::Spawn(void)
{
	Precache();
	SetModel(GRENADE_MODEL);

	const DataExplosiveItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetExplosiveDataForType(EXPLOSIVE_TYPE_GRENADE);
	if (data)
	{
		m_flDamage = ((GetOwnerEntity() && GetOwnerEntity()->IsPlayer()) ? data->flPlayerDamage : data->flNPCDamage);
		m_DmgRadius = data->flRadius;
	}

	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

	SetSize(-Vector(4, 4, 4), Vector(4, 4, 4));
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	CreateVPhysics();
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	BaseClass::Spawn();
	SetBlocksLOS(false);
}

bool CGrenadeFrag::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}

// this will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
class CTraceFilterCollisionGroupDelta : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGroupDelta );
	
	CTraceFilterCollisionGroupDelta( const IHandleEntity *passentity, int collisionGroupAlreadyChecked, int newCollisionGroup )
		: m_pPassEnt(passentity), m_collisionGroupAlreadyChecked( collisionGroupAlreadyChecked ), m_newCollisionGroup( newCollisionGroup )
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

		if ( pEntity )
		{
			if ( g_pGameRules->ShouldCollide( m_collisionGroupAlreadyChecked, pEntity->GetCollisionGroup() ) )
				return false;
			if ( g_pGameRules->ShouldCollide( m_newCollisionGroup, pEntity->GetCollisionGroup() ) )
				return true;
		}

		return false;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	int		m_collisionGroupAlreadyChecked;
	int		m_newCollisionGroup;
};

static bool CanBounceGrenade(CBaseCombatCharacter *pThrower, CBaseCombatCharacter *pHit)
{
	if (pThrower && pHit && HL2MPRules() && HL2MPRules()->IsTeamplay())
	{
		// Players and friendly NPCs don't collide with nades / bounce!
		if (pThrower->IsHuman(true) && !pThrower->IsMercenary() && pHit->IsHuman(true) && !pHit->IsMercenary())
			return false;

		// Bandidos don't hit each other with nades.
		if (pThrower->IsMercenary() && pHit->IsMercenary())
			return false;
	}
	return true;
}

void CGrenadeFrag::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	BaseClass::VPhysicsUpdate(pPhysics);

	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity(&vel, &angVel);
	const Vector &start = GetAbsOrigin();

	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGroupDelta filter(this, GetCollisionGroup(), COLLISION_GROUP_NONE);
	trace_t tr;
	UTIL_TraceLine(start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr);

	if (!CanBounceGrenade(GetThrower(), ToBaseCombatCharacter(tr.m_pEnt)))
		return;

	if (tr.startsolid)
	{
		if (!m_inSolid)
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -GRENADE_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity(&vel, NULL);
		}
		m_inSolid = true;
		return;
	}
	m_inSolid = false;

	if (!tr.DidHit())
		return;

	Vector dir = vel;
	VectorNormalize(dir);
	// send a tiny amount of damage so the character will react to getting bonked
	CTakeDamageInfo info(this, GetThrower(), pPhysics->GetMass() * vel, GetAbsOrigin(), 0.1f, DMG_CRUSH);
	tr.m_pEnt->TakeDamage(info);

	// reflect velocity around normal
	vel = -2.0f * tr.plane.normal * DotProduct(vel, tr.plane.normal) + vel;

	// absorb 80% in impact
	vel *= GRENADE_COEFFICIENT_OF_RESTITUTION;
	angVel *= -0.5f;
	pPhysics->SetVelocity(&vel, &angVel);
}

void CGrenadeFrag::Precache( void )
{
	PrecacheModel( GRENADE_MODEL );
	PrecacheModel( "sprites/redglow1.vmt" );
	PrecacheModel( "sprites/bluelaser1.vmt" );
	BaseClass::Precache();
}

void CGrenadeFrag::SetTimer(float detonateDelay, float warnDelay)
{
	m_flDetonateTime = gpGlobals->curtime + detonateDelay;
	m_flWarnAITime = gpGlobals->curtime + warnDelay;
	SetThink(&CGrenadeFrag::DelayThink);
	SetNextThink(gpGlobals->curtime + 0.125f);
}

void CGrenadeFrag::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	SetThrower( pPhysGunUser );
	m_bHasWarnedAI = true;
	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );
}

void CGrenadeFrag::DelayThink()
{
	if ((GetCollisionGroup() == COLLISION_GROUP_PROJECTILE) && ((GetGroundEntity() != NULL) || (GetAbsVelocity().Length() == 0.0f)))
	{
		Detonate();
		return;
	}

	if (gpGlobals->curtime > m_flDetonateTime)
	{
		Detonate();
		return;
	}

	if (!m_bHasWarnedAI && gpGlobals->curtime >= m_flWarnAITime)
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound(SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this);
#endif
		m_bHasWarnedAI = true;
	}

	SetNextThink(gpGlobals->curtime + 0.1);
}

void CGrenadeFrag::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

int CGrenadeFrag::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Grenades only suffer blast damage and burn damage.
	if( !(inputInfo.GetDamageType() & (DMG_BLAST|DMG_BURN) ) )
		return 0;

	return BaseClass::OnTakeDamage( inputInfo );
}

void CGrenadeFrag::InputSetTimer( inputdata_t &inputdata )
{
	SetTimer( inputdata.value.Float(), inputdata.value.Float() - FRAG_GRENADE_WARN_TIME );
}

CBaseGrenade *Fraggrenade_Create(const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool bExplodeOnImpact)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CGrenadeFrag *pGrenade = (CGrenadeFrag *)CBaseEntity::Create( "npc_grenade_frag", position, angles, pOwner );
	
	pGrenade->m_classType = (pOwner ? pOwner->Classify() : CLASS_NONE);
	pGrenade->SetTimer( timer, timer - FRAG_GRENADE_WARN_TIME );
	pGrenade->SetVelocity( velocity, angVelocity );
	pGrenade->SetThrower( ToBaseCombatCharacter( pOwner ) );
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;
	if (bExplodeOnImpact)
		pGrenade->ExplodeOnTouch();

	return pGrenade;
}
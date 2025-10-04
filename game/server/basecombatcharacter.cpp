//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base combat character with no AI
//
//=============================================================================//

#include "cbase.h"
#include "basecombatcharacter.h"
#include "basecombatweapon.h"
#include "animation.h"
#include "entitylist.h"
#include "gamerules.h"
#include "ai_basenpc.h"
#include "ai_squadslot.h"
#include "ammodef.h"
#include "ndebugoverlay.h"
#include "player.h"
#include "physics.h"
#include "engine/IEngineSound.h"
#include "tier1/strtools.h"
#include "sendproxy.h"
#include "EntityFlame.h"
#include "CRagdollMagnet.h"
#include "IEffects.h"
#include "igamesystem.h"
#include "globals.h"
#include "physics_prop_ragdoll.h"
#include "physics_impact_damage.h"
#include "eventqueue.h"
#include "world.h"
#include "items.h"
#include "movevars_shared.h"
#include "RagdollBoogie.h"
#include "rumble_shared.h"
#include "nav_mesh.h"
#include "particle_parse.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"
#include "random_extended.h"

#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_show_hull_attacks( "ai_show_hull_attacks", "0" );
ConVar ai_force_serverside_ragdoll( "ai_force_serverside_ragdoll", "0" );

#ifndef _RETAIL
ConVar ai_use_visibility_cache( "ai_use_visibility_cache", "1" );
#define ShouldUseVisibilityCache() ai_use_visibility_cache.GetBool()
#else
#define ShouldUseVisibilityCache() true
#endif

BEGIN_DATADESC( CBaseCombatCharacter )
	DEFINE_KEYFIELD( m_RelationshipString, FIELD_STRING, "Relationship" ),
	DEFINE_INPUT( m_impactEnergyScale, FIELD_FLOAT, "physdamagescale" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KilledNPC", InputKilledNPC ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
Relationship_t**	CBaseCombatCharacter::m_DefaultRelationship	= NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCleanupDefaultRelationShips : public CAutoGameSystem
{
public:
	CCleanupDefaultRelationShips( char const *name ) : CAutoGameSystem( name )
	{
	}

	virtual void Shutdown()
	{
		if ( !CBaseCombatCharacter::m_DefaultRelationship )
			return;

		for ( int i=0; i<NUM_AI_CLASSES; ++i )
		{
			delete[] CBaseCombatCharacter::m_DefaultRelationship[ i ];
		}

		delete[] CBaseCombatCharacter::m_DefaultRelationship;
		CBaseCombatCharacter::m_DefaultRelationship = NULL;
	}
};

static CCleanupDefaultRelationShips g_CleanupDefaultRelationships( "CCleanupDefaultRelationShips" );

void *SendProxy_SendBaseCombatCharacterLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Only send to local player if this is a player
	pRecipients->ClearAllRecipients();
	
	CBaseCombatCharacter *pBCC = ( CBaseCombatCharacter * )pStruct;
	if ( pBCC != NULL)
	{
		if (pBCC->IsPlayer())
			pRecipients->SetOnly(pBCC->entindex() - 1);
	}
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendBaseCombatCharacterLocalDataTable );

// Only send active weapon index to local player
BEGIN_SEND_TABLE_NOBASE( CBaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	SendPropTime( SENDINFO( m_flNextAttack ) ),
END_SEND_TABLE();

//-----------------------------------------------------------------------------
// This table encodes the CBaseCombatCharacter
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(CBaseCombatCharacter, DT_BaseCombatCharacter)
	// Data that only gets sent to the local player.
	SendPropDataTable( "bcc_localdata", 0, &REFERENCE_SEND_TABLE(DT_BCCLocalPlayerExclusive), SendProxy_SendBaseCombatCharacterLocalDataTable ),

	SendPropEHandle( SENDINFO( m_hActiveWeapon ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_hMyWeapons), SendPropEHandle( SENDINFO_ARRAY(m_hMyWeapons) ) ),
	SendPropInt(SENDINFO(m_nGibFlags), MAX_GIB_BITS, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nMaterialOverlayFlags), MAX_MAT_OVERLAYS_BITS, SPROP_UNSIGNED),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Visibility caching
//-----------------------------------------------------------------------------

struct VisibilityCacheEntry_t
{
	CBaseEntity *pEntity1;
	CBaseEntity *pEntity2;
	EHANDLE		pBlocker;
	float		time;
};

class CVisibilityCacheEntryLess
{
public:
	CVisibilityCacheEntryLess( int ) {}
	bool operator!() const { return false; }
	bool operator()( const VisibilityCacheEntry_t &lhs, const VisibilityCacheEntry_t &rhs ) const
	{
		return ( memcmp( &lhs, &rhs, offsetof( VisibilityCacheEntry_t, pBlocker ) ) < 0 );
	}
};

static CUtlRBTree<VisibilityCacheEntry_t, unsigned short, CVisibilityCacheEntryLess> g_VisibilityCache;
const float VIS_CACHE_ENTRY_LIFE = ( !IsXbox() ) ? .090 : .500;

bool CBaseCombatCharacter::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	VPROF( "CBaseCombatCharacter::FVisible" );

	if ( traceMask != MASK_BLOCKLOS || !ShouldUseVisibilityCache() || pEntity == this
#if defined(HL2_DLL)
		 || Classify() == CLASS_BULLSEYE || pEntity->Classify() == CLASS_BULLSEYE 
#endif
		 )
	{
		return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
	}

	VisibilityCacheEntry_t cacheEntry;

	if ( this < pEntity )
	{
		cacheEntry.pEntity1 = this;
		cacheEntry.pEntity2 = pEntity;
	}
	else
	{
		cacheEntry.pEntity1 = pEntity;
		cacheEntry.pEntity2 = this;
	}

	int iCache = g_VisibilityCache.Find( cacheEntry );

	if ( iCache != g_VisibilityCache.InvalidIndex() )
	{
		if ( gpGlobals->curtime - g_VisibilityCache[iCache].time < VIS_CACHE_ENTRY_LIFE )
		{
			bool bCachedResult = !g_VisibilityCache[iCache].pBlocker.IsValid();
			if ( bCachedResult )
			{
				if ( ppBlocker )
				{
					*ppBlocker = g_VisibilityCache[iCache].pBlocker;
					if ( !*ppBlocker )
					{
						*ppBlocker = GetWorldEntity();
					}
				}
			}
			else
			{
				if ( ppBlocker )
				{
					*ppBlocker = NULL;
				}
			}
			return bCachedResult;
		}
	}
	else
	{
		if ( g_VisibilityCache.Count() != g_VisibilityCache.InvalidIndex() )
		{
			iCache = g_VisibilityCache.Insert( cacheEntry );
		}
		else
		{
			return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
		}
	}

	CBaseEntity *pBlocker = NULL;
	if ( ppBlocker == NULL )
	{
		ppBlocker = &pBlocker;
	}

	bool bResult = BaseClass::FVisible( pEntity, traceMask, ppBlocker );

	if ( !bResult )
	{
		g_VisibilityCache[iCache].pBlocker = *ppBlocker;
	}
	else
	{
		g_VisibilityCache[iCache].pBlocker = NULL;
	}

	g_VisibilityCache[iCache].time = gpGlobals->curtime;

	return bResult;
}

void CBaseCombatCharacter::ResetVisibilityCache( CBaseCombatCharacter *pBCC )
{
	VPROF( "CBaseCombatCharacter::ResetVisibilityCache" );
	if ( !pBCC )
	{
		g_VisibilityCache.RemoveAll();
		return;
	}

	int i = g_VisibilityCache.FirstInorder();
	CUtlVector<unsigned short> removals;
	while ( i != g_VisibilityCache.InvalidIndex() )
	{
		if ( g_VisibilityCache[i].pEntity1 == pBCC || g_VisibilityCache[i].pEntity2 == pBCC )
		{
			removals.AddToTail( i );
		}
		i = g_VisibilityCache.NextInorder( i );
	}

	for ( i = 0; i < removals.Count(); i++ )
	{
		g_VisibilityCache.RemoveAt( removals[i] );
	}
}

//-----------------------------------------------------------------------------

//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInViewCone( CBaseEntity *pEntity )
{
	return FInViewCone( pEntity->WorldSpaceCenter() );
}

//=========================================================
// FInViewCone - returns true is the passed Vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInViewCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - EyePosition() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = EyeDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > m_flFieldOfView )
		return true;

	return false;
}

//=========================================================
// FInAimCone - returns true is the passed ent is in
// the caller's forward aim cone. The dot product is performed
// in 2d, making the aim cone infinitely tall. 
//=========================================================
bool CBaseCombatCharacter::FInAimCone( CBaseEntity *pEntity )
{
	return FInAimCone( pEntity->BodyTarget( EyePosition() ) );
}

//=========================================================
// FInAimCone - returns true is the passed Vector is in
// the caller's forward aim cone. The dot product is performed
// in 2d, making the view cone infinitely tall. By default, the
// callers aim cone is assumed to be very narrow
//=========================================================
bool CBaseCombatCharacter::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.994 )//!!!BUGBUG - magic number same as FacingIdeal(), what is this?
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor : Initialize some fields
//-----------------------------------------------------------------------------
CBaseCombatCharacter::CBaseCombatCharacter( void )
{
#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_HackedGunPos.Init();
#endif

	// Zero the damage accumulator.
	m_flDamageAccumulator = 0.0f;

	// Init weapon and Ammo data
	m_hActiveWeapon			= NULL;
	
	// not alive yet
	m_nGibFlags = 0;
	m_nMaterialOverlayFlags = 0;
	m_pActiveSkillEffects.Purge();

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		m_hMyWeapons.Set( i, NULL );
	}

	// Default so that spawned entities have this set
	m_impactEnergyScale = 1.0f;

	m_bForceServerRagdoll = ai_force_serverside_ragdoll.GetBool();
}

//------------------------------------------------------------------------------
// Purpose : Destructor
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseCombatCharacter::~CBaseCombatCharacter( void )
{
	ResetVisibilityCache( this );
	ClearLastKnownArea();
	m_pActiveSkillEffects.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Put the combat character into the environment
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Spawn( void )
{
	BaseClass::Spawn();

	SetBlocksLOS( false );
	m_nGibFlags = 0;
	m_nMaterialOverlayFlags = 0;
	m_pActiveSkillEffects.Purge();

	// not standing on a nav area yet
	ClearLastKnownArea();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "BaseCombatCharacter.CorpseGib" );
	PrecacheScriptSound( "BaseCombatCharacter.StopWeaponSounds" );
	PrecacheScriptSound( "BaseCombatCharacter.AmmoPickup" );

	PrecacheScriptSound("Gibs.Explode");
	PrecacheScriptSound("Gibs.Head");
	PrecacheScriptSound("Gibs.Arm");
	PrecacheScriptSound("Gibs.Leg");

	for ( int i = m_Relationship.Count() - 1; i >= 0 ; i--) 
	{
		if ( !m_Relationship[i].entity && m_Relationship[i].classType == CLASS_NONE ) 
		{
			DevMsg( 2, "Removing relationship for lost entity\n" );
			m_Relationship.FastRemove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::UpdateOnRemove( void )
{
	int i;
	// Make sure any weapons I didn't drop get removed.
	for (i=0;i<MAX_WEAPONS;i++) 
	{
		if (m_hMyWeapons[i]) 
		{
			UTIL_Remove( m_hMyWeapons[i] );
		}
	}

	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

// UNDONE: Should these operate on a list of weapon/items
Activity CBaseCombatCharacter::Weapon_TranslateActivity(Activity baseAct)
{
	Activity translated = baseAct;

	if (m_hActiveWeapon)
		translated = m_hActiveWeapon->ActivityOverride(baseAct);

	return translated;
}

//-----------------------------------------------------------------------------
// Purpose: NPCs should override this function to translate activities
//			such as ACT_WALK, etc.
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CBaseCombatCharacter::NPC_TranslateActivity( Activity baseAct )
{
	return baseAct;
}


void CBaseCombatCharacter::Weapon_SetActivity( Activity newActivity, float duration )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->SetActivity( newActivity, duration );
	}
}

void CBaseCombatCharacter::Weapon_FrameUpdate( void )
{
	if ( m_hActiveWeapon )
	{
		m_hActiveWeapon->Operator_FrameUpdate( this );
	}
}


//------------------------------------------------------------------------------
// Purpose :	expects a length to trace, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::CheckTraceHullAttack(float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC, bool bDirect)
{
	Vector forward;
	Vector vStart = GetAbsOrigin();
	vStart.z += (WorldAlignSize().z - maxs.z) + 4.0f;
	if (bDirect && GetEnemy())
	{
		forward = (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter());
		VectorNormalize(forward);
	}
	else
		AngleVectors(GetAbsAngles(), &forward);
	Vector vEnd = vStart + (forward * flDist);
	return CheckTraceHullAttack(vStart, vEnd, mins, maxs, iDamage, iDmgType, forceScale, bDamageAnyNPC);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pHandleEntity - 
//			contentsMask - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTraceFilterMelee::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		return false;

	// Don't test if the game code tells us we should ignore this collision...
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	
	if ( pEntity )
	{
		if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
			return false;
		
		if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
			return false;

		if ((pEntity->m_takedamage == DAMAGE_NO) || pEntity->IsBaseCombatWeapon() || (pEntity->GetCollisionGroup() == COLLISION_GROUP_WEAPON))
			return false;

		Vector	attackDir = pEntity->WorldSpaceCenter() - m_dmgInfo->GetAttacker()->WorldSpaceCenter();
		VectorNormalize( attackDir );

		CTakeDamageInfo info = (*m_dmgInfo);				
		CalculateMeleeDamageForce( &info, attackDir, info.GetAttacker()->WorldSpaceCenter(), m_flForceScale );

		CBaseCombatCharacter *pBCC = info.GetAttacker()->MyCombatCharacterPointer();
		CBaseCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

		// Only do these comparisons between NPCs
		if ( pBCC && pVictimBCC )
		{
			// Can only damage other NPCs that we hate
			if ( m_bDamageAnyNPC || pBCC->IRelationType( pEntity ) == D_HT )
			{
				if ( info.GetDamage() )
				{
					pEntity->TakeDamage( info );
				}
				
				// Put a combat sound in
				CSoundEnt::InsertSound( SOUND_COMBAT, info.GetDamagePosition(), 200, 0.2f, info.GetAttacker() );

				m_pHit = pEntity;
				return true;
			}
		}
		else
		{
			m_pHit = pEntity;

			// Make sure if the player is holding this, he drops it
			Pickup_ForcePlayerToDropThisObject( pEntity );

			// Otherwise just damage passive objects in our way
			if ( info.GetDamage() )
			{
				pEntity->TakeDamage( info );
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose :	start and end trace position, amount 
//				of damage to do, and damage type. Returns a pointer to
//				the damaged entity in case the NPC wishes to do
//				other stuff to the victim (punchangle, etc)
//
//				Used for many contact-range melee attacks. Bites, claws, etc.
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity *CBaseCombatCharacter::CheckTraceHullAttack( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC )
{
	// Handy debuging tool to visualize HullAttack trace
	if ( ai_show_hull_attacks.GetBool() )
	{
		float length	 = (vEnd - vStart ).Length();
		Vector direction = (vEnd - vStart );
		VectorNormalize( direction );
		Vector hullMaxs = maxs;
		hullMaxs.x = length + hullMaxs.x;
		NDebugOverlay::BoxDirection(vStart, mins, hullMaxs, direction, 100,255,255,20,1.0);
		NDebugOverlay::BoxDirection(vStart, mins, maxs, direction, 255,0,0,20,1.0);
	}

	CTakeDamageInfo	dmgInfo( this, this, iDamage, iDmgType );
	
	// COLLISION_GROUP_PROJECTILE does some handy filtering that's very appropriate for this type of attack, as well. (sjb) 7/25/2007
	CTraceFilterMelee traceFilter( this, COLLISION_GROUP_PROJECTILE, &dmgInfo, flForceScale, bDamageAnyNPC );

	Ray_t ray;
	ray.Init( vStart, vEnd, mins, maxs );

	trace_t tr;
	enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );

	CBaseEntity *pEntity = traceFilter.m_pHit;
	
	if ( pEntity == NULL )
	{
		// See if perhaps I'm trying to claw/bash someone who is standing on my head.
		Vector vecTopCenter;
		Vector vecEnd;
		Vector vecMins, vecMaxs;

		// Do a tracehull from the top center of my bounding box.
		vecTopCenter = GetAbsOrigin();
		CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );
		vecTopCenter.z = vecMaxs.z + 1.0f;
		vecEnd = vecTopCenter;
		vecEnd.z += 2.0f;
		
		ray.Init( vecTopCenter, vEnd, mins, maxs );
		enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );

		pEntity = traceFilter.m_pHit;
	}

	if( pEntity && !pEntity->CanBeHitByMeleeAttack(this) )
	{
		// If we touched something, but it shouldn't be hit, return nothing.
		pEntity = NULL;
	}

	return pEntity;
}

Vector CBaseCombatCharacter::CalcDamageForceVector( const CTakeDamageInfo &info )
{
	// Already have a damage force in the data, use that.
	bool bNoPhysicsForceDamage = g_pGameRules->Damage_NoPhysicsForce( info.GetDamageType() );
	if ( info.GetDamageForce() != vec3_origin || bNoPhysicsForceDamage )
	{
		if( info.GetDamageType() & DMG_BLAST )
		{
			// Fudge blast forces a little bit, so that each
			// victim gets a slightly different trajectory. 
			// This simulates features that usually vary from
			// person-to-person variables such as bodyweight,
			// which are all indentical for characters using the same model.
			float scale = random->RandomFloat( 0.85, 1.15 );
			Vector force = info.GetDamageForce();
			force.x *= scale;
			force.y *= scale;
			// Try to always exaggerate the upward force because we've got pretty harsh gravity
			force.z *= (force.z > 0) ? 1.15 : scale;
			return force;
		}

		return info.GetDamageForce();
	}

	CBaseEntity *pForce = info.GetInflictor();
	if ( !pForce )
	{
		pForce = info.GetAttacker();
	}

	if ( pForce )
	{
		// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
		float forceScale = info.GetDamage() * 75 * 4;

		Vector forceVector;
		// If the damage is a blast, point the force vector higher than usual, this gives 
		// the ragdolls a bodacious "really got blowed up" look.
		if( info.GetDamageType() & DMG_BLAST )
		{
			// exaggerate the force from explosions a little (37.5%)
			forceVector = (GetLocalOrigin() + Vector(0, 0, WorldAlignSize().z) ) - pForce->GetLocalOrigin();
			VectorNormalize(forceVector);
			forceVector *= 1.375f;
		}
		else
		{
			// taking damage from self?  Take a little random force, but still try to collapse on the spot.
			if ( this == pForce )
			{
				forceVector.x = random->RandomFloat( -1.0f, 1.0f );
				forceVector.y = random->RandomFloat( -1.0f, 1.0f );
				forceVector.z = 0.0;
				forceScale = random->RandomFloat( 1000.0f, 2000.0f );
			}
			else
			{
				// UNDONE: Collision forces are baked in to CTakeDamageInfo now
				// UNDONE: Is this MOVETYPE_VPHYSICS code still necessary?
				if ( pForce->GetMoveType() == MOVETYPE_VPHYSICS )
				{
					// killed by a physics object
					IPhysicsObject *pPhysics = VPhysicsGetObject();
					if ( !pPhysics )
					{
						pPhysics = pForce->VPhysicsGetObject();
					}
					pPhysics->GetVelocity( &forceVector, NULL );
					forceScale = pPhysics->GetMass();
				}
				else
				{
					forceVector = GetLocalOrigin() - pForce->GetLocalOrigin();
					VectorNormalize(forceVector);
				}
			}
		}
		return forceVector * forceScale;
	}
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::FixupBurningServerRagdoll( CBaseEntity *pRagdoll )
{
	if ( !IsOnFire() )
		return;

	// Move the fire effects entity to the ragdoll
	CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>( GetEffectEntity() );
	if ( pFireChild )
	{
		SetEffectEntity( NULL );
		pRagdoll->AddFlag( FL_ONFIRE );
		pFireChild->SetAbsOrigin( pRagdoll->GetAbsOrigin() );
		pFireChild->AttachToEntity( pRagdoll );
		pFireChild->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
 		pRagdoll->SetEffectEntity( pFireChild );

		color32 color = GetRenderColor();
		pRagdoll->SetRenderColor( color.r, color.g, color.b );
	}
}

bool CBaseCombatCharacter::BecomeRagdollBoogie( CBaseEntity *pKiller, const Vector &forceVector, float duration, int flags )
{
	Assert( CanBecomeRagdoll() );

	CTakeDamageInfo info( pKiller, pKiller, 1.0f, DMG_GENERIC );
	info.SetDamageForce( forceVector );

	CBaseEntity *pRagdoll = CreateServerRagdoll(this, 0, info, COLLISION_GROUP_INTERACTIVE_DEBRIS);
	pRagdoll->SetCollisionBounds( CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs() );
	CRagdollBoogie::Create( pRagdoll, 200, gpGlobals->curtime, duration, flags );

	CTakeDamageInfo ragdollInfo( pKiller, pKiller, 10000.0, DMG_GENERIC | DMG_REMOVENORAGDOLL );
	ragdollInfo.SetDamagePosition(WorldSpaceCenter());
	ragdollInfo.SetDamageForce( Vector( 0, 0, 1) );
	TakeDamage( ragdollInfo );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	return BecomeRagdollOnClient( forceVector );
}

/*
============
Killed
============
*/
void CBaseCombatCharacter::Event_Killed( const CTakeDamageInfo &info )
{
	extern ConVar npc_vphysics;

	// Advance life state to dying
	m_lifeState = LIFE_DYING;

	// Calculate death force
	Vector forceVector = CalcDamageForceVector( info );

	// See if there's a ragdoll magnet that should influence our force.
	CRagdollMagnet *pMagnet = CRagdollMagnet::FindBestMagnet( this );
	if( pMagnet )
	{
		forceVector += pMagnet->GetForceVector( this );
	}

	CBaseCombatWeapon *pDroppedWeapon = m_hActiveWeapon.Get();

	// Drop any weapon that I own
	if ( VPhysicsGetObject() )
	{
		Vector weaponForce = forceVector * VPhysicsGetObject()->GetInvMass();
		Weapon_Drop( m_hActiveWeapon, NULL, &weaponForce );
	}
	else
	{
		Weapon_Drop( m_hActiveWeapon );
	}
	
	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EmitSound( "BaseCombatCharacter.StopWeaponSounds" );

	// Tell my killer that he got me!
	if( info.GetAttacker() )
	{
		info.GetAttacker()->Event_KilledOther(this, info);
		g_EventQueue.AddEvent( info.GetAttacker(), "KilledNPC", 0.3, this, this );
	}
	SendOnKilledGameEvent( info );

	bool bRagdollCreated = false;
	if ((info.GetDamageType() & DMG_DISSOLVE) && CanBecomeRagdoll())
	{
		int nDissolveType = ENTITY_DISSOLVE_NORMAL;
		if (info.GetDamageType() & DMG_SHOCK)
		{
			nDissolveType = ENTITY_DISSOLVE_ELECTRICAL;
		}

		bRagdollCreated = Dissolve(NULL, gpGlobals->curtime, false, nDissolveType);

		// Also dissolve any weapons we dropped
		if (pDroppedWeapon)
		{
			pDroppedWeapon->Dissolve(NULL, gpGlobals->curtime, false, nDissolveType);
		}
	}

	if (!bRagdollCreated && (info.GetDamageType() & DMG_REMOVENORAGDOLL) == 0)
	{
		BecomeRagdoll(info, forceVector);
	}
	
	// no longer standing on a nav area
	ClearLastKnownArea();
}

void CBaseCombatCharacter::Event_Dying( const CTakeDamageInfo &info )
{
}

void CBaseCombatCharacter::Event_Dying()
{
	CTakeDamageInfo info;
	Event_Dying( info );
}


// ===========================================================================
//  > Weapons
// ===========================================================================
bool CBaseCombatCharacter::Weapon_Detach( CBaseCombatWeapon *pWeapon )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( pWeapon == m_hMyWeapons[i] )
		{
			pWeapon->Detach();
			if ( pWeapon->HolsterOnDetach() )
			{
				pWeapon->FullHolster();
			}
			m_hMyWeapons.Set( i, NULL );
			pWeapon->SetOwner( NULL );

			if ( pWeapon == m_hActiveWeapon )
				ClearActiveWeapon();
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// For weapon strip
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::ThrowDirForWeaponStrip( CBaseCombatWeapon *pWeapon, const Vector &vecForward, Vector *pVecThrowDir )
{
	// Nowhere in particular; just drop it.
	VMatrix zRot;
	MatrixBuildRotateZ(zRot, random->RandomFloat(-60.0f, 60.0f));

	Vector vecThrow;
	Vector3DMultiply(zRot, vecForward, *pVecThrowDir);

	pVecThrowDir->z = random->RandomFloat(-0.5f, 0.5f);
	VectorNormalize(*pVecThrowDir);
}


//-----------------------------------------------------------------------------
// For weapon strip
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::DropWeaponForWeaponStrip( CBaseCombatWeapon *pWeapon, 
	const Vector &vecForward, const QAngle &vecAngles, float flDiameter )
{
	Vector vecOrigin;
	CollisionProp()->RandomPointInBounds( Vector( 0.5f, 0.5f, 0.5f ), Vector( 0.5f, 0.5f, 1.0f ), &vecOrigin );

	// Nowhere in particular; just drop it.
	Vector vecThrow;
	ThrowDirForWeaponStrip( pWeapon, vecForward, &vecThrow );

	Vector vecOffsetOrigin;
	VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );

	trace_t	tr;
	UTIL_TraceLine( vecOrigin, vecOffsetOrigin, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		
	if ( tr.startsolid || tr.allsolid || ( tr.fraction < 1.0f && tr.m_pEnt != pWeapon ) )
	{
		//FIXME: Throw towards a known safe spot?
		vecThrow.Negate();
		VectorMA( vecOrigin, flDiameter, vecThrow, vecOffsetOrigin );
	}

	vecThrow *= random->RandomFloat( 400.0f, 600.0f );

	pWeapon->SetAbsOrigin( vecOrigin );
	pWeapon->SetAbsAngles( vecAngles );
	pWeapon->Drop( vecThrow );
	Weapon_Detach( pWeapon );
}



//-----------------------------------------------------------------------------
// For weapon strip
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_DropAll( bool bDisallowWeaponPickup )
{
	if ( GetFlags() & FL_NPC )
	{
		for (int i=0; i<MAX_WEAPONS; ++i) 
		{
			CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
			if (!pWeapon)
				continue;

 			Weapon_Drop( pWeapon );
		}
		return;
	}

	QAngle gunAngles;
	VectorAngles( BodyDirection2D(), gunAngles );

	Vector vecForward;
	AngleVectors( gunAngles, &vecForward, NULL, NULL );

	float flDiameter = sqrt( CollisionProp()->OBBSize().x * CollisionProp()->OBBSize().x +
		CollisionProp()->OBBSize().y * CollisionProp()->OBBSize().y );

	CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	for (int i=0; i<MAX_WEAPONS; ++i) 
	{
		CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
		if (!pWeapon)
			continue;

		// Have to drop this after we've dropped everything else, so autoswitch doesn't happen
		if ( pWeapon == pActiveWeapon )
			continue;

		DropWeaponForWeaponStrip( pWeapon, vecForward, gunAngles, flDiameter );

		// HACK: This hack is required to allow weapons to be disintegrated
		// in the citadel weapon-strip scene
		// Make them not pick-uppable again. This also has the effect of allowing weapons
		// to collide with triggers. 
		if ( bDisallowWeaponPickup )
		{
			pWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
			
			IPhysicsObject *pObj = pWeapon->VPhysicsGetObject();
			
			if ( pObj != NULL )
			{	
				pObj->SetGameFlags( FVPHYSICS_NO_PLAYER_PICKUP );
			}
		}
	}

	// Drop the active weapon normally...
	if ( pActiveWeapon )
	{
		// Nowhere in particular; just drop it.
		Vector vecThrow;
		ThrowDirForWeaponStrip( pActiveWeapon, vecForward, &vecThrow );

		// Throw a little more vigorously; it starts closer to the player
		vecThrow *= random->RandomFloat( 800.0f, 1000.0f );

		Weapon_Drop( pActiveWeapon, NULL, &vecThrow );

		// HACK: This hack is required to allow weapons to be disintegrated
		// in the citadel weapon-strip scene
		// Make them not pick-uppable again. This also has the effect of allowing weapons
		// to collide with triggers. 
		if ( bDisallowWeaponPickup )
		{
			pActiveWeapon->RemoveSolidFlags( FSOLID_TRIGGER );
		}
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: Drop the active weapon, optionally throwing it at the given target position.
// Input  : pWeapon - Weapon to drop/throw.
//			pvecTarget - Position to throw it at, NULL for none.
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	if ( !pWeapon )
		return;

	// If I'm an NPC, fill the weapon with ammo before I drop it.
	if (GetFlags() & FL_NPC)
	{
		if (pWeapon->UsesClipsForAmmo())
			pWeapon->m_iClip = pWeapon->GetDefaultClip();
	}

	if ( IsPlayer() )
	{
		Vector vThrowPos = Weapon_ShootPosition() - Vector(0,0,12);

		if( UTIL_PointContents(vThrowPos) & CONTENTS_SOLID )
		{
			Msg("Weapon spawning in solid!\n");
		}

		pWeapon->SetAbsOrigin( vThrowPos );

		QAngle gunAngles;
		VectorAngles( BodyDirection2D(), gunAngles );
		pWeapon->SetAbsAngles( gunAngles );
	}
	else
	{
		int iBIndex = -1;
		int iWeaponBoneIndex = -1;

		CStudioHdr *hdr = pWeapon->GetModelPtr();
		// If I have a hand, set the weapon position to my hand bone position.
		if ( hdr && hdr->numbones() > 0 )
		{
			// Assume bone zero is the root
			for ( iWeaponBoneIndex = 0; iWeaponBoneIndex < hdr->numbones(); ++iWeaponBoneIndex )
			{
				iBIndex = LookupBone( hdr->pBone( iWeaponBoneIndex )->pszName() );
				// Found one!
				if ( iBIndex != -1 )
				{
					break;
				}
			}

			if ( iBIndex == -1 )
			{
				iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
			}
		}
		else
		{
			iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
		}

		if ( iBIndex != -1)  
		{
			Vector origin;
			QAngle angles;
			matrix3x4_t transform;

			// Get the transform for the weapon bonetoworldspace in the NPC
			GetBoneTransform( iBIndex, transform );

			// find offset of root bone from origin in local space
			// Make sure we're detached from hierarchy before doing this!!!
			pWeapon->StopFollowingEntity();
			pWeapon->SetAbsOrigin( Vector( 0, 0, 0 ) );
			pWeapon->SetAbsAngles( QAngle( 0, 0, 0 ) );
			pWeapon->InvalidateBoneCache();
			matrix3x4_t rootLocal;
			pWeapon->GetBoneTransform( iWeaponBoneIndex, rootLocal );

			// invert it
			matrix3x4_t rootInvLocal;
			MatrixInvert( rootLocal, rootInvLocal );

			matrix3x4_t weaponMatrix;
			ConcatTransforms( transform, rootInvLocal, weaponMatrix );
			MatrixAngles( weaponMatrix, angles, origin );
			
			pWeapon->Teleport( &origin, &angles, NULL );
		}
		// Otherwise just set in front of me.
		else 
		{
			Vector vFacingDir = BodyDirection2D();
			vFacingDir = vFacingDir * 10.0; 
			pWeapon->SetAbsOrigin( Weapon_ShootPosition() + vFacingDir );
		}
	}

	Vector vecThrow;
	if (pvecTarget)
	{
		// I've been told to throw it somewhere specific.
		vecThrow = VecCheckToss( this, pWeapon->GetAbsOrigin(), *pvecTarget, 0.2, 1.0, false );
	}
	else
	{
		if ( pVelocity )
		{
			vecThrow = *pVelocity;
			float flLen = vecThrow.Length();
			if (flLen > 400)
			{
				VectorNormalize(vecThrow);
				vecThrow *= 400;
			}
		}
		else
		{
			// Nowhere in particular; just drop it.
			float throwForce = ( IsPlayer() ) ? 400.0f : random->RandomInt( 64, 128 );
			vecThrow = BodyDirection3D() * throwForce;
		}
	}

	pWeapon->Drop( vecThrow );
	Weapon_Detach( pWeapon );

	if ( HasSpawnFlags( SF_NPC_NO_WEAPON_DROP ) )
	{
		// Don't drop weapons when the super physgun is happening.
		UTIL_Remove( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Lighting origin
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetLightingOriginRelative( CBaseEntity *pLightingOrigin )
{
	BaseClass::SetLightingOriginRelative( pLightingOrigin );
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->SetLightingOriginRelative( pLightingOrigin );
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Add new weapon to the character
// Input  : New weapon
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	// Add the weapon to my weapon inventory
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (!m_hMyWeapons[i])
		{
			m_hMyWeapons.Set(i, pWeapon);
			break;
		}
	}

	// Weapon is now on my team
	pWeapon->ChangeTeam( GetTeamNumber() );
	pWeapon->Equip( this );

	// Players don't automatically holster their current weapon
	if ( !IsPlayer() )
	{
		if ( m_hActiveWeapon )
		{
			m_hActiveWeapon->FullHolster();
			// FIXME: isn't this handeled by the weapon?
			m_hActiveWeapon->AddEffects( EF_NODRAW );
		}
		SetActiveWeapon( pWeapon );
		m_hActiveWeapon->RemoveEffects( EF_NODRAW );

		// If SF_NPC_LONG_RANGE spawn flags is set let weapon work from any distance
		if ( HasSpawnFlags(SF_NPC_LONG_RANGE) )
		{
			m_hActiveWeapon->m_fMaxRange1 = 999999999;
			m_hActiveWeapon->m_fMaxRange2 = 999999999;
		}
	}

	// Pass the lighting origin over to the weapon if we have one
	pWeapon->SetLightingOriginRelative( GetLightingOriginRelative() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the weapon passed in would occupy a slot already occupied by the carrier
// Input  : *pWeapon - weapon to test for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon )
{
	if ( pWeapon == NULL )
		return false;

	//Check to see if there's a resident weapon already in this slot
	if ( Weapon_GetSlot( pWeapon->GetSlot() ) == NULL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon (if any) in the requested slot
// Input  : slot - which slot to poll
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetSlot( int slot ) const
{
	int	targetSlot = slot;

	// Check for that slot being occupied already
	for ( int i=0; i < MAX_WEAPONS; i++ )
	{
		if ( m_hMyWeapons[i].Get() != NULL )
		{
			// If the slots match, it's already occupied
			if ( m_hMyWeapons[i]->GetSlot() == targetSlot )
				return m_hMyWeapons[i];
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseCombatWeapon *CBaseCombatCharacter::Weapon_Create( const char *pWeaponName )
{
	CBaseCombatWeapon *pWeapon = static_cast<CBaseCombatWeapon *>( Create( pWeaponName, GetLocalOrigin(), GetLocalAngles(), this ) );
	return pWeapon;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::Weapon_HandleAnimEvent( animevent_t *pEvent )
{
	// UNDONE: Some check to make sure that pEvent->pSource is a weapon I'm holding?
	if ( m_hActiveWeapon )
	{
		// UNDONE: Pass to pEvent->pSource instead?
		m_hActiveWeapon->Operator_HandleAnimEvent( pEvent, this );
	}
}

void CBaseCombatCharacter::RemoveAllWeapons()
{
	ClearActiveWeapon();
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( m_hMyWeapons[i] )
		{
			m_hMyWeapons[i]->Delete( );
			m_hMyWeapons.Set( i, NULL );
		}
	}
}


// take health
int CBaseCombatCharacter::TakeHealth (float flHealth, int bitsDamageType)
{
	if (!m_takedamage)
		return 0;
	
	return BaseClass::TakeHealth(flHealth, bitsDamageType);
}


/*
============
OnTakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the NPC is within the trigger_hurt.
When a NPC is poisoned via an arrow etc it takes all the poison damage at once.
============
*/
int CBaseCombatCharacter::OnTakeDamage(const CTakeDamageInfo &info)
{
	int retVal = 0;

	if (!m_takedamage)
		return 0;

	m_iDamageCount++;

	if (info.GetDamageType() & DMG_SHOCK)
	{
		g_pEffects->Sparks(info.GetDamagePosition(), 2, 2);
		UTIL_Smoke(info.GetDamagePosition(), random->RandomInt(10, 15), 10);
	}

	// Bernt: Scale the damage depending on which entity we hit.
	// We handle players in hl2mp_player, if we check it here the armor code will expect the non scaled damage...
	CTakeDamageInfo damageCopy = info;
	CBaseEntity *pAttacker = damageCopy.GetAttacker();
	if (pAttacker && this->IsNPC())
	{
		float flDamageScale = GameBaseServer()->GetDamageScaleForEntity(pAttacker, this, damageCopy.GetDamageType(), damageCopy.GetDamageCustom());
		damageCopy.ScaleDamage(flDamageScale);
	}

	switch (m_lifeState)
	{
	case LIFE_ALIVE:
		retVal = OnTakeDamage_Alive(damageCopy);
		if (retVal)
		{
			CanGibEntity(damageCopy);
			if (m_iHealth > 0)
				OnAffectedBySkill(damageCopy);
		}

		if (m_iHealth <= 0)
		{
			IPhysicsObject *pPhysics = VPhysicsGetObject();
			if (pPhysics)
			{
				pPhysics->EnableCollisions(false);
			}

			Event_Killed(damageCopy);
			Event_Dying(damageCopy);
		}
		return retVal;
		break;

	case LIFE_DYING:
		return OnTakeDamage_Dying(damageCopy);

	default:
	case LIFE_DEAD:
		retVal = OnTakeDamage_Dead(damageCopy);
		return retVal;
	}
}

int CBaseCombatCharacter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	if (info.GetInflictor())
	{
		vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector(0, 0, 10) - WorldSpaceCenter();
		VectorNormalize(vecDir);
	}
	g_vecAttackDir = vecDir;

	//!!!LATER - make armor consideration here!
	// do the damage
	if (m_takedamage != DAMAGE_EVENTS_ONLY)
	{
		// Separate the fractional amount of damage from the whole
		float flFractionalDamage = info.GetDamage() - floor(info.GetDamage());
		float flIntegerDamage = info.GetDamage() - flFractionalDamage;

		// Add fractional damage to the accumulator
		m_flDamageAccumulator += flFractionalDamage;

		// If the accumulator is holding a full point of damage, move that point
		// of damage into the damage we're about to inflict.
		if (m_flDamageAccumulator >= 1.0)
		{
			flIntegerDamage += 1.0;
			m_flDamageAccumulator -= 1.0;
		}

		if (flIntegerDamage <= 0)
			return 0;

		m_iHealth -= flIntegerDamage;
	}

	CHL2MP_Player *pClient = ToHL2MPPlayer(info.GetAttacker());
	CBaseCombatWeapon *pWeapon = NULL;

	if (pClient)
	{
		// Blood Splats on player.
		pWeapon = pClient->GetActiveWeapon();
		if (pWeapon && pClient->GetAbsOrigin().DistTo(this->GetAbsOrigin()) < 150.0f)
			pWeapon->SetBloodState(true);

		pClient->DispatchDamageText(this, -info.GetDamage());

		if ((pClient != this) && pClient->IsZombie() && (pClient->GetSkillValue(PLAYER_SKILL_ZOMBIE_LIFE_LEECH) > 0))
		{
			double perc = (pClient->GetSkillValue(PLAYER_SKILL_ZOMBIE_LIFE_LEECH, TEAM_DECEASED) / PERCENT_BASE);
			if (TryTheLuck(perc))
			{
				float healAmount = info.GetDamage() * 0.5f;
				pClient->TakeHealth(((int)healAmount), DMG_GENERIC);
				pClient->DispatchDamageText(this, (int)healAmount);
			}
		}

		if (pClient != this)
			pClient->GetAchievementStats()->OnDidDamage(info);
	}

	// Blood Splats on player.
	pWeapon = GetActiveWeapon();
	if (IsPlayer() && pWeapon)
		pWeapon->SetBloodState(true);

	return 1;
}

int CBaseCombatCharacter::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	return 1;
}

int CBaseCombatCharacter::OnTakeDamage_Dead( const CTakeDamageInfo &info )
{
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		m_iHealth -= info.GetDamage();
	}

	return 1;
}

float CBaseCombatCharacter::GetIdealSpeed() const
{
	float speed = BaseClass::GetIdealSpeed();

	if (IsAffectedBySkillFlag(SKILL_FLAG_COLDSNAP))
	{
		speed *= (GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flSlowDownPercent / 100.0f);
	}

	return speed;
}

float CBaseCombatCharacter::GetIdealAccel() const
{
	float speed = BaseClass::GetIdealAccel();

	if (IsAffectedBySkillFlag(SKILL_FLAG_COLDSNAP))
	{
		speed *= (GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flSlowDownPercent / 100.0f);
	}

	return speed;
}

// The NPC or player has been affected by a skill!
void CBaseCombatCharacter::OnAffectedBySkill(const CTakeDamageInfo &info)
{
	int skillFlag = info.GetSkillFlags();
	if (!IsNPC() || !skillFlag)
		return;

	CHL2MP_Player *pAttacker = ToHL2MPPlayer(info.GetAttacker());
	if (!pAttacker || IsAffectedBySkillFlag(skillFlag))
		return;

	CBaseCombatWeapon *pKillerWeapon = pAttacker->GetActiveWeapon();

	int nFlag = 0;
	int nOverlayFlag = 0;
	float duration = 0.0f;
	float damage = 0.0f;
	float timeBetweenDamage = 0.0f;

	if (skillFlag & SKILL_FLAG_BLEED)
	{
		nFlag = SKILL_FLAG_BLEED;
		nOverlayFlag = MAT_OVERLAY_BLEEDING;

		duration = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flBleedDuration;
		damage = 10.0f;
		timeBetweenDamage = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flBleedFrequency;

		if (pKillerWeapon)
			damage = (info.GetDamage() / 100.0f) * pKillerWeapon->GetWpnData().m_flSkillBleedFactor;

		pAttacker->PlaySkillSoundCue(SKILL_SOUND_CUE_MELEE_BLEED);
	}
	else if (skillFlag & SKILL_FLAG_BLAZINGAMMO)
	{
		nFlag = SKILL_FLAG_BLAZINGAMMO;
		nOverlayFlag = MAT_OVERLAY_BURNING;

		duration = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flNPCBurnDuration;
		damage = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flNPCBurnDamage;
		timeBetweenDamage = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flNPCBurnFrequency;

		if (info.GetForcedWeaponID() != WEAPON_ID_FLAMETHROWER)
			pAttacker->PlaySkillSoundCue(SKILL_SOUND_CUE_AMMO_BLAZE);
	}
	else if (skillFlag & SKILL_FLAG_COLDSNAP)
	{
		nFlag = SKILL_FLAG_COLDSNAP;
		nOverlayFlag = MAT_OVERLAY_COLDSNAP;
		duration = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flSlowDownDuration;
		pAttacker->PlaySkillSoundCue(SKILL_SOUND_CUE_AMMO_FROST);
	}
	else if (skillFlag & SKILL_FLAG_CRIPPLING_BLOW)
	{
		nFlag = SKILL_FLAG_CRIPPLING_BLOW;
		nOverlayFlag = MAT_OVERLAY_CRIPPLED;
		duration = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flStunDuration;
		pAttacker->PlaySkillSoundCue(SKILL_SOUND_CUE_MELEE_STUN);
	}

	if (!nFlag)
		return;

	AddMaterialOverlayFlag(nOverlayFlag);

	playerSkillAffectionItem_t item;
	item.flag = nFlag;
	item.overlayFlag = nOverlayFlag;
	item.damage = damage;
	item.duration = gpGlobals->curtime + duration;
	item.nextTimeToTakeDamage = 0.0f;
	item.timeToTakeDamage = timeBetweenDamage;
	item.misc = info.GetForcedWeaponID();
	item.m_pAttacker = pAttacker;

	m_pActiveSkillEffects.AddToTail(item);

	OnSkillFlagState(nFlag, true);
}

bool CBaseCombatCharacter::IsAffectedBySkillFlag(int flag) const
{
	for (int i = 0; i < m_pActiveSkillEffects.Count(); i++)
	{
		if ((m_pActiveSkillEffects[i].flag == flag) || (m_pActiveSkillEffects[i].flag & flag))
			return true;
	}

	return false;
}

void CBaseCombatCharacter::CheckSkillAffections(void)
{
	if (!m_pActiveSkillEffects.Count())
		return;

	for (int i = (m_pActiveSkillEffects.Count() - 1); i >= 0; i--)
	{
		playerSkillAffectionItem_t *effectItem = &m_pActiveSkillEffects[i];
		CBaseEntity *pAttacker = effectItem->m_pAttacker.Get();

		if ((effectItem->duration < gpGlobals->curtime) || !pAttacker)
		{
			int flag = effectItem->flag;
			RemoveMaterialOverlayFlag(effectItem->overlayFlag);
			m_pActiveSkillEffects.Remove(i);
			OnSkillFlagState(flag, false);
			continue;
		}

		// If we want to take damage, then take damage when the frequency has run out.
		if (effectItem->damage > 0)
		{
			if (effectItem->nextTimeToTakeDamage <= gpGlobals->curtime)
			{
				effectItem->nextTimeToTakeDamage = gpGlobals->curtime + effectItem->timeToTakeDamage;

				CTakeDamageInfo info(pAttacker, pAttacker, effectItem->damage, GetSkillAffectionDamageType(effectItem->flag));
				info.SetSkillFlags(effectItem->flag);
				info.SetForcedWeaponID(effectItem->misc);

				int activeFlag = effectItem->flag;
				if (activeFlag == SKILL_FLAG_BLAZINGAMMO)
					info.SetDamageCustom(DMG_BURN);
				else if (activeFlag == SKILL_FLAG_BLEED)
					info.SetDamageCustom(DMG_SLASH);

				TakeDamage(info);
				SpawnBlood(GetAbsOrigin() + Vector(0, 0, 35), g_vecAttackDir, BloodColor(), 100);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets vBodyDir to the body direction (2D) of the combat character.  
//			Used as NPC's and players extract facing direction differently
// Input  :
// Output :
//-----------------------------------------------------------------------------
QAngle CBaseCombatCharacter::BodyAngles()
{
	return GetAbsAngles();
}

Vector CBaseCombatCharacter::BodyDirection2D( void )
{
	Vector vBodyDir = BodyDirection3D( );
	vBodyDir.z = 0;
	vBodyDir.AsVector2D().NormalizeInPlace();
	return vBodyDir;
}


Vector CBaseCombatCharacter::BodyDirection3D( void )
{
	QAngle angles = BodyAngles();

	// FIXME: cache this
	Vector vBodyDir;
	AngleVectors( angles, &vBodyDir );
	return vBodyDir;
}


void CBaseCombatCharacter::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Skip this work if we're already marked for transmission.
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	bool bLocalPlayer = ( pInfo->m_pClientEnt == edict() );

	if ( bLocalPlayer )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CBaseCombatWeapon *pWeapon = m_hMyWeapons[i];
			if ( !pWeapon )
				continue;

			// The local player is sent all of his weapons.
			pWeapon->SetTransmit( pInfo, bAlways );
		}
	}
	else
	{
		// The check for EF_NODRAW is useless because the weapon will be networked anyway. In CBaseCombatWeapon::
		// UpdateTransmitState all weapons with owners will transmit to clients in the PVS.
		if ( m_hActiveWeapon && !m_hActiveWeapon->IsEffectActive( EF_NODRAW ) )
			m_hActiveWeapon->SetTransmit( pInfo, bAlways );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Add or Change a class relationship for this entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AddClassRelationship ( Class_T class_type, Disposition_t disposition, int priority )
{
	// First check to see if a relationship has already been declared for this class
	// If so, update it with the new relationship
	for (int i=m_Relationship.Count()-1;i >= 0;i--) 
	{
		if (m_Relationship[i].classType == class_type) 
		{
			m_Relationship[i].disposition = disposition;
			if ( priority != DEF_RELATIONSHIP_PRIORITY )
				m_Relationship[i].priority	  = priority;
			return;
		}
	}

	int index = m_Relationship.AddToTail();
	// Add the new class relationship to our relationship table
	m_Relationship[index].classType		= class_type;
	m_Relationship[index].entity		= NULL;
	m_Relationship[index].disposition	= disposition;
	m_Relationship[index].priority		= ( priority != DEF_RELATIONSHIP_PRIORITY ) ? priority : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Add or Change a entity relationship for this entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AddEntityRelationship ( CBaseEntity* pEntity, Disposition_t disposition, int priority )
{
	// First check to see if a relationship has already been declared for this entity
	// If so, update it with the new relationship
	for (int i=m_Relationship.Count()-1;i >= 0;i--) 
	{
		if (m_Relationship[i].entity == pEntity) 
		{
			m_Relationship[i].disposition	= disposition;
			if ( priority != DEF_RELATIONSHIP_PRIORITY )
				m_Relationship[i].priority	= priority;
			return;
		}
	}

	int index = m_Relationship.AddToTail();
	// Add the new class relationship to our relationship table
	m_Relationship[index].classType		= CLASS_NONE;
	m_Relationship[index].entity		= pEntity;
	m_Relationship[index].disposition	= disposition;
	m_Relationship[index].priority		= ( priority != DEF_RELATIONSHIP_PRIORITY ) ? priority : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Removes an entity relationship from our list
// Input  : *pEntity - Entity with whom the relationship should be ended
// Output : True is entity was removed, false if it was not found
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::RemoveEntityRelationship( CBaseEntity *pEntity )
{
	// Find the entity in our list, if it exists
	for ( int i = m_Relationship.Count()-1; i >= 0; i-- ) 
	{
		if ( m_Relationship[i].entity == pEntity )
		{
			// Done, remove it
			m_Relationship.Remove( i );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Allocates default relationships
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::AllocateDefaultRelationships( )
{
	if (!m_DefaultRelationship)
	{
		m_DefaultRelationship = new Relationship_t*[NUM_AI_CLASSES];

		for (int i=0; i<NUM_AI_CLASSES; ++i)
		{
			// Be default all relationships are neutral of priority zero
			m_DefaultRelationship[i] = new Relationship_t[NUM_AI_CLASSES];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return an interaction ID (so we have no collisions)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetDefaultRelationship(Class_T nClass, Class_T nClassTarget, Disposition_t nDisposition, int nPriority)
{
	if (m_DefaultRelationship)
	{
		m_DefaultRelationship[nClass][nClassTarget].disposition	= nDisposition;
		m_DefaultRelationship[nClass][nClassTarget].priority	= nPriority;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fetch the default (ignore ai_relationship changes) relationship
// Input  :
// Output :
//-----------------------------------------------------------------------------
Disposition_t CBaseCombatCharacter::GetDefaultRelationshipDisposition( Class_T nClassTarget )
{
	Assert( m_DefaultRelationship != NULL );

	return m_DefaultRelationship[Classify()][nClassTarget].disposition;
}


//-----------------------------------------------------------------------------
// Purpose: describes the relationship between two types of NPC.
// Input  :
// Output :
//-----------------------------------------------------------------------------
Relationship_t *CBaseCombatCharacter::FindEntityRelationship(CBaseEntity *pTarget, int relation)
{
	if ((pTarget == NULL) && (relation == CLASS_NONE))
	{
		static Relationship_t dummy;
		return &dummy;
	}

	if (pTarget)
		relation = pTarget->Classify();

	// First check for specific relationship with this edict
	int i;
	for (i = 0; i < m_Relationship.Count(); i++)
	{
		if (((relation == m_Relationship[i].classType) && (relation != CLASS_NONE)) || (pTarget == (CBaseEntity *)m_Relationship[i].entity))
			return &m_Relationship[i];
	}

	if (relation != CLASS_NONE)
	{
		// Then check for relationship with this edict's class
		for (i = 0; i < m_Relationship.Count(); i++)
		{
			if (relation == m_Relationship[i].classType)
				return &m_Relationship[i];
		}
	}

	AllocateDefaultRelationships();
	// If none found return the default
	return &m_DefaultRelationship[Classify()][relation];
}

Disposition_t CBaseCombatCharacter::IRelationType(CBaseEntity *pTarget, int relation)
{
	if (pTarget || (relation != CLASS_NONE))
		return FindEntityRelationship(pTarget, relation)->disposition;
	return D_NU;
}

//-----------------------------------------------------------------------------
// Purpose: describes the relationship between two types of NPC.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CBaseCombatCharacter::IRelationPriority(CBaseEntity *pTarget, int relation)
{
	if (pTarget || (relation != CLASS_NONE))
		return FindEntityRelationship(pTarget, relation)->priority;
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at current position/orientation
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CBaseCombatCharacter::Weapon_ShootPosition( )
{
	Vector forward, right, up;

	AngleVectors( GetAbsAngles(), &forward, &right, &up );

	Vector vecSrc = GetAbsOrigin() 
					+ forward * m_HackedGunPos.y 
					+ right * m_HackedGunPos.x 
					+ up * m_HackedGunPos.z;

	return vecSrc;
}

ConVar	phys_stressbodyweights( "phys_stressbodyweights", "5.0" );
void CBaseCombatCharacter::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	ApplyStressDamage( pPhysics, false );
	BaseClass::VPhysicsUpdate( pPhysics );
}

float CBaseCombatCharacter::CalculatePhysicsStressDamage( vphysics_objectstress_t *pStressOut, IPhysicsObject *pPhysics )
{
	// stress damage hack.
	float mass = pPhysics->GetMass();
	CalculateObjectStress( pPhysics, this, pStressOut );
	float stress = (pStressOut->receivedStress * m_impactEnergyScale) / mass;

	// Make sure the stress isn't from being stuck inside some static object.
	// how many times your own weight can you hold up?
	if ( pStressOut->hasNonStaticStress && stress > phys_stressbodyweights.GetFloat() )
	{
		// if stuck, don't do this!
		if ( !(pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING) )
			return 200;
	}

	return 0;
}

void CBaseCombatCharacter::ApplyStressDamage( IPhysicsObject *pPhysics, bool bRequireLargeObject )
{
	vphysics_objectstress_t stressOut;
	float damage = CalculatePhysicsStressDamage( &stressOut, pPhysics );
	if ( damage > 0 )
	{
		if ( bRequireLargeObject && !stressOut.hasLargeObjectContact )
			return;

		//Msg("Stress! %.2f / %.2f\n", stressOut.exertedStress, stressOut.receivedStress );
		CTakeDamageInfo dmgInfo( GetWorldEntity(), GetWorldEntity(), vec3_origin, vec3_origin, damage, DMG_CRUSH );
		dmgInfo.SetDamageForce( Vector( 0, 0, -stressOut.receivedStress * GetCurrentGravity() * gpGlobals->frametime ) );
		dmgInfo.SetDamagePosition( GetAbsOrigin() );
		TakeDamage( dmgInfo );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const impactdamagetable_t
//-----------------------------------------------------------------------------
const impactdamagetable_t &CBaseCombatCharacter::GetPhysicsImpactDamageTable( void )
{
	return gDefaultNPCImpactDamageTable;
}

// how much to amplify impact forces
// This is to account for the ragdolls responding differently than
// the shadow objects.  Also this makes the impacts more dramatic.
ConVar	phys_impactforcescale( "phys_impactforcescale", "1.0" ); 

void CBaseCombatCharacter::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];
	IPhysicsObject *pOtherPhysics = pEvent->pObjects[otherIndex];
	if ( !pOther )
		return;

	// Ragdolls are marked as dying.
	if ( pOther->m_lifeState == LIFE_DYING )
		return;

	if ( pOther->GetMoveType() != MOVETYPE_VPHYSICS )
		return;
	
	if ( !pOtherPhysics->IsMoveable() )
		return;
	
	if ( pOther == GetGroundEntity() )
		return;

	// Player can't damage himself if he's was physics attacker *on this frame*
	// which can occur owing to ordering issues it appears.
	float flOtherAttackerTime = 0.0f;

	if ( this == pOther->HasPhysicsAttacker( flOtherAttackerTime ) )
		return;

	int damageType = 0;
	float damage = 0;

	damage = CalculatePhysicsImpactDamage( index, pEvent, GetPhysicsImpactDamageTable(), m_impactEnergyScale, false, damageType );
	
	if ( damage <= 0 )
		return;
	
	// NOTE: We really need some rotational motion for some of these collisions.
	// REVISIT: Maybe resolve this collision on death with a different (not approximately infinite like AABB tensor)
	// inertia tensor to get torque?
	Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass() * phys_impactforcescale.GetFloat();
	Vector damagePos;
	pEvent->pInternalData->GetContactPoint( damagePos );
	CTakeDamageInfo dmgInfo( pOther, pOther, damageForce, damagePos, damage, damageType );

	// FIXME: is there a better way for physics objects to keep track of what root entity responsible for them moving?
	CBasePlayer *pPlayer = pOther->HasPhysicsAttacker( 1.0 );
	if (pPlayer)
	{
		dmgInfo.SetAttacker( pPlayer );
	}

	// UNDONE: Find one near damagePos?
	m_nForceBone = 0;
	PhysCallbackDamage( this, dmgInfo, *pEvent, index );
}


//-----------------------------------------------------------------------------
// Purpose: this entity is exploding, or otherwise needs to inflict damage upon 
//			entities within a certain range.  only damage ents that can clearly 
//			be seen by the explosion!
// Input  :
// Output :
//-----------------------------------------------------------------------------	
void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	// NOTE: I did this this way so I wouldn't have to change a whole bunch of
	// code unnecessarily. We need TF2 specific rules for RadiusDamage, so I moved
	// the implementation of radius damage into gamerules. All existing code calls
	// this method, which calls the game rules method
	g_pGameRules->RadiusDamage( info, vecSrc, flRadius, iClassIgnore, pEntityIgnore );

	// Let the world know if this was an explosion.
	if( info.GetDamageType() & DMG_BLAST )
	{
		// Even the tiniest explosion gets attention. Don't let the radius
		// be less than 128 units.
		float soundRadius = MAX( 128.0f, flRadius * 1.5 );

		CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_EXPLOSION, vecSrc, soundRadius, 0.25, info.GetInflictor() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Change active weapon and notify derived classes
//			
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::SetActiveWeapon( CBaseCombatWeapon *pNewWeapon )
{
	CBaseCombatWeapon *pOldWeapon = m_hActiveWeapon;
	if ( pNewWeapon != pOldWeapon )
	{
		m_hActiveWeapon = pNewWeapon;
		OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
	}
}

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CBaseCombatCharacter::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	return (pWeapon ? pWeapon->GetBulletSpread() : VECTOR_CONE_5DEGREES);
}

//-----------------------------------------------------------------------------
float CBaseCombatCharacter::GetSpreadBias( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	return 1.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define MAX_MISS_CANDIDATES 16
CBaseEntity *CBaseCombatCharacter::FindMissTarget( void )
{
	CBaseEntity *pMissCandidates[ MAX_MISS_CANDIDATES ];
	int numMissCandidates = 0;

	CBasePlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
	CBaseEntity *pEnts[256];
	Vector		radius( 100, 100, 100);
	Vector		vecSource = GetAbsOrigin();

	int numEnts = UTIL_EntitiesInBox( pEnts, 256, vecSource-radius, vecSource+radius, 0 );

	for ( int i = 0; i < numEnts; i++ )
	{
		if ( pEnts[i] == NULL )
			continue;

		// New rule for this system. Don't shoot what the player won't see.
		if ( pPlayer && !pPlayer->FInViewCone( pEnts[ i ] ) )
			continue;

		if ( numMissCandidates >= MAX_MISS_CANDIDATES )
			break;

		//See if it's a good target candidate
		if ( FClassnameIs( pEnts[i], "prop_dynamic" ) || 
			 FClassnameIs( pEnts[i], "prop_physics" ) || 
			 FClassnameIs( pEnts[i], "physics_prop" ) )
		{
			pMissCandidates[numMissCandidates++] = pEnts[i];
			continue;
		}
	}

	if( numMissCandidates == 0 )
		return NULL;

	return pMissCandidates[ random->RandomInt( 0, numMissCandidates - 1 ) ];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CBaseCombatCharacter::ShouldShootMissTarget( CBaseCombatCharacter *pAttacker )
{
	// Don't shoot at NPC's right now.
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::InputKilledNPC( inputdata_t &inputdata )
{
	OnKilledNPC( inputdata.pActivator ? inputdata.pActivator->MyCombatCharacterPointer() : NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::DoMuzzleFlash()
{
}

//-----------------------------------------------------------------------------
// Purpose: Changing team, maintain associated data
//-----------------------------------------------------------------------------
void CBaseCombatCharacter::ChangeTeam( int iTeamNum )
{
	// old team member no longer in the nav mesh
	ClearLastKnownArea();

	BaseClass::ChangeTeam( iTeamNum );
}
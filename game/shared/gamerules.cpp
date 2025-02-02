//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "ammodef.h"
#include "tier0/vprof.h"
#include "KeyValues.h"

#ifdef CLIENT_DLL
	#include "usermessages.h"
#else
	#include "player.h"
	#include "teamplay_gamerules.h"
	#include "game.h"
	#include "entitylist.h"
	#include "basecombatweapon.h"
	#include "voice_gamemgr.h"
	#include "player_resource.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
ConVar log_verbose_enable( "log_verbose_enable", "0", FCVAR_GAMEDLL, "Set to 1 to enable verbose server log on the server." );
ConVar log_verbose_interval( "log_verbose_interval", "3.0", FCVAR_GAMEDLL, "Determines the interval (in seconds) for the verbose server log." );
#endif // CLIENT_DLL

static CViewVectors g_DefaultViewVectors(
	Vector( 0, 0, 64 ),			//VEC_VIEW (m_vView)
								
	Vector(-16, -16, 0 ),		//VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),		//VEC_HULL_MAX (m_vHullMax)
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),		//VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),			//VEC_DUCK_VIEW		(m_vDuckView)
													
	Vector(-10, -10, -10 ),		//VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),		//VEC_OBS_HULL_MAX	(m_vObsHullMax)
													
	Vector( 0, 0, 14 ),			//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0),	  //VEC_SLIDE_HULL_MIN (m_vSlideHullMin)
	Vector(16, 16, 36),	  //VEC_SLIDE_HULL_MAX	(m_vSlideHullMax)
	Vector(0, 0, 28)		  //VEC_SLIDE_VIEW		(m_vSlideView)
);													
													

// ------------------------------------------------------------------------------------ //
// CGameRulesProxy implementation.
// ------------------------------------------------------------------------------------ //

CGameRulesProxy *CGameRulesProxy::s_pGameRulesProxy = NULL;

IMPLEMENT_NETWORKCLASS_ALIASED( GameRulesProxy, DT_GameRulesProxy )

// Don't send any of the CBaseEntity stuff..
BEGIN_NETWORK_TABLE_NOBASE( CGameRulesProxy, DT_GameRulesProxy )
END_NETWORK_TABLE()


CGameRulesProxy::CGameRulesProxy()
{
	// allow map placed proxy entities to overwrite the static one
	if ( s_pGameRulesProxy )
	{
#ifndef CLIENT_DLL
		UTIL_Remove( s_pGameRulesProxy );
#endif
		s_pGameRulesProxy = NULL;
	}
	s_pGameRulesProxy = this;
}

CGameRulesProxy::~CGameRulesProxy()
{
	if ( s_pGameRulesProxy == this )
	{
		s_pGameRulesProxy = NULL;
	}
}

int CGameRulesProxy::UpdateTransmitState()
{
#ifndef CLIENT_DLL
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
#else
	return 0;
#endif

}

void CGameRulesProxy::NotifyNetworkStateChanged()
{
	if ( s_pGameRulesProxy )
		s_pGameRulesProxy->NetworkStateChanged();
}

ConVar	old_radius_damage( "old_radiusdamage", "0.0", FCVAR_REPLICATED );

#ifdef CLIENT_DLL //{

bool CGameRules::IsLocalPlayer( int nEntIndex )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	return ( pLocalPlayer && pLocalPlayer == ClientEntityList().GetEnt( nEntIndex ) );
}

CGameRules::CGameRules() : CAutoGameSystemPerFrame( "CGameRules" )
{
	Assert( !g_pGameRules );
	g_pGameRules = this;
}	

#else //}{

// In tf_gamerules.cpp or hl_gamerules.cpp.
extern IVoiceGameMgrHelper *g_pVoiceGameMgrHelper;

CGameRules*	g_pGameRules = NULL;
extern bool	g_fGameOver;

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CGameRules::CGameRules() : CAutoGameSystemPerFrame( "CGameRules" )
{
	Assert( !g_pGameRules );
	g_pGameRules = this;

	GetVoiceGameMgr()->Init( g_pVoiceGameMgrHelper, gpGlobals->maxClients );
	ClearMultiDamage();

	m_flNextVerboseLogOutput = 0.0f;
}

//=========================================================
//=========================================================
CBaseEntity *CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();
	Assert( pSpawnSpot );

	pPlayer->SetLocalOrigin( pSpawnSpot->GetAbsOrigin() + Vector(0,0,1) );
	pPlayer->SetAbsVelocity( vec3_origin );
	pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
	pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
	pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
	pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );

	return pSpawnSpot;
}

// checks if the spot is clear of players
bool CGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer  )
{
	CBaseEntity *ent = NULL;

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 45 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		if ( (ent->IsPlayer() && ent != pPlayer) || ent->IsNPC() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool IsExplosionTraceBlocked( trace_t *ptr )
{
	if( ptr->DidHitWorld() )
		return true;

	if( ptr->m_pEnt == NULL )
		return false;

	if( ptr->m_pEnt->GetMoveType() == MOVETYPE_PUSH )
	{
		// All doors are push, but not all things that push are doors. This 
		// narrows the search before we start to do classname compares.
		if( FClassnameIs(ptr->m_pEnt, "prop_door_rotating") ||
        FClassnameIs(ptr->m_pEnt, "func_door") ||
        FClassnameIs(ptr->m_pEnt, "func_door_rotating") ||
		FClassnameIs(ptr->m_pEnt, "prop_door_breakable"))
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Default implementation of radius damage
//-----------------------------------------------------------------------------
#define ROBUST_RADIUS_PROBE_DIST 16.0f // If a solid surface blocks the explosion, this is how far to creep along the surface looking for another way to the target
void CGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&(~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;

#ifdef HL2_DLL
	if( bInWater )
	{
		// Only muffle the explosion if deeper than 2 feet in water.
		if( !(UTIL_PointContents(vecSrc + Vector(0, 0, 24)) & MASK_WATER) )
		{
			bInWater = false;
		}
	}
#endif // HL2_DLL
	
	vecSrc.z += 1;// in case grenade is lying on the ground

	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;

		// Simple ent rel. ignore checks:
		if ((pEntity == pEntityIgnore) || (pEntity->m_takedamage == DAMAGE_NO) || ((iClassIgnore != CLASS_NONE) && (pEntity->Classify() == iClassIgnore)))
			continue;

		// Entity relationship extended check:
		if ((info.GetRelationshipLink() != CLASS_NONE) && pEntity->MyCombatCharacterPointer() && IsTeamplay() && (info.GetAttacker() != pEntity)) // Prevent accidental FF...
		{
			if (((info.GetAttacker() == NULL) || info.IsMiscFlagActive(TAKEDMGINFO_FORCE_RELATIONSHIP_CHECK)) && (pEntity->MyCombatCharacterPointer()->IRelationType(NULL, info.GetRelationshipLink()) != D_HT))
				continue;

			// Non-infected and infected plrs are still allies!
			if (((info.GetRelationshipLink() == CLASS_PLAYER) || (info.GetRelationshipLink() == CLASS_PLAYER_INFECTED) || (info.GetAttacker() && info.GetAttacker()->IsPlayer() && (info.GetAttacker()->GetTeamNumber() < TEAM_HUMANS))) && pEntity->IsHuman(true) && !pEntity->IsMercenary())
				continue;
		}

		// blast's don't tavel into or out of water
		if (bInWater && pEntity->GetWaterLevel() == 0)
			continue;

		if (!bInWater && pEntity->GetWaterLevel() == 3)
			continue;

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->BodyTarget( vecSrc, false );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

		if( old_radius_damage.GetBool() )
		{
			if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			continue;
		}
		else
		{
			if ( tr.fraction != 1.0 )
			{
				if ( IsExplosionTraceBlocked(&tr) )
				{
					if( ShouldUseRobustRadiusDamage( pEntity ) )
					{
						if( vecSpot.DistToSqr( vecSrc ) > flHalfRadiusSqr )
						{
							// Only use robust model on a target within one-half of the explosion's radius.
							continue;
						}

						Vector vecToTarget = vecSpot - tr.endpos;
						VectorNormalize( vecToTarget );

						// We're going to deflect the blast along the surface that 
						// interrupted a trace from explosion to this target.
						Vector vecUp, vecDeflect;
						CrossProduct( vecToTarget, tr.plane.normal, vecUp );
						CrossProduct( tr.plane.normal, vecUp, vecDeflect );
						VectorNormalize( vecDeflect );

						// Trace along the surface that intercepted the blast...
						UTIL_TraceLine( tr.endpos, tr.endpos + vecDeflect * ROBUST_RADIUS_PROBE_DIST, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
						//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 255, 0, false, 10 );

						// ...to see if there's a nearby edge that the explosion would 'spill over' if the blast were fully simulated.
						UTIL_TraceLine( tr.endpos, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
						//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, false, 10 );

						if( tr.fraction != 1.0 && tr.DidHitWorld() )
						{
							// Still can't reach the target.
							continue;
						}
						// else fall through
					}
					else
					{
						continue;
					}
				}

				// UNDONE: Probably shouldn't let children block parents either?  Or maybe those guys should set their owner if they want this behavior?
				// HL2 - Dissolve damage is not reduced by interposing non-world objects
				if( tr.m_pEnt && tr.m_pEnt != pEntity && tr.m_pEnt->GetOwnerEntity() != pEntity )
				{
					// Some entity was hit by the trace, meaning the explosion does not have clear
					// line of sight to the entity that it's trying to hurt. If the world is also
					// blocking, we do no damage.
					CBaseEntity *pBlockingEntity = tr.m_pEnt;
					//Msg( "%s may be blocked by %s...", pEntity->GetClassname(), pBlockingEntity->GetClassname() );

					UTIL_TraceLine( vecSrc, vecSpot, CONTENTS_SOLID, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

					if( tr.fraction != 1.0 )
					{
						continue;
					}
					
					// Now, if the interposing object is physics, block some explosion force based on its mass.
					if( pBlockingEntity->VPhysicsGetObject() )
					{
						const float MASS_ABSORB_ALL_DAMAGE = 350.0f;
						float flMass = pBlockingEntity->VPhysicsGetObject()->GetMass();
						float scale = flMass / MASS_ABSORB_ALL_DAMAGE;

						// Absorbed all the damage.
						if( scale >= 1.0f )
						{
							continue;
						}

						ASSERT( scale > 0.0f );
						flBlockedDamagePercent = scale;
						//Msg("  Object (%s) weighing %fkg blocked %f percent of explosion damage\n", pBlockingEntity->GetClassname(), flMass, scale * 100.0f);
					}
					else
					{
						// Some object that's not the world and not physics. Generically block 25% damage
						flBlockedDamagePercent = 0.25f;
					}
				}
			}
		}

		// decrease damage for an ent that's farther from the bomb.
		flAdjustedDamage = ( vecSrc - tr.endpos ).Length() * falloff;
		flAdjustedDamage = info.GetDamage() - (info.IsMiscFlagActive(TAKEDMGINFO_USE_DMG_AS_PERCENT) ? 0.0f : flAdjustedDamage);

		if (flAdjustedDamage <= 0)
			continue;

		// the explosion can 'see' this entity, so hurt them!
		if (tr.startsolid)
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}
		
		CTakeDamageInfo adjustedInfo = info;
		//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );

		if (info.IsMiscFlagActive(TAKEDMGINFO_USE_DMG_AS_PERCENT))
		{
			float damageToTake = ceil(((float)pEntity->GetHealth()) * (info.GetDamage() / 100.0f));
			damageToTake = MAX(damageToTake, 1.0f);
			adjustedInfo.SetDamage((damageToTake - (damageToTake * flBlockedDamagePercent)));
		}

		Vector dir = vecSpot - vecSrc;
		VectorNormalize( dir );

		// If we don't have a damage force, manufacture one
		if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
		{
			if ( !( adjustedInfo.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) )
			{
				CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
			}
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * (info.IsMiscFlagActive(TAKEDMGINFO_DISABLE_FORCELIMIT) ? 1.0f : falloff);
			adjustedInfo.SetDamageForce(dir * flForce);
			adjustedInfo.SetDamagePosition(vecSrc);
		}

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
		{
			ClearMultiDamage( );
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
			ApplyMultiDamage();
		}
		else
		{
			pEntity->TakeDamage( adjustedInfo );
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers(adjustedInfo, vecSrc, tr.endpos, dir);
	}
}


bool CGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	if( pEdict->IsPlayer() )
	{
		if( GetVoiceGameMgr()->ClientCommand( static_cast<CBasePlayer*>(pEdict), args ) )
			return true;
	}

	return false;
}


void CGameRules::FrameUpdatePostEntityThink()
{
	VPROF( "CGameRules::FrameUpdatePostEntityThink" );
	Think();
}

void CGameRules::Think()
{
	GetVoiceGameMgr()->Update( gpGlobals->frametime );
	if ( log_verbose_enable.GetBool() )
	{
		if ( m_flNextVerboseLogOutput < gpGlobals->curtime )
		{
			ProcessVerboseLogOutput();
			m_flNextVerboseLogOutput = gpGlobals->curtime + log_verbose_interval.GetFloat();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called at the end of GameFrame (i.e. after all game logic has run this frame)
//-----------------------------------------------------------------------------
void CGameRules::EndGameFrame( void )
{
	// If you hit this assert, it means something called AddMultiDamage() and didn't ApplyMultiDamage().
	// The g_MultiDamage.m_hAttacker & g_MultiDamage.m_hInflictor should give help you figure out the culprit.
	Assert( g_MultiDamage.IsClear() );
	if ( !g_MultiDamage.IsClear() )
	{
		Warning("Unapplied multidamage left in the system:\nTarget: %s\nInflictor: %s\nAttacker: %s\nDamage: %.2f\n", 
			g_MultiDamage.GetTarget()->GetDebugName(),
			g_MultiDamage.GetInflictor()->GetDebugName(),
			g_MultiDamage.GetAttacker()->GetDebugName(),
			g_MultiDamage.GetDamage() );
		ApplyMultiDamage();
	}
}

//-----------------------------------------------------------------------------
// trace line rules
//-----------------------------------------------------------------------------
float CGameRules::WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd,
					 unsigned int mask, trace_t *ptr )
{
	UTIL_TraceEntity( pEntity, vecStart, vecEnd, mask, ptr );
	return 1.0f;
}


void CGameRules::CreateStandardEntities()
{
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );
	g_pPlayerResource->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
}

#endif //} !CLIENT_DLL


// ----------------------------------------------------------------------------- //
// Shared CGameRules implementation.
// ----------------------------------------------------------------------------- //

CGameRules::~CGameRules()
{
	Assert( g_pGameRules == this );
	g_pGameRules = NULL;
}

bool CGameRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	return false;
}

CBaseCombatWeapon *CGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	return NULL;
}

bool CGameRules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	if (collisionGroup0 > collisionGroup1)
	{
		// swap so that lowest is always first
		::V_swap(collisionGroup0, collisionGroup1);
	}

	// let debris and multiplayer objects collide
	if (collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY)
		return true;

	if ((collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER) && (collisionGroup0 != COLLISION_GROUP_NPC))
		return false;

	if ((collisionGroup0 == COLLISION_GROUP_DOOR_BLOCKER) && ((collisionGroup1 != COLLISION_GROUP_NPC_ZOMBIE) && (collisionGroup1 != COLLISION_GROUP_NPC_ZOMBIE_CRAWLER) && (collisionGroup1 != COLLISION_GROUP_NPC_ZOMBIE_BOSS) && (collisionGroup1 != COLLISION_GROUP_NPC_MILITARY) && (collisionGroup1 != COLLISION_GROUP_NPC_MERCENARY)))
		return false;

	if ((collisionGroup0 == COLLISION_GROUP_PLAYER) && (collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR))
		return false;

	if (collisionGroup0 == COLLISION_GROUP_DEBRIS || collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER)
	{
		// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
		return false;
	}

	// Dissolving guys only collide with COLLISION_GROUP_NONE
	if ((collisionGroup0 == COLLISION_GROUP_DISSOLVING) || (collisionGroup1 == COLLISION_GROUP_DISSOLVING))
	{
		if (collisionGroup0 != COLLISION_GROUP_NONE)
			return false;
	}

	// doesn't collide with other members of this group
	// or debris, but that's handled above
	if (collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS)
		return false;

	if (collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS)
		return false;

	// interactive objects collide with everything except debris & interactive debris
	if (collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE)
		return false;

	// Projectiles hit everything but debris, weapons, + other projectiles
	if (collisionGroup1 == COLLISION_GROUP_PROJECTILE)
	{
		if (collisionGroup0 == COLLISION_GROUP_DEBRIS ||
			collisionGroup0 == COLLISION_GROUP_WEAPON ||
			collisionGroup0 == COLLISION_GROUP_PROJECTILE)
		{
			return false;
		}
	}

	return true;
}

const CViewVectors* CGameRules::GetViewVectors() const
{
	return &g_DefaultViewVectors;
}

#ifndef CLIENT_DLL
const char *CGameRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( pPlayer && pPlayer->IsAlive() == false )
	{
		if ( bTeamOnly )
			return "*DEAD*(TEAM)";
		else
			return "*DEAD*";
	}
	
	return "";
}

void CGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue(pPlayer->entindex(), "name");
	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive

	if (pszOldName[0] != 0 && Q_strcmp(pszOldName, pszName))
	{
		IGameEvent * event = gameeventmanager->CreateEvent("player_changename");
		if (event)
		{
			event->SetInt("userid", pPlayer->GetUserID());
			event->SetString("oldname", pszOldName);
			event->SetString("newname", pszName);
			gameeventmanager->FireEvent(event);
		}
		pPlayer->SetPlayerName(pszName);
	}

	//const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	//if ( pszFov )
	//{
	//	int iFov = atoi(pszFov);
	//	iFov = clamp( iFov, 75, 90 );
	//	pPlayer->SetDefaultFOV( iFov );
	//}
}

#endif
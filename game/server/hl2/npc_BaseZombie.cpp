//========= Copyright Bernt A Eide. ============//
//
// Purpose: Base Zombie Class
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npc_BaseZombie.h"
#include "player.h"
#include "game.h"
#include "ai_network.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_memory.h"
#include "ai_senses.h"
#include "bitstring.h"
#include "EntityFlame.h"
#include "hl2_shareddefs.h"
#include "npcevent.h"
#include "activitylist.h"
#include "entitylist.h"
#include "gib.h"
#include "soundenvelope.h"
#include "ndebugoverlay.h"
#include "rope.h"
#include "rope_shared.h"
#include "igamesystem.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "hl2_shared_misc.h"
#include "ammodef.h"
#include "vehicle_base.h"
#include "hl2mp_gamerules.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_interactionZombieMeleeWarning;

envelopePoint_t envDefaultZombieMoanVolumeFast[] =
{
	{	7.0f, 7.0f,
	0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
	0.2f, 0.3f,
	},
};

envelopePoint_t envDefaultZombieMoanVolume[] =
{
	{	1.0f, 1.0f,
	0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
	0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
	0.3f, 0.4f,
	},
};

//
// After taking damage, ignore further damage for n seconds. This keeps the zombie
// from being interrupted while.
//
#define ZOMBIE_FLINCH_DELAY			3
#define ZOMBIE_BURN_TIME		10 // If ignited, burn for this many seconds
#define ZOMBIE_BURN_TIME_NOISE	2  // Give or take this many seconds.

//=========================================================
// private activities
//=========================================================
int CNPC_BaseZombie::ACT_ZOM_FALL;

// When a zombie spawns, he will select a 'base' pitch value
// that's somewhere between basepitchmin & basepitchmax
ConVar zombie_basemin( "zombie_basemin", "100" );
ConVar zombie_basemax( "zombie_basemax", "100" );

ConVar zombie_changemin( "zombie_changemin", "0" );
ConVar zombie_changemax( "zombie_changemax", "0" );

// play a sound once in every zombie_stepfreq steps
ConVar zombie_stepfreq( "zombie_stepfreq", "4" );
ConVar zombie_moanfreq( "zombie_moanfreq", "1" );

ConVar zombie_decaymin( "zombie_decaymin", "0.1" );
ConVar zombie_decaymax( "zombie_decaymax", "0.4" );

//=========================================================
// For a couple of reasons, we keep a running count of how
// many zombies in the world are angry at any given time.
//=========================================================
static int s_iAngryZombies = 0;
int g_pZombiesInWorld = 0;

//=========================================================
//=========================================================
class CAngryZombieCounter : public CAutoGameSystem
{
public:
	CAngryZombieCounter( char const *name ) : CAutoGameSystem( name )
	{
	}
	// Level init, shutdown
	virtual void LevelInitPreEntity()
	{
		s_iAngryZombies = 0;
	}
};

CAngryZombieCounter	AngryZombieCounter( "CAngryZombieCounter" );

int AE_ZOMBIE_ATTACK_RIGHT;
int AE_ZOMBIE_ATTACK_LEFT;
int AE_ZOMBIE_ATTACK_BOTH;
int AE_ZOMBIE_STEP_LEFT;
int AE_ZOMBIE_STEP_RIGHT;
int AE_ZOMBIE_SCUFF_LEFT;
int AE_ZOMBIE_SCUFF_RIGHT;
int AE_ZOMBIE_ATTACK_SCREAM;
int AE_ZOMBIE_GET_UP;
int AE_ZOMBIE_POUND;
int AE_ZOMBIE_ALERTSOUND;

//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_BaseZombie )

	DEFINE_SOUNDPATCH( m_pMoanSound ),
	DEFINE_FIELD( m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( m_flBurnDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBurnDamageResetTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextMoanSound, FIELD_TIME ),
	DEFINE_FIELD( m_flMoanPitch, FIELD_FLOAT ),
	DEFINE_FIELD( m_iMoanSound, FIELD_INTEGER ),
	DEFINE_FIELD( m_bIsSlumped, FIELD_BOOLEAN ),
	DEFINE_FIELD(m_hBlockingEntity, FIELD_EHANDLE),
	DEFINE_EMBEDDED(m_DurationDoorBash),
	DEFINE_EMBEDDED(m_NextTimeToStartDoorBash),

	END_DATADESC()

	//---------------------------------------------------------
	//---------------------------------------------------------
	int CNPC_BaseZombie::g_numZombies = 0;

//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::CNPC_BaseZombie() : 
m_DurationDoorBash(1, 2),
m_NextTimeToStartDoorBash(2.0)
{
	// Prevent same moan sound on x zombies spawned
	m_iMoanSound = g_numZombies;
	// count of zombs in world
	g_numZombies++;

	m_hLastIgnitionSource = NULL;

	g_pZombiesInWorld++;
}


//---------------------------------------------------------
//---------------------------------------------------------
CNPC_BaseZombie::~CNPC_BaseZombie()
{
	g_numZombies--;
	g_pZombiesInWorld--;
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseZombie::Classify( void )
{
	if ( IsSlumped() )
		return CLASS_NONE;

	return( CLASS_ZOMBIE ); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_BaseZombie::IRelationType( CBaseEntity *pTarget )
{
	// Slumping should not affect Zombie's opinion of others
	if ( IsSlumped() )
	{
		m_bIsSlumped = false;
		Disposition_t result = BaseClass::IRelationType( pTarget );
		m_bIsSlumped = true;
		return result;
	}

	return BaseClass::IRelationType( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the maximum yaw speed based on the monster's current activity.
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::MaxYawSpeed( void )
{
	if (IsMoving() && HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
		return( 15 );
	else
	{
		switch( GetActivity() )
		{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			return 100;
			break;
		case ACT_RUN:
			return 15;
			break;
		case ACT_WALK:
		case ACT_IDLE:
			return 25;
			break;
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 120;
		default:
			return 90;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
		return BaseClass::OverrideMoveFacing( move, flInterval );

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );
	float idealYaw = UTIL_AngleMod( flMoveYaw );

	if (GetEnemy())
	{
		float flEDist = UTIL_DistApprox2D( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter() );

		if (flEDist < 256.0)
		{
			float flEYaw = UTIL_VecToYaw( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );

			if (flEDist < 128.0)
				idealYaw = flEYaw;
			else
				idealYaw = flMoveYaw + UTIL_AngleDiff( flEYaw, flMoveYaw ) * (2 - flEDist / 128.0);
		}
	}

	GetMotor()->SetIdealYawAndUpdate( idealYaw );

	// find movement direction to compensate for not being turned far enough
	float fSequenceMoveYaw = GetSequenceMoveYaw( GetSequence() );
	float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y + fSequenceMoveYaw );
	SetPoseParameter( m_poseMove_Yaw, GetPoseParameter( m_poseMove_Yaw ) + flDiff );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseZombie::MeleeAttack1Conditions ( float flDot, float flDist )
{
	float range = GetClawAttackRange();

	if (flDist > range )
		return COND_TOO_FAR_TO_ATTACK;

	if (flDot < 0.7)
		return COND_NOT_FACING_ATTACK;

	// Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	trace_t	tr;
	CTraceFilterNav traceFilter( this, false, this, GetCollisionGroup() );
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * GetClawAttackRange(), vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

	if( tr.fraction == 1.0 || !tr.m_pEnt )
		// This attack would miss completely. Trick the zombie into moving around some more.
		return COND_TOO_FAR_TO_ATTACK;

	if( tr.m_pEnt == GetEnemy() || 
		tr.m_pEnt->IsNPC() || 
		( tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt) ) ) )
	{
		// -Let the zombie swipe at his enemy if he's going to hit them.
		// -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
		//  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
		// -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
		return COND_CAN_MELEE_ATTACK1;
	}

	Vector vecTrace = tr.endpos - tr.startpos;
	float lenTraceSq = vecTrace.Length2DSqr();

	if ( GetEnemy() && GetEnemy()->MyCombatCharacterPointer() && tr.m_pEnt == static_cast<CBaseCombatCharacter *>(GetEnemy())->GetVehicleEntity() )
	{
		if ( lenTraceSq < Square( GetClawAttackRange() * 0.75f ) )
			return COND_CAN_MELEE_ATTACK1;
	}

	if( tr.m_pEnt->IsBSPModel() )
	{
		// The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
		// the enemy is, treat this as an obstruction.
		Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();

		if( lenTraceSq < vecToEnemy.Length2DSqr() )
			return COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION;
	}

	// Move around some more
	return COND_TOO_FAR_TO_ATTACK;
}

float CNPC_BaseZombie::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info )
{
	return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
#define ZOMBIE_SCORCH_RATE		8
#define ZOMBIE_MIN_RENDERCOLOR	50
int CNPC_BaseZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// BB2 no dmg for zombie if player zombie attack us
	CBaseEntity *pAttacker = inputInfo.GetAttacker();
	if (pAttacker)
	{
		if ((pAttacker->Classify() == CLASS_PLAYER_ZOMB) && (HL2MPRules()->m_bRoundStarted))
			return 0;
	}

	if( inputInfo.GetDamageType() & DMG_BURN )
	{
		// If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
		// This is to stop zombies from burning to death 10x faster when they're standing around
		// 10 fire entities.
		if( IsOnFire() && !(inputInfo.GetDamageType() & DMG_DIRECT) )
			return 0;

		Scorch( ZOMBIE_SCORCH_RATE, ZOMBIE_MIN_RENDERCOLOR );
	}

	if ( ShouldIgnite( info ) )
		Ignite( 100.0f );

	int tookDamage = BaseClass::OnTakeDamage_Alive( info );

	if( tookDamage > 0 && (info.GetDamageType() & (DMG_BURN|DMG_DIRECT)) && m_ActBusyBehavior.IsActive() ) 
	{
		//!!!HACKHACK- Stuff a light_damage condition if an actbusying zombie takes direct burn damage. This will cause an
		// ignited zombie to 'wake up' and rise out of its actbusy slump. (sjb)
		SetCondition( COND_LIGHT_DAMAGE );
	}

	return tookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: make a sound Alyx can hear when in darkness mode
// Input  : volume (radius) of the sound.
// Output :
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::MakeAISpookySound( float volume, float duration )
{
	CSoundEnt::InsertSound( SOUND_COMBAT, EyePosition(), volume, duration, this, SOUNDENT_CHANNEL_SPOOKY_NOISE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::CanPlayMoanSound()
{
	if( HasSpawnFlags( SF_NPC_GAG ) )
		return false;

	// Burning zombies play their moan loop at full volume for as long as they're
	// burning. Don't let a moan envelope play cause it will turn the volume down when done.
	if( IsOnFire() )
		return false;

	// Members of a small group of zombies can vocalize whenever they want
	if( s_iAngryZombies <= 4 )
		return true;

	// This serves to limit the number of zombies that can moan at one time when there are a lot. 
	if( random->RandomInt( 1, zombie_moanfreq.GetInt() * (s_iAngryZombies/2) ) == 1 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Open a window and let a little bit of the looping moan sound
//			come through.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( HasSpawnFlags( SF_NPC_GAG ) )
		// Not yet!
		return;

	if( !m_pMoanSound )
	{
		// Don't set this up until the code calls for it.
		const char *pszSound = GetMoanSound( m_iMoanSound );
		m_flMoanPitch = random->RandomInt( zombie_basemin.GetInt(), zombie_basemax.GetInt() );

		HL2MPRules()->EmitSoundToClient(this, pszSound, GetNPCType(), GetGender());

		//m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( entindex(), CHAN_STATIC, pszSound, ATTN_NORM );
		CPASAttenuationFilter filter( this );
		m_pMoanSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, pszSound, ATTN_NORM );

		ENVELOPE_CONTROLLER.Play( m_pMoanSound, 1.0, m_flMoanPitch );
	}

	//HACKHACK get these from chia chin's console vars.
	envDefaultZombieMoanVolumeFast[ 1 ].durationMin = zombie_decaymin.GetFloat();
	envDefaultZombieMoanVolumeFast[ 1 ].durationMax = zombie_decaymax.GetFloat();

	if( random->RandomInt( 1, 2 ) == 1 )
	{
		IdleSound();
	}

	float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pMoanSound, SOUNDCTRL_CHANGE_VOLUME, pEnvelope, iEnvelopeSize );

	float flPitch = random->RandomInt( m_flMoanPitch + zombie_changemin.GetInt(), m_flMoanPitch + zombie_changemax.GetInt() );
	ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, flPitch, 0.3 );

	m_flNextMoanSound = gpGlobals->curtime + duration + 9999;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this gibbing zombie should ignite its gibs
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgniteZombieGib( void )
{
	return IsOnFire();
}

//-----------------------------------------------------------------------------
// Purpose: damage has been done. Should the zombie ignite?
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgnite( const CTakeDamageInfo &info )
{
	if (!AllowedToIgnite())
		return false;

	if ( IsOnFire() )
	{
		// Already burning!
		return false;
	}

	if ( info.GetDamageType() & DMG_BURN )
	{
		//
		// If we take more than ten percent of our health in burn damage within a five
		// second interval, we should catch on fire.
		//
		m_flBurnDamage += info.GetDamage();
		m_flBurnDamageResetTime = gpGlobals->curtime + 5;

		if (m_flBurnDamage >= m_iMaxHealth * 0.1)
		{
			// remember who put us on fire.
			if (info.GetAttacker() && (info.GetAttacker()->IsPlayer() || info.GetAttacker()->IsNPC()))
				m_hLastIgnitionSource = info.GetAttacker();

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sufficient fire damage has been done. Zombie ignites!
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

	if ( GetEffectEntity() != NULL )
		GetEffectEntity()->AddEffects( EF_DIMLIGHT );

	// Set the zombie up to burn to death in about ten seconds.
	SetHealth( MIN( m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC * (ZOMBIE_BURN_TIME + random->RandomFloat( -ZOMBIE_BURN_TIME_NOISE, ZOMBIE_BURN_TIME_NOISE)) ) );

	if( !m_ActBusyBehavior.IsActive() )
	{
		Activity activity = GetActivity();
		Activity burningActivity = activity;

		if ( activity == ACT_WALK )
			burningActivity = ACT_WALK_ON_FIRE;
		else if ( activity == ACT_RUN )
			burningActivity = ACT_RUN_ON_FIRE;
		else if ( activity == ACT_IDLE )
			burningActivity = ACT_IDLE_ON_FIRE;

		if( HaveSequenceForActivity(burningActivity) )
			// Make sure we have a sequence for this activity (torsos don't have any, for instance) 
			// to prevent the baseNPC & baseAnimating code from throwing red level errors.
			SetActivity( burningActivity );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::CopyRenderColorTo( CBaseEntity *pOther )
{
	color32 color = GetRenderColor();
	pOther->SetRenderColor( color.r, color.g, color.b, color.a );
}

//-----------------------------------------------------------------------------
// Purpose: Look in front and see if the claw hit anything.
//
// Input  :	flDist				distance to trace		
//			iDamage				damage to do if attack hits
//			vecViewPunch		camera punch (if attack hits player)
//			vecVelocityPunch	velocity punch (if attack hits player)
//
// Output : The entity hit by claws. NULL if nothing.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_BaseZombie::ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin  )
{
	// Added test because claw attack anim sometimes used when for cases other than melee
	int iDriverInitialHealth = -1;
	CBaseEntity *pDriver = NULL;
	if ( GetEnemy() )
	{
		trace_t	tr;
		AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if (tr.fraction < 1.0f)
			return NULL;

		// CheckTraceHullAttack() can damage player in vehicle as side effect of melee attack damaging physics objects, which the car forwards to the player
		// need to detect this to get correct damage effects
		CBaseCombatCharacter *pCCEnemy = ( GetEnemy() != NULL ) ? GetEnemy()->MyCombatCharacterPointer() : NULL;
		CBaseEntity *pVehicleEntity;
		if ( pCCEnemy != NULL && ( pVehicleEntity = pCCEnemy->GetVehicleEntity() ) != NULL )
		{
			if ( pVehicleEntity->GetServerVehicle() && dynamic_cast<CPropVehicleDriveable *>(pVehicleEntity) )
			{
				pDriver = static_cast<CPropVehicleDriveable *>(pVehicleEntity)->GetDriver();
				if ( pDriver && pDriver->IsPlayer() )
					iDriverInitialHealth = pDriver->GetHealth();
				else
					pDriver = NULL;
			}
		}
	}

	//
	// Trace out a cubic section of our hull and see what we hit.
	//
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	CBaseEntity *pHurt = NULL;
	// Try to hit them with a trace
	pHurt = CheckTraceHullAttack( flDist, vecMins, vecMaxs, iDamage, DMG_ZOMBIE );

	if ( pDriver && iDriverInitialHealth != pDriver->GetHealth() )
		pHurt = pDriver;

	if ( pHurt )
	{
		AttackHitSound();

		CBasePlayer *pPlayer = ToBasePlayer(pHurt);
		if (pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE))
		{
			int iRandomChance = random->RandomInt(0, 100);
			if (iRandomChance <= 50)
			{
				pPlayer->ViewPunch((qaViewPunch / 4.0f));
				pPlayer->VelocityPunch(vecVelocityPunch);
			}
		}

		Vector vecBloodPos;

		switch (BloodOrigin)
		{
		case ZOMBIE_BLOOD_LEFT_HAND:
			if (GetAttachment("blood_left", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));
			break;

		case ZOMBIE_BLOOD_RIGHT_HAND:
			if (GetAttachment("blood_right", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));
			break;

		case ZOMBIE_BLOOD_BOTH_HANDS:
			if (GetAttachment("blood_left", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));

			if (GetAttachment("blood_right", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));
			break;

		case ZOMBIE_BLOOD_BITE:
			// No blood for these.
			break;
		}
	}
	else 
	{
		AttackMissSound();
	}

	return pHurt;
}

//-----------------------------------------------------------------------------
// Purpose: The zombie is frustrated and pounding walls/doors. Make an appropriate noise
// Input  : 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::PoundSound()
{
	trace_t		tr;
	Vector		forward;

	GetVectors( &forward, NULL, NULL );

	AI_TraceLine( EyePosition(), EyePosition() + forward * 128, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction == 1.0 )
	{
		// Didn't hit anything!
		return;
	}

	if( tr.fraction < 1.0 && tr.m_pEnt )
	{
		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if( psurf )
		{
			EmitSound( physprops->GetString(psurf->sounds.impactHard) );
			return;
		}
	}

	// Otherwise fall through to the default sound.
	CPASAttenuationFilter filter( this,"NPC_BaseZombie.PoundDoor" );
	EmitSound( filter, entindex(),"NPC_BaseZombie.PoundDoor" );
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_NPC_ATTACK_BROADCAST )
	{
		if( GetEnemy() && GetEnemy()->IsNPC() )
		{
			if( HasCondition(COND_CAN_MELEE_ATTACK1) )
			{
				// This animation is sometimes played by code that doesn't intend to attack the enemy
				// (For instance, code that makes a zombie take a frustrated swipe at an obstacle). 
				// Try not to trigger a reaction from our enemy unless we're really attacking. 
				GetEnemy()->MyNPCPointer()->DispatchInteraction( g_interactionZombieMeleeWarning, NULL, this );
			}
		}
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_POUND )
	{
		PoundSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ALERTSOUND )
	{
		AlertSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( false );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( true );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_GET_UP )
	{
		MakeAIFootstepSound( 180.0f, 3.0f );
		if( !IsOnFire() )
		{
			// If you let this code run while a zombie is burning, it will stop wailing. 
			m_flNextMoanSound = gpGlobals->curtime;
			MoanSound( envDefaultZombieMoanVolumeFast, ARRAYSIZE( envDefaultZombieMoanVolumeFast ) );
		}
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( false );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( true );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
	{
		AttackSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );

		right = right * 100;
		forward = forward * 200;

		QAngle qa( -15, -20, -10 );
		Vector vec = right + forward;
		ClawAttack( GetClawAttackRange(), m_iDamageOneHand, qa, vec, ZOMBIE_BLOOD_RIGHT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );

		right = right * -100;
		forward = forward * 200;

		QAngle qa( -15, 20, -10 );
		Vector vec = right + forward;
		ClawAttack(GetClawAttackRange(), m_iDamageOneHand, qa, vec, ZOMBIE_BLOOD_LEFT_HAND);
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_BOTH )
	{
		Vector forward;
		QAngle qaPunch( 45, random->RandomInt(-5,5), random->RandomInt(-5,5) );
		AngleVectors( GetLocalAngles(), &forward );
		forward = forward * 200;
		ClawAttack(GetClawAttackRange(), m_iDamageBothHands, qaPunch, forward, ZOMBIE_BLOOD_BOTH_HANDS);
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the base zombie.
//
// !!!IMPORTANT!!! YOUR DERIVED CLASS'S SPAWN() RESPONSIBILITIES:
//
//		Call Precache();
//		Set blood color
//		Set health
//		Set field of view
//		Call CapabilitiesClear() & then set relevant capabilities
//		THEN Call BaseClass::Spawn()
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Spawn( void )
{
	// Customized:
	if (ParseNPC(entindex()))
	{
		SetHealth(m_iTotalHP);
		SetMaxHealth(m_iTotalHP);
		SetNPCName(GetNPCName());
		SetBoss(IsBoss());

		PrecacheModel(GetNPCModelName());

		Hull_t lastHull = GetHullType();

		SetModel(GetNPCModelName());
		SetHullType(HULL_HUMAN);
		SetHullSizeNormal(true);
		SetDefaultEyeOffset();

		if (lastHull != GetHullType())
		{
			if (VPhysicsGetObject())
				SetupVPhysicsHull();
		}

		m_nSkin = m_iModelSkin;
		UpdateNPCScaling();
	}
	else
	{
		UTIL_Remove(this);
		return;
	}

	// If Pure Classic is enabled we will not spawn non Fred or Walker npcs...
	if (GameBaseServer()->IsClassicMode() && !FClassnameIs(this, "npc_fred") && !FClassnameIs(this, "npc_walker"))
	{
		UTIL_Remove(this);
		return;
	}

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_SQUAD );

	m_pMoanSound = NULL;

	m_flNextMoanSound = gpGlobals->curtime + 9999;

	NPCInit();

	m_bIsSlumped = false;

	// Zombies get to cheat for 6 seconds (sjb)
	GetEnemies()->SetFreeKnowledgeDuration( 6.0 );

	m_ActBusyBehavior.SetUseRenderBounds(true);

	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE);
	m_flSpawnTime = gpGlobals->curtime + ((random->RandomFloat(bb2_zombie_lifespan_min.GetFloat(), bb2_zombie_lifespan_max.GetFloat())) * 60.0f);

	if (!IsBoss() && !HL2MPRules()->CanSpawnZombie())
	{
		DevMsg(2, "Zombie limit has been reached, removing zombie!\n");
		UTIL_Remove(this);
	}

	m_flLastObstructionCheck = 0.0f;
	m_bLifeTimeOver = false;

	OnSetGibHealth();
}


//-----------------------------------------------------------------------------
// Purpose: Pecaches all resources this NPC needs.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Precache( void )
{
	PrecacheScriptSound( "NPC_BaseZombie.AttackHit" );
	PrecacheScriptSound( "NPC_BaseZombie.AttackMiss" );
	PrecacheScriptSound( "NPC_BaseZombie.FootstepRight" );
	PrecacheScriptSound( "NPC_BaseZombie.FootstepLeft" );
	PrecacheScriptSound( "NPC_BaseZombie.ScuffRight" );
	PrecacheScriptSound( "NPC_BaseZombie.ScuffLeft" );
	PrecacheScriptSound( "E3_Phystown.Slicer" );
	PrecacheScriptSound( "NPC_BaseZombie.PoundDoor" );
	PrecacheParticleSystem( "blood_impact_zombie_01" );

	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if( IsSlumped() )
	{
		if( FClassnameIs( pOther, "prop_physics" ) )
		{
			// Get up!
			m_ActBusyBehavior.StopBusying();
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::CreateBehaviors()
{
	AddBehavior( &m_ActBusyBehavior );

	return BaseClass::CreateBehaviors();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		if ( HasCondition( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION ) && !HasCondition(COND_TASK_FAILED) && IsCurSchedule( SCHED_ZOMBIE_CHASE_ENEMY, false ) )
		{
			return SCHED_COMBAT_PATROL;
		}
		return SCHED_ZOMBIE_CHASE_ENEMY;
		break;

	case SCHED_STANDOFF:
		return SCHED_ZOMBIE_WANDER_STANDOFF;

	case SCHED_MELEE_ATTACK1:
		return SCHED_ZOMBIE_MELEE_ATTACK1;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::BuildScheduleTestBits( void )
{
	// Ignore damage if we were recently damaged or we're attacking.
	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
#ifndef HL2_EPISODIC
	else if ( m_flNextFlinch >= gpGlobals->curtime )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
#endif // !HL2_EPISODIC

	BaseClass::BuildScheduleTestBits();
}


//-----------------------------------------------------------------------------
// Purpose: Called when we change schedules.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnScheduleChange( void )
{
	//
	// If we took damage and changed schedules, ignore further damage for a few seconds.
	//
	if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))
		m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;

	BaseClass::OnScheduleChange();
}


//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_BaseZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_ZOMBIE_WANDER_MEDIUM )
	{
		return SCHED_ZOMBIE_WANDER_FAIL;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::SelectSchedule ( void )
{
	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
		{
			// TODO
		}

		if (HasCondition(COND_LOST_ENEMY) || (HasCondition(COND_ENEMY_UNREACHABLE) && MustCloseToAttack()))
		{
			if (HasCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT))
				return SCHED_ZOMBIE_BASH_DOOR;

			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}

		break;

	case NPC_STATE_ALERT:
		if ( HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_DEAD ) || ( HasCondition( COND_ENEMY_UNREACHABLE ) && MustCloseToAttack() ) )
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );

			if (HasCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT))
				return SCHED_ZOMBIE_BASH_DOOR;

			// Just lost track of our enemy. 
			// Wander around a bit so we don't look like a dingus.
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}
		break;
	}

	if (HasCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT) && !GetEnemy())
		return SCHED_ZOMBIE_BASH_DOOR;

	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::IsSlumped( void )
{
	if( hl2_episodic.GetBool() )
	{
		if( m_ActBusyBehavior.IsInsideActBusy() && !m_ActBusyBehavior.IsStopBusying() )
		{
			return true;
		}
	}
	else
	{
		int sequence = GetSequence();
		if ( sequence != -1 )
		{
			return ( strncmp( GetSequenceName( sequence ), "slump", 5 ) == 0 );
		}
	}

	return false;
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::IsGettingUp( void )
{
	if( m_ActBusyBehavior.IsActive() && m_ActBusyBehavior.IsStopBusying() )
	{
		return true;
	}
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if (IsAllowedToBreakDoors() && (gpGlobals->curtime > m_flLastObstructionCheck))
	{
		m_flLastObstructionCheck = gpGlobals->curtime + 0.1f;

		ClearCondition(COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION);
		ClearCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT);
		m_hBlockingEntity = NULL;

		CBaseEntity *pObstruction = GetObstructionBreakableEntity();
		if (pObstruction)
		{
			m_hBlockingEntity = pObstruction;
			SetCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT);
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	//
	// Cool off if we aren't burned for five seconds or so. 
	//
	if ((m_flBurnDamageResetTime) && (gpGlobals->curtime >= m_flBurnDamageResetTime))
		m_flBurnDamage = 0;

	// Non boss zombies/monsters have a limited lifetime:
	if (!IsBoss() && !GameBaseServer()->IsTutorialModeEnabled())
	{
		if ((GetLifeSpan() < gpGlobals->curtime) && !m_bLifeTimeOver)
		{
			m_bLifeTimeOver = true;
			KillMe();
		}
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_YAW_TO_DOOR:
	{
		CBaseEntity *pObstruction = m_hBlockingEntity.Get();
		AssertMsg(pObstruction != NULL, "Expected condition handling to break schedule before landing here");
		if (pObstruction != NULL)
		{
			float idealYaw = UTIL_VecToYaw(pObstruction->WorldSpaceCenter() - WorldSpaceCenter());
			GetMotor()->SetIdealYaw(idealYaw);
		}

		TaskComplete();
		break;
	}

	case TASK_ZOMBIE_PATH_TO_OBSTRUCTION:
	{
		CBaseEntity *pObstruction = m_hBlockingEntity.Get();
		if (pObstruction)
		{
			AI_NavGoal_t goal(pObstruction->WorldSpaceCenter());
			goal.pTarget = pObstruction;
			GetNavigator()->SetGoal(goal);
			TaskComplete();
		}
		else
			TaskFail(FAIL_NO_TARGET);

		break;
	}

	case TASK_ZOMBIE_ATTACK_DOOR:
	{
		m_DurationDoorBash.Reset();
		SetIdealActivity(SelectDoorBash());
		break;
	}

	case TASK_ZOMBIE_DIE:
		// Go to ragdoll
		KillMe();
		TaskComplete();
		break;

	case TASK_ZOMBIE_WAIT_POST_MELEE:
		{
#ifndef HL2_EPISODIC
			TaskComplete();
			return;
#endif

			// Don't wait when attacking the player
			if ( GetEnemy() && GetEnemy()->IsPlayer() )
			{
				TaskComplete();
				return;
			}

			// Wait a single think
			SetWait( 0.1 );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_ATTACK_DOOR:
	{
		if (IsActivityFinished())
		{
			if (m_DurationDoorBash.Expired())
			{
				TaskComplete();
				m_NextTimeToStartDoorBash.Reset();
			}
			else
				ResetIdealActivity(SelectDoorBash());
		}
		break;
	}

	case TASK_ZOMBIE_WAIT_POST_MELEE:
		{
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
Activity CNPC_BaseZombie::SelectDoorBash()
{
	return ACT_MELEE_ATTACK1;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::Event_Killed( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_VEHICLE )
	{
		Vector vecDamageDir = info.GetDamageForce();
		VectorNormalize( vecDamageDir );

		// Big blood splat
		UTIL_BloodSpray( WorldSpaceCenter(), vecDamageDir, BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_CLOUD );
	}

	CTakeDamageInfo pDamageCopy = info;

	CBaseEntity *pIgniter = m_hLastIgnitionSource.Get();
	if (IsOnFire() && pIgniter && (pDamageCopy.GetDamageType() & DMG_BURN))
		pDamageCopy.SetAttacker(pIgniter);

	HL2MPRules()->DeathNotice(this, pDamageCopy);

	BaseClass::Event_Killed(pDamageCopy);

	m_hLastIgnitionSource = NULL;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector )
{
	return BaseClass::BecomeRagdoll( info, forceVector );

	if( !(GetFlags()&FL_TRANSRAGDOLL) )
		RemoveDeferred();

	return true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StopLoopingSounds()
{
	ENVELOPE_CONTROLLER.SoundDestroy( m_pMoanSound );
	m_pMoanSound = NULL;

	BaseClass::StopLoopingSounds();
}

bool CNPC_BaseZombie::ShouldPlayFootstepMoan( void )
{
	if( random->RandomInt( 1, zombie_stepfreq.GetInt() * s_iAngryZombies ) == 1 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	switch( NewState )
	{
	case NPC_STATE_COMBAT:
		{
			RemoveSpawnFlags( SF_NPC_GAG );
			s_iAngryZombies++;
		}
		break;

	default:
		if( OldState == NPC_STATE_COMBAT )
		{
			// Only decrement if coming OUT of combat state.
			s_iAngryZombies--;
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Refines a base activity into something more specific to our internal state.
//-----------------------------------------------------------------------------
Activity CNPC_BaseZombie::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_WALK && IsCurSchedule( SCHED_COMBAT_PATROL, false) )
		baseAct = ACT_RUN;

	if ( IsOnFire() )
	{
		switch ( baseAct )
		{
		case ACT_RUN_ON_FIRE:
			{
				return ( Activity )ACT_WALK_ON_FIRE;
			}

		case ACT_WALK:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_WALK_ON_FIRE;
			}

		case ACT_IDLE:
			{
				// I'm on fire. Put ME out.
				return ( Activity )ACT_IDLE_ON_FIRE;
			}
		}
	}

	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_BaseZombie::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 	
	if( IsCurSchedule(SCHED_BIG_FLINCH) || m_ActBusyBehavior.IsActive() )
	{
		// This zombie is assumed to be standing up. 
		// Return a position that's centered over the absorigin,
		// halfway between the origin and the head. 
		Vector vecTarget = GetAbsOrigin();
		vecTarget.z = (vecTarget.z + ( CollisionProp()->OBBSize().z - 2.5f ) * 0.5f);
		return vecTarget;
	}

	return BaseClass::BodyTarget( posSrc, bNoisy );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//			&chasePosition - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition )
{
	// If our enemy is in a vehicle, we need them to tell us where to navigate to them
	if ( pEnemy == NULL )
		return;

	CBaseCombatCharacter *pBCC = pEnemy->MyCombatCharacterPointer();
	if ( pBCC && pBCC->IsInAVehicle() )
	{
		Vector vecForward, vecRight;
		pBCC->GetVectors( &vecForward, &vecRight, NULL );

		chasePosition = pBCC->WorldSpaceCenter() + ( vecForward * 24.0f ) + ( vecRight * 48.0f );
		return;
	}

	BaseClass::TranslateNavGoal( pEnemy, chasePosition );
}

bool CNPC_BaseZombie::OverrideShouldAddToLookList(CBaseEntity *pEntity)
{
	if (!pEntity->IsPlayer())
		return true;

	if ((pEntity->Classify() != CLASS_PLAYER) || !pEntity->IsAlive() || (pEntity->GetTeamNumber() != TEAM_HUMANS))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( base_zombie, CNPC_BaseZombie )

DECLARE_TASK( TASK_ZOMBIE_DIE )
DECLARE_TASK( TASK_ZOMBIE_WAIT_POST_MELEE )
DECLARE_TASK(TASK_ZOMBIE_YAW_TO_DOOR)
DECLARE_TASK(TASK_ZOMBIE_ATTACK_DOOR)
DECLARE_TASK(TASK_ZOMBIE_PATH_TO_OBSTRUCTION)

DECLARE_ACTIVITY( ACT_ZOM_FALL )

DECLARE_CONDITION( COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION )
DECLARE_CONDITION(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT)

//Adrian: events go here
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_RIGHT )
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_LEFT )
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_BOTH )
DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_LEFT )
DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_RIGHT )
DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_LEFT )
DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_RIGHT )
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_SCREAM )
DECLARE_ANIMEVENT( AE_ZOMBIE_GET_UP )
DECLARE_ANIMEVENT( AE_ZOMBIE_POUND )
DECLARE_ANIMEVENT( AE_ZOMBIE_ALERTSOUND )

DECLARE_INTERACTION( g_interactionZombieMeleeWarning )

//=========================================================
// AttackDoor
//=========================================================
DEFINE_SCHEDULE
(
SCHED_ZOMBIE_BASH_DOOR,

"	Tasks"
"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
"       TASK_ZOMBIE_PATH_TO_OBSTRUCTION 0"
"		TASK_WALK_PATH					0"
"		TASK_WAIT_FOR_MOVEMENT			0"
"		TASK_ZOMBIE_YAW_TO_DOOR			0"
"		TASK_FACE_IDEAL					0"
"		TASK_ZOMBIE_ATTACK_DOOR			0"
""
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_LIGHT_DAMAGE"
"		COND_HEAVY_DAMAGE"
)

//=========================================================
// ChaseEnemy
//=========================================================
DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_CHASE_ENEMY,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
	"		 TASK_SET_TOLERANCE_DISTANCE	24"
	"		 TASK_GET_CHASE_PATH_TO_ENEMY	900"
	"		 TASK_RUN_PATH					0"
	"		 TASK_WAIT_FOR_MOVEMENT			0"
	"		 TASK_FACE_ENEMY				0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_TASK_FAILED"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_MOVE_TO_AMBUSH,

	"	Tasks"
	"		TASK_WAIT						1.0" // don't react as soon as you see the player.
	"		TASK_FIND_COVER_FROM_ENEMY		0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_TURN_LEFT					180"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WAIT_AMBUSH"
	"	"
	"	Interrupts"
	"		COND_TASK_FAILED"
	"		COND_NEW_ENEMY"
	)


	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WAIT_AMBUSH,

	"	Tasks"
	"		TASK_WAIT_FACE_ENEMY	99999"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	)

	//=========================================================
	// Wander around for a while so we don't look stupid. 
	// this is done if we ever lose track of our enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_MEDIUM,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_WANDER						480384" // 4 feet to 32 feet
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM" // keep doing it
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_STANDOFF,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_WANDER						480384" // 4 feet to 32 feet
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	)

	//=========================================================
	// If you fail to wander, wait just a bit and try again.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_FAIL,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_WAIT				1"
	"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	)

	//=========================================================
	// Like the base class, only don't stop in the middle of 
	// swinging if the enemy is killed, hides, or new enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_MELEE_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_MELEE_ATTACK1		0"
	"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_POST_MELEE_WAIT"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// Make the zombie wait a frame after a melee attack, to
	// allow itself & it's enemy to test for dynamic scripted sequences.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_POST_MELEE_WAIT,

	"	Tasks"
	"		TASK_ZOMBIE_WAIT_POST_MELEE		0"
	)

	AI_END_CUSTOM_NPC()
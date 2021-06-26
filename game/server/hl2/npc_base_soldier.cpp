//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Soldier Class AI.
//
//==============================================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_route.h"
#include "ai_senses.h"
#include "ai_tacticalservices.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "npc_base_soldier.h"
#include "activitylist.h"
#include "player.h"
#include "basecombatweapon.h"
#include "basegrenade_shared.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "grenade_frag.h"
#include "ndebugoverlay.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "ammodef.h"
#include "hl2mp_gamerules.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SOLDIER_CHARGE_MIN_ATTACK_DIST_SQR (192.0f * 192.0f)

#define SOLDIER_GRENADE_THROW_SPEED 650
#define SOLDIER_GRENADE_TIMER		3.5
#define SOLDIER_GRENADE_FLUSH_TIME	3.0		// Don't try to flush an enemy who has been out of sight for longer than this.
#define SOLDIER_GRENADE_FLUSH_DIST	256.0	// Don't try to flush an enemy who has moved farther than this distance from the last place I saw him.
#define	SOLDIER_MIN_GRENADE_CLEAR_DIST	250

#define SOLDIER_EYE_STANDING_POSITION	Vector( 0, 0, 66 )
#define SOLDIER_GUN_STANDING_POSITION	Vector( 0, 0, 57 )
#define SOLDIER_EYE_CROUCHING_POSITION	Vector( 0, 0, 40 )
#define SOLDIER_GUN_CROUCHING_POSITION	Vector( 0, 0, 36 )
#define SOLDIER_SHOTGUN_STANDING_POSITION	Vector( 0, 0, 36 )
#define SOLDIER_SHOTGUN_CROUCHING_POSITION	Vector( 0, 0, 36 )

#define TAKE_COVER_DELAY 15.0f

//=========================================================
// Soldiers's Anim Events Go Here
//=========================================================
#define SOLDIER_AE_RELOAD			( 2 )
#define SOLDIER_AE_KICK				( 3 )
#define SOLDIER_AE_AIM				( 4 )
#define SOLDIER_AE_GREN_TOSS		( 7 )
#define SOLDIER_AE_GREN_DROP		( 9 )
#define SOLDIER_AE_CAUGHT_ENEMY		( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.

//=========================================================
// Soldier activities
//=========================================================
Activity ACT_COMBINE_THROW_GRENADE;

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{	
	SQUAD_SLOT_GRENADE1 = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_GRENADE2,
	SQUAD_SLOT_ATTACK_OCCLUDER,
};

enum TacticalVariant_T
{
	TACTICAL_VARIANT_DEFAULT = 0,
	TACTICAL_VARIANT_PRESSURE_ENEMY,				// Always try to close in on the player.
	TACTICAL_VARIANT_PRESSURE_ENEMY_UNTIL_CLOSE,	// Act like VARIANT_PRESSURE_ENEMY, but go to VARIANT_DEFAULT once within 30 feet
};

#define bits_MEMORY_PAIN_LIGHT_SOUND		bits_MEMORY_CUSTOM1
#define bits_MEMORY_PAIN_HEAVY_SOUND		bits_MEMORY_CUSTOM2
#define bits_MEMORY_PLAYER_HURT				bits_MEMORY_CUSTOM3

LINK_ENTITY_TO_CLASS( npc_soldier, CNPC_BaseSoldier );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_BaseSoldier)

DEFINE_KEYFIELD(m_iNumGrenades, FIELD_INTEGER, "NumGrenades"),

DEFINE_INPUTFUNC(FIELD_VOID, "LookOff", InputLookOff),
DEFINE_INPUTFUNC(FIELD_VOID, "LookOn", InputLookOn),

DEFINE_INPUTFUNC(FIELD_VOID, "StartPatrolling", InputStartPatrolling),
DEFINE_INPUTFUNC(FIELD_VOID, "StopPatrolling", InputStopPatrolling),

DEFINE_INPUTFUNC(FIELD_STRING, "Assault", InputAssault),

DEFINE_INPUTFUNC(FIELD_STRING, "ThrowGrenadeAtTarget", InputThrowGrenadeAtTarget),

DEFINE_KEYFIELD(m_iTacticalVariant, FIELD_INTEGER, "tacticalvariant"),
DEFINE_KEYFIELD(m_iPathfindingVariant, FIELD_INTEGER, "pathfindingvariant"),

END_DATADESC()

//------------------------------------------------------------------------------
// Constructor.
//------------------------------------------------------------------------------
CNPC_BaseSoldier::CNPC_BaseSoldier()
{
	m_vecTossVelocity = vec3_origin;
	m_flLastTimeRanForCover = 0.0f;
}

//------------------------------------------------------------------------------
// Purpose: Don't look, only get info from squad.
//------------------------------------------------------------------------------
void CNPC_BaseSoldier::InputLookOff( inputdata_t &inputdata )
{
	m_spawnflags |= SF_SOLDIER_NO_LOOK;
}

//------------------------------------------------------------------------------
// Purpose: Enable looking.
//------------------------------------------------------------------------------
void CNPC_BaseSoldier::InputLookOn( inputdata_t &inputdata )
{
	m_spawnflags &= ~SF_SOLDIER_NO_LOOK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::InputStartPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::InputStopPatrolling( inputdata_t &inputdata )
{
	m_bShouldPatrol = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::InputAssault( inputdata_t &inputdata )
{
}

//-----------------------------------------------------------------------------
// Purpose: Force the soldier soldier to throw a grenade at the target
//			If I'm a soldier elite, fire my soldier ball at the target instead.
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::InputThrowGrenadeAtTarget( inputdata_t &inputdata )
{
	// Ignore if we're inside a scripted sequence
	if ( m_NPCState == NPC_STATE_SCRIPT && m_hCine )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pEntity )
	{
		DevMsg("%s (%s) received ThrowGrenadeAtTarget input, but couldn't find target entity '%s'\n", GetClassname(), GetDebugName(), inputdata.value.String() );
		return;
	}

	m_hForcedGrenadeTarget = pEntity;
	m_flNextGrenadeCheck = 0;

	ClearSchedule( "Told to throw grenade via input" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::Precache()
{
	PrecacheModel("models/weapons/explosives/frag/w_frag_thrown.mdl");
	PrecacheScriptSound( "NPC_Combine.GrenadeLaunch" );
	PrecacheScriptSound( "NPC_Combine.WeaponBash" );
	PrecacheScriptSound( "BaseEnemyHumanoid.WeaponBash" );
	PrecacheScriptSound( "Weapon_CombineGuard.Special1" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::Activate()
{
	BaseClass::Activate();
}

void CNPC_BaseSoldier::FireBullets(const FireBulletsInfo_t &info)
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	FireBulletsInfo_t modinfo = info;

	float flDamage = 1.0f;
	if (GameBaseShared()->GetNPCData() && pWeapon)
	{
		flDamage = GameBaseShared()->GetNPCData()->GetFirearmDamage(GetNPCName(), pWeapon->GetClassname());
		modinfo.m_vecFirstStartPos = GetLocalOrigin();
		modinfo.m_flDropOffDist = pWeapon->GetWpnData().m_flDropOffDistance;
	}

	flDamage += ((flDamage / 100.0f) * m_flDamageScaleValue);

	modinfo.m_flDamage = flDamage;
	modinfo.m_iPlayerDamage = (int)flDamage;

	BaseClass::FireBullets(modinfo);
}

int CNPC_BaseSoldier::AllowEntityToBeGibbed(void)
{
	if (m_iHealth > 0)
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::Spawn( void )
{
	if (ParseNPC(this))
	{
		SetHealth(m_iTotalHP);
		SetMaxHealth(m_iTotalHP);
		SetKickDamage(m_iDamageKick);
		SetNPCName(GetNPCName());
		SetBoss(IsBoss());

		PrecacheModel(GetNPCModelName());
		SetModel(GetNPCModelName());

		m_nSkin = m_iModelSkin;
		UpdateNPCScaling();
	}
	else
	{
		UTIL_Remove(this);
		return;
	}

	// If Pure Classic is enabled we will not spawn non Military npcs...
	if (GameBaseServer()->IsClassicMode() && !FClassnameIs(this, "npc_military"))
	{
		UTIL_Remove(this);
		return;
	}

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_flFieldOfView			= -0.2;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;
	m_flNextGrenadeCheck	= gpGlobals->curtime + 1;
	m_flNextPainSoundTime	= 0;
	m_flNextAlertSoundTime	= 0;
	m_bShouldPatrol			= false;

	//	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB);
	// JAY: Disabled jump for now - hard to compare to HL1
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND );

	CapabilitiesAdd( bits_CAP_AIM_GUN );

	// Innate range attack for grenade
	// CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK2 );

	// Innate range attack for kicking
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1 );

	// Can be in a squad
	CapabilitiesAdd( bits_CAP_SQUAD);
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );

	// CapabilitiesAdd( bits_CAP_DUCK );				// In reloading and cover <- BB2 npcs have no crouch anims!!
	CapabilitiesAdd( bits_CAP_USE );
	CapabilitiesAdd(bits_CAP_MOVE_JUMP);

	CapabilitiesAdd( bits_CAP_NO_HIT_SQUADMATES );

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_flStopMoveShootTime = FLT_MAX; // Move and shoot defaults on.
	m_MoveAndShootOverlay.SetInitialDelay( 0.75 ); // But with a bit of a delay.

	m_flNextLostSoundTime		= 0;
	m_flAlertPatrolTime			= 0;

	NPCInit();

	if (!IsBoss())
	{
		m_flDistTooFar = 500.0f;
		GetSenses()->SetDistLook(650.0f);
	}

	OnSetGibHealth();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::CreateBehaviors()
{
	AddBehavior( &m_RappelBehavior );
	AddBehavior( &m_FuncTankBehavior );

	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::PostNPCInit()
{
	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::GatherConditions()
{
	BaseClass::GatherConditions();

	ClearCondition( COND_SOLDIER_ATTACK_SLOT_AVAILABLE );

	if( GetState() == NPC_STATE_COMBAT )
	{
		if( IsCurSchedule( SCHED_SOLDIER_WAIT_IN_COVER, false ) )
		{
			// Soldiers that are standing around doing nothing poll for attack slots so
			// that they can respond quickly when one comes available. If they can 
			// occupy a vacant attack slot, they do so. This holds the slot until their
			// schedule breaks and schedule selection runs again, essentially reserving this
			// slot. If they do not select an attack schedule, then they'll release the slot.
			if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				SetCondition( COND_SOLDIER_ATTACK_SLOT_AVAILABLE );
			}
		}

		if( IsUsingTacticalVariant(TACTICAL_VARIANT_PRESSURE_ENEMY_UNTIL_CLOSE) )
		{
			if( GetEnemy() != NULL && !HasCondition(COND_ENEMY_OCCLUDED) )
			{
				// Now we're close to our enemy, stop using the tactical variant.
				if( GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin()) < Square(30.0f * 12.0f) )
					m_iTacticalVariant = TACTICAL_VARIANT_DEFAULT;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	extern ConVar ai_debug_shoot_positions;
	if ( ai_debug_shoot_positions.GetBool() )
		NDebugOverlay::Cross3D( EyePosition(), 16, 0, 255, 0, false, 0.1 );

	if( gpGlobals->curtime >= m_flStopMoveShootTime )
	{
		// Time to stop move and shoot and start facing the way I'm running.
		// This makes the soldier look attentive when disengaging, but prevents
		// them from always running around facing you.
		//
		// Only do this if it won't be immediately shut off again.
		if( GetNavigator()->GetPathTimeToGoal() > 1.0f )
		{
			m_MoveAndShootOverlay.SuspendMoveAndShoot( 5.0f );
			m_flStopMoveShootTime = FLT_MAX;
		}
	}

	if( m_flGroundSpeed > 0 && GetState() == NPC_STATE_COMBAT && m_MoveAndShootOverlay.IsSuspended() )
	{
		// Return to move and shoot when near my goal so that I 'tuck into' the location facing my enemy.
		if( GetNavigator()->GetPathTimeToGoal() <= 1.0f )
		{
			m_MoveAndShootOverlay.SuspendMoveAndShoot( 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: degrees to turn in 0.1 seconds
//-----------------------------------------------------------------------------
float CNPC_BaseSoldier::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 45;
		break;
	case ACT_RUN:
		return 15;
		break;
	case ACT_WALK:
	case ACT_WALK_CROUCH:
		return 25;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		return 35;
	default:
		return 35;
		break;
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::ShouldMoveAndShoot()
{
	// Set this timer so that gpGlobals->curtime can't catch up to it. 
	// Essentially, we're saying that we're not going to interfere with 
	// what the AI wants to do with move and shoot. 
	//
	// If any code below changes this timer, the code is saying 
	// "It's OK to move and shoot until gpGlobals->curtime == m_flStopMoveShootTime"
	m_flStopMoveShootTime = FLT_MAX;

	if( IsCurSchedule( SCHED_SOLDIER_HIDE_AND_RELOAD, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	if( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND, false ) )
		return false;

	if( IsCurSchedule( SCHED_SOLDIER_TAKE_COVER_FROM_BEST_SOUND, false ) )
		return false;

	if( IsCurSchedule( SCHED_SOLDIER_RUN_AWAY_FROM_BEST_SOUND, false ) )
		return false;

	if( HasCondition( COND_NO_PRIMARY_AMMO, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	if( m_pSquad && IsCurSchedule( SCHED_SOLDIER_TAKE_COVER1, false ) )
		m_flStopMoveShootTime = gpGlobals->curtime + random->RandomFloat( 0.4f, 0.6f );

	return BaseClass::ShouldMoveAndShoot();
}

//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseSoldier::Classify ( void )
{
	return CLASS_COMBINE;
}


//-----------------------------------------------------------------------------
// Continuous movement tasks
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::IsCurTaskContinuousMove()
{
	const Task_t* pTask = GetTask();
	if ( pTask && (pTask->iTask == TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY) )
		return true;

	return BaseClass::IsCurTaskContinuousMove();
}


//-----------------------------------------------------------------------------
// Chase the enemy, updating the target position as the player moves
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::StartTaskChaseEnemyContinuously( const Task_t *pTask )
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
	{
		TaskFail( FAIL_NO_ENEMY );
		return;
	}

	// We're done once we get close enough
	if ( WorldSpaceCenter().DistToSqr( pEnemy->WorldSpaceCenter() ) <= pTask->flTaskData * pTask->flTaskData )
	{
		TaskComplete();
		return;
	}

	// TASK_GET_PATH_TO_ENEMY
	if ( IsUnreachable( pEnemy ) )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	if ( !GetNavigator()->SetGoal( GOALTYPE_ENEMY, AIN_NO_PATH_TASK_FAIL ) )
	{
		// no way to get there =( 
		DevWarning( 2, "GetPathToEnemy failed!!\n" );
		RememberUnreachable( pEnemy );
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	// NOTE: This is TaskRunPath here.
	if ( TranslateActivity( ACT_RUN ) != ACT_INVALID )
	{
		GetNavigator()->SetMovementActivity( ACT_RUN );
	}
	else
	{
		GetNavigator()->SetMovementActivity(ACT_WALK);
	}

	// Cover is void once I move
	Forget( bits_MEMORY_INCOVER );

	if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
	{
		TaskComplete();
		GetNavigator()->ClearGoal();		// Clear residual state
		return;
	}

	// No shooting delay when in this mode
	m_MoveAndShootOverlay.SetInitialDelay( 0.0 );

	if (!GetNavigator()->IsGoalActive())
	{
		SetIdealActivity( GetStoppedActivity() );
	}
	else
	{
		// Check validity of goal type
		ValidateNavGoal();
	}

	// set that we're probably going to stop before the goal
	GetNavigator()->SetArrivalDistance( pTask->flTaskData );
	m_vSavePosition = GetEnemy()->WorldSpaceCenter();
}

void CNPC_BaseSoldier::RunTaskChaseEnemyContinuously( const Task_t *pTask )
{
	if (!GetNavigator()->IsGoalActive())
	{
		SetIdealActivity( GetStoppedActivity() );
	}
	else
	{
		// Check validity of goal type
		ValidateNavGoal();
	}

	CBaseEntity *pEnemy = GetEnemy();
	if ( !pEnemy )
	{
		TaskFail( FAIL_NO_ENEMY );
		return;
	}

	// We're done once we get close enough
	if ( WorldSpaceCenter().DistToSqr( pEnemy->WorldSpaceCenter() ) <= pTask->flTaskData * pTask->flTaskData )
	{
		GetNavigator()->StopMoving();
		TaskComplete();
		return;
	}

	// Recompute path if the enemy has moved too much
	if ( m_vSavePosition.DistToSqr( pEnemy->WorldSpaceCenter() ) < (pTask->flTaskData * pTask->flTaskData) )
		return;

	if ( IsUnreachable( pEnemy ) )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	if ( !GetNavigator()->RefindPathToGoal() )
	{
		TaskFail(FAIL_NO_ROUTE);
		return;
	}

	m_vSavePosition = pEnemy->WorldSpaceCenter();
}


//=========================================================
// start task
//=========================================================
void CNPC_BaseSoldier::StartTask( const Task_t *pTask )
{
	// NOTE: This reset is required because we change it in TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY
	m_MoveAndShootOverlay.SetInitialDelay( 0.75 );

	switch ( pTask->iTask )
	{
	case TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY:
		StartTaskChaseEnemyContinuously( pTask );
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			// If Primary Attack
			if ((int)pTask->flTaskData == 1)
			{
				// -----------------------------------------------------------
				// If enemy isn't facing me and I haven't attacked in a while
				// annouce my attack before I start wailing away
				// -----------------------------------------------------------
				CBaseCombatCharacter *pBCC = GetEnemyCombatCharacterPointer();

				if	(pBCC && pBCC->IsPlayer() && (!pBCC->FInViewCone ( this )) &&
					((gpGlobals->curtime - m_flLastAttackTime) > 3.0f) )
				{
					m_flLastAttackTime = gpGlobals->curtime;

					if (random->RandomInt(1, 2) == 1)
						HL2MPRules()->EmitSoundToClient(this, "Incoming", GetNPCType(), GetGender());
					else
						HL2MPRules()->EmitSoundToClient(this, "Contact", GetNPCType(), GetGender());

					// Wait two seconds
					SetWait( 2.0 );
					SetActivity(ACT_IDLE);
				}
				// -------------------------------------------------------------
				//  Otherwise move on
				// -------------------------------------------------------------
				else
				{
					TaskComplete();
				}
			}
			else
			{
				HL2MPRules()->EmitSoundToClient(this, "Grenade", GetNPCType(), GetGender()); // throw frag
				SetActivity(ACT_IDLE);
				// Wait two seconds
				SetWait( 2.0 );
			}
			break;
		}	

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		BaseClass::StartTask( pTask );
		break;

	case TASK_SOLDIER_FACE_TOSS_DIR:
		break;

	case TASK_SOLDIER_GET_PATH_TO_FORCED_GREN_LOS:
		{
			if ( !m_hForcedGrenadeTarget )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			float flMaxRange = 2000;
			float flMinRange = 0;

			Vector vecEnemy = m_hForcedGrenadeTarget->GetAbsOrigin();
			Vector vecEnemyEye = vecEnemy + m_hForcedGrenadeTarget->GetViewOffset();

			Vector posLos;
			bool found = false;

			if ( GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
			{
				float dist = ( posLos - vecEnemyEye ).Length();
				if ( dist < flMaxRange && dist > flMinRange )
					found = true;
			}

			if ( !found && GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, &posLos ) )
			{
				found = true;
			}

			if ( !found )
			{
				TaskFail( FAIL_NO_SHOOT );
			}
			else
			{
				// else drop into run task to offer an interrupt
				m_vInterruptSavePosition = posLos;
			}
		}
		break;

	case TASK_SOLDIER_IGNORE_ATTACKS:
		// must be in a squad
		if (m_pSquad && m_pSquad->NumMembers() > 2)
		{
			// the enemy must be far enough away
			if (GetEnemy() && (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).Length() > 512.0 )
			{
				m_flNextAttack	= gpGlobals->curtime + pTask->flTaskData;
			}
		}
		TaskComplete( );
		break;

	case TASK_SOLDIER_DEFER_SQUAD_GRENADES:
		{
			if ( m_pSquad )
			{
				// iterate my squad and stop everyone from throwing grenades for a little while.
				AISquadIter_t iter;

				CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember( &iter ) : NULL;
				while ( pSquadmate )
				{
					CNPC_BaseSoldier *pSoldier = dynamic_cast<CNPC_BaseSoldier*>(pSquadmate);

					if( pSoldier )
					{
						pSoldier->m_flNextGrenadeCheck = gpGlobals->curtime + 5;
					}

					pSquadmate = m_pSquad->GetNextMember( &iter );
				}
			}

			TaskComplete();
			break;
		}

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		{
			if( pTask->iTask == TASK_FACE_ENEMY && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			{
				TaskComplete();
				return;
			}

			BaseClass::StartTask( pTask );
			bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
			if (bIsFlying)
			{
				SetIdealActivity( ACT_GLIDE );
			}

		}
		break;

	case TASK_FIND_COVER_FROM_ENEMY:
		{
			CBaseEntity *pEntity = GetEnemy();
			// FIXME: this should be generalized by the schedules that are selected, or in the definition of 
			// what "cover" means (i.e., trace attack vulnerability vs. physical attack vulnerability
			if (pEntity)
			{
				// NOTE: This is a good time to check to see if the player is hurt.
				// Have the soldier notice this and call out
				if (!HasMemory(bits_MEMORY_PLAYER_HURT) && pEntity->IsPlayer() && pEntity->GetHealth() <= 20)
				{
					if (m_pSquad)
					{
						m_pSquad->SquadRemember(bits_MEMORY_PLAYER_HURT);
					}

					HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());
					JustMadeSound(SENTENCE_PRIORITY_HIGH);
				}
				if (pEntity->MyNPCPointer())
				{
					if (!(pEntity->MyNPCPointer()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1) &&
						!(pEntity->MyNPCPointer()->CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1))
					{
						TaskComplete();
						return;
					}
				}
			}
			BaseClass::StartTask( pTask );
		}
		break;
	case TASK_RANGE_ATTACK1:
	{
		if (GetActiveWeapon())
		{
			m_nShots = (IsWeaponShotgun() ? 1 : GetActiveWeapon()->GetRandomBurst());
			m_flShotDelay = GetActiveWeapon()->GetFireRate();
		}

		m_flNextAttack = (gpGlobals->curtime + m_flShotDelay);
		ResetIdealActivity(ACT_RANGE_ATTACK1);
		m_flLastAttackTime = gpGlobals->curtime;
	}
	break;

	default: 
		BaseClass:: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_BaseSoldier::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY:
		RunTaskChaseEnemyContinuously( pTask );
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			// Stop waiting if enemy facing me or lost enemy
			CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();
			if	(!pBCC || pBCC->FInViewCone( this ))
			{
				TaskComplete();
			}

			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_SOLDIER_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			GetMotor()->SetIdealYawToTargetAndUpdate( GetLocalOrigin() + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED );

			if ( FacingIdeal() )
			{
				TaskComplete( true );
			}
			break;
		}

	case TASK_SOLDIER_GET_PATH_TO_FORCED_GREN_LOS:
		{
			if ( !m_hForcedGrenadeTarget )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			if ( GetTaskInterrupt() > 0 )
			{
				ClearTaskInterrupt();

				Vector vecEnemy = m_hForcedGrenadeTarget->GetAbsOrigin();
				AI_NavGoal_t goal( m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE );

				GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
				GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
			}
			else
			{
				TaskInterrupt();
			}
		}
		break;

	case TASK_RANGE_ATTACK1:
		{
			AutoMovement( );

			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
			}
			else
			{
				GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
			}

			if ( gpGlobals->curtime >= m_flNextAttack )
			{
				bool bIsUsingShotgun = IsWeaponShotgun();
				if (!bIsUsingShotgun || (bIsUsingShotgun && IsActivityFinished())) // If the NPC is using a shotgun, wait for the fire anim to finish!
				{
					if (--m_nShots > 0)
					{
						// DevMsg("ACT_RANGE_ATTACK1\n");
						ResetIdealActivity(ACT_RANGE_ATTACK1);
						m_flLastAttackTime = gpGlobals->curtime;
						m_flNextAttack = (gpGlobals->curtime + m_flShotDelay);
					}
					else
					{
						// DevMsg("TASK_RANGE_ATTACK1 complete\n");
						TaskComplete();
					}
				}
			}
			else
			{
				// DevMsg("Wait\n");
			}
		}
		break;

	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Override to always shoot at eyes (for ducking behind things)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_BaseSoldier::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	Vector result = BaseClass::BodyTarget( posSrc, bNoisy );

	// @TODO (toml 02-02-04): this seems wrong. Isn't this already be accounted for 
	// with the eye position used in the base BodyTarget()
	if ( GetFlags() & FL_DUCKING )
		result -= Vector(0,0,24);

	return result;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_BaseSoldier::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	if( m_spawnflags & SF_SOLDIER_NO_LOOK )
	{
		// When no look is set, if enemy has eluded the squad, 
		// he's always invisble to me
		if (GetEnemies()->HasEludedMe(pEntity))
		{
			return false;
		}
	}
	return BaseClass::FVisible(pEntity, traceMask, ppBlocker);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::Event_Killed( const CTakeDamageInfo &info )
{
	HL2MPRules()->DeathNotice(this, info);

	// if I was killed before I could finish throwing my grenade, drop
	// a grenade item that the player can retrieve.
	if ((GetActivity() == ACT_RANGE_ATTACK2) && !HasSpawnFlags(SF_NPC_NO_WEAPON_DROP))
	{
		if( m_iLastAnimEventHandled != SOLDIER_AE_GREN_TOSS )
		{
			// Drop the grenade as an item.
			Vector vecStart;
			GetAttachment( "lefthand", vecStart );

			CBaseEntity *pItem = DropItem( "weapon_frag", vecStart, RandomAngle(0,360) );
			if ( pItem )
			{
				pItem->AddSpawnFlags(SF_NORESPAWN);
				IPhysicsObject *pObj = pItem->VPhysicsGetObject();
				if ( pObj )
				{
					Vector			vel;
					vel.x = random->RandomFloat( -100.0f, 100.0f );
					vel.y = random->RandomFloat( -100.0f, 100.0f );
					vel.z = random->RandomFloat( 800.0f, 1200.0f );
					AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

					vel[2] = 0.0f;
					pObj->AddVelocity( &vel, &angImp );
				}
			}
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Override.  Don't update if I'm not looking
// Input  :
// Output : Returns true is new enemy, false is known enemy
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer )
{
	if( m_spawnflags & SF_SOLDIER_NO_LOOK )
	{
		return false;
	}

	return BaseClass::UpdateEnemyMemory( pEnemy, position, pInformer );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if (gpGlobals->curtime < m_flNextAttack)
	{
		ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK1 );
		ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Translate base class activities into combot activites
//-----------------------------------------------------------------------------
Activity CNPC_BaseSoldier::NPC_TranslateActivity(Activity eNewActivity)
{
	if (ai_show_active_military_activities.GetBool())
		Msg("Running Activity %i Act Name: %s\n", eNewActivity, GetActivityName(eNewActivity));

	if (eNewActivity == ACT_RAPPEL_LOOP || eNewActivity == ACT_COWER || eNewActivity == ACT_COVER)
		return ACT_IDLE;

	switch (eNewActivity)
	{
	case ACT_WALK_CROUCH_RIFLE:
		return ACT_WALK_RIFLE;

	case ACT_WALK_CROUCH_AIM_RIFLE:
		return ACT_WALK_AIM_RIFLE;

	case ACT_RUN_CROUCH_RIFLE:
		return ACT_RUN_RIFLE;

	case ACT_RUN_CROUCH_AIM_RIFLE:
		return ACT_RUN_AIM_RIFLE;

	case ACT_IDLE_ANGRY_PISTOL:
		return ACT_IDLE_PISTOL;

	case ACT_RANGE_ATTACK2:
		return (Activity)ACT_COMBINE_THROW_GRENADE;
	}

	if (eNewActivity == ACT_IDLE)
	{
		if (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT)
			eNewActivity = ACT_IDLE_ANGRY;
	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}


//-----------------------------------------------------------------------------
// Purpose: Overidden for human grunts because they  hear the DANGER sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseSoldier::GetSoundInterests( void )
{
	return	SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER | SOUND_PHYSICS_DANGER | SOUND_BULLET_IMPACT | SOUND_MOVE_AWAY;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this NPC can hear the specified sound
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::QueryHearSound( CSound *pSound )
{
	if ( pSound->SoundContext() & SOUND_CONTEXT_COMBINE_ONLY )
		return true;

	if ( pSound->SoundContext() & SOUND_CONTEXT_EXCLUDE_COMBINE )
		return false;

	return BaseClass::QueryHearSound( pSound );
}


//-----------------------------------------------------------------------------
// Purpose: Announce an assault if the enemy can see me and we are pretty 
//			close to him/her
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::AnnounceAssault(void)
{
	if (random->RandomInt(0,5) > 1)
		return;

	// If enemy can see me make assualt sound
	CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();

	if (!pBCC)
		return;

	if (!FOkToMakeSound())
		return;

	// Make sure we are pretty close
	if ( WorldSpaceCenter().DistToSqr( pBCC->WorldSpaceCenter() ) > (2000 * 2000))
		return;

	// Make sure we are in view cone of player
	if (!pBCC->FInViewCone ( this ))
		return;

	// Make sure player can see me
	if ( FVisible( pBCC ) )
	{
		// Speak assault ?
	}
}


void CNPC_BaseSoldier::AnnounceEnemyType( CBaseEntity *pEnemy )
{
	HL2MPRules()->EmitSoundToClient(this, "Contact", GetNPCType(), GetGender());
}

void CNPC_BaseSoldier::AnnounceEnemyKill( CBaseEntity *pEnemy )
{
	if (!pEnemy )
		return;

	HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
// Select the combat schedule
//-----------------------------------------------------------------------------
int CNPC_BaseSoldier::SelectCombatSchedule()
{
	// -----------
	// dead enemy
	// -----------
	if ( HasCondition( COND_ENEMY_DEAD ) )
	{
		// call base class, all code to handle dead enemies is centralized there.
		return SCHED_NONE;
	}

	// -----------
	// new enemy
	// -----------
	if ( HasCondition( COND_NEW_ENEMY ) )
	{
		CBaseEntity *pEnemy = GetEnemy();
		bool bFirstContact = false;
		float flTimeSinceFirstSeen = gpGlobals->curtime - GetEnemies()->FirstTimeSeen( pEnemy );

		if( flTimeSinceFirstSeen < 3.0f )
			bFirstContact = true;

		if ( m_pSquad && pEnemy )
		{
			if ( HasCondition( COND_SEE_ENEMY ) )
			{
				AnnounceEnemyType( pEnemy );
			}

			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlot( SQUAD_SLOT_ATTACK1 ) )
			{
				// Start suppressing if someone isn't firing already (SLOT_ATTACK1). This means
				// I'm the guy who spotted the enemy, I should react immediately.
				return SCHED_SOLDIER_SUPPRESS;
			}

			if ( m_pSquad->IsLeader( this ) || ( m_pSquad->GetLeader() && m_pSquad->GetLeader()->GetEnemy() != pEnemy ) )
			{
				// I'm the leader, but I didn't get the job suppressing the enemy. We know this because
				// This code only runs if the code above didn't assign me SCHED_SOLDIER_SUPPRESS.
				if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
			else
			{
				if ( m_pSquad->GetLeader() && FOkToMakeSound( SENTENCE_PRIORITY_MEDIUM ) )
				{
					JustMadeSound( SENTENCE_PRIORITY_MEDIUM );	// squelch anything that isn't high priority so the leader can speak
				}

				// First contact, and I'm solo, or not the squad leader.
				if( HasCondition( COND_SEE_ENEMY ) && CanGrenadeEnemy() )
				{
					if( OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
					{
						return SCHED_RANGE_ATTACK2;
					}
				}

				if( !bFirstContact && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					if( random->RandomInt(0, 100) < 60 )
					{
						return SCHED_ESTABLISH_LINE_OF_FIRE;
					}
					else
					{
						return SCHED_SOLDIER_PRESS_ATTACK;
					}
				}

				// We're tried of running, even if our squad says NO, we will still attack!
				if ((gpGlobals->curtime - m_flLastTimeRanForCover) < TAKE_COVER_DELAY)
				{
					if (HasCondition(COND_CAN_RANGE_ATTACK1))
						return SCHED_SOLDIER_SUPPRESS;
					else if (HasCondition(COND_CAN_MELEE_ATTACK1))
						return SCHED_MELEE_ATTACK1;
					else
						return SCHED_SOLDIER_PRESS_ATTACK;
				}
					
				return SCHED_TAKE_COVER_FROM_ENEMY;
			}
		}
	}

	// ---------------------
	// no ammo
	// ---------------------
	if ( ( HasCondition ( COND_NO_PRIMARY_AMMO ) || HasCondition ( COND_LOW_PRIMARY_AMMO ) ) && !HasCondition( COND_CAN_MELEE_ATTACK1) )
	{
		return SCHED_HIDE_AND_RELOAD;
	}

	// ----------------------
	// LIGHT DAMAGE
	// ----------------------
	if ( HasCondition( COND_LIGHT_DAMAGE ) && ((gpGlobals->curtime - m_flLastTimeRanForCover) > TAKE_COVER_DELAY) )
	{
		if ( GetEnemy() != NULL )
		{
			// only try to take cover if we actually have an enemy!
			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
		else
		{
			// How am I wounded in combat with no enemy?
			Assert( GetEnemy() != NULL );
		}
	}

	// If I'm scared of this enemy run away
	if ( IRelationType( GetEnemy() ) == D_FR )
	{
		if (HasCondition( COND_SEE_ENEMY )	|| 
			HasCondition( COND_SEE_FEAR )	|| 
			HasCondition( COND_LIGHT_DAMAGE ) || 
			HasCondition( COND_HEAVY_DAMAGE ))
		{
			FearSound();
			//ClearCommandGoal();
			return SCHED_RUN_FROM_ENEMY;
		}

		// If I've seen the enemy recently, cower. Ignore the time for unforgettable enemies.
		AI_EnemyInfo_t *pMemory = GetEnemies()->Find( GetEnemy() );
		if ( (pMemory && pMemory->bUnforgettable) || (GetEnemyLastTimeSeen() > (gpGlobals->curtime - 5.0)) )
		{
			// If we're facing him, just look ready. Otherwise, face him.
			if ( FInAimCone( GetEnemy()->EyePosition() ) )
				return SCHED_COMBAT_STAND;

			return SCHED_FEAR_FACE;
		}
	}

	int attackSchedule = SelectScheduleAttack();
	if ( attackSchedule != SCHED_NONE )
		return attackSchedule;

	if (HasCondition(COND_ENEMY_OCCLUDED))
	{
		// stand up, just in case
		Stand();
		DesireStand();

		if( GetEnemy() && !(GetEnemy()->GetFlags() & FL_NOTARGET) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		{
			// Charge in and break the enemy's cover!
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}

		// If I'm a long, long way away, establish a LOF anyway. Once I get there I'll
		// start respecting the squad slots again.
		float flDistSq = GetEnemy()->WorldSpaceCenter().DistToSqr( WorldSpaceCenter() );
		if ( flDistSq > Square(3000) )
			return SCHED_ESTABLISH_LINE_OF_FIRE;

		// Otherwise tuck in.
		Remember( bits_MEMORY_INCOVER );
		return SCHED_SOLDIER_WAIT_IN_COVER;
	}

	// --------------------------------------------------------------
	// Enemy not occluded but isn't open to attack
	// --------------------------------------------------------------
	if ( HasCondition( COND_SEE_ENEMY ) && !HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		if ( (HasCondition( COND_TOO_FAR_TO_ATTACK ) || IsUsingTacticalVariant(TACTICAL_VARIANT_PRESSURE_ENEMY) ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ))
		{
			return SCHED_SOLDIER_PRESS_ATTACK;
		}

		AnnounceAssault(); 
		return SCHED_SOLDIER_ASSAULT;
	}

	return SCHED_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseSoldier::SelectSchedule( void )
{
	if ( IsWaitingToRappel() && BehaviorSelectSchedule() )
	{
		return BaseClass::SelectSchedule();
	}

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( m_hForcedGrenadeTarget )
	{
		if ( m_flNextGrenadeCheck < gpGlobals->curtime )
		{
			Vector vecTarget = m_hForcedGrenadeTarget->WorldSpaceCenter();

			// If we can, throw a grenade at the target. 
			// Ignore grenade count / distance / etc
			if ( CheckCanThrowGrenade( vecTarget ) )
			{
				m_hForcedGrenadeTarget = NULL;
				return SCHED_SOLDIER_FORCED_GRENADE_THROW;
			}
		}

		// Can't throw at the target, so lets try moving to somewhere where I can see it
		if ( !FVisible( m_hForcedGrenadeTarget ) )
		{
			return SCHED_SOLDIER_MOVE_TO_FORCED_GREN_LOS;
		}
	}

	if ( m_NPCState != NPC_STATE_SCRIPT)
	{
		// We've been told to move away from a target to make room for a grenade to be thrown at it
		if ( HasCondition( COND_HEAR_MOVE_AWAY ) && ((gpGlobals->curtime - m_flLastTimeRanForCover) > TAKE_COVER_DELAY) )
		{
			return SCHED_MOVE_AWAY;
		}

		// These things are done in any state but dead and prone
		if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE )
		{
			// Cower when physics objects are thrown at me
			if ( HasCondition( COND_HEAR_PHYSICS_DANGER ) && CanFlinch() )
			{
				return SCHED_FLINCH_PHYSICS;
			}

			// grunts place HIGH priority on running away from danger sounds.
			if (HasCondition(COND_HEAR_DANGER) && ((gpGlobals->curtime - m_flLastTimeRanForCover) > TAKE_COVER_DELAY))
			{
				CSound *pSound;
				pSound = GetBestSound();

				Assert( pSound != NULL );
				if ( pSound)
				{
					if (pSound->m_iType & SOUND_DANGER)
					{
						// I hear something dangerous, probably need to take cover.
						// dangerous sound nearby!, call it out.

						HL2MPRules()->EmitSoundToClient(this, "Incoming", GetNPCType(), GetGender());

						// If the sound is approaching danger, I have no enemy, and I don't see it, turn to face.
						if( !GetEnemy() && pSound->IsSoundType(SOUND_CONTEXT_DANGER_APPROACH) && pSound->m_hOwner && !FInViewCone(pSound->GetSoundReactOrigin()) )
						{
							GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
							return SCHED_SOLDIER_FACE_IDEAL_YAW;
						}

						return SCHED_TAKE_COVER_FROM_BEST_SOUND;
					}

					// JAY: This was disabled in HL1.  Test?
					if (!HasCondition( COND_SEE_ENEMY ) && ( pSound->m_iType & (SOUND_PLAYER | SOUND_COMBAT) ))
					{
						GetMotor()->SetIdealYawToTarget( pSound->GetSoundReactOrigin() );
					}
				}
			}
		}

		if( BehaviorSelectSchedule() )
		{
			return BaseClass::SelectSchedule();
		}
	}

	switch	( m_NPCState )
	{
	case NPC_STATE_IDLE:
		{
			if ( m_bShouldPatrol )
				return SCHED_SOLDIER_PATROL;
		}
		// NOTE: Fall through!

	case NPC_STATE_ALERT:
		{
			if ((HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE)))
			{
				AI_EnemyInfo_t *pDanger = GetEnemies()->GetDangerMemory();
				if( pDanger && FInViewCone(pDanger->vLastKnownLocation) && !BaseClass::FVisible(pDanger->vLastKnownLocation) )
				{
					// I've been hurt, I'm facing the danger, but I don't see it, so move from this position.
					return SCHED_TAKE_COVER_FROM_ORIGIN;
				}
			}

			if( HasCondition( COND_HEAR_COMBAT ) )
			{
				CSound *pSound = GetBestSound();

				if( pSound && pSound->IsSoundType( SOUND_COMBAT ) )
				{
					if( m_pSquad && m_pSquad->GetSquadMemberNearestTo( pSound->GetSoundReactOrigin() ) == this && OccupyStrategySlot( SQUAD_SLOT_INVESTIGATE_SOUND ) )
					{
						return SCHED_INVESTIGATE_SOUND;
					}
				}
			}

			if (m_bShouldPatrol || HasCondition(COND_SOLDIER_SHOULD_PATROL))
				return SCHED_SOLDIER_PATROL;
		}
		break;

	case NPC_STATE_COMBAT:
		{
			int nSched = SelectCombatSchedule();
			if ( nSched != SCHED_NONE )
				return nSched;
		}
		break;
	}

	// no special cases here, call the base class
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_BaseSoldier::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_SOLDIER_TAKE_COVER1 )
	{
		m_flLastTimeRanForCover = gpGlobals->curtime;
		if( IsInSquad() && IsStrategySlotRangeOccupied(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2) && HasCondition(COND_SEE_ENEMY) )
		{
			// This eases the effects of an unfortunate bug that usually plagues shotgunners. Since their rate of fire is low,
			// they spend relatively long periods of time without an attack squad slot. If you corner a shotgunner, usually 
			// the other memebers of the squad will hog all of the attack slots and pick schedules to move to establish line of
			// fire. During this time, the shotgunner is prevented from attacking. If he also cannot find cover (the fallback case)
			// he will stand around like an idiot, right in front of you. Instead of this, we have him run up to you for a melee attack.
			return SCHED_SOLDIER_MOVE_TO_MELEE;
		}

		if (GetEnemy() != NULL)
		{
			int sched = SelectCombatSchedule();
			if (sched != SCHED_NONE)
				return sched;
		}
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Should we charge the player?
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::ShouldChargePlayer()
{
	return false;
}


//-----------------------------------------------------------------------------
// Select attack schedules
//-----------------------------------------------------------------------------

int CNPC_BaseSoldier::SelectScheduleAttack()
{
	// Kick attack?
	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		return SCHED_MELEE_ATTACK1;
	}

	// When fighting against the player who's wielding a mega-physcannon, 
	// always close the distance if possible
	// But don't do it if you're in a nav-limited hint group
	if ( ShouldChargePlayer() )
	{
		float flDistSq = GetEnemy()->WorldSpaceCenter().DistToSqr( WorldSpaceCenter() );
		if (flDistSq <= SOLDIER_CHARGE_MIN_ATTACK_DIST_SQR)
		{
			if( HasCondition(COND_SEE_ENEMY) )
			{
				if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
					return SCHED_RANGE_ATTACK1;
			}
			else
			{
				if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
					return SCHED_SOLDIER_PRESS_ATTACK;
			}
		}

		if ( HasCondition(COND_SEE_ENEMY) && !IsUnreachable( GetEnemy() ) )
		{
			return SCHED_SOLDIER_CHARGE_PLAYER;
		}
	}

	// Can I shoot?
	if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
	{
		// Engage if allowed
		if ( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
		{
			return SCHED_RANGE_ATTACK1;
		}

		// Throw a grenade if not allowed to engage with weapon.
		if ( CanGrenadeEnemy() )
		{
			if ( OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
			{
				return SCHED_RANGE_ATTACK2;
			}
		}

		// I recently hided, just suppress right away:
		if ((gpGlobals->curtime - m_flLastTimeRanForCover) < TAKE_COVER_DELAY)		
			return SCHED_RANGE_ATTACK1;		

		return SCHED_TAKE_COVER_FROM_ENEMY;
	}

	if ( GetEnemy() && !HasCondition(COND_SEE_ENEMY) )
	{
		// We don't see our enemy. If it hasn't been long since I last saw him,
		// and he's pretty close to the last place I saw him, throw a grenade in 
		// to flush him out. A wee bit of cheating here...

		float flTime;
		float flDist;

		flTime = gpGlobals->curtime - GetEnemies()->LastTimeSeen( GetEnemy() );
		flDist = ( GetEnemy()->GetAbsOrigin() - GetEnemies()->LastSeenPosition( GetEnemy() ) ).Length();

		//Msg("Time: %f   Dist: %f\n", flTime, flDist );
		if ( flTime <= SOLDIER_GRENADE_FLUSH_TIME && flDist <= SOLDIER_GRENADE_FLUSH_DIST && CanGrenadeEnemy( false ) && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) )
		{
			return SCHED_RANGE_ATTACK2;
		}
	}

	if (HasCondition(COND_WEAPON_SIGHT_OCCLUDED))
	{
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if ( pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlot( SQUAD_SLOT_ATTACK_OCCLUDER ) )
		{
			return SCHED_SHOOT_ENEMY_COVER;
		}
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseSoldier::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			m_flLastTimeRanForCover = gpGlobals->curtime;
			if ( m_pSquad )
			{
				// Have to explicitly check innate range attack condition as may have weapon with range attack 2
				if (HasCondition(COND_CAN_RANGE_ATTACK2) && OccupyStrategySlot(SQUAD_SLOT_GRENADE1))
				{
					HL2MPRules()->EmitSoundToClient(this, "Grenade", GetNPCType(), GetGender()); // throw frag
					return SCHED_SOLDIER_TOSS_GRENADE_COVER1;
				}
				else
				{
					if (ShouldChargePlayer() && !IsUnreachable(GetEnemy()))
						return SCHED_SOLDIER_CHARGE_PLAYER;

					return SCHED_SOLDIER_TAKE_COVER1;
				}
			}
			else
			{
				// Have to explicitly check innate range attack condition as may have weapon with range attack 2
				if ( random->RandomInt(0,1) && HasCondition(COND_CAN_RANGE_ATTACK2) )
				{
					return SCHED_SOLDIER_GRENADE_COVER1;
				}
				else
				{
					if ( ShouldChargePlayer() && !IsUnreachable( GetEnemy() ) )
						return SCHED_SOLDIER_CHARGE_PLAYER;

					return SCHED_SOLDIER_TAKE_COVER1;
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			m_flLastTimeRanForCover = gpGlobals->curtime;
			return SCHED_SOLDIER_TAKE_COVER_FROM_BEST_SOUND;
		}
		break;
	case SCHED_SOLDIER_TAKECOVER_FAILED:
		{
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			{
				return TranslateSchedule( SCHED_RANGE_ATTACK1 );
			}

			// Run somewhere randomly
			return TranslateSchedule( SCHED_FAIL ); 
			break;
		}
		break;
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			if( HasCondition( COND_SEE_ENEMY ) )
			{
				return TranslateSchedule( SCHED_TAKE_COVER_FROM_ENEMY );
			}
			else 
			{
				// Don't patrol if I'm in the middle of an assault, because 
				// I'll never return to the assault. 
				if (GetEnemy())
					RememberUnreachable(GetEnemy());

				return TranslateSchedule(SCHED_SOLDIER_PATROL);
			}
		}
		break;
	case SCHED_SOLDIER_ASSAULT:
		{
			CBaseEntity *pEntity = GetEnemy();
			// FIXME: this should be generalized by the schedules that are selected, or in the definition of 
			// what "cover" means (i.e., trace attack vulnerability vs. physical attack vulnerability
			if (pEntity && pEntity->MyNPCPointer())
			{
				if ( !(pEntity->MyNPCPointer()->CapabilitiesGet( ) & bits_CAP_WEAPON_RANGE_ATTACK1))
				{
					return TranslateSchedule( SCHED_ESTABLISH_LINE_OF_FIRE );
				}
			}
			return SCHED_SOLDIER_ASSAULT;
		}
	case SCHED_ESTABLISH_LINE_OF_FIRE:
		{
			if( IsUsingTacticalVariant( TACTICAL_VARIANT_PRESSURE_ENEMY ) && !IsRunningBehavior() )
			{
				if( OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
				{
					return SCHED_SOLDIER_PRESS_ATTACK;
				}
			}

			return SCHED_SOLDIER_ESTABLISH_LINE_OF_FIRE;
		}
		break;
	case SCHED_HIDE_AND_RELOAD:
		{
			// stand up, just in case
			// Stand();
			// DesireStand();
			if( CanGrenadeEnemy() && OccupyStrategySlot( SQUAD_SLOT_GRENADE1 ) && random->RandomInt( 0, 100 ) < 20 )
			{
				// If I COULD throw a grenade and I need to reload, 20% chance I'll throw a grenade before I hide to reload.
				return SCHED_SOLDIER_GRENADE_AND_RELOAD;
			}

			// No running away in the citadel!
			if ( ShouldChargePlayer() )
				return SCHED_RELOAD;

			return SCHED_SOLDIER_HIDE_AND_RELOAD;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			if ( HasCondition( COND_NO_PRIMARY_AMMO ) || HasCondition( COND_LOW_PRIMARY_AMMO ) )
			{
				// Ditch the strategy slot for attacking (which we just reserved!)
				VacateStrategySlot();
				return TranslateSchedule( SCHED_HIDE_AND_RELOAD );
			}

			Stand();
			return SCHED_SOLDIER_RANGE_ATTACK1;
		}
	case SCHED_RANGE_ATTACK2:
		{
			// If my weapon can range attack 2 use the weapon
			if (GetActiveWeapon() && GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK2)
			{
				return SCHED_RANGE_ATTACK2;
			}
			// Otherwise use innate attack
			else
			{
				return SCHED_SOLDIER_RANGE_ATTACK2;
			}
		}
	case SCHED_FAIL:
		{
			if ( GetEnemy() != NULL )
			{
				return SCHED_SOLDIER_COMBAT_FAIL;
			}
			return SCHED_FAIL;
		}

	case SCHED_SOLDIER_PATROL:
		{
			// If I have an enemy, don't go off into random patrol mode.
			if ( GetEnemy() && GetEnemy()->IsAlive() )
				return SCHED_SOLDIER_PATROL_ENEMY;

			return SCHED_SOLDIER_PATROL;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
//=========================================================
void CNPC_BaseSoldier::OnStartSchedule( int scheduleType ) 
{
}

float CNPC_BaseSoldier::GetIdealSpeed() const
{
	return (BaseClass::GetIdealSpeed() * m_flSpeedFactorValue);
}

float CNPC_BaseSoldier::GetIdealAccel() const
{
	return (BaseClass::GetIdealAccel() * m_flSpeedFactorValue);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_BaseSoldier::HandleAnimEvent( animevent_t *pEvent )
{
	Vector vecShootDir;
	Vector vecShootOrigin;
	bool handledEvent = false;

	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		BaseClass::HandleAnimEvent( pEvent );
	}
	else
	{
		switch( pEvent->event )
		{
		case SOLDIER_AE_AIM:	
			{
				handledEvent = true;
				break;
			}
		case SOLDIER_AE_RELOAD:

			// We never actually run out of ammo, just need to refill the clip
			if (GetActiveWeapon())
			{
				GetActiveWeapon()->WeaponSound( RELOAD_NPC );
				GetActiveWeapon()->m_iClip = GetActiveWeapon()->GetMaxClip();
			}
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
			handledEvent = true;
			break;

		case SOLDIER_AE_GREN_TOSS:
			{
				Vector vecSpin;
				vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
				vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
				vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );

				Vector vecStart;
				GetAttachment( "lefthand", vecStart );

				if( m_NPCState == NPC_STATE_SCRIPT )
				{
					// Use a fixed velocity for grenades thrown in scripted state.
					// Grenades thrown from a script do not count against grenades remaining for the AI to use.
					Vector forward, up, vecThrow;

					GetVectors( &forward, NULL, &up );
					vecThrow = forward * 750 + up * 175;
					Fraggrenade_Create(vecStart, vec3_angle, vecThrow, vecSpin, this, SOLDIER_GRENADE_TIMER, (Classify() == CLASS_MILITARY));
				}
				else
				{
					// Use the Velocity that AI gave us.
					Fraggrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, SOLDIER_GRENADE_TIMER, (Classify() == CLASS_MILITARY));
					m_iNumGrenades--;
				}

				// wait six seconds before even looking again to see if a grenade can be thrown.
				m_flNextGrenadeCheck = gpGlobals->curtime + 6;
			}
			handledEvent = true;
			break;

		case SOLDIER_AE_GREN_DROP:
			{
				Vector vecStart;
				GetAttachment( "lefthand", vecStart );

				Fraggrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vec3_origin, this, SOLDIER_GRENADE_TIMER, (Classify() == CLASS_MILITARY));
				m_iNumGrenades--;
			}
			handledEvent = true;
			break;

		case SOLDIER_AE_KICK:
			{
				// Does no damage, because damage is applied based upon whether the target can handle the interaction
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, -Vector(16,16,18), Vector(16,16,18), 0, DMG_CLUB );
				CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
				if (pBCC)
				{
					Vector forward, up;
					AngleVectors( GetLocalAngles(), &forward, NULL, &up );

					if (pBCC->IsPlayer())
					{
						pBCC->ViewPunch(QAngle(-12, -7, 0));
						pHurt->ApplyAbsVelocityImpulse(forward * 100 + up * 50);
					}

					CTakeDamageInfo info(this, this, m_nKickDamage, DMG_CLUB);
					CalculateMeleeDamageForce(&info, forward, pBCC->GetAbsOrigin());
					pBCC->TakeDamage(info);

					EmitSound("BaseEnemyHumanoid.WeaponBash");
				}			

				// kick grunt sound?
				handledEvent = true;
				break;
			}

		case SOLDIER_AE_CAUGHT_ENEMY:
			HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());
			handledEvent = true;
			break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
		}
	}

	if( handledEvent )
	{
		m_iLastAnimEventHandled = pEvent->event;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get shoot position of BCC at an arbitrary position
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_BaseSoldier::Weapon_ShootPosition( )
{
	bool bStanding = !IsCrouching();
	Vector right;
	GetVectors( NULL, &right, NULL );

	if ((CapabilitiesGet() & bits_CAP_DUCK))
	{
		if ( IsCrouchedActivity( GetActivity() ) )
		{
			bStanding = false;
		}
	}

	// FIXME: rename this "estimated" since it's not based on animation
	// FIXME: the orientation won't be correct when testing from arbitary positions for arbitary angles

	if  ( bStanding )
	{
		if (IsWeaponShotgun())
		{
			return GetAbsOrigin() + SOLDIER_SHOTGUN_STANDING_POSITION + right * 8;
		}
		else
		{
			return GetAbsOrigin() + SOLDIER_GUN_STANDING_POSITION + right * 8;
		}
	}
	else
	{
		if (IsWeaponShotgun())
		{
			return GetAbsOrigin() + SOLDIER_SHOTGUN_CROUCHING_POSITION + right * 8;
		}
		else
		{
			return GetAbsOrigin() + SOLDIER_GUN_CROUCHING_POSITION + right * 8;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CNPC_BaseSoldier::PainSound ( void )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	if ( gpGlobals->curtime > m_flNextPainSoundTime )
	{
		const char *pSentenceName = "Taunt";
		float healthRatio = (float)GetHealth() / (float)GetMaxHealth();
		if ( !HasMemory(bits_MEMORY_PAIN_LIGHT_SOUND) && healthRatio > 0.9 )
		{
			Remember( bits_MEMORY_PAIN_LIGHT_SOUND );
			pSentenceName = "Contact";
		}
		else if ( !HasMemory(bits_MEMORY_PAIN_HEAVY_SOUND) && healthRatio < 0.25 )
		{
			Remember( bits_MEMORY_PAIN_HEAVY_SOUND );
			pSentenceName = "Pain";
		}

		HL2MPRules()->EmitSoundToClient(this, pSentenceName, GetNPCType(), GetGender());
		m_flNextPainSoundTime = gpGlobals->curtime + 2;
	}
}

//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::LostEnemySound( void)
{
	if (gpGlobals->curtime <= m_flNextLostSoundTime)
		return;

	HL2MPRules()->EmitSoundToClient(this, "ContactLost", GetNPCType(), GetGender());
	m_flNextLostSoundTime = gpGlobals->curtime + random->RandomFloat(5.0, 15.0);
}

//-----------------------------------------------------------------------------
// Purpose: implemented by subclasses to give them an opportunity to make
//			a sound when they lose their enemy
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::FoundEnemySound( void)
{
	HL2MPRules()->EmitSoundToClient(this, "Contact", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
// Purpose: Implemented by subclasses to give them an opportunity to make
//			a sound before they attack
// Input  :
// Output :
//-----------------------------------------------------------------------------

// BUGBUG: It looks like this is never played because soldier don't do SCHED_WAKE_ANGRY or anything else that does a TASK_SOUND_WAKE
void CNPC_BaseSoldier::AlertSound( void)
{
	if ( gpGlobals->curtime > m_flNextAlertSoundTime )
	{
		// Alert go snd?
		m_flNextAlertSoundTime = gpGlobals->curtime + 10.0f;
	}
}

//=========================================================
// NotifyDeadFriend
//=========================================================
void CNPC_BaseSoldier::NotifyDeadFriend ( CBaseEntity* pFriend )
{
	if (GetSquad()->NumMembers() < 2)
	{
		// SOLDIER_LAST_OF_SQUAD
		JustMadeSound();
		return;
	}
	// relaxed visibility test so that guys say this more often
	//if( FInViewCone( pFriend ) && FVisible( pFriend ) )
	// SOLDIER_MAN_DOWN

	BaseClass::NotifyDeadFriend(pFriend);
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_BaseSoldier::DeathSound ( void )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

//=========================================================
// IdleSound 
//=========================================================
void CNPC_BaseSoldier::IdleSound( void )
{
	if (GetIdleState() || random->RandomInt(0,1))
	{
		if (!GetIdleState())
		{
			// ask question or make statement
			switch (random->RandomInt(0,2))
			{
			case 0: // check in
				HL2MPRules()->EmitSoundToClient(this, "Question2", GetNPCType(), GetGender());
				SetIdleState(1);
				break;

			case 1: // question
				HL2MPRules()->EmitSoundToClient(this, "Question2", GetNPCType(), GetGender());
				SetIdleState(2);
				break;

			case 2: // statement
				HL2MPRules()->EmitSoundToClient(this, "Idle", GetNPCType(), GetGender());
				break;
			}
		}
		else
		{
			switch (GetIdleState())
			{
			case 1: // check in
				HL2MPRules()->EmitSoundToClient(this, "Answer1", GetNPCType(), GetGender());
				SetIdleState(0);
				break;
			case 2: // question 
				HL2MPRules()->EmitSoundToClient(this, "Answer2", GetNPCType(), GetGender());
				SetIdleState(0);
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//			This is for Grenade attacks.  As the test for grenade attacks
//			is expensive we don't want to do it every frame.  Return true
//			if we meet minimum set of requirements and then test for actual
//			throw later if we actually decide to do a grenade attack.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_BaseSoldier::RangeAttack2Conditions( float flDot, float flDist ) 
{
	return COND_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the soldier has grenades, hasn't checked lately, and
//			can throw a grenade at the target point.
// Input  : &vecTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::CanThrowGrenade( const Vector &vecTarget )
{
	if( m_iNumGrenades < 1 )
	{
		// Out of grenades!
		return false;
	}

	if (gpGlobals->curtime < m_flNextGrenadeCheck )
	{
		// Not allowed to throw another grenade right now.
		return false;
	}

	float flDist;
	flDist = ( vecTarget - GetAbsOrigin() ).Length();

	if( flDist > 1024 || flDist < 128 )
	{
		// Too close or too far!
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}

	// -----------------------
	// If moving, don't check.
	// -----------------------
	if ( m_flGroundSpeed != 0 )
		return false;

	// ---------------------------------------------------------------------
	// Are any of my squad members near the intended grenade impact area?
	// ---------------------------------------------------------------------
	if ( m_pSquad )
	{
		if (m_pSquad->SquadMemberInRange( vecTarget, SOLDIER_MIN_GRENADE_CLEAR_DIST ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.

			// Tell my squad members to clear out so I can get a grenade in
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY | SOUND_CONTEXT_COMBINE_ONLY, vecTarget, SOLDIER_MIN_GRENADE_CLEAR_DIST, 0.1 );
			return false;
		}
	}

	return CheckCanThrowGrenade( vecTarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the soldier can throw a grenade at the specified target point
// Input  : &vecTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::CheckCanThrowGrenade( const Vector &vecTarget )
{
	//NDebugOverlay::Line( EyePosition(), vecTarget, 0, 255, 0, false, 5 );

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: this is only valid for hand grenades, not RPG's
	Vector vecToss;
	Vector vecMins = -Vector(4,4,4);
	Vector vecMaxs = Vector(4,4,4);
	if( FInViewCone( vecTarget ) && CBaseEntity::FVisible( vecTarget ) )
	{
		vecToss = VecCheckThrow( this, EyePosition(), vecTarget, SOLDIER_GRENADE_THROW_SPEED, 1.0, &vecMins, &vecMaxs );
	}
	else
	{
		// Have to try a high toss. Do I have enough room?
		trace_t tr;
		AI_TraceLine( EyePosition(), EyePosition() + Vector( 0, 0, 64 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		if( tr.fraction != 1.0 )
		{
			return false;
		}

		vecToss = VecCheckToss( this, EyePosition(), vecTarget, -1, 1.0, true, &vecMins, &vecMaxs );
	}

	if ( vecToss != vec3_origin )
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // 1/3 second.
		return true;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::CanGrenadeEnemy( bool bUseFreeKnowledge )
{
	CBaseEntity *pEnemy = GetEnemy();

	Assert( pEnemy != NULL );

	if( pEnemy )
	{
		// I'm not allowed to throw grenades during dustoff
		if ( IsCurSchedule(SCHED_DROPSHIP_DUSTOFF) )
			return false;

		if( bUseFreeKnowledge )
		{
			// throw to where we think they are.
			return CanThrowGrenade( GetEnemies()->LastKnownPosition( pEnemy ) );
		}
		else
		{
			// hafta throw to where we last saw them.
			return CanThrowGrenade( GetEnemies()->LastSeenPosition( pEnemy ) );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: For soldier melee attack (kick/hit)
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseSoldier::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 64)
	{
		return COND_NONE; // COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NONE; // COND_NOT_FACING_ATTACK;
	}

	// Check Z
	if ( GetEnemy() && fabs(GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > 64 )
		return COND_NONE;

	// Make sure not trying to kick through a window or something. 
	trace_t tr;
	Vector vecSrc, vecEnd;

	vecSrc = WorldSpaceCenter();
	vecEnd = GetEnemy()->WorldSpaceCenter();

	AI_TraceLine(vecSrc, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	if( tr.m_pEnt != GetEnemy() )
	{
		return COND_NONE;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_BaseSoldier::EyePosition( void ) 
{
	if ( !IsCrouching() )
	{
		return GetAbsOrigin() + SOLDIER_EYE_STANDING_POSITION;
	}
	else
	{
		return GetAbsOrigin() + SOLDIER_EYE_CROUCHING_POSITION;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nActivity - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_BaseSoldier::EyeOffset( Activity nActivity )
{
	if (CapabilitiesGet() & bits_CAP_DUCK)
	{
		if ( IsCrouchedActivity( nActivity ) )
			return SOLDIER_EYE_CROUCHING_POSITION;

	}
	// if the hint doesn't tell anything, assume current state
	if ( !IsCrouching() )
	{
		return SOLDIER_EYE_STANDING_POSITION;
	}
	else
	{
		return SOLDIER_EYE_CROUCHING_POSITION;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_BaseSoldier::GetCrouchEyeOffset( void )
{
	return SOLDIER_EYE_CROUCHING_POSITION;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::SetActivity( Activity NewActivity )
{
	BaseClass::SetActivity( NewActivity );

	m_iLastAnimEventHandled = -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
NPC_STATE CNPC_BaseSoldier::SelectIdealState( void )
{
	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() == NULL )
			{
				if ( !HasCondition( COND_ENEMY_DEAD ) )
				{
					// Lost track of my enemy. Patrol.
					SetCondition( COND_SOLDIER_SHOULD_PATROL );
				}
				return NPC_STATE_ALERT;
			}
			else if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				AnnounceEnemyKill(GetEnemy());
			}
		}

	default:
		{
			return BaseClass::SelectIdealState();
		}
	}

	return GetIdealState();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::OnBeginMoveAndShoot()
{
	if ( BaseClass::OnBeginMoveAndShoot() )
	{
		if( HasStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true; // already have the slot I need

		if( !HasStrategySlotRange( SQUAD_SLOT_GRENADE1, SQUAD_SLOT_ATTACK_OCCLUDER ) && OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseSoldier::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}

//-----------------------------------------------------------------------------
// Only supports weapons that use clips.
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::ActiveWeaponIsFullyLoaded()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if (!pWeapon || !pWeapon->UsesClipsForAmmo())
		return false;

	return (pWeapon->Clip() >= pWeapon->GetMaxClip());
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
const char* CNPC_BaseSoldier::GetSquadSlotDebugName( int iSquadSlot )
{
	switch( iSquadSlot )
	{
	case SQUAD_SLOT_GRENADE1:			return "SQUAD_SLOT_GRENADE1";	
		break;
	case SQUAD_SLOT_GRENADE2:			return "SQUAD_SLOT_GRENADE2";	
		break;
	case SQUAD_SLOT_ATTACK_OCCLUDER:	return "SQUAD_SLOT_ATTACK_OCCLUDER";	
		break;
	}

	return BaseClass::GetSquadSlotDebugName( iSquadSlot );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::IsUsingTacticalVariant( int variant )
{
	if( variant == TACTICAL_VARIANT_PRESSURE_ENEMY && m_iTacticalVariant == TACTICAL_VARIANT_PRESSURE_ENEMY_UNTIL_CLOSE )
	{
		// Essentially, fib. Just say that we are a 'pressure enemy' soldier.
		return true;
	}

	return m_iTacticalVariant == variant;
}

//-----------------------------------------------------------------------------
// For the purpose of determining whether to use a pathfinding variant, this
// function determines whether the current schedule is a schedule that 
// 'approaches' the enemy. 
//-----------------------------------------------------------------------------
bool CNPC_BaseSoldier::IsRunningApproachEnemySchedule()
{
	if( IsCurSchedule( SCHED_CHASE_ENEMY ) )
		return true;

	if( IsCurSchedule( SCHED_ESTABLISH_LINE_OF_FIRE ) )
		return true;

	if( IsCurSchedule( SCHED_SOLDIER_PRESS_ATTACK, false ) )
		return true;

	return false;
}

bool CNPC_BaseSoldier::IsLightDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsLightDamage(info);
}

bool CNPC_BaseSoldier::IsHeavyDamage(const CTakeDamageInfo &info)
{
	if (info.GetDamageType() & DMG_BUCKSHOT)
		return true;

	int ammoType = info.GetAmmoType();
	static int heavyDamageTypes[] = {
		GetAmmoDef()->Index("Minigun"),
		GetAmmoDef()->Index("Sniper"),
		GetAmmoDef()->Index("Buckshot"),
		GetAmmoDef()->Index("Magnum")
	};

	for (int i = 0; i < _ARRAYSIZE(heavyDamageTypes); i++)
	{
		if (ammoType == heavyDamageTypes[i])
			return true;
	}

	return BaseClass::IsHeavyDamage(info);
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_combine, CNPC_BaseSoldier )

//Tasks
DECLARE_TASK( TASK_SOLDIER_FACE_TOSS_DIR )
DECLARE_TASK( TASK_SOLDIER_IGNORE_ATTACKS )
DECLARE_TASK( TASK_SOLDIER_DEFER_SQUAD_GRENADES )
DECLARE_TASK( TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY )
DECLARE_TASK( TASK_SOLDIER_GET_PATH_TO_FORCED_GREN_LOS )

//Activities
DECLARE_ACTIVITY(ACT_COMBINE_THROW_GRENADE)

DECLARE_SQUADSLOT( SQUAD_SLOT_GRENADE1 )
DECLARE_SQUADSLOT( SQUAD_SLOT_GRENADE2 )

DECLARE_CONDITION( COND_SOLDIER_SHOULD_PATROL )
DECLARE_CONDITION( COND_SOLDIER_ATTACK_SLOT_AVAILABLE )

//=========================================================
// SCHED_SOLDIER_TAKE_COVER_FROM_BEST_SOUND
//
//	hide from the loudest sound source (to run from grenade)
//=========================================================
DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_TAKE_COVER_FROM_BEST_SOUND,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SOLDIER_RUN_AWAY_FROM_BEST_SOUND"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_FIND_COVER_FROM_BEST_SOUND	0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"		 TASK_REMEMBER						MEMORY:INCOVER"
	"		 TASK_FACE_REASONABLE				0"
	""
	"	Interrupts"
	)

	DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_RUN_AWAY_FROM_BEST_SOUND,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_COWER"
	"		 TASK_GET_PATH_AWAY_FROM_BEST_SOUND		600"
	"		 TASK_RUN_PATH_TIMED					2"
	"		 TASK_STOP_MOVING						0"
	""
	"	Interrupts"
	)
	//=========================================================
	//	SCHED_SOLDIER_COMBAT_FAIL
	//=========================================================
	DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_COMBAT_FAIL,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE "
	"		TASK_WAIT_FACE_ENEMY		2"
	"		TASK_WAIT_PVS				0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	)

	//=========================================================
	// SCHED_SOLDIER_ASSAULT
	//=========================================================
	DEFINE_SCHEDULE 
	(
	SCHED_SOLDIER_ASSAULT,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_SOLDIER_ESTABLISH_LINE_OF_FIRE"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_GET_PATH_TO_ENEMY_LKP		0"
	"		TASK_SOLDIER_IGNORE_ATTACKS		0.2"
	"		TASK_RUN_PATH					0"
	//		"		TASK_SOLDIER_MOVE_AND_AIM		0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SOLDIER_IGNORE_ATTACKS		0.0"
	""
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_FAR_TO_ATTACK"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	)

	DEFINE_SCHEDULE 
	(
	SCHED_SOLDIER_ESTABLISH_LINE_OF_FIRE,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_FAIL_ESTABLISH_LINE_OF_FIRE"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_GET_PATH_TO_ENEMY_LKP_LOS	0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_SOLDIER_IGNORE_ATTACKS		0.0"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_COMBAT_FACE"
	"	"
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	//"		COND_CAN_RANGE_ATTACK1"
	//"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// SCHED_SOLDIER_PRESS_ATTACK
	//=========================================================
	DEFINE_SCHEDULE 
	(
	SCHED_SOLDIER_PRESS_ATTACK,

	"	Tasks "
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_SOLDIER_ESTABLISH_LINE_OF_FIRE"
	"		TASK_SET_TOLERANCE_DISTANCE		72"
	"		TASK_GET_PATH_TO_ENEMY_LKP		0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_LOW_PRIMARY_AMMO"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	)

	//=========================================================
	// 	SCHED_HIDE_AND_RELOAD	
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_HIDE_AND_RELOAD,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_RELOAD"
	"		TASK_FIND_COVER_FROM_ENEMY	0"
	"		TASK_RUN_PATH				0"
	"		TASK_WAIT_FOR_MOVEMENT		0"
	"		TASK_REMEMBER				MEMORY:INCOVER"
	"		TASK_FACE_ENEMY				0"
	"		TASK_RELOAD					0"
	""
	"	Interrupts"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	)

	//=========================================================
	// SCHED_SOLDIER_SUPPRESS
	//=========================================================
	DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_SUPPRESS,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_ENEMY				0"
	"		TASK_RANGE_ATTACK1			0"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"
	)

	//=========================================================
	// SCHED_SOLDIER_WAIT_IN_COVER
	//	we don't allow danger or the ability
	//	to attack to break a soldier's run to cover schedule but
	//	when a soldier is in cover we do want them to attack if they can.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_WAIT_IN_COVER,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"	// Translated to cover
	"		TASK_WAIT_FACE_ENEMY			1"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	"		COND_SOLDIER_ATTACK_SLOT_AVAILABLE"
	)

	//=========================================================
	// SCHED_SOLDIER_TAKE_COVER1
	//=========================================================
	DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_TAKE_COVER1  ,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_SOLDIER_TAKECOVER_FAILED"
	"		TASK_STOP_MOVING			0"
	"		TASK_WAIT					0.2"
	"		TASK_FIND_COVER_FROM_ENEMY	0"
	"		TASK_RUN_PATH				0"
	"		TASK_WAIT_FOR_MOVEMENT		0"
	"		TASK_REMEMBER				MEMORY:INCOVER"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_SOLDIER_WAIT_IN_COVER"
	""
	"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_TAKECOVER_FAILED,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	""
	"	Interrupts"
	)

	//=========================================================
	// SCHED_SOLDIER_GRENADE_COVER1
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_GRENADE_COVER1,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_FIND_COVER_FROM_ENEMY			99"
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
	"		TASK_CLEAR_MOVE_WAIT				0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_SOLDIER_WAIT_IN_COVER"
	""
	"	Interrupts"
	)

	//=========================================================
	// SCHED_SOLDIER_TOSS_GRENADE_COVER1
	//
	//	 drop grenade then run to cover.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_TOSS_GRENADE_COVER1,

	"	Tasks"
	"		TASK_FACE_ENEMY						0"
	"		TASK_RANGE_ATTACK2 					0"
	"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
	""
	"	Interrupts"
	)

	//=========================================================
	// SCHED_SOLDIER_RANGE_ATTACK1
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_RANGE_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_ENEMY					0"
	"		TASK_ANNOUNCE_ATTACK			1"	// 1 = primary attack
	"		TASK_WAIT_RANDOM				0.3"
	"		TASK_RANGE_ATTACK1				0"
	"		TASK_SOLDIER_IGNORE_ATTACKS		0.5"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAVY_DAMAGE"
	"		COND_LIGHT_DAMAGE"
	"		COND_LOW_PRIMARY_AMMO"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_WEAPON_BLOCKED_BY_FRIEND"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_GIVE_WAY"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	""
	// Enemy_Occluded				Don't interrupt on this.  Means
	//								comibine will fire where player was after
	//								he has moved for a little while.  Good effect!!
	// WEAPON_SIGHT_OCCLUDED		Don't block on this! Looks better for railings, etc.
	)

	//=========================================================
	// Mapmaker forced grenade throw
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_FORCED_GRENADE_THROW,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SOLDIER_FACE_TOSS_DIR			0"
	"		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
	"		TASK_SOLDIER_DEFER_SQUAD_GRENADES	0"
	""
	"	Interrupts"
	)

	//=========================================================
	// Move to LOS of the mapmaker's forced grenade throw target
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_MOVE_TO_FORCED_GREN_LOS,

	"	Tasks "
	"		TASK_SET_TOLERANCE_DISTANCE					48"
	"		TASK_SOLDIER_GET_PATH_TO_FORCED_GREN_LOS	0"
	"		TASK_RUN_PATH								0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	"	"
	"	Interrupts "
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// 	SCHED_SOLDIER_RANGE_ATTACK2	
	//
	//	secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
	//	soldiers's grenade toss requires the enemy be occluded.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_RANGE_ATTACK2,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SOLDIER_FACE_TOSS_DIR			0"
	"		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
	"		TASK_SOLDIER_DEFER_SQUAD_GRENADES	0"
	"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_SOLDIER_WAIT_IN_COVER"	// don't run immediately after throwing grenade.
	""
	"	Interrupts"
	)


	//=========================================================
	// Throw a grenade, then run off and reload.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_GRENADE_AND_RELOAD,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SOLDIER_FACE_TOSS_DIR			0"
	"		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
	"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
	"		TASK_SOLDIER_DEFER_SQUAD_GRENADES	0"
	"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_HIDE_AND_RELOAD"	// don't run immediately after throwing grenade.
	""
	"	Interrupts"
	)

	DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_PATROL,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_WANDER						900540" 
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_FACE_REASONABLE			0"
	"		TASK_WAIT						3"
	"		TASK_WAIT_RANDOM				3"
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_SOLDIER_PATROL" // keep doing it
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	)

	//=========================================================
	// SCHED_SOLDIER_CHARGE_PLAYER
	//
	//	Used to run straight at enemy player since physgun combat
	//  is more fun when the enemies are close
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_CHARGE_PLAYER,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
	"		TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY		192"
	"		TASK_FACE_ENEMY						0"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TASK_FAILED"
	"		COND_LOST_ENEMY"
	"		COND_HEAR_DANGER"
	)

	//=========================================================
	// SCHED_SOLDIER_PATROL_ENEMY
	//
	// Used instead if SCHED_SOLDIER_PATROL if I have an enemy.
	// Wait for the enemy a bit in the hopes of ambushing him.
	//=========================================================
	DEFINE_SCHEDULE	
	(
	SCHED_SOLDIER_PATROL_ENEMY,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_WAIT_FACE_ENEMY				1" 
	"		TASK_WAIT_FACE_ENEMY_RANDOM			3" 
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_MOVE_AWAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	)

	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_FACE_IDEAL_YAW,

	"	Tasks"
	"		TASK_FACE_IDEAL				0"
	"	"
	"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
	SCHED_SOLDIER_MOVE_TO_MELEE,

	"	Tasks"
	"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
	"		TASK_GET_PATH_TO_SAVEPOSITION				0"
	"		TASK_RUN_PATH								0"
	"		TASK_WAIT_FOR_MOVEMENT						0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_MELEE_ATTACK1"
	)

	AI_END_CUSTOM_NPC()
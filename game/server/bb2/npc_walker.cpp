//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A slow walker, he wants your braain!
//
//========================================================================================//

#include "cbase.h"
#include "doors.h"
#include "simtimer.h"
#include "npc_BaseZombie.h"
#include "ai_hull.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "gib.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "GameBase_Server.h"
#include "hl2mp_gamerules.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

envelopePoint_t envZombieMoanVolumeFast[] =
{
	{	7.0f, 7.0f,
	0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
	0.2f, 0.3f,
	},
};

envelopePoint_t envZombieMoanVolume[] =
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

envelopePoint_t envZombieMoanVolumeLong[] =
{
	{	1.0f, 1.0f,
	0.3f, 0.5f,
	},
	{	1.0f, 1.0f,
	0.6f, 1.0f,
	},
	{	0.0f, 0.0f,
	0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanIgnited[] =
{
	{	1.0f, 1.0f,
	0.5f, 1.0f,
	},
	{	1.0f, 1.0f,
	30.0f, 30.0f,
	},
	{	0.0f, 0.0f,
	0.5f, 1.0f,
	},
};

//=============================================================================
//=============================================================================

class CNPCWalker : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CNPCWalker, CAI_BlendingHost<CNPC_BaseZombie> );

public:
	CNPCWalker()
	{
	}

	void Spawn( void );
	void Precache( void );
	Class_T Classify( void );

	const char *GetNPCName();

	int AllowEntityToBeGibbed(void);

	void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );

	void GatherConditions( void );

	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int TranslateSchedule( int scheduleType );

	void CheckFlinches() {} // Zombie has custom flinch code

	Activity NPC_TranslateActivity( Activity newActivity );

	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	void Extinguish();
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	bool IsHeavyDamage( const CTakeDamageInfo &info );
	void BuildScheduleTestBits( void );

	void PrescheduleThink( void );
	int SelectSchedule ( void );

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot );

	void FadeIn();

	const char *GetMoanSound( int nSound );

	bool CanAlwaysSeePlayers();
	bool UsesNavMesh(void) { return true; }

	bool IsAllowedToBreakDoors(void);

	void EnterCrawlMode(void);
	void LeaveCrawlMode(void);
	void BecomeCrawler(void);
	bool IsCrawlingWithNoLegs(void)
	{
		return (IsGibFlagActive(GIB_NO_LEG_RIGHT) || IsGibFlagActive(GIB_NO_LEG_LEFT));
	}

	bool HasNoArms(void)
	{
		return (IsGibFlagActive(GIB_NO_ARM_RIGHT) && IsGibFlagActive(GIB_NO_ARM_LEFT));
	}

	float MaxYawSpeed(void);

public:
	DEFINE_CUSTOM_AI;

protected:
	static const char *pMoanSounds[];

	float GetAmbushDist(void) { return 100.0f; }
	void OnGibbedGroup(int hitgroup, bool bExploded);

private:

	Activity GetAttackActivity(void);

	bool m_bIsRunner;
	bool m_bIsReady;
	bool m_bCheckCollisionGroupChange;
	float m_flNextTimeToCheckCollisionChange;

	Vector m_vPositionCharged;
};

LINK_ENTITY_TO_CLASS( npc_walker, CNPCWalker );
LINK_ENTITY_TO_CLASS(npc_runner, CNPCWalker);

//---------------------------------------------------------
//---------------------------------------------------------
const char *CNPCWalker::pMoanSounds[] =
{
	"Moan1",
	"Moan2",
	"Moan3",
	"Moan4",
};

//=========================================================
// Conditions
//=========================================================
enum
{
	COND_ZOMBIE_CHARGE_TARGET_MOVED = LAST_BASE_ZOMBIE_CONDITION,
};

//=========================================================
// Schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_WANDER_ANGRILY = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_CHARGE_ENEMY,
	SCHED_ZOMBIE_FAIL,
	SCHED_WALKER_RISE_IDLE,
	SCHED_WALKER_RISE,
};

//=========================================================
// Tasks
//=========================================================
enum
{
	TASK_ZOMBIE_EXPRESS_ANGER = LAST_BASE_ZOMBIE_TASK,
	TASK_ZOMBIE_CHARGE_ENEMY,
	TASK_ZOMBIE_SPAWN,
};

//-----------------------------------------------------------------------------

int ACT_ZOMBIE_TANTRUM;

int ACT_MELEE_CHARGE;

int ACT_MELEE_ATTACK_HEAVY;
int ACT_MELEE_ATTACK_LEFT;
int ACT_MELEE_ATTACK_RIGHT;

int ACT_CRAWL;
int ACT_CRAWL_IDLE;
int ACT_CRAWL_ATTACK_LEFT;
int ACT_CRAWL_ATTACK_RIGHT;

int ACT_CRAWL_NOLEGS;
int ACT_CRAWL_NOLEGS_IDLE;
int ACT_CRAWL_NOLEGS_ATTACK_LEFT;
int ACT_CRAWL_NOLEGS_ATTACK_RIGHT;

BEGIN_DATADESC( CNPCWalker )

	DEFINE_FIELD( m_bIsReady, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vPositionCharged, FIELD_POSITION_VECTOR ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCWalker::Precache(void)
{
	BaseClass::Precache();
}

Class_T	CNPCWalker::Classify( void )
{
	if ( IsSlumped() || ( !m_bIsReady ) )
		return CLASS_NONE;

	return( CLASS_ZOMBIE ); 
}

const char *CNPCWalker::GetNPCName()
{
	if (m_bIsRunner)
		return "Runner";

	return "Walker";
}

int CNPCWalker::AllowEntityToBeGibbed(void)
{
	if (IsSlumped() || (!m_bIsReady))
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

// Handles fading.
void CNPCWalker::FadeIn()
{
	int iAlpha = GetRenderColor().a;
	iAlpha = MIN(iAlpha + (400 * gpGlobals->frametime), 255);

	SetRenderMode(kRenderTransAlpha);
	SetRenderColorA(iAlpha);

	if (iAlpha >= 255)
	{
		SetRenderMode(kRenderNormal);
		SetRenderColorA(255);
		m_bIsReady = true;
		SetSchedule(SCHED_WALKER_RISE);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCWalker::Spawn( void )
{
	m_bIsRunner = FClassnameIs(this, "npc_runner");
	m_bIsReady = false;
	m_bCheckCollisionGroupChange = false;
	Precache();

	SetBloodColor( BLOOD_COLOR_RED );

	m_flFieldOfView = -1.0;

	CapabilitiesClear();

	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_CRAWL);

	BaseClass::Spawn();

	m_flNextTimeToCheckCollisionChange = 0.0f;
	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE_SPAWNING);

	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 4.0 );

	SetRenderMode( kRenderTransAlpha );
	SetRenderColorA( 0 );

	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);

	SetState(NPC_STATE_IDLE);
	SetActivity((Activity)ACT_RISE_IDLE);
	SetSchedule(SCHED_WALKER_RISE_IDLE);

	// Reduce zombies view dist:
	m_flDistTooFar = 128.0;
	GetSenses()->SetDistLook(256.0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCWalker::PrescheduleThink( void )
{
	if ( !m_bIsReady )
		FadeIn();
	else if ((GetCollisionGroup() == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING) && m_bCheckCollisionGroupChange)
	{
		if (gpGlobals->curtime > m_flNextTimeToCheckCollisionChange)
		{
			m_flNextTimeToCheckCollisionChange = gpGlobals->curtime + 1.0f; // Change to the new collision group as fast as possible.
			trace_t tr;
			CTraceFilterOnlyNPCsAndPlayer filter(this, COLLISION_GROUP_NPC_ZOMBIE);
			UTIL_TraceHull(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, 36), WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, &filter, &tr);
			if (!tr.startsolid)
			{
				SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE);
				m_flNextTimeToCheckCollisionChange = 0.0f;
				m_bCheckCollisionGroupChange = false;
			}
		}
	}

	if( gpGlobals->curtime > m_flNextMoanSound )
	{
		if( CanPlayMoanSound() )
		{
			// Classic guy idles instead of moans.
			IdleSound();

			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 5.0 );
		}
		else
		{
			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 2.0 );
		}
	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPCWalker::SelectSchedule ( void )
{
	if (!m_bIsReady)
		return SCHED_WALKER_RISE_IDLE;

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CNPCWalker::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
		EmitSound(  "NPC_BaseZombie.FootstepRight" );
	else
		EmitSound( "NPC_BaseZombie.FootstepLeft" );
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a foot sliding/scraping
//-----------------------------------------------------------------------------
void CNPCWalker::FootscuffSound( bool fRightFoot )
{
	if( fRightFoot )
		EmitSound( "NPC_BaseZombie.ScuffRight" );
	else
		EmitSound( "NPC_BaseZombie.ScuffLeft" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CNPCWalker::AttackHitSound( void )
{
	EmitSound( "NPC_BaseZombie.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CNPCWalker::AttackMissSound( void )
{
	// Play a random attack miss sound
	EmitSound( "NPC_BaseZombie.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCWalker::PainSound( const CTakeDamageInfo &info )
{
	// We're constantly taking damage when we are on fire. Don't make all those noises!
	if (IsOnFire() || (GetLifeSpan() <= gpGlobals->curtime))
		return;

	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCWalker::DeathSound( const CTakeDamageInfo &info ) 
{
	if (GetLifeSpan() <= gpGlobals->curtime)
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCWalker::AlertSound( void )
{
	HL2MPRules()->EmitSoundToClient(this, "Alert", GetNPCType(), GetGender());

	// Don't let a moan sound cut off the alert sound.
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CNPCWalker::GetMoanSound( int nSound )
{
	return pMoanSounds[ nSound % ARRAYSIZE( pMoanSounds ) ];
}

//-----------------------------------------------------------------------------
// Purpose: Should we be able to see the player no matter what?
//-----------------------------------------------------------------------------
bool CNPCWalker::CanAlwaysSeePlayers()
{
	if (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == MODE_ARENA))
		return true;

	if (GetWorldEntity() && GetWorldEntity()->CanZombiesAlwaysSeeYou())
		return true;

	return false;
}

bool CNPCWalker::IsAllowedToBreakDoors(void)
{
	if (IsCrawlingWithNoLegs() || m_bIsInCrawlMode)
		return false;

	return true;
}

void CNPCWalker::EnterCrawlMode(void)
{
	if (m_bIsInCrawlMode || IsCrawlingWithNoLegs())
		return;

	m_bIsInCrawlMode = true;

	bool bWasMoving = IsMoving();
	if (bWasMoving)
		GetMotor()->MoveStop();

	SetHullType(HULL_TINY);
	SetHullSizeNormal(true);
	SetDefaultEyeOffset();

	if (VPhysicsGetObject())
	{
		SetupVPhysicsHull();
	}

	ResetIdealActivity((Activity)ACT_CRAWL_IDLE);

	if (bWasMoving)
		GetMotor()->MoveStart();
}

void CNPCWalker::LeaveCrawlMode(void)
{
	if (IsCrawlingWithNoLegs() || !m_bIsInCrawlMode)
		return;

	trace_t trace;
	AI_TraceHull(GetAbsOrigin(), GetAbsOrigin(), Vector(-20, -20, 0), Vector(20, 20, 74), MASK_NPCSOLID, this, COLLISION_GROUP_NPC, &trace);
	if (trace.DidHit())
		return;

	m_bIsInCrawlMode = false;

	bool bWasMoving = IsMoving();
	if (bWasMoving)
		GetMotor()->MoveStop();

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal(true);
	SetDefaultEyeOffset();

	if (VPhysicsGetObject())
	{
		SetupVPhysicsHull();
	}

	ResetIdealActivity(ACT_IDLE);

	if (bWasMoving)
		GetMotor()->MoveStart();
}

void CNPCWalker::BecomeCrawler(void)
{
	bool bWasMoving = IsMoving();
	if (bWasMoving)
		GetMotor()->MoveStop();

	CapabilitiesRemove(bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_CRAWL | bits_CAP_DOORS_GROUP); // We also remove the crawl flag because we can already crawl by default in this state.

	SetHullType(HULL_TINY);
	SetHullSizeNormal(true);
	SetDefaultEyeOffset();

	if (VPhysicsGetObject())
	{
		SetupVPhysicsHull();
	}

	ResetIdealActivity((Activity)ACT_CRAWL_NOLEGS_IDLE);

	if (bWasMoving)
		GetMotor()->MoveStart();
}

float CNPCWalker::MaxYawSpeed(void)
{
	if ((GetActivity() == (Activity)ACT_MELEE_ATTACK_HEAVY) ||
		(GetActivity() == (Activity)ACT_MELEE_ATTACK_LEFT) ||
		(GetActivity() == (Activity)ACT_MELEE_ATTACK_RIGHT))
		return 100;

	if (IsCrawlingWithNoLegs() || m_bIsInCrawlMode)
	{
		if ((GetActivity() == (Activity)ACT_CRAWL_NOLEGS_ATTACK_LEFT) ||
			(GetActivity() == (Activity)ACT_CRAWL_NOLEGS_ATTACK_RIGHT))
			return 15;

		if ((GetActivity() == (Activity)ACT_CRAWL_ATTACK_LEFT) ||
			(GetActivity() == (Activity)ACT_CRAWL_ATTACK_RIGHT))
			return 15;

		switch (GetActivity())
		{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			return 20;
			break;
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 20;
		default:
			return 10;
			break;
		}
	}

	return BaseClass::MaxYawSpeed();
}

void CNPCWalker::OnGibbedGroup(int hitgroup, bool bExploded)
{
	if (bExploded)
		return;

	if (IsCrawlingWithNoLegs())
		BecomeCrawler();
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CNPCWalker::IdleSound( void )
{
	if( GetState() == NPC_STATE_IDLE && random->RandomFloat( 0, 1 ) == 0 )
	{
		// Moan infrequently in IDLE state.
		return;
	}

	if( IsSlumped() )
	{
		// Sleeping zombies are quiet.
		return;
	}

	HL2MPRules()->EmitSoundToClient(this, "Idle", GetNPCType(), GetGender());
	MakeAISpookySound( 360.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CNPCWalker::AttackSound( void )
{
	HL2MPRules()->EmitSoundToClient(this, "Attack", GetNPCType(), GetGender());
}

//---------------------------------------------------------
// Classic zombie only uses moan sound if on fire.
//---------------------------------------------------------
void CNPCWalker::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( IsOnFire() )
	{
		BaseClass::MoanSound( pEnvelope, iEnvelopeSize );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPCWalker::GatherConditions( void )
{
	BaseClass::GatherConditions();

	ClearCondition(COND_ZOMBIE_CHARGE_TARGET_MOVED);

	if ( ConditionInterruptsCurSchedule( COND_ZOMBIE_CHARGE_TARGET_MOVED ) )
	{
		if ( GetNavigator()->IsGoalActive() )
		{
			const float CHARGE_RESET_TOLERANCE = 60.0;
			if ( !GetEnemy() ||
				( m_vPositionCharged - GetEnemyLKP()  ).Length() > CHARGE_RESET_TOLERANCE )
			{
				SetCondition( COND_ZOMBIE_CHARGE_TARGET_MOVED );
			}			 
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

int CNPCWalker::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedSchedule != SCHED_ZOMBIE_CHARGE_ENEMY && 
		IsPathTaskFailure( taskFailCode ) &&
		random->RandomInt( 1, 100 ) < 50 )
	{
		return SCHED_ZOMBIE_CHARGE_ENEMY;
	}

	if ( failedSchedule != SCHED_ZOMBIE_WANDER_ANGRILY &&
		( failedSchedule == SCHED_TAKE_COVER_FROM_ENEMY || 
		failedSchedule == SCHED_CHASE_ENEMY_FAILED ) )
	{
		return SCHED_ZOMBIE_WANDER_ANGRILY;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//---------------------------------------------------------
//---------------------------------------------------------

int CNPCWalker::TranslateSchedule( int scheduleType )
{
	if ( scheduleType == SCHED_COMBAT_FACE && IsUnreachable( GetEnemy() ) )
		return SCHED_TAKE_COVER_FROM_ENEMY;

	if ( scheduleType == SCHED_FAIL )
		return SCHED_ZOMBIE_FAIL;

	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------

Activity CNPCWalker::NPC_TranslateActivity( Activity newActivity )
{
	bool bIsIdle = ((newActivity == ACT_IDLE) || (newActivity == (Activity)ACT_ZOMBIE_TANTRUM));

	newActivity = BaseClass::NPC_TranslateActivity( newActivity );

	if ((newActivity == ACT_IDLE) || (newActivity == (Activity)ACT_ZOMBIE_TANTRUM))
		bIsIdle = true;

	if ((newActivity == ACT_WALK || newActivity == ACT_RUN) && !GameBaseServer()->IsClassicMode() && !IsCrawlingWithNoLegs() && !m_bIsInCrawlMode && m_bIsRunner)
			return (Activity)ACT_MELEE_CHARGE;

	if (IsCrawlingWithNoLegs())
	{
		if ((newActivity == ACT_RUN) || (newActivity == ACT_WALK))
			return (Activity)ACT_CRAWL_NOLEGS;
		else if (bIsIdle)
			return (Activity)ACT_CRAWL_NOLEGS_IDLE;
	}
	else if (m_bIsInCrawlMode)
	{
		if ((newActivity == ACT_RUN) || (newActivity == ACT_WALK))
			return (Activity)ACT_CRAWL;
		else if (bIsIdle)
			return (Activity)ACT_CRAWL_IDLE;
	}

	if ( newActivity == ACT_RUN )
		return ACT_WALK;

	if (newActivity == ACT_MELEE_ATTACK1 || newActivity == ACT_MELEE_ATTACK2)
		return GetAttackActivity();

	return newActivity;
}

Activity CNPCWalker::GetAttackActivity(void)
{
	if (HasNoArms())
	{
		if (IsCrawlingWithNoLegs())
			return (Activity)ACT_CRAWL_NOLEGS_IDLE;
		else if (m_bIsInCrawlMode)
			return (Activity)ACT_CRAWL_IDLE;

		return ACT_IDLE;
	}

	int randomAct = -1;
	if (IsCrawlingWithNoLegs())
	{
		randomAct = random->RandomInt(1, 2);
		if (!HasNoArms())
		{
			if (randomAct == 1)
				return (Activity)ACT_CRAWL_NOLEGS_ATTACK_LEFT;
			else
				return (Activity)ACT_CRAWL_NOLEGS_ATTACK_RIGHT;
		}

		if (!IsGibFlagActive(GIB_NO_ARM_RIGHT))
			return (Activity)ACT_CRAWL_NOLEGS_ATTACK_RIGHT;

		if (!IsGibFlagActive(GIB_NO_ARM_LEFT))
			return (Activity)ACT_CRAWL_NOLEGS_ATTACK_LEFT;

		return (Activity)ACT_CRAWL_NOLEGS_IDLE;
	}
	else if (m_bIsInCrawlMode)
	{
		randomAct = random->RandomInt(1, 2);
		if (!HasNoArms())
		{
			if (randomAct == 1)
				return (Activity)ACT_CRAWL_ATTACK_LEFT;
			else
				return (Activity)ACT_CRAWL_ATTACK_RIGHT;
		}

		if (!IsGibFlagActive(GIB_NO_ARM_RIGHT))
			return (Activity)ACT_CRAWL_ATTACK_RIGHT;

		if (!IsGibFlagActive(GIB_NO_ARM_LEFT))
			return (Activity)ACT_CRAWL_ATTACK_LEFT;

		return (Activity)ACT_CRAWL_IDLE;
	}

	randomAct = random->RandomInt(1, 3);
	if (!HasNoArms())
	{
		if (randomAct == 3)
			return (Activity)ACT_MELEE_ATTACK_HEAVY;
		else if (randomAct == 2)
			return (Activity)ACT_MELEE_ATTACK_RIGHT;
		else
			return (Activity)ACT_MELEE_ATTACK_LEFT;
	}

	if (!IsGibFlagActive(GIB_NO_ARM_RIGHT))
		return (Activity)ACT_MELEE_ATTACK_RIGHT;

	if (!IsGibFlagActive(GIB_NO_ARM_LEFT))
		return (Activity)ACT_MELEE_ATTACK_LEFT;

	return ACT_IDLE;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPCWalker::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	BaseClass::OnStateChange( OldState, NewState );
}

//---------------------------------------------------------
//---------------------------------------------------------

void CNPCWalker::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( random->RandomInt( 1, 4 ) == 2 )
			{
				SetIdealActivity( (Activity)ACT_ZOMBIE_TANTRUM );
			}
			else
			{
				TaskComplete();
			}

			break;
		}

	case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			if ( !GetEnemy() )
				TaskFail( FAIL_NO_ENEMY );
			else if ( GetNavigator()->SetVectorGoalFromTarget( GetEnemy()->GetLocalOrigin() ) )
			{
				m_vPositionCharged = GetEnemy()->GetLocalOrigin();
				TaskComplete();
			}
			else
				TaskFail( FAIL_NO_ROUTE );
			break;
		}

	case TASK_ZOMBIE_SPAWN:
		{
			m_bCheckCollisionGroupChange = true;
			TaskComplete();
			break;
		}

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

void CNPCWalker::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_SPAWN:
		break;
	case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			break;
		}

	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
// Zombies should scream continuously while burning, so long
// as they are alive... but NOT IN GERMANY!
//---------------------------------------------------------
void CNPCWalker::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	if( !IsOnFire() && IsAlive() )
	{
		BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

		if ( !UTIL_IsLowViolence() )
		{
			RemoveSpawnFlags( SF_NPC_GAG );

			MoanSound( envZombieMoanIgnited, ARRAYSIZE( envZombieMoanIgnited ) );

			if ( m_pMoanSound )
			{
				ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, 120, 1.0 );
				ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 1, 1.0 );
			}
		}
	}
}

//---------------------------------------------------------
// If a zombie stops burning and hasn't died, quiet him down
//---------------------------------------------------------
void CNPCWalker::Extinguish()
{
	if( m_pMoanSound )
	{
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 0, 2.0 );
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, 100, 2.0 );
		m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 4.0 );
	}

	BaseClass::Extinguish();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPCWalker::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	if ( !m_bIsReady )
		return 0;

	int ret = BaseClass::OnTakeDamage_Alive(inputInfo);

	return ret;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPCWalker::IsHeavyDamage( const CTakeDamageInfo &info )
{
	return BaseClass::IsHeavyDamage(info);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPCWalker::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();
}


//=============================================================================

AI_BEGIN_CUSTOM_NPC( npc_walker, CNPCWalker )

DECLARE_CONDITION( COND_ZOMBIE_CHARGE_TARGET_MOVED )

DECLARE_TASK( TASK_ZOMBIE_EXPRESS_ANGER )
DECLARE_TASK( TASK_ZOMBIE_CHARGE_ENEMY )
DECLARE_TASK(TASK_ZOMBIE_SPAWN)

DECLARE_ACTIVITY( ACT_ZOMBIE_TANTRUM );

DECLARE_ACTIVITY(ACT_MELEE_CHARGE);

DECLARE_ACTIVITY(ACT_MELEE_ATTACK_HEAVY);
DECLARE_ACTIVITY(ACT_MELEE_ATTACK_LEFT);
DECLARE_ACTIVITY(ACT_MELEE_ATTACK_RIGHT);

DECLARE_ACTIVITY(ACT_CRAWL);
DECLARE_ACTIVITY(ACT_CRAWL_IDLE);
DECLARE_ACTIVITY(ACT_CRAWL_ATTACK_LEFT);
DECLARE_ACTIVITY(ACT_CRAWL_ATTACK_RIGHT);

DECLARE_ACTIVITY(ACT_CRAWL_NOLEGS);
DECLARE_ACTIVITY(ACT_CRAWL_NOLEGS_IDLE);
DECLARE_ACTIVITY(ACT_CRAWL_NOLEGS_ATTACK_LEFT);
DECLARE_ACTIVITY(ACT_CRAWL_NOLEGS_ATTACK_RIGHT);

	DEFINE_SCHEDULE
	(
	SCHED_WALKER_RISE_IDLE,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_RISE_IDLE"
	""
	"	Interrupts"
	"		COND_TASK_FAILED"
	)

	DEFINE_SCHEDULE
	(
	SCHED_WALKER_RISE,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_RISE"
	"		TASK_ZOMBIE_SPAWN		0"
	""
	"	Interrupts"
	"		COND_TASK_FAILED"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_ANGRILY,

	"	Tasks"
	"		TASK_WANDER						480240" // 48 units to 240 units.
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			4"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_CHARGE_ENEMY,


	"	Tasks"
	"		TASK_ZOMBIE_CHARGE_ENEMY		0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ZOMBIE_TANTRUM" /* placeholder until frustration/rage/fence shake animation available */
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	"		COND_ZOMBIE_CHARGE_TARGET_MOVED"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_FAIL,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_ZOMBIE_TANTRUM"
	"		TASK_WAIT				1"
	"		TASK_WAIT_PVS			0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1 "
	"		COND_CAN_RANGE_ATTACK2 "
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_GIVE_WAY"
	)

	AI_END_CUSTOM_NPC()

	//=============================================================================
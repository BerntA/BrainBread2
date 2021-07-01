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
#include "engine/IEngineSound.h"
#include "GameBase_Server.h"
#include "hl2mp_gamerules.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum ZombieSpawningFlags
{
	ZOMBIE_SPAWN_FLAG_READY = 0x01,
	ZOMBIE_SPAWN_FLAG_CHECKCOLLISION = 0x02,
	ZOMBIE_SPAWN_FLAG_SPAWNED = 0x04,
	ZOMBIE_SPAWN_FLAG_RAPID_SPAWN = 0x08,
};

class CNPCWalker : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_CLASS( CNPCWalker, CAI_BlendingHost<CNPC_BaseZombie> );
	DEFINE_CUSTOM_AI;

public:
	CNPCWalker()
	{
		m_nZombieSpawnFlags = 0;
		m_bGibbedForCrawl = false;
	}

	void Spawn( void );
	void SpawnDirectly(void);
	Class_T Classify( void );
	const char *GetNPCName();
	int AllowEntityToBeGibbed(void);
	void MoanSound(void);
	int TranslateSchedule( int scheduleType );
	void CheckFlinches() {} // Zombie has custom flinch code

	Activity NPC_TranslateActivity( Activity newActivity );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	void Extinguish();
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );

	void PrescheduleThink(void);
	void PostscheduleThink(void);
	bool SpawnRunSchedule(CBaseEntity* pTarget, Activity act, bool pathcorner, int interruptType);
	int SelectSchedule ( void );
	int	SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot );

	void OnFullyFadedIn(void)
	{
		BaseClass::OnFullyFadedIn();
		m_nZombieSpawnFlags |= ZOMBIE_SPAWN_FLAG_READY;
	}

	bool CanAlwaysSeePlayers();
	bool UsesNavMesh(void) { return true; }

	bool IsAllowedToBreakDoors(void);
	bool ShouldUseNormalSpeedForSchedule(int scheduleType);

	void EnterCrawlMode(void);
	void LeaveCrawlMode(void);
	void BecomeCrawler(void);
	void HullChangeUnstuck(void);
	bool IsInCrawlMode(void)
	{
		return (IsCrawlingWithNoLegs() || m_bIsInCrawlMode || m_bGibbedForCrawl);
	}

	bool IsCrawlingWithNoLegs(void)
	{
		return (IsGibFlagActive(GIB_NO_LEG_RIGHT) || IsGibFlagActive(GIB_NO_LEG_LEFT));
	}

	bool HasNoArms(void)
	{
		return (IsGibFlagActive(GIB_NO_ARM_RIGHT) && IsGibFlagActive(GIB_NO_ARM_LEFT));
	}

	float MaxYawSpeed(void);

protected:

	bool IsZombieSpawnFlagActive(int flag)
	{
		return ((m_nZombieSpawnFlags & flag) != 0);
	}

	void OnGibbedGroup(int hitgroup, bool bExploded);

private:

	Activity GetAttackActivity(void);

	bool m_bIsRunner;
	bool m_bGibbedForCrawl;
	int m_nZombieSpawnFlags;
	float m_flNextTimeToCheckCollisionChange;

	// Target Script Schedule Pathing:
	EHANDLE m_hScriptTarget;
	Activity m_actScriptActivity;
	bool m_bScriptPathcorner;
	int m_iScriptInterruptType;
};

LINK_ENTITY_TO_CLASS(npc_walker, CNPCWalker);
LINK_ENTITY_TO_CLASS(npc_runner, CNPCWalker);

//=========================================================
// Schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_FAIL = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_SPAWN,
};

//=========================================================
// Tasks
//=========================================================
enum
{
	TASK_ZOMBIE_EXPRESS_ANGER = LAST_BASE_ZOMBIE_TASK,
	TASK_ZOMBIE_SPAWN_IDLE,
	TASK_ZOMBIE_SPAWN_RISE,
	TASK_ZOMBIE_SPAWNED,
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

Class_T	CNPCWalker::Classify( void )
{
	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_READY) == false)
		return CLASS_NONE;

	return CLASS_ZOMBIE;
}

const char *CNPCWalker::GetNPCName()
{
	if (m_bIsRunner)
		return "Runner";

	return "Walker";
}

int CNPCWalker::AllowEntityToBeGibbed(void)
{
	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_READY) == false)
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCWalker::Spawn( void )
{
	m_bIsRunner = FClassnameIs(this, "npc_runner");

	Precache();
	SetBloodColor( BLOOD_COLOR_RED );
	m_flFieldOfView = -1.0;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_CRAWL);

	BaseClass::Spawn();

	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 4.0 );

	m_flNextTimeToCheckCollisionChange = 0.0f;
	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE_SPAWNING);

	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_RAPID_SPAWN))
		m_nZombieSpawnFlags |= (ZOMBIE_SPAWN_FLAG_READY | ZOMBIE_SPAWN_FLAG_CHECKCOLLISION | ZOMBIE_SPAWN_FLAG_SPAWNED);
	else
		m_nZombieSpawnFlags &= ~(ZOMBIE_SPAWN_FLAG_READY | ZOMBIE_SPAWN_FLAG_CHECKCOLLISION | ZOMBIE_SPAWN_FLAG_SPAWNED);

	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);

	// Reduce zombies view dist:
	m_flDistTooFar = 500.0f;
	GetSenses()->SetDistLook(750.0f);
}

void CNPCWalker::SpawnDirectly(void)
{
	m_nZombieSpawnFlags |= ZOMBIE_SPAWN_FLAG_RAPID_SPAWN;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCWalker::PrescheduleThink( void )
{
	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_READY) && (GetCollisionGroup() == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING) && IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_CHECKCOLLISION))
	{
		if (gpGlobals->curtime > m_flNextTimeToCheckCollisionChange)
		{
			m_flNextTimeToCheckCollisionChange = gpGlobals->curtime + 1.0f; // Change to the new collision group as fast as possible.
			trace_t tr;
			CTraceFilterOnlyNPCsAndPlayer filter(this, COLLISION_GROUP_NPC_ZOMBIE);
			Vector vecCheckPos = GetLocalOrigin() + Vector(0, 0, 10);
			AI_TraceHull(vecCheckPos, vecCheckPos, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, &filter, &tr);
			if (!tr.startsolid)
			{
				SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE);
				m_flNextTimeToCheckCollisionChange = 0.0f;
				m_nZombieSpawnFlags &= ~(ZOMBIE_SPAWN_FLAG_CHECKCOLLISION);
			}
		}
	}

	if (gpGlobals->curtime > m_flNextMoanSound)
	{
		if (CanPlayMoanSound())
		{
			// Classic guy idles instead of moans.
			IdleSound();
			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat(2.0, 5.0);
		}
		else
			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat(1.0, 2.0);
	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCWalker::PostscheduleThink(void)
{
	BaseClass::PostscheduleThink();

	// Check if we want to run a 'late' schedule thing.
	CBaseEntity* pTarget = m_hScriptTarget.Get();
	if (pTarget && IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_SPAWNED) && (GetCollisionGroup() != COLLISION_GROUP_NPC_ZOMBIE_SPAWNING))
	{
		BaseClass::SpawnRunSchedule(pTarget, m_actScriptActivity, m_bScriptPathcorner, m_iScriptInterruptType);
		m_hScriptTarget = NULL;
	}
}

//-----------------------------------------------------------------------------
// Delay until we've fully spawned / faded in, etc.
//-----------------------------------------------------------------------------
bool CNPCWalker::SpawnRunSchedule(CBaseEntity* pTarget, Activity act, bool pathcorner, int interruptType)
{
	m_hScriptTarget = pTarget;
	m_actScriptActivity = act;
	m_bScriptPathcorner = pathcorner;
	m_iScriptInterruptType = interruptType;
	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPCWalker::SelectSchedule ( void )
{
	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_SPAWNED) == false)
		return SCHED_ZOMBIE_SPAWN;

	return BaseClass::SelectSchedule();
}

int	CNPCWalker::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	if (failedSchedule == SCHED_ZOMBIE_SPAWN)
		m_nZombieSpawnFlags |= (ZOMBIE_SPAWN_FLAG_CHECKCOLLISION | ZOMBIE_SPAWN_FLAG_SPAWNED);

	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
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
	return !IsInCrawlMode();
}

bool CNPCWalker::ShouldUseNormalSpeedForSchedule(int scheduleType)
{
	if (scheduleType == SCHED_ZOMBIE_SPAWN)
		return true;

	return BaseClass::ShouldUseNormalSpeedForSchedule(scheduleType);
}

void CNPCWalker::EnterCrawlMode(void)
{
	if (IsInCrawlMode())
		return;

	m_bIsInCrawlMode = true;

	bool bWasMoving = IsMoving();
	if (bWasMoving)
		GetMotor()->MoveStop();

	SetHullType(HULL_WIDE_SHORT);
	SetHullSizeSmall(true);
	SetDefaultEyeOffset();

	if (VPhysicsGetObject())
		SetupVPhysicsHull();

	HullChangeUnstuck();
	ResetIdealActivity((Activity)ACT_CRAWL_IDLE);

	if (bWasMoving)
		GetMotor()->MoveStart();
}

void CNPCWalker::LeaveCrawlMode(void)
{
	if (IsCrawlingWithNoLegs() || m_bGibbedForCrawl || !m_bIsInCrawlMode)
		return;

	trace_t trace;
	AI_TraceHull(GetAbsOrigin(), GetAbsOrigin(), Vector(-20, -20, 0), Vector(20, 20, 74), MASK_NPCSOLID, this, GetCollisionGroup(), &trace);
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
		SetupVPhysicsHull();

	UpdateMeleeRange(NAI_Hull::Bounds(GetHullType(), IsUsingSmallHull()));
	ResetIdealActivity(ACT_IDLE);

	if (bWasMoving)
		GetMotor()->MoveStart();
}

void CNPCWalker::BecomeCrawler(void)
{
	if (m_bGibbedForCrawl)
		return;

	bool bWasMoving = IsMoving();
	if (bWasMoving)
		GetMotor()->MoveStop();

	CapabilitiesRemove(bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CRAWL | bits_CAP_DOORS_GROUP); // We also remove the crawl flag because we can already crawl by default in this state.
	
	SetHullType(HULL_WIDE_SHORT);
	SetHullSizeSmall(true);
	SetDefaultEyeOffset();

	if (VPhysicsGetObject())	
		SetupVPhysicsHull();	

	HullChangeUnstuck();
	ResetIdealActivity((Activity)ACT_CRAWL_NOLEGS_IDLE);

	if (bWasMoving)
		GetMotor()->MoveStart();

	m_bGibbedForCrawl = true;
}

void CNPCWalker::HullChangeUnstuck(void)
{
	m_nZombieSpawnFlags |= ZOMBIE_SPAWN_FLAG_CHECKCOLLISION;
	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE_SPAWNING);

	const Vector &vMins = WorldAlignMins();
	const Vector &vMaxs = WorldAlignMaxs();
	const Vector &vOrigin = GetAbsOrigin();
	Vector vStart = vOrigin + Vector(0, 0, 1.0f + (vMaxs.z - vMins.z));

	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	AI_TraceHull(vStart, vStart + Vector(0, 0, -1.0f) * MAX_TRACE_LENGTH, vMins, vMaxs, MASK_SOLID_BRUSHONLY, &filter, &tr);

	SetGroundEntity(NULL);
	UTIL_SetOrigin(this, tr.endpos);
	SetGroundEntity(tr.m_pEnt, &tr);
	UpdateMeleeRange(NAI_Hull::Bounds(GetHullType(), IsUsingSmallHull()));
}

float CNPCWalker::MaxYawSpeed(void)
{
	if (IsInCrawlMode())
	{
		if ((GetActivity() == (Activity)ACT_CRAWL_NOLEGS_ATTACK_LEFT) ||
			(GetActivity() == (Activity)ACT_CRAWL_NOLEGS_ATTACK_RIGHT))
			return 15.0f;

		if ((GetActivity() == (Activity)ACT_CRAWL_ATTACK_LEFT) ||
			(GetActivity() == (Activity)ACT_CRAWL_ATTACK_RIGHT))
			return 15.0f;

		switch (GetActivity())
		{
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 15.0f;
		}

		return 10.0f;
	}

	if ((GetActivity() == (Activity)ACT_MELEE_ATTACK_HEAVY) ||
		(GetActivity() == (Activity)ACT_MELEE_ATTACK_LEFT) ||
		(GetActivity() == (Activity)ACT_MELEE_ATTACK_RIGHT))
		return 100.0f;

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
void CNPCWalker::MoanSound( void )
{
	if( IsOnFire() )
		BaseClass::MoanSound();
}

//---------------------------------------------------------
//---------------------------------------------------------

int CNPCWalker::TranslateSchedule( int scheduleType )
{
	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_SPAWNED) == false)
		return SCHED_ZOMBIE_SPAWN;

	if ( scheduleType == SCHED_COMBAT_FACE && IsUnreachable( GetEnemy() ) )
		return SCHED_TAKE_COVER_FROM_ENEMY;

	if ( scheduleType == SCHED_FAIL )
		return SCHED_ZOMBIE_FAIL;

	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------

Activity CNPCWalker::NPC_TranslateActivity( Activity newActivity )
{
	if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_SPAWNED) == false) // Don't translate any activities while spawning in!
		return newActivity;

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

void CNPCWalker::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if (random->RandomInt(1, 4) == 2)
				SetIdealActivity((Activity)ACT_ZOMBIE_TANTRUM);
			else			
				TaskComplete();						
		}
		break;

	case TASK_ZOMBIE_SPAWN_IDLE:
		SetIdealActivity(ACT_RISE_IDLE);
		break;

	case TASK_ZOMBIE_SPAWN_RISE:
		SetIdealActivity(ACT_RISE);
		break;

	case TASK_ZOMBIE_SPAWNED:
		{
			m_nZombieSpawnFlags |= (ZOMBIE_SPAWN_FLAG_CHECKCOLLISION | ZOMBIE_SPAWN_FLAG_SPAWNED);
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
	case TASK_ZOMBIE_SPAWN_IDLE:
	{
		if (IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_READY))
			TaskComplete();
		else if (IsActivityFinished())
			SetIdealActivity(ACT_RISE_IDLE);
	}
	break;

	case TASK_ZOMBIE_SPAWN_RISE:
	{
		if (IsActivityFinished())
			TaskComplete();
	}
	break;

	case TASK_ZOMBIE_SPAWNED:
		break;

	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( IsActivityFinished() )			
				TaskComplete();						
		}
		break;

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
			MoanSound();
		}
	}
}

//---------------------------------------------------------
// If a zombie stops burning and hasn't died, quiet him down
//---------------------------------------------------------
void CNPCWalker::Extinguish()
{
	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 4.0 );
	BaseClass::Extinguish();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPCWalker::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	if (!IsZombieSpawnFlagActive(ZOMBIE_SPAWN_FLAG_READY))
		return 0;

	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

AI_BEGIN_CUSTOM_NPC( npc_walker, CNPCWalker )

DECLARE_TASK(TASK_ZOMBIE_EXPRESS_ANGER)
DECLARE_TASK(TASK_ZOMBIE_SPAWN_IDLE)
DECLARE_TASK(TASK_ZOMBIE_SPAWN_RISE)
DECLARE_TASK(TASK_ZOMBIE_SPAWNED)

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
	SCHED_ZOMBIE_SPAWN,

	"	Tasks"
	"		TASK_ZOMBIE_SPAWN_IDLE	0"
	"		TASK_ZOMBIE_SPAWN_RISE	0"
	"		TASK_ZOMBIE_SPAWNED		0"
	""
	"	Interrupts"
	"		COND_TASK_FAILED"
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
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	)

	AI_END_CUSTOM_NPC()

	//=============================================================================
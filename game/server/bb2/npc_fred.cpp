//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A fast, strong, mutated & wild zombie.
//
//========================================================================================//

#include "cbase.h"
#include "npc_BaseZombie.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "engine/IEngineSound.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_npc_boss_fred_rage_damage("sk_npc_boss_fred_rage_damage", "80", FCVAR_GAMEDLL, "When Fred is taking damage higher or equal to this value he will insta rage.", true, 10.0f, true, 1000.0f);
ConVar sk_npc_boss_fred_rage_health("sk_npc_boss_fred_rage_health", "20", FCVAR_GAMEDLL, "When Fred has this much % left of his max health he will go into a rage mode.", true, 0.0f, true, 100.0f);
ConVar sk_npc_boss_fred_rage_duration("sk_npc_boss_fred_rage_duration", "40", FCVAR_GAMEDLL, "For how long should Fred rage?", true, 5.0f, true, 140.0f);
ConVar sk_npc_boss_fred_max_jump_height("sk_npc_boss_fred_max_jump_height", "240", FCVAR_GAMEDLL, "Set how high Fred can jump!", true, 80.0f, true, 500.0f);

class CNPCFred : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_CLASS( CNPCFred, CAI_BlendingHost<CNPC_BaseZombie> );
	DEFINE_CUSTOM_AI;

public:
	CNPCFred()
	{
	}

	void Spawn(void);
	int AllowEntityToBeGibbed(void);
	const char *GetNPCName() { return "Fred"; }
	BB2_SoundTypes GetNPCType() { return TYPE_FRED; }
	bool IsBoss() { return true; }
	bool CanAlwaysSeePlayers() { return true; }
    bool AllowedToIgnite( void ) { return false; }
	bool UsesNavMesh(void) { return true; }
	bool ShouldAlwaysThink() { return true; }
	Class_T Classify(void) { return CLASS_ZOMBIE_BOSS; }

	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int TranslateSchedule( int scheduleType );

	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
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

	bool GetGender() { return true; } // force male
	Activity NPC_TranslateActivity( Activity newActivity );

protected:
	float m_flRageTime;
	bool m_bIsInRageMode;

	void StartRageMode(void);
	void EndRageMode(void);
	void CheckRageState(void);

	float GetMaxJumpRise() const { return sk_npc_boss_fred_max_jump_height.GetFloat(); }
	float GetAmbushDist(void) { return 16000.0f; }
};

int ACT_RAGE;
int ACT_MELEE_ATTACK_RUN;

//=========================================================
// Schedules
//=========================================================
enum
{
	SCHED_FRED_RAGE_START = LAST_BASE_ZOMBIE_SCHEDULE,
};

LINK_ENTITY_TO_CLASS( npc_fred, CNPCFred );

int CNPCFred::AllowEntityToBeGibbed(void)
{
	return GIB_FULL_GIBS;
}

void CNPCFred::Spawn( void )
{
	Precache();

	SetBloodColor( BLOOD_COLOR_RED );

	m_flFieldOfView = 0.2;

	AddSpawnFlags( SF_NPC_LONG_RANGE );

	CapabilitiesClear();

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_JUMP );

	BaseClass::Spawn();

	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 4.0 );

	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE_BOSS);

	SetHullType(HULL_MEDIUM_TALL);
	SetHullSizeNormal(true);
	SetDefaultEyeOffset();

	m_flRageTime = 0.0f;
	m_bIsInRageMode = false;
}

Activity CNPCFred::NPC_TranslateActivity( Activity newActivity )
{
	Activity wantedActivity = BaseClass::NPC_TranslateActivity(newActivity);

	if (m_bIsInRageMode)
	{
		if (wantedActivity == ACT_MELEE_ATTACK1)
			return (Activity)ACT_MELEE_ATTACK_RUN;

		if (wantedActivity == ACT_WALK)
			return ACT_RUN;
	}
	else
	{
		if (wantedActivity == ACT_RUN)
			return ACT_WALK;
	}

	return wantedActivity;
}

void CNPCFred::PrescheduleThink( void )
{
	if( gpGlobals->curtime > m_flNextMoanSound )
	{
		if( CanPlayMoanSound() )
		{
			IdleSound();
			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 5.0 );
		}
		else
		{
			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 2.0 );
		}
	}

	CheckRageState();

	BaseClass::PrescheduleThink();
}

void CNPCFred::StartRageMode(void)
{
	if (m_bIsInRageMode)
		return;

	m_bIsInRageMode = true;
	m_flRageTime = gpGlobals->curtime + sk_npc_boss_fred_rage_duration.GetFloat();

	SetSchedule(SCHED_FRED_RAGE_START);
}

void CNPCFred::EndRageMode(void)
{
	if (!m_bIsInRageMode)
		return;

	m_bIsInRageMode = false;
	m_flRageTime = 0.0f;
}

void CNPCFred::CheckRageState(void)
{
	if (m_bIsInRageMode)
	{
		if (gpGlobals->curtime > m_flRageTime)
		{
			EndRageMode();
		}
	}
}

int CNPCFred::SelectSchedule ( void )
{
	if( HasCondition( COND_PHYSICS_DAMAGE ) && CanFlinch() )
	{
		return SCHED_FLINCH_PHYSICS;
	}

	return BaseClass::SelectSchedule();
}

void CNPCFred::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
		EmitSound(  "NPC_BaseZombie.FootstepRight" );
	else
		EmitSound( "NPC_BaseZombie.FootstepLeft" );
}

void CNPCFred::FootscuffSound( bool fRightFoot )
{
	if( fRightFoot )
		EmitSound( "NPC_BaseZombie.ScuffRight" );
	else
		EmitSound( "NPC_BaseZombie.ScuffLeft" );
}

void CNPCFred::AttackHitSound( void )
{
	EmitSound( "NPC_BaseZombie.AttackHit" );
}

void CNPCFred::AttackMissSound( void )
{
	// Play a random attack miss sound
	EmitSound( "NPC_BaseZombie.AttackMiss" );
}

void CNPCFred::PainSound( const CTakeDamageInfo &info )
{
	// We're constantly taking damage when we are on fire. Don't make all those noises!
	if ( IsOnFire() )
		return;

	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
}

void CNPCFred::DeathSound( const CTakeDamageInfo &info ) 
{
	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

void CNPCFred::AlertSound( void )
{
	HL2MPRules()->EmitSoundToClient(this, "Alert", GetNPCType(), GetGender());

	// Don't let a moan sound cut off the alert sound.
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

void CNPCFred::IdleSound( void )
{
	if( GetState() == NPC_STATE_IDLE && random->RandomFloat( 0, 1 ) == 0 )
	{
		// Moan infrequently in IDLE state.
		return;
	}

	HL2MPRules()->EmitSoundToClient(this, "Idle", GetNPCType(), GetGender());
	MakeAISpookySound( 360.0f );
}

void CNPCFred::AttackSound( void )
{
	HL2MPRules()->EmitSoundToClient(this, "Attack", GetNPCType(), GetGender());
}

int CNPCFred::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

int CNPCFred::TranslateSchedule( int scheduleType )
{
	return BaseClass::TranslateSchedule( scheduleType );
}

int CNPCFred::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	int ret = BaseClass::OnTakeDamage_Alive(inputInfo);

	float damageTaken = inputInfo.GetDamage();
	if (damageTaken >= sk_npc_boss_fred_rage_damage.GetFloat())
		StartRageMode();

	float flHealthPercent = ((float)(((float)GetHealth() / (float)GetMaxHealth())));
	if ((flHealthPercent * 100.0f) <= sk_npc_boss_fred_rage_health.GetFloat())
		StartRageMode();

	return ret;
}

void CNPCFred::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if( !IsCurSchedule( SCHED_FLINCH_PHYSICS ) )
	{
		SetCustomInterruptCondition( COND_PHYSICS_DAMAGE );
	}
}

AI_BEGIN_CUSTOM_NPC( npc_fred, CNPCFred )

DECLARE_ACTIVITY(ACT_RAGE);
DECLARE_ACTIVITY(ACT_MELEE_ATTACK_RUN);

DEFINE_SCHEDULE
(
SCHED_FRED_RAGE_START,

"	Tasks"
"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_RAGE"
""
"	Interrupts"
"		COND_TASK_FAILED"
)

AI_END_CUSTOM_NPC()
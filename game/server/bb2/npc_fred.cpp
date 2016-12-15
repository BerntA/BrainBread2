//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A fast, strong, mutated & wild zombie.
//
//========================================================================================//

#include "cbase.h"
#include "doors.h"
#include "simtimer.h"
#include "npc_BaseZombie.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "gib.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//=============================================================================

ConVar sk_npc_boss_fred_rage_damage("sk_npc_boss_fred_rage_damage", "80", FCVAR_REPLICATED, "When Fred is taking damage higher or equal to this value he will insta rage.", true, 10.0f, true, 1000.0f);
ConVar sk_npc_boss_fred_rage_health("sk_npc_boss_fred_rage_health", "20", FCVAR_REPLICATED, "When Fred has this much % left of his max health he will go into a rage mode.", true, 0.0f, true, 100.0f);
ConVar sk_npc_boss_fred_rage_duration("sk_npc_boss_fred_rage_duration", "40", FCVAR_REPLICATED, "For how long should Fred rage?", true, 5.0f, true, 140.0f);
ConVar sk_npc_boss_fred_max_jump_height("sk_npc_boss_fred_max_jump_height", "240", FCVAR_REPLICATED, "Set how high Fred can jump!", true, 80.0f, true, 500.0f);

class CNPCFred : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_CLASS( CNPCFred, CAI_BlendingHost<CNPC_BaseZombie> );
	DEFINE_CUSTOM_AI;

public:
	CNPCFred()
	{
	}

	void Spawn( void );
	void Precache( void );

	int AllowEntityToBeGibbed(void);
	const char *GetNPCName() { return "Fred"; }
	BB2_SoundTypes GetNPCType() { return TYPE_FRED; }
	bool IsBoss() { return true; }
	bool CanAlwaysSeePlayers() { return true; }
    bool AllowedToIgnite( void ) { return false; }
	virtual Class_T Classify(void);

	void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );

	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int TranslateSchedule( int scheduleType );

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

	bool GetGender() { return true; } // force male
	Activity NPC_TranslateActivity( Activity newActivity );

	const char *GetMoanSound( int nSound );

protected:
	static const char *pMoanSounds[];

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

//---------------------------------------------------------
//---------------------------------------------------------
const char *CNPCFred::pMoanSounds[] =
{
	"Moan1",
	"Moan2",
	"Moan3",
	"Moan4",
};

int CNPCFred::AllowEntityToBeGibbed(void)
{
	if (IsSlumped())
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCFred::Precache(void)
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPCFred::Classify(void)
{
	if (IsSlumped())
		return CLASS_NONE;

	return CLASS_ZOMBIE_BOSS;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCFred::PrescheduleThink( void )
{
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPCFred::SelectSchedule ( void )
{
	if( HasCondition( COND_PHYSICS_DAMAGE ) && !m_ActBusyBehavior.IsActive() )
	{
		return SCHED_FLINCH_PHYSICS;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CNPCFred::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
		EmitSound(  "NPC_BaseZombie.FootstepRight" );
	else
		EmitSound( "NPC_BaseZombie.FootstepLeft" );
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a foot sliding/scraping
//-----------------------------------------------------------------------------
void CNPCFred::FootscuffSound( bool fRightFoot )
{
	if( fRightFoot )
		EmitSound( "NPC_BaseZombie.ScuffRight" );
	else
		EmitSound( "NPC_BaseZombie.ScuffLeft" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CNPCFred::AttackHitSound( void )
{
	EmitSound( "NPC_BaseZombie.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CNPCFred::AttackMissSound( void )
{
	// Play a random attack miss sound
	EmitSound( "NPC_BaseZombie.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCFred::PainSound( const CTakeDamageInfo &info )
{
	// We're constantly taking damage when we are on fire. Don't make all those noises!
	if ( IsOnFire() )
		return;

	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCFred::DeathSound( const CTakeDamageInfo &info ) 
{
	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCFred::AlertSound( void )
{
	HL2MPRules()->EmitSoundToClient(this, "Alert", GetNPCType(), GetGender());

	// Don't let a moan sound cut off the alert sound.
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CNPCFred::GetMoanSound( int nSound )
{
	return pMoanSounds[ nSound % ARRAYSIZE( pMoanSounds ) ];
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CNPCFred::IdleSound( void )
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
void CNPCFred::AttackSound( void )
{
	HL2MPRules()->EmitSoundToClient(this, "Attack", GetNPCType(), GetGender());
}

//---------------------------------------------------------
// Classic zombie only uses moan sound if on fire.
//---------------------------------------------------------
void CNPCFred::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( IsOnFire() )
		BaseClass::MoanSound( pEnvelope, iEnvelopeSize );
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPCFred::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//---------------------------------------------------------
//---------------------------------------------------------

int CNPCFred::TranslateSchedule( int scheduleType )
{
	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------
//---------------------------------------------------------
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPCFred::IsHeavyDamage( const CTakeDamageInfo &info )
{
	return BaseClass::IsHeavyDamage(info);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPCFred::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if( !IsCurSchedule( SCHED_FLINCH_PHYSICS ) && !m_ActBusyBehavior.IsActive() )
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
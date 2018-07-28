//=========       Copyright © Reperio Studios 2013-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie NPC BaseClass
//
//==============================================================================================//

#ifndef NPC_BASEZOMBIE_H
#define NPC_BASEZOMBIE_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_blended_movement.h"
#include "ai_behavior_actbusy.h"
#include "npc_base_properties.h"

extern int AE_ZOMBIE_ATTACK_RIGHT;
extern int AE_ZOMBIE_ATTACK_LEFT;
extern int AE_ZOMBIE_ATTACK_BOTH;
extern int AE_ZOMBIE_STEP_LEFT;
extern int AE_ZOMBIE_STEP_RIGHT;
extern int AE_ZOMBIE_SCUFF_LEFT;
extern int AE_ZOMBIE_SCUFF_RIGHT;
extern int AE_ZOMBIE_ATTACK_SCREAM;
extern int AE_ZOMBIE_GET_UP;
extern int AE_ZOMBIE_POUND;

// Pass these to claw attack so we know where to draw the blood.
#define ZOMBIE_BLOOD_LEFT_HAND		0
#define ZOMBIE_BLOOD_RIGHT_HAND		1
#define ZOMBIE_BLOOD_BOTH_HANDS		2
#define ZOMBIE_BLOOD_BITE			3

//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_ZOMBIE_MOVE_TO_AMBUSH,
	SCHED_ZOMBIE_WAIT_AMBUSH,
	SCHED_ZOMBIE_WANDER_MEDIUM,	// medium range wandering behavior.
	SCHED_ZOMBIE_WANDER_FAIL,
	SCHED_ZOMBIE_WANDER_STANDOFF,
	SCHED_ZOMBIE_MELEE_ATTACK1,
	SCHED_ZOMBIE_BASH_DOOR,

	LAST_BASE_ZOMBIE_SCHEDULE,
};

//=========================================================
// tasks
//=========================================================
enum
{
	TASK_ZOMBIE_YAW_TO_DOOR = LAST_SHARED_TASK,
	TASK_ZOMBIE_ATTACK_DOOR,
	TASK_ZOMBIE_PATH_TO_OBSTRUCTION,
	TASK_ZOMBIE_OBSTRUCTION_CLEARED,

	LAST_BASE_ZOMBIE_TASK,
};

//=========================================================
// conditions
//=========================================================
enum 
{
	COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION = LAST_SHARED_CONDITION,
	COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT,

	LAST_BASE_ZOMBIE_CONDITION,
};

typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BaseNPC> > CAI_BaseZombieBase;

abstract_class CNPC_BaseZombie : public CAI_BaseZombieBase, public CNPCBaseProperties
{
	DECLARE_CLASS(CNPC_BaseZombie, CAI_BaseZombieBase);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

public:
	CNPC_BaseZombie(void);
	virtual ~CNPC_BaseZombie(void);

	void Spawn(void);
	void Precache(void);
	virtual float MaxYawSpeed(void);
	bool OverrideMoveFacing(const AILocalMoveGoal_t &move, float flInterval);
	virtual Class_T Classify(void);
	void HandleAnimEvent(animevent_t *pEvent);
	void OnStateChange(NPC_STATE OldState, NPC_STATE NewState);
	void KillMe(void)
	{
		m_iHealth = 5;
		OnTakeDamage(CTakeDamageInfo(this, this, m_iHealth * 2, DMG_GENERIC));
	}

	int MeleeAttack1Conditions(float flDot, float flDist);
	virtual float GetClawAttackRange() const { return m_flRange; }
	virtual bool CanFlinch(void) { return false; }

	// No range attacks
	int RangeAttack1Conditions(float flDot, float flDist) { return 0; }

	virtual int OnTakeDamage_Alive(const CTakeDamageInfo &info);
	virtual float GetReactionDelay(CBaseEntity *pEnemy) { return 0.0f; }

	virtual int SelectSchedule(void);
	virtual int	SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	virtual void BuildScheduleTestBits(void);

	virtual int TranslateSchedule(int scheduleType);
	virtual Activity NPC_TranslateActivity(Activity baseAct);

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

	void GatherConditions(void);
	void PrescheduleThink(void);

	virtual void Event_Killed(const CTakeDamageInfo &info);
	virtual void OnScheduleChange(void);
	virtual void PoundSound();

	// Custom damage/death 
	bool ShouldIgnite(const CTakeDamageInfo &info);
	bool ShouldIgniteZombieGib(void);
	virtual void Ignite(float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false);
	void CopyRenderColorTo(CBaseEntity *pOther);

	// Life Span Accessor:
	float GetLifeSpan(void) { return m_flSpawnTime; }

	// Returns whether we must be very near our enemy to attack them.
	virtual bool MustCloseToAttack(void) { return true; }

	virtual CBaseEntity *ClawAttack(float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin);

	// Sounds
	virtual void PainSound(const CTakeDamageInfo &info) = 0;
	virtual void AlertSound(void) = 0;
	virtual void IdleSound(void) = 0;
	virtual void AttackSound(void) = 0;
	virtual void AttackHitSound(void) = 0;
	virtual void AttackMissSound(void) = 0;
	virtual void FootstepSound(bool fRightFoot) = 0;
	virtual void FootscuffSound(bool fRightFoot) = 0;

	void		MakeAISpookySound(float volume, float duration = 0.5);

	virtual bool ValidateNavGoal() { return true; }
	virtual bool CanPlayMoanSound();
	virtual void MoanSound(void);
	bool ShouldPlayIdleSound(void) { return false; }

	virtual Vector BodyTarget(const Vector &posSrc, bool bNoisy);
	virtual void TranslateNavGoal(CBaseEntity *pEnemy, Vector &chasePosition);

	virtual	bool		AllowedToIgnite(void) { return true; }
	virtual bool		OverrideShouldAddToLookList(CBaseEntity *pEntity);

	virtual float		GetIdealSpeed() const;
	virtual float		GetIdealAccel() const;

protected:

	float	m_flNextFlinch;

	// Zombies catch on fire if they take too much burn damage in a given time period.
	float	m_flBurnDamage;				// Keeps track of how much burn damage we've incurred in the last few seconds.
	float	m_flBurnDamageResetTime;	// Time at which we reset the burn damage.

	float m_flNextMoanSound;

	virtual BB2_SoundTypes GetNPCType() { return TYPE_ZOMBIE; }
	virtual Activity SelectDoorBash();
	virtual bool IsAllowedToBreakDoors(void) { return true; }
	virtual float GetAmbushDist(void) { return 16000.0f; }

	EHANDLE m_hLastIgnitionSource;
	EHANDLE m_hBlockingEntity;

	CRandSimTimer 		 m_DurationDoorBash;
	CSimTimer 	  		 m_NextTimeToStartDoorBash;

private:

	float m_flLastObstructionCheck;
	float m_flSpawnTime;
	bool m_bLifeTimeOver;
};

extern int g_pZombiesInWorld;

#endif // NPC_BASEZOMBIE_H
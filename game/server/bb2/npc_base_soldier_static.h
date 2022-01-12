//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Stationary Soldier AI
//
//========================================================================================//

#ifndef NPC_BASE_SOLDIER_STATIC_H
#define NPC_BASE_SOLDIER_STATIC_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_basehumanoid.h"
#include "ai_behavior.h"
#include "ai_behavior_functank.h"
#include "ai_behavior_rappel.h"
#include "ai_baseactor.h"
#include "npc_base_properties.h"

class CNPCBaseSoldierStatic : public CAI_BaseActor, public CNPCBaseProperties
{
	DEFINE_CUSTOM_AI;
	DECLARE_CLASS(CNPCBaseSoldierStatic, CAI_BaseActor);

public:
	CNPCBaseSoldierStatic();

	virtual bool	CanThrowGrenade(const Vector &vecTarget);
	virtual bool	CheckCanThrowGrenade(const Vector &vecTarget);
	virtual	bool	CanGrenadeEnemy(bool bUseFreeKnowledge = true);
	int				RangeAttack2Conditions(float flDot, float flDist); // For innate grenade attack
	int				MeleeAttack1Conditions(float flDot, float flDist); // For kick/punch

	void Event_Killed(const CTakeDamageInfo &info);

	void SetActivity(Activity NewActivity);
	NPC_STATE		SelectIdealState(void);

	void			Spawn(void);
	void			Precache(void);

	virtual Class_T Classify(void);
	float			MaxYawSpeed(void);

	bool			ShouldMoveAndShoot() { return false; }
	bool			UsesNavMesh(void) { return false; }
	bool			IsStaticNPC(void) { return true; }

	void			HandleAnimEvent(animevent_t *pEvent);

	Vector			Weapon_ShootPosition();
	Vector			EyeOffset(Activity nActivity);
	Vector			EyePosition(void);

	void			StartTask(const Task_t *pTask);
	void			RunTask(const Task_t *pTask);

	virtual void GatherConditions();
	virtual void PrescheduleThink();

	virtual void FireBullets(const FireBulletsInfo_t &info);
	virtual int AllowEntityToBeGibbed(void);
	virtual bool CanFlinch(void) { return false; }

	Activity		NPC_TranslateActivity(Activity eNewActivity);
	void			BuildScheduleTestBits(void);
	virtual int		SelectSchedule(void);
	virtual int		SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	int				SelectScheduleAttack();

	// Combat
	bool			ActiveWeaponIsFullyLoaded();

	// -------------
	// Sounds
	// -------------
	void			DeathSound(void);
	void			PainSound(void);
	void			IdleSound(void);
	void			AlertSound(void);
	void			LostEnemySound(void);
	void			FoundEnemySound(void);
	void			AnnounceEnemyType(CBaseEntity *pEnemy);
	void			AnnounceEnemyKill(CBaseEntity *pEnemy);
	void			NotifyDeadFriend(CBaseEntity* pFriend);

	virtual float	HearingSensitivity(void) { return 1.0; };
	int				GetSoundInterests(void);
	virtual bool	QueryHearSound(CSound *pSound);

	virtual int		TranslateSchedule(int scheduleType);
	virtual void    OnStartSchedule(int scheduleType);

protected:
	virtual BB2_SoundTypes GetNPCType() { return TYPE_SOLDIER; }
	void SetKickDamage(int nDamage) { m_nKickDamage = nDamage; }

private:
	//=========================================================
	// Soldier schedules
	//=========================================================
	enum
	{
		SCHED_SOLDIER_STATIC_SUPPRESS = BaseClass::NEXT_SCHEDULE,
		SCHED_SOLDIER_STATIC_COMBAT_FAIL,
		SCHED_SOLDIER_STATIC_COMBAT_FACE,
		SCHED_SOLDIER_STATIC_RANGE_ATTACK1,
		SCHED_SOLDIER_STATIC_RANGE_ATTACK2,
		SCHED_SOLDIER_STATIC_RANGE_RELOAD,
		SCHED_SOLDIER_STATIC_FACE_IDEAL_YAW,
		SCHED_SOLDIER_STATIC_RESET,
		NEXT_SCHEDULE,
	};

	//=========================================================
	// Soldier Tasks
	//=========================================================
	enum
	{
		TASK_SOLDIER_STATIC_FACE_TOSS_DIR = BaseClass::NEXT_TASK,
		TASK_SOLDIER_STATIC_IGNORE_ATTACKS,
		TASK_SOLDIER_STATIC_DEFER_SQUAD_GRENADES,
		TASK_SOLDIER_STATIC_CLEAR_ENEMY_STATES,
		NEXT_TASK
	};

	//=========================================================
	// Soldier Conditions
	//=========================================================
	enum 
	{
		COND_SOLDIER_STATIC_NO_FIRE = BaseClass::NEXT_CONDITION,
		COND_SOLDIER_STATIC_ATTACK_SLOT_AVAILABLE,
		NEXT_CONDITION
	};

	// Select the combat schedule
	int SelectCombatSchedule();

	int				m_nKickDamage;
	Vector			m_vecTossVelocity;

	// Time Variables
	float			m_flNextPainSoundTime;
	float			m_flNextAlertSoundTime;
	float			m_flNextGrenadeCheck;
	float			m_flNextLostSoundTime;

	int				m_nShots;
	float			m_flShotDelay;

	int			m_iNumGrenades;

public:
	int m_iLastAnimEventHandled;
};

#endif // NPC_BASE_SOLDIER_STATIC_H
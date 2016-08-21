//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom ally/enemy/villain actor NPC.
//
//========================================================================================//

#ifndef	NPC_CUSTOM_ACTOR_H
#define	NPC_CUSTOM_ACTOR_H

#include "npc_playercompanion.h"
#include "ai_behavior_functank.h"

//-------------------------------------
// Spawnflags
//-------------------------------------

#define SF_CITIZEN_FOLLOW			( 1 << 16 )	//65536 follow the player as soon as I spawn.
#define SF_CITIZEN_NOT_COMMANDABLE	( 1 << 20 ) // Nope.
#define SF_CITIZEN_IGNORE_SEMAPHORE ( 1 << 21 ) // Work outside the speech semaphore system
#define SF_CITIZEN_USE_RENDER_BOUNDS ( 1 << 24 )//16777216

//-------------------------------------

class CNPC_CustomActor : public CNPC_PlayerCompanion
{
	DECLARE_CLASS(CNPC_CustomActor, CNPC_PlayerCompanion);
public:
	CNPC_CustomActor();

	//---------------------------------
	bool			CreateBehaviors();
	void			Precache();
	void			Spawn();
	void			PostNPCInit();
	void			Activate();
	void	        OnGivenWeapon(CBaseCombatWeapon *pNewWeapon);

	float	        GetJumpGravity() const { return 1.8f; }

	void			OnRestore();
	void            Event_Killed(const CTakeDamageInfo &info);

	Class_T 		Classify();

	bool 			ShouldAlwaysThink();
	bool            GetGender() { return m_bGender; }
	bool            UsesNavMesh(void) { return true; }

	//---------------------------------
	// Behavior
	//---------------------------------
	bool			ShouldBehaviorSelectSchedule(CAI_BehaviorBase *pBehavior);
	void 			GatherConditions();
	void			PredictPlayerPush();
	void 			PrescheduleThink();
	void			BuildScheduleTestBits();

	bool			FInViewCone(CBaseEntity *pEntity);

	int				SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	int				SelectSchedule();

	int 			SelectSchedulePriorityAction();
	int 			SelectScheduleRetrieveItem();
	int 			SelectScheduleNonCombat();
	int 			SelectScheduleCombat();
	bool			ShouldDeferToFollowBehavior();
	int 			TranslateSchedule(int scheduleType);

	void 			StartTask(const Task_t *pTask);
	void 			RunTask(const Task_t *pTask);

	Activity		NPC_TranslateActivity(Activity eNewActivity);
	void 			HandleAnimEvent(animevent_t *pEvent);
	void			TaskFail(AI_TaskFailureCode_t code);

	void 			PickupItem(CBaseEntity *pItem);
	void 			SimpleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	bool			IgnorePlayerPushing(void);

	const char *SelectRandomExpressionForState(NPC_STATE state);

	//---------------------------------
	// Combat
	//---------------------------------
	bool 			OnBeginMoveAndShoot();
	void 			OnEndMoveAndShoot();

	bool	        UseAttackSquadSlots()	{ return false; }
	void 			LocateEnemySound();

	Vector 			GetActualShootPosition(const Vector &shootOrigin);
	void 			OnChangeActiveWeapon(CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon);

	bool			ShouldLookForBetterWeapon();

	//---------------------------------
	// Damage handling
	//---------------------------------
	int 			OnTakeDamage_Alive(const CTakeDamageInfo &info);

	//---------------------------------
	// Following Logic:
	//---------------------------------
	bool 			IsFollowingTarget();
	bool			CanFollowTarget();
	void 			CommanderUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	//---------------------------------
	// Hints
	//---------------------------------
	bool			FValidateHintType(CAI_Hint *pHint);

	//---------------------------------
	// Inputs
	//---------------------------------
	void 			InputStartPatrolling(inputdata_t &inputdata);
	void 			InputStopPatrolling(inputdata_t &inputdata);
	void 			InputSetCommandable(inputdata_t &inputdata);

	//---------------------------------
	//	Sounds & speech
	//---------------------------------
	void			FearSound(void);
	void			DeathSound(const CTakeDamageInfo &info);
	bool			UseSemaphore(void);

	void	OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior);

	void    PlaySound(const char *sound, float eventtime = -1.0f);

private:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		SCHED_CITIZEN_PATROL = BaseClass::NEXT_SCHEDULE,
		SCHED_CITIZEN_MOURN_PLAYER,

		TASK_CIT_SPEAK_MOURNING = BaseClass::NEXT_TASK,
	};

	//-----------------------------------------------------

	// Customization
	string_t m_iszNPCName;
	string_t m_iszSoundScriptBase;
	bool m_bGender;
	bool m_bBossState;
	int             m_iTotalHP;
	bool            m_bIsAlly;

	bool			m_bShouldPatrol;
	float			m_flTimeLastCloseToPlayer;
	float			m_flTimePlayerStare;	// The game time at which the player started staring at me.
	float           m_flLastFollowTargetCheck;

	// Sound Logic
	int m_iTimesGreeted;

	//-----------------------------------------------------
	//	Outputs
	//-----------------------------------------------------
	COutputEvent		m_OnPlayerUse;
	COutputEvent		m_OnNavFailBlocked;

	//-----------------------------------------------------
	CAI_FuncTankBehavior	m_FuncTankBehavior;

	bool					m_bNotifyNavFailBlocked;

	//-----------------------------------------------------

	DECLARE_DATADESC();
#ifdef _XBOX
protected:
#endif
	DEFINE_CUSTOM_AI;
};

#endif // NPC_CUSTOM_ACTOR_H
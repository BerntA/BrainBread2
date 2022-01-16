//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Soldier Class AI.
//
//==============================================================================================//

#ifndef NPC_BASE_SOLDIER_H
#define NPC_BASE_SOLDIER_H
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

// Used when only what soldier to react to what the spotlight sees
#define SF_SOLDIER_NO_LOOK	(1 << 16)

class CNPC_BaseSoldier : public CAI_BaseActor, public CNPCBaseProperties
{
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
	DECLARE_CLASS( CNPC_BaseSoldier, CAI_BaseActor );

public:
	CNPC_BaseSoldier();

	virtual bool	CanThrowGrenade( const Vector &vecTarget );
	virtual bool	CheckCanThrowGrenade( const Vector &vecTarget );
	virtual	bool	CanGrenadeEnemy( bool bUseFreeKnowledge = true );
	int				RangeAttack2Conditions( float flDot, float flDist ); // For innate grenade attack
	int				MeleeAttack1Conditions( float flDot, float flDist ); // For kick/punch
	bool			FVisible( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	virtual bool	IsCurTaskContinuousMove();

	virtual float GetJumpGravity() const	{ return 1.8f; }
	virtual bool CanFlinch(void) { return false; }
	virtual float CoverRadius(void) { return 700.0f; } // Default cover radius
	virtual bool IsCrouchedActivity(Activity activity) { return false; }
	virtual bool ValidateNavGoal() { return true; }
	virtual float MinFleeDistance(void) { return 20.0f; }

	virtual Vector  GetCrouchEyeOffset( void );

	virtual int OnTakeDamage_Alive(const CTakeDamageInfo &info);
	virtual void Event_Killed( const CTakeDamageInfo &info );

	void SetActivity( Activity NewActivity );
	NPC_STATE		SelectIdealState ( void );

	// Input handlers.
	void InputLookOn( inputdata_t &inputdata );
	void InputLookOff( inputdata_t &inputdata );
	void InputStartPatrolling( inputdata_t &inputdata );
	void InputStopPatrolling( inputdata_t &inputdata );
	void InputAssault( inputdata_t &inputdata );
	void InputThrowGrenadeAtTarget( inputdata_t &inputdata );

	virtual bool	UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer = NULL );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	void			Activate();

	virtual Class_T Classify( void );
	float			MaxYawSpeed( void );
	bool			ShouldMoveAndShoot();
	bool			OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	void			HandleAnimEvent( animevent_t *pEvent );
	Vector			Weapon_ShootPosition( );

	Vector			EyeOffset( Activity nActivity );
	Vector			EyePosition( void );
	Vector			BodyTarget( const Vector &posSrc, bool bNoisy = true );

	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	void			PostNPCInit();
	virtual void	GatherConditions();
	virtual void	PrescheduleThink();
	virtual void	FireBullets(const FireBulletsInfo_t &info);
	virtual int		AllowEntityToBeGibbed(void);

	Activity		NPC_TranslateActivity( Activity eNewActivity );
	virtual void	BuildScheduleTestBits( void );
	virtual int		SelectSchedule( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int				SelectScheduleAttack();

	bool			CreateBehaviors();

	bool			OnBeginMoveAndShoot();
	void			OnEndMoveAndShoot();

	// Combat
	bool			ActiveWeaponIsFullyLoaded();

	const char*		GetSquadSlotDebugName( int iSquadSlot );

	bool			IsUsingTacticalVariant( int variant );
	bool			IsUsingPathfindingVariant( int variant ) { return m_iPathfindingVariant == variant; }

	bool			IsRunningApproachEnemySchedule();

	virtual bool	IsLightDamage(const CTakeDamageInfo &info);
	virtual bool	IsHeavyDamage(const CTakeDamageInfo &info);

	// -------------
	// Sounds
	// -------------
	virtual void			DeathSound(void);
	virtual void			PainSound(void);
	virtual void			IdleSound(void);
	virtual void			AlertSound(void);
	virtual void			LostEnemySound(void);
	virtual void			FoundEnemySound(void);
	virtual void			AnnounceAssault(void);
	virtual void			AnnounceEnemyType(CBaseEntity *pEnemy);
	virtual void			AnnounceEnemyKill( CBaseEntity *pEnemy );

	virtual void			NotifyDeadFriend(CBaseEntity* pFriend);

	virtual float	HearingSensitivity( void ) { return 1.0; };
	int				GetSoundInterests( void );
	virtual bool	QueryHearSound( CSound *pSound );

	virtual int		TranslateSchedule( int scheduleType );
	virtual void	OnStartSchedule( int scheduleType );

	virtual int GetIdleState(void) { return 0; }
	virtual void SetIdleState(int state) {}

	virtual float		GetIdealSpeed() const;
	virtual float		GetIdealAccel() const;

protected:

	int m_iNumGrenades;
	float m_flNextGrenadeCheck;
	virtual BB2_SoundTypes GetNPCType() { return TYPE_SOLDIER; }
	void SetKickDamage( int nDamage ) { m_nKickDamage = nDamage; }

	// Select the combat schedule
	virtual int SelectCombatSchedule();

	// Should we charge the player?
	virtual bool ShouldChargePlayer();

private:
	//=========================================================
	// Soldier schedules
	//=========================================================
	enum
	{
		SCHED_SOLDIER_SUPPRESS = BaseClass::NEXT_SCHEDULE,
		SCHED_SOLDIER_COMBAT_FAIL,
		SCHED_SOLDIER_HIDE_AND_RELOAD,
		SCHED_SOLDIER_ASSAULT,
		SCHED_SOLDIER_ESTABLISH_LINE_OF_FIRE,
		SCHED_SOLDIER_PRESS_ATTACK,
		SCHED_SOLDIER_WAIT_IN_COVER,
		SCHED_SOLDIER_RANGE_ATTACK1,
		SCHED_SOLDIER_RANGE_ATTACK2,
		SCHED_SOLDIER_TAKE_COVER1,
		SCHED_SOLDIER_TAKE_COVER_FROM_BEST_SOUND,
		SCHED_SOLDIER_RUN_AWAY_FROM_BEST_SOUND,
		SCHED_SOLDIER_GRENADE_COVER1,
		SCHED_SOLDIER_TOSS_GRENADE_COVER1,
		SCHED_SOLDIER_TAKECOVER_FAILED,
		SCHED_SOLDIER_GRENADE_AND_RELOAD,
		SCHED_SOLDIER_PATROL,
		SCHED_SOLDIER_CHARGE_PLAYER,
		SCHED_SOLDIER_PATROL_ENEMY,
		SCHED_SOLDIER_FORCED_GRENADE_THROW,
		SCHED_SOLDIER_MOVE_TO_FORCED_GREN_LOS,
		SCHED_SOLDIER_FACE_IDEAL_YAW,
		SCHED_SOLDIER_MOVE_TO_MELEE,
		NEXT_SCHEDULE,
	};

	//=========================================================
	// Soldier Tasks
	//=========================================================
	enum 
	{
		TASK_SOLDIER_FACE_TOSS_DIR = BaseClass::NEXT_TASK,
		TASK_SOLDIER_IGNORE_ATTACKS,
		TASK_SOLDIER_DEFER_SQUAD_GRENADES,
		TASK_SOLDIER_CHASE_ENEMY_CONTINUOUSLY,
		TASK_SOLDIER_GET_PATH_TO_FORCED_GREN_LOS,
		NEXT_TASK
	};

	//=========================================================
	// Soldier Conditions
	//=========================================================
	enum 
	{
		COND_SOLDIER_SHOULD_PATROL = BaseClass::NEXT_CONDITION,
		COND_SOLDIER_ATTACK_SLOT_AVAILABLE,
		NEXT_CONDITION
	};

private:

	// Chase the enemy, updating the target position as the player moves
	void StartTaskChaseEnemyContinuously( const Task_t *pTask );
	void RunTaskChaseEnemyContinuously( const Task_t *pTask );

	// Rappel
	virtual bool IsWaitingToRappel( void ) { return m_RappelBehavior.IsWaitingToRappel(); }
	void BeginRappel() { m_RappelBehavior.BeginRappel(); }

private:

	int				m_nKickDamage;
	Vector			m_vecTossVelocity;
	EHANDLE			m_hForcedGrenadeTarget;
	bool			m_bShouldPatrol;

	// Time Variables
	float			m_flNextPainSoundTime;
	float			m_flNextAlertSoundTime;
	float			m_flNextLostSoundTime;
	float			m_flAlertPatrolTime;		// When to stop doing alert patrol

	int				m_nShots;
	float			m_flShotDelay;
	float			m_flStopMoveShootTime;
	float			m_flLastTimeRanForCover;

protected:
	CAI_FuncTankBehavior		m_FuncTankBehavior;
	CAI_RappelBehavior			m_RappelBehavior;

public:
	int				m_iLastAnimEventHandled;
	int				m_iTacticalVariant;
	int				m_iPathfindingVariant;
};

#endif // NPC_BASE_SOLDIER_H
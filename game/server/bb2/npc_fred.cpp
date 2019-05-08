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
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_npc_boss_fred_rage_damage("sk_npc_boss_fred_rage_damage", "80", FCVAR_GAMEDLL, "When Fred is taking damage higher or equal to this value he will insta rage.", true, 10.0f, true, 1000.0f);
ConVar sk_npc_boss_fred_rage_health("sk_npc_boss_fred_rage_health", "20", FCVAR_GAMEDLL, "When Fred has this much % left of his max health he will go into a rage mode.", true, 0.0f, true, 100.0f);
ConVar sk_npc_boss_fred_rage_duration("sk_npc_boss_fred_rage_duration", "40", FCVAR_GAMEDLL, "For how long should Fred rage?", true, 5.0f, true, 140.0f);

ConVar sk_npc_boss_fred_max_jump_height("sk_npc_boss_fred_max_jump_height", "240", FCVAR_GAMEDLL, "Set how high Fred can jump!", true, 80.0f, true, 500.0f);

ConVar sk_npc_boss_fred_rage_blastdmg("sk_npc_boss_fred_rage_blastdmg", "50", FCVAR_GAMEDLL | FCVAR_CHEAT, "When Fred enters rage mode how much radius damage % should he do?", true, 10.0f, true, 100.0f);
ConVar sk_npc_boss_fred_camp_dmg("sk_npc_boss_fred_camp_dmg", "20", FCVAR_GAMEDLL | FCVAR_CHEAT, "When someone stands on Fred's head, how much % of their total HP should they lose as punishment?", true, 10.0f, true, 100.0f);

ConVar sk_npc_boss_fred_navmesh("sk_npc_boss_fred_navmesh", "1", FCVAR_GAMEDLL, "Should Fred use navmesh?", true, 0, true, 1);
ConVar sk_npc_boss_fred_regenrate("sk_npc_boss_fred_regenrate", "0.135", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set the health regen rate for Fred.", true, 0.0f, true, 100.0f);

#define CAMPER_CHECK_TIME 2.5f
#define CAMPER_MAX_LAST_TIME_SEEN 20.0f // SEC
#define CAMPER_Z_DIFF_MIN 100.0f
#define CAMPER_MAX_DISTANCE 500.0f
#define CAMPER_SHOCKWAVE_RADIUS 175.0f
#define CAMPER_VELOCITY_PUNCH 1500.0f
#define CAMPER_VELOCITY_PUNCH_Z 550.0f

#define SHOCKWAVE_COOLDOWN random->RandomFloat(22.0f, 33.0f)

class CNPCFred : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_CLASS( CNPCFred, CAI_BlendingHost<CNPC_BaseZombie> );
	DEFINE_CUSTOM_AI;

public:
	CNPCFred()
	{
		m_flLastCamperCheck = m_flRageTime = 0.0f;
		m_bIsInRageMode = false;
		m_hTargetPlayer = NULL;
	}

	virtual ~CNPCFred()
	{
		m_hPlayersOnMyHead.RemoveAll();
	}

	void Precache(void)
	{
		PrecacheParticleSystem("bb2_healing_effect");
		PrecacheScriptSound("Fred.Shockwave");
		BaseClass::Precache();
	}

	void Spawn(void);
	int AllowEntityToBeGibbed(void);
	const char *GetNPCName() { return "Fred"; }
	BB2_SoundTypes GetNPCType() { return TYPE_FRED; }
	bool IsBoss() { return true; }
	bool CanAlwaysSeePlayers() { return true; }
    bool AllowedToIgnite( void ) { return false; }
	bool UsesNavMesh(void) { return sk_npc_boss_fred_navmesh.GetBool(); }
	bool ShouldAlwaysThink() { return true; }
	Class_T Classify(void) { return CLASS_ZOMBIE_BOSS; }

	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int TranslateSchedule( int scheduleType );

	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	void BuildScheduleTestBits( void );

	void PrescheduleThink(void);
	void PostscheduleThink(void);

	int SelectSchedule ( void );
	void OnStartSchedule(int scheduleType);

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);

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
	void		OnChangeActivity(Activity eNewActivity);

	bool CanRegenHealth(void) { return true; }
	float GetHealthRegenRate(void) { return sk_npc_boss_fred_regenrate.GetFloat(); }

	void OnEntityStandingOnMe(CBaseEntity *pOther);

protected:
	float m_flRageTime;
	float m_flLastCamperCheck;
	bool m_bIsInRageMode;

	void StartRageMode(void);
	void EndRageMode(void);
	void CheckRageState(void);

	float GetMaxJumpRise() const { return sk_npc_boss_fred_max_jump_height.GetFloat(); }
	void FindAndPunishCampers(void);

private:
	CHandle<CHL2MP_Player> m_hTargetPlayer;
	CUtlVector<EHANDLE> m_hPlayersOnMyHead;
};

int ACT_RAGE;
int ACT_MELEE_ATTACK_RUN;

//=========================================================
// Schedules
//=========================================================
enum
{
	SCHED_FRED_RAGE_START = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_FRED_SHOCKWAVE,
};

//=========================================================
// Tasks
//=========================================================
enum
{
	TASK_FRED_SHOCK_PLAYERS = LAST_BASE_ZOMBIE_TASK,
	TASK_FRED_FACE_TARGET,
	TASK_FRED_SHOCKWAVE_FINISH,
};

//=========================================================
// Conditions
//=========================================================
enum
{
	COND_FRED_SHOULD_SHOCKWAVE = LAST_BASE_ZOMBIE_CONDITION,
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

void CNPCFred::OnChangeActivity(Activity eNewActivity)
{
	BaseClass::OnChangeActivity(eNewActivity);

	// Playing rage anim now, do the attack!
	if (eNewActivity == ACT_RAGE)
	{
		const float RAGE_RADIUS = 500.0f;
		const float MAX_FORCE = (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE) ? 1100.0 : 500.0;
		const Vector &vecStart = WorldSpaceCenter();
		Vector vecEnd, vecDown, vecForce = Vector(MAX_FORCE, 0.0, 0.0);
		AngleVectors(GetAbsAngles(), NULL, NULL, &vecDown);
		VectorNormalize(vecDown);
		vecDown *= -1.0f;
		vecEnd = vecStart + vecDown * MAX_TRACE_LENGTH;

		trace_t tr;
		CTraceFilterNoNPCsOrPlayer filter(this, this->GetCollisionGroup());
		UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID_BRUSHONLY, &filter, &tr);

		UTIL_DecalTrace(&tr, "Scorch");
		UTIL_ScreenShake(vecStart, 20.0, 100.0, 1.0, RAGE_RADIUS, SHAKE_START);

		CTakeDamageInfo info(this, this, sk_npc_boss_fred_rage_blastdmg.GetFloat(), DMG_CLUB);
		info.SetDamageForce(vecForce);
		info.SetDamagePosition(vecStart);
		info.SetMiscFlag((TAKEDMGINFO_FORCE_RELATIONSHIP_CHECK | TAKEDMGINFO_DISABLE_FORCELIMIT | TAKEDMGINFO_USE_DMG_AS_PERCENT));
		info.SetRelationshipLink(Classify());
		RadiusDamage(info, vecStart, RAGE_RADIUS, CLASS_NONE, NULL);

		IPredictionSystem::SuppressHostEvents(NULL);
		DispatchParticleEffect("bb2_item_spawn", vecStart, vec3_angle, this);
	}
}

void CNPCFred::OnEntityStandingOnMe(CBaseEntity *pOther)
{
	if (pOther && pOther->IsPlayer() && pOther->IsHuman() && pOther->IsAlive())
	{
		EHANDLE hPlayer = pOther;
		m_hPlayersOnMyHead.AddToTail(hPlayer);
	}
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
	FindAndPunishCampers();

	BaseClass::PrescheduleThink();
}

void CNPCFred::PostscheduleThink(void)
{
	BaseClass::PostscheduleThink();

	if (m_hPlayersOnMyHead.Count())
	{
		for (int i = (m_hPlayersOnMyHead.Count() - 1); i >= 0; i--)
		{
			CBaseEntity *pOther = m_hPlayersOnMyHead[i].Get();
			if (pOther)
			{
				float damage = ceil(((float)pOther->GetMaxHealth()) * (sk_npc_boss_fred_camp_dmg.GetFloat() / 100.0f));

				Vector vecPush;
				const float maxVEL = 300.0f;
				vecPush.x = maxVEL * ((random->RandomInt(0, 1) == 0) ? 1.0f : -1.0f);
				vecPush.y = maxVEL * ((random->RandomInt(0, 1) == 0) ? 1.0f : -1.0f);
				vecPush.z = 135.0f;

				IPredictionSystem::SuppressHostEvents(NULL);
				DispatchParticleEffect("bb2_item_spawn", PATTACH_ROOTBONE_FOLLOW, pOther);

				if (pOther->m_takedamage == DAMAGE_YES)
				{
					CTakeDamageInfo info(this, this, ((int)damage), DMG_FALL);
					info.SetDamageForce(vec3_origin);
					info.SetDamagePosition(vec3_origin);
					AddMultiDamage(info, pOther);
					ApplyMultiDamage();
				}

				pOther->ApplyAbsVelocityImpulse(vecPush, true);
			}
			m_hPlayersOnMyHead.Remove(i);
		}
	}
}

void CNPCFred::StartRageMode(void)
{
	if (m_bIsInRageMode || IsCurSchedule(SCHED_FRED_SHOCKWAVE))
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

void CNPCFred::FindAndPunishCampers(void)
{
	if (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE))
		return;

	if (gpGlobals->curtime >= m_flLastCamperCheck)
	{
		m_flLastCamperCheck = gpGlobals->curtime + CAMPER_CHECK_TIME;

		if (GetEnemy() && GetEnemies() && (GetEnemies()->NumEnemies() > 0) && 
			!IsCurSchedule(SCHED_FRED_RAGE_START) && !IsCurSchedule(SCHED_FRED_SHOCKWAVE) &&
			!HasCondition(COND_FRED_SHOULD_SHOCKWAVE))
		{
			const Vector &vecMyPos = GetAbsOrigin();
			CUtlVector<CHL2MP_Player*> pListOfCampingAssholes;
			AIEnemiesIter_t iter;
			trace_t tr;
			CTraceFilterNoNPCsOrPlayer filter(this, GetCollisionGroup());
			for (AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst(&iter); pEMemory != NULL; pEMemory = GetEnemies()->GetNext(&iter))
			{
				if (pEMemory->bDangerMemory || pEMemory->hEnemy.Get() == NULL || ((gpGlobals->curtime - pEMemory->timeLastSeen) > CAMPER_MAX_LAST_TIME_SEEN))
					continue;

				CHL2MP_Player *pPlayer = ToHL2MPPlayer(pEMemory->hEnemy.Get());
				if (pPlayer == NULL || 
					!pPlayer->IsAlive() || 
					(pPlayer->GetTeamNumber() != TEAM_HUMANS) || 
					pPlayer->IsPlayerInfected() || 
					pPlayer->IsObserver() ||
					pPlayer->IsBot() ||
					(pPlayer->GetFlags() & (FL_NOTARGET | FL_FROZEN)))
					continue;

				const Vector &vecPos = pPlayer->GetLocalOrigin();

				if (vecPos.AsVector2D().DistTo(vecMyPos.AsVector2D()) > CAMPER_MAX_DISTANCE)
					continue;

				if (abs(vecPos.z - vecMyPos.z) < CAMPER_Z_DIFF_MIN)
					continue;

				// Check validity:
				AI_TraceLine(WorldSpaceCenter(), pPlayer->WorldSpaceCenter(), MASK_SHOT, &filter, &tr);
				if (tr.startsolid || (tr.fraction != 1.0f))
					continue;

				pListOfCampingAssholes.AddToTail(pPlayer);
			}

			int items = pListOfCampingAssholes.Count();
			if (items)
			{
				m_hTargetPlayer = pListOfCampingAssholes[random->RandomInt(0, (items - 1))];
				SetCondition(COND_FRED_SHOULD_SHOCKWAVE);
				pListOfCampingAssholes.RemoveAll();
			}
		}
	}
}

int CNPCFred::SelectSchedule ( void )
{
	if (HasCondition(COND_FRED_SHOULD_SHOCKWAVE))
		return SCHED_FRED_SHOCKWAVE;

	if( HasCondition( COND_PHYSICS_DAMAGE ) && CanFlinch() )	
		return SCHED_FLINCH_PHYSICS;	

	return BaseClass::SelectSchedule();
}

void CNPCFred::OnStartSchedule(int scheduleType)
{
	BaseClass::OnStartSchedule(scheduleType);
}

void CNPCFred::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{

	case TASK_FRED_SHOCK_PLAYERS:
	{
		CHL2MP_Player *pTarget = m_hTargetPlayer.Get();
		if (pTarget)
		{
			IPredictionSystem::SuppressHostEvents(NULL);

			DispatchParticleEffect("bb2_healing_effect", GetLocalOrigin(), vec3_angle, this);
			HL2MPRules()->EmitSoundToClient(this, "Laugh", GetNPCType(), GetGender());
			EmitSound("Fred.Shockwave");

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pOther = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				if (!pOther || !pOther->IsAlive() || pOther->IsObserver() || pOther->IsBot() || (pOther->GetTeamNumber() != TEAM_HUMANS))
					continue;

				if (pOther->GetLocalOrigin().DistTo(pTarget->GetLocalOrigin()) > CAMPER_SHOCKWAVE_RADIUS)
					continue;

				Vector vecForward = (WorldSpaceCenter() - pOther->WorldSpaceCenter());
				VectorNormalize(vecForward);

				vecForward.x *= CAMPER_VELOCITY_PUNCH;
				vecForward.y *= CAMPER_VELOCITY_PUNCH;
				vecForward.z *= CAMPER_VELOCITY_PUNCH_Z;
			
				pOther->ApplyAbsVelocityImpulse(vecForward, true);

				DispatchParticleEffect("bb2_item_spawn", PATTACH_ROOTBONE_FOLLOW, pOther);
			}
		}

		m_hTargetPlayer = NULL;
		ClearCondition(COND_FRED_SHOULD_SHOCKWAVE);
		TaskComplete();
		break;
	}

	case TASK_FRED_FACE_TARGET:
	{
		CHL2MP_Player *pTarget = m_hTargetPlayer.Get();
		if (pTarget)
		{
			float idealYaw = UTIL_VecToYaw(pTarget->WorldSpaceCenter() - WorldSpaceCenter());
			GetMotor()->SetIdealYaw(idealYaw);
		}
		TaskComplete();
		break;
	}

	case TASK_FRED_SHOCKWAVE_FINISH:
		m_flLastCamperCheck = gpGlobals->curtime + SHOCKWAVE_COOLDOWN;
		TaskComplete();
		break;

	default:
		BaseClass::StartTask(pTask);
		break;

	}
}

void CNPCFred::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_FRED_SHOCK_PLAYERS:
	case TASK_FRED_FACE_TARGET:
	case TASK_FRED_SHOCKWAVE_FINISH:
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
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
	if (IsOnFire() || IsCurSchedule(SCHED_FRED_SHOCKWAVE))
		return;

	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
}

void CNPCFred::DeathSound( const CTakeDamageInfo &info ) 
{
	if (IsCurSchedule(SCHED_FRED_SHOCKWAVE))
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

void CNPCFred::AlertSound( void )
{
	if (IsCurSchedule(SCHED_FRED_SHOCKWAVE))
		return;

	HL2MPRules()->EmitSoundToClient(this, "Alert", GetNPCType(), GetGender());

	// Don't let a moan sound cut off the alert sound.
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

void CNPCFred::IdleSound( void )
{
	if (IsCurSchedule(SCHED_FRED_SHOCKWAVE))
		return;

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
	if (IsCurSchedule(SCHED_FRED_SHOCKWAVE))
		return;

	HL2MPRules()->EmitSoundToClient(this, "Attack", GetNPCType(), GetGender());
}

int CNPCFred::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if (failedSchedule == SCHED_FRED_SHOCKWAVE)
	{
		ClearCondition(COND_FRED_SHOULD_SHOCKWAVE);
		m_flLastCamperCheck = gpGlobals->curtime + (SHOCKWAVE_COOLDOWN / 2.0f);
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

int CNPCFred::TranslateSchedule( int scheduleType )
{
	return BaseClass::TranslateSchedule( scheduleType );
}

int CNPCFred::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	int ret = BaseClass::OnTakeDamage_Alive(inputInfo);
	if (ret)
	{
		float damageTaken = inputInfo.GetDamage();
		if (damageTaken >= sk_npc_boss_fred_rage_damage.GetFloat())
			StartRageMode();

		float flHealthPercent = ((float)(((float)GetHealth() / (float)GetMaxHealth())));
		if ((flHealthPercent * 100.0f) <= sk_npc_boss_fred_rage_health.GetFloat())
			StartRageMode();
	}

	return ret;
}

void CNPCFred::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if (!IsCurSchedule(SCHED_FLINCH_PHYSICS))
		SetCustomInterruptCondition(COND_PHYSICS_DAMAGE);

	if (IsCurSchedule(SCHED_ZOMBIE_CHASE_ENEMY) ||
		IsCurSchedule(SCHED_ZOMBIE_BASH_DOOR) ||
		IsCurSchedule(SCHED_ZOMBIE_WANDER_MEDIUM) ||
		IsCurSchedule(SCHED_ZOMBIE_MELEE_ATTACK1))
		SetCustomInterruptCondition(COND_FRED_SHOULD_SHOCKWAVE);
}

AI_BEGIN_CUSTOM_NPC( npc_fred, CNPCFred )

DECLARE_ACTIVITY(ACT_RAGE);
DECLARE_ACTIVITY(ACT_MELEE_ATTACK_RUN);

DECLARE_TASK(TASK_FRED_SHOCK_PLAYERS);
DECLARE_TASK(TASK_FRED_FACE_TARGET);
DECLARE_TASK(TASK_FRED_SHOCKWAVE_FINISH);

DECLARE_CONDITION(COND_FRED_SHOULD_SHOCKWAVE);

DEFINE_SCHEDULE
(
SCHED_FRED_RAGE_START,

"	Tasks"
"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_RAGE"
""
"	Interrupts"
"		COND_TASK_FAILED"
)

DEFINE_SCHEDULE
(
SCHED_FRED_SHOCKWAVE,

"	Tasks"
"		TASK_STOP_MOVING		0"
"		TASK_FRED_FACE_TARGET	0"
"		TASK_FACE_IDEAL			0"
"		TASK_FRED_SHOCK_PLAYERS	0"
"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_RAGE"
"		TASK_FRED_SHOCKWAVE_FINISH	0"
""
"	Interrupts"
"		COND_TASK_FAILED"
)

AI_END_CUSTOM_NPC()
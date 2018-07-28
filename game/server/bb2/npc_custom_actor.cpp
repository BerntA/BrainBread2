//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom ally/enemy/villain actor NPC.
//
//========================================================================================//

#include "cbase.h"
#include "npc_custom_actor.h"
#include "ammodef.h"
#include "globalstate.h"
#include "soundent.h"
#include "BasePropDoor.h"
#include "hl2_shared_misc.h"
#include "hl2_player.h"
#include "items.h"
#include "eventqueue.h"
#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_senses.h"
#include "ai_hint.h"
#include "ai_interactions.h"
#include "ai_looktarget.h"
#include "sceneentity.h"
#include "GameBase_Shared.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBasePlayer *GetNearestMatchingPlayer(CAI_BaseNPC *pLooker, bool bVisibleMask = false, int mask = MASK_SOLID_BRUSHONLY)
{
	if (!pLooker)
		return NULL;

	float distToNearest = FLT_MAX;
	CBasePlayer *pNearest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer)
			continue;

		if (!pPlayer->IsAlive() || pPlayer->IsObserver() || (pPlayer->GetTeamNumber() <= TEAM_SPECTATOR))
			continue;

		if (pLooker->IRelationType(pPlayer) != D_LI)
			continue;

		if (bVisibleMask)
		{
			if (!pLooker->FVisible(pPlayer, mask))
				continue;
		}

		float flDist = (pPlayer->GetAbsOrigin() - pLooker->GetAbsOrigin()).LengthSqr();
		if ((flDist < distToNearest))
		{
			pNearest = pPlayer;
			distToNearest = flDist;
		}
	}

	return pNearest;
}

const char *actorCustomSoundScripts[] =
{
	"ManDown",
	"FootstepLeft",
	"FootstepRight",
	"EnemySpotted",
	"Stop",
	"WhatNow",
	"Greetings",
	"Death",
	"Scared",
	"Taunt",
	"Pain",
	"PlayerDown",
};

LINK_ENTITY_TO_CLASS(npc_custom_actor, CNPC_CustomActor);

BEGIN_DATADESC(CNPC_CustomActor)

DEFINE_FIELD(m_bShouldPatrol, FIELD_BOOLEAN),
DEFINE_FIELD(m_flTimeLastCloseToPlayer, FIELD_TIME),
DEFINE_FIELD(m_flTimePlayerStare, FIELD_TIME),
DEFINE_KEYFIELD(m_bNotifyNavFailBlocked, FIELD_BOOLEAN, "notifynavfailblocked"),

DEFINE_KEYFIELD(m_bIsAlly, FIELD_BOOLEAN, "friendly"),
DEFINE_KEYFIELD(m_bGender, FIELD_BOOLEAN, "gender"),
DEFINE_KEYFIELD(m_bBossState, FIELD_BOOLEAN, "boss"),
DEFINE_KEYFIELD(m_iTotalHP, FIELD_INTEGER, "totalhealth"),
DEFINE_KEYFIELD(m_iszNPCName, FIELD_STRING, "npcname"),

DEFINE_KEYFIELD(m_flDamageScaleFactor, FIELD_FLOAT, "damageScale"),
DEFINE_KEYFIELD(m_flHealthScaleFactor, FIELD_FLOAT, "healthScale"),

DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_OUTPUT(m_OnNavFailBlocked, "OnNavFailBlocked"),

DEFINE_INPUTFUNC(FIELD_VOID, "StartPatrolling", InputStartPatrolling),
DEFINE_INPUTFUNC(FIELD_VOID, "StopPatrolling", InputStopPatrolling),
DEFINE_INPUTFUNC(FIELD_VOID, "SetCommandable", InputSetCommandable),

DEFINE_USEFUNC(CommanderUse),
DEFINE_USEFUNC(SimpleUse),

END_DATADESC()

CNPC_CustomActor::CNPC_CustomActor()
{
	m_flLastFollowTargetCheck = 0.0f;
	m_bIsAlly = true;
	m_bGender = true;
	m_bBossState = false;
	m_iszNPCName = NULL_STRING;
	m_iTotalHP = 100;
	
	m_flDamageScaleValue = m_flHealthScaleValue = 0.0f;
	m_flDamageScaleFactor = m_flHealthScaleFactor = 1.0f;

	ListenForGameEvent("player_connection");
}

bool CNPC_CustomActor::CreateBehaviors()
{
	BaseClass::CreateBehaviors();
	AddBehavior(&m_FuncTankBehavior);

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::Precache()
{
	PrecacheModel(STRING(GetModelName()));

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::Spawn()
{
	// NO model? force default.
	const char *model = STRING(GetModelName());
	if (!model || (model && (strlen(STRING(GetModelName())) <= 0)))
		SetModelName(AllocPooledString("models/characters/marine/marine_soldier01.mdl"));

	BaseClass::Spawn();

	m_bShouldPatrol = false;
	m_iHealth = 100;

	m_flTimePlayerStare = FLT_MAX;

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	NPCInit();

	SetUse(&CNPC_CustomActor::CommanderUse);

	//Assert(!ShouldAutosquad() || !IsInPlayerSquad());

	// Use render bounds instead of human hull for guys sitting in chairs, etc.
	m_ActBusyBehavior.SetUseRenderBounds(HasSpawnFlags(SF_CUSTOM_ACTOR_USE_RENDER_BOUNDS));

	// Init default:
	if (m_iTotalHP > 0)
	{
		SetHealth(m_iTotalHP);
		SetMaxHealth(m_iTotalHP);
	}

	const char *npcName = STRING(m_iszNPCName);
	if (!npcName)
		npcName = "";

	SetNPCName(npcName);
	SetBoss(m_bBossState);

	m_flFieldOfView = -1.0;
	m_flDistTooFar = 512.0;
	GetSenses()->SetDistLook(1024.0);

	if (m_bIsAlly)
		SetCollisionGroup(COLLISION_GROUP_NPC_MILITARY);
	else
		SetCollisionGroup(COLLISION_GROUP_NPC_MERCENARY);

	UpdateScaling();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::PostNPCInit()
{
	if ((m_spawnflags & SF_CUSTOM_ACTOR_FOLLOW))
	{
		m_FollowBehavior.SetFollowTarget(GetNearestMatchingPlayer(this));
		m_FollowBehavior.SetParameters(AIF_SIMPLE);
	}

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CNPC_CustomActor::Activate()
{
	BaseClass::Activate();
}

void CNPC_CustomActor::Event_Killed(const CTakeDamageInfo &info)
{
	HL2MPRules()->DeathNotice(this, info);

	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Define our relationship towards the players here.
//
Class_T	CNPC_CustomActor::Classify()
{
	if (m_bIsAlly)
		return CLASS_COMBINE;

	return CLASS_MILITARY;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::ShouldAlwaysThink()
{
	return (BaseClass::ShouldAlwaysThink() || IsFollowingTarget());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define CUSTOM_ACTOR_FOLLOWER_DESERT_FUNCTANK_DIST	45.0f*12.0f
bool CNPC_CustomActor::ShouldBehaviorSelectSchedule(CAI_BehaviorBase *pBehavior)
{
	if (pBehavior == &m_FollowBehavior)
	{
		// Suppress follow behavior if I have a func_tank and the func tank is near
		// what I'm supposed to be following.
		if (m_FuncTankBehavior.CanSelectSchedule())
		{
			// Is the tank close to the follow target?
			Vector vecTank = m_FuncTankBehavior.GetFuncTank()->WorldSpaceCenter();
			Vector vecFollowGoal = m_FollowBehavior.GetFollowGoalInfo().position;

			float flTankDistSqr = (vecTank - vecFollowGoal).LengthSqr();
			float flAllowDist = m_FollowBehavior.GetFollowGoalInfo().followPointTolerance * 2.0f;
			float flAllowDistSqr = flAllowDist * flAllowDist;
			if (flTankDistSqr < flAllowDistSqr)
			{
				// Deny follow behavior so the tank can go.
				return false;
			}
		}
	}
	else if (IsInPlayerSquad() && pBehavior == &m_FuncTankBehavior && m_FuncTankBehavior.IsMounted())
	{
		if (m_FollowBehavior.GetFollowTarget())
		{
			Vector vecFollowGoal = m_FollowBehavior.GetFollowTarget()->GetAbsOrigin();
			if (vecFollowGoal.DistToSqr(GetAbsOrigin()) > Square(CUSTOM_ACTOR_FOLLOWER_DESERT_FUNCTANK_DIST))
			{
				return false;
			}
		}
	}

	return BaseClass::ShouldBehaviorSelectSchedule(pBehavior);
}

void CNPC_CustomActor::OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior)
{
	if (pNewBehavior == &m_FuncTankBehavior)
	{
		m_bReadinessCapable = false;
	}
	else if (pOldBehavior == &m_FuncTankBehavior)
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior(pOldBehavior, pNewBehavior);
}

void CNPC_CustomActor::PlaySound(const char *sound, float eventtime)
{
	char pchSoundScript[32];
	Q_strncpy(pchSoundScript, sound, 32);
	HL2MPRules()->EmitSoundToClient(this, pchSoundScript, GetNPCType(), GetGender());
}

NPC_STATE CNPC_CustomActor::SelectIdealState(void)
{
	switch (m_NPCState)
	{
	case NPC_STATE_COMBAT:
	{
		if (HasCondition(COND_ENEMY_DEAD))
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

void CNPC_CustomActor::AnnounceEnemyKill(CBaseEntity *pEnemy)
{
	if (!pEnemy)
		return;

	if (pEnemy->IsPlayer() && !m_bIsAlly)
		PlaySound("PlayerDown");
	else
		PlaySound("Taunt");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::GatherConditions()
{
	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::PredictPlayerPush()
{
	BaseClass::PredictPlayerPush();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	if (m_flLastFollowTargetCheck > gpGlobals->curtime)
		return;

	m_flLastFollowTargetCheck = gpGlobals->curtime + 0.5f;

	CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
	if (pFollowTarget)
	{
		if (!pFollowTarget->IsAlive() || (IRelationType(pFollowTarget) != D_LI) || !pFollowTarget->IsHuman())
		{
			m_FollowBehavior.SetFollowTarget(NULL);
			m_FollowBehavior.SetParameters(AIF_SIMPLE);
		}
		else
			m_flTimeLastCloseToPlayer = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_CustomActor::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if (!IsCurSchedule(SCHED_NEW_WEAPON))
	{
		SetCustomInterruptCondition(COND_RECEIVED_ORDERS);
	}

	if (GetCurSchedule()->HasInterrupt(COND_IDLE_INTERRUPT))
	{
		SetCustomInterruptCondition(COND_BETTER_WEAPON_AVAILABLE);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	switch (failedSchedule)
	{
	case SCHED_NEW_WEAPON:
		// If failed trying to pick up a weapon, try again in one second. This is because other AI code
		// has put this off for 10 seconds under the assumption that the NPC would be able to 
		// pick up the weapon that they found. 
		m_flNextWeaponSearchTime = gpGlobals->curtime + 1.0f;
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		if (!IsMortar(GetEnemy()))
		{
			if (GetActiveWeapon() && (GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1) && random->RandomInt(0, 1) && HasCondition(COND_SEE_ENEMY) && !HasCondition(COND_NO_PRIMARY_AMMO))
				return TranslateSchedule(SCHED_RANGE_ATTACK1);

			return SCHED_STANDOFF;
		}
		break;
	}

	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::SelectSchedule()
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::SelectSchedulePriorityAction()
{
	int schedule = BaseClass::SelectSchedulePriorityAction();
	if (schedule != SCHED_NONE)
		return schedule;

	schedule = SelectScheduleRetrieveItem();
	if (schedule != SCHED_NONE)
		return schedule;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::SelectScheduleRetrieveItem()
{
	if (HasCondition(COND_BETTER_WEAPON_AVAILABLE))
	{
		CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>(Weapon_FindUsable(WEAPON_SEARCH_DELTA));
		if (pWeapon)
		{
			m_flNextWeaponSearchTime = gpGlobals->curtime + 10.0;
			// Now lock the weapon for several seconds while we go to pick it up.
			pWeapon->Lock(10.0, this);
			SetTarget(pWeapon);
			return SCHED_NEW_WEAPON;
		}
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::SelectScheduleNonCombat()
{
	if (m_bShouldPatrol)
		return SCHED_CUSTOM_ACTOR_PATROL;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::SelectScheduleCombat()
{
	return BaseClass::SelectScheduleCombat();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::ShouldDeferToFollowBehavior()
{
	return BaseClass::ShouldDeferToFollowBehavior();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::TranslateSchedule(int scheduleType)
{
	CBasePlayer *pLocalPlayer = GetNearestMatchingPlayer(this, true);

	switch (scheduleType)
	{
	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
		if (m_NPCState != NPC_STATE_COMBAT && pLocalPlayer && !pLocalPlayer->IsAlive() && CanFollowTarget())
		{
			// Player is dead! 
			float flDist;
			flDist = (pLocalPlayer->GetAbsOrigin() - GetAbsOrigin()).Length();

			if (flDist < 50 * 12)
			{
				return SCHED_CUSTOM_ACTOR_MOURN_PLAYER;
			}
		}
		break;

	case SCHED_ESTABLISH_LINE_OF_FIRE:
	case SCHED_MOVE_TO_WEAPON_RANGE:
		if (!IsMortar(GetEnemy()))
		{
			if (GetActiveWeapon() && (GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1) && random->RandomInt(0, 1) && HasCondition(COND_SEE_ENEMY) && !HasCondition(COND_NO_PRIMARY_AMMO))
				return TranslateSchedule(SCHED_RANGE_ATTACK1);

			return SCHED_STANDOFF;
		}
		break;

	case SCHED_CHASE_ENEMY:
		if (!IsMortar(GetEnemy()))
		{
			return SCHED_STANDOFF;
		}
		break;
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_CUSTOM_ACTOR_SPEAK_MOURNING:
		if (!IsSpeaking() && CanSpeakAfterMyself())
		{
			PlaySound("ManDown");
		}
		TaskComplete();
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_WAIT_FOR_MOVEMENT:
	{
		BaseClass::RunTask(pTask);
		break;
	}

	case TASK_MOVE_TO_TARGET_RANGE:
	{
		BaseClass::RunTask(pTask);
		break;
	}

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CNPC_CustomActor::TaskFail(AI_TaskFailureCode_t code)
{
	if (code == FAIL_NO_ROUTE_BLOCKED && m_bNotifyNavFailBlocked)
	{
		m_OnNavFailBlocked.FireOutput(this, this);
	}

	BaseClass::TaskFail(code);
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
//-----------------------------------------------------------------------------
Activity CNPC_CustomActor::NPC_TranslateActivity(Activity activity)
{
	return BaseClass::NPC_TranslateActivity(activity);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::HandleAnimEvent(animevent_t *pEvent)
{	
	switch (pEvent->event)
	{
	case AE_NPC_LEFTFOOT:
	case NPC_EVENT_LEFTFOOT:
	{
		PlaySound("FootstepLeft", pEvent->eventtime);
	}
	break;

	case AE_NPC_RIGHTFOOT:
	case NPC_EVENT_RIGHTFOOT:
	{
		PlaySound("FootstepRight", pEvent->eventtime);
	}
	break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::PickupItem(CBaseEntity *pItem)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::IgnorePlayerPushing(void)
{
	// Friendly NPCs will have no-collide with players! Therefore we don't normally care about pushing, however for func_tanks, we might want to do pushing:
	if (m_FuncTankBehavior.IsMounted())
	{
		CFuncTank *pTank = m_FuncTankBehavior.GetFuncTank();
		if (pTank && pTank->IsControllable())
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return a random expression for the specified state to play over 
//			the state's expression loop.
//-----------------------------------------------------------------------------
const char *CNPC_CustomActor::SelectRandomExpressionForState(NPC_STATE state)
{
	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::SimpleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	// Under these conditions, NPC will refuse to go with the player.
	// Robin: NPCs should always respond to +USE even if someone else has the semaphore.
	m_bDontUseSemaphore = true;

	// First, try to speak the +USE concept
	if (!SelectPlayerUseSpeech())
	{
		if (HasSpawnFlags(SF_CUSTOM_ACTOR_NOT_COMMANDABLE) || IRelationType(pActivator) == D_NU)
		{
			// If I'm denying commander mode because a level designer has made that decision,
			// then fire this output in case they've hooked it to an event.
			m_OnDenyCommanderUse.FireOutput(this, this);
		}
	}

	m_bDontUseSemaphore = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::OnBeginMoveAndShoot()
{
	if (BaseClass::OnBeginMoveAndShoot())
	{
		if (m_iMySquadSlot == SQUAD_SLOT_ATTACK1 || m_iMySquadSlot == SQUAD_SLOT_ATTACK2)
			return true; // already have the slot I need

		if (m_iMySquadSlot == SQUAD_SLOT_NONE && OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2))
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::OnEndMoveAndShoot()
{
	VacateStrategySlot();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::LocateEnemySound()
{
	PlaySound("EnemySpotted");
}

//-----------------------------------------------------------------------------
// Purpose: Return the actual position the NPC wants to fire at when it's trying
//			to hit it's current enemy.
//-----------------------------------------------------------------------------
Vector CNPC_CustomActor::GetActualShootPosition(const Vector &shootOrigin)
{
	Vector vecTarget = BaseClass::GetActualShootPosition(shootOrigin);

	return vecTarget;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::OnChangeActiveWeapon(CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon)
{
	if (pNewWeapon)
	{
		GetShotRegulator()->SetParameters(pNewWeapon->GetMinBurst(), pNewWeapon->GetMaxBurst(), pNewWeapon->GetMinRestTime(), pNewWeapon->GetMaxRestTime());
	}

	BaseClass::OnChangeActiveWeapon(pOldWeapon, pNewWeapon);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SHOTGUN_DEFER_SEARCH_TIME	20.0f
#define OTHER_DEFER_SEARCH_TIME		FLT_MAX
bool CNPC_CustomActor::ShouldLookForBetterWeapon()
{
	if (BaseClass::ShouldLookForBetterWeapon())
	{
		if (IsInPlayerSquad() && (GetActiveWeapon() && IsMoving()) && (m_FollowBehavior.GetFollowTarget() && m_FollowBehavior.GetFollowTarget()->IsPlayer()))
		{
			// For npcs in the player squad, you must be unarmed, or standing still (if armed) in order to 
			// divert attention to looking for a new weapon.
			return false;
		}

		if (GetActiveWeapon() && IsMoving())
			return false;

#ifdef DBGFLAG_ASSERT
		// Cached off to make sure you change this if you ask the code to defer.
		float flOldWeaponSearchTime = m_flNextWeaponSearchTime;
#endif

		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if (pWeapon)
		{
			bool bDefer = false;

			if (pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE)
			{
				// Content to keep this weapon forever
				m_flNextWeaponSearchTime = OTHER_DEFER_SEARCH_TIME;
				bDefer = true;
			}
			else if (pWeapon->GetWeaponType() == WEAPON_TYPE_SHOTGUN)
			{
				// Shotgunners do not defer their weapon search indefinitely.
				// If more than one npc in the squad has a shotgun, we force
				// some of them to trade for another weapon.
				if ((NumWeaponsInSquad("weapon_remington") > 1) || (NumWeaponsInSquad("weapon_sawedoff") > 1) ||
					(NumWeaponsInSquad("weapon_benelli_m4") > 1))
				{
					// Check for another weapon now. If I don't find one, this code will
					// retry in 2 seconds or so.
					bDefer = false;
				}
				else
				{
					// I'm the only shotgunner in the group right now, so I'll check
					// again in 3 0seconds or so. This code attempts to distribute
					// the desire to reduce shotguns amongst squadmates so that all 
					// shotgunners do not discard their weapons when they suddenly realize
					// the squad has too many.
					if (random->RandomInt(0, 1) == 0)
					{
						m_flNextWeaponSearchTime = gpGlobals->curtime + SHOTGUN_DEFER_SEARCH_TIME;
					}
					else
					{
						m_flNextWeaponSearchTime = gpGlobals->curtime + SHOTGUN_DEFER_SEARCH_TIME + 10.0f;
					}

					bDefer = true;
				}
			}

			if (bDefer)
			{
				// I'm happy with my current weapon. Don't search now.
				// If you ask the code to defer, you must have set m_flNextWeaponSearchTime to when
				// you next want to try to search.
				Assert(m_flNextWeaponSearchTime != flOldWeaponSearchTime);
				return false;
			}
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_CustomActor::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{
	int ret = BaseClass::OnTakeDamage_Alive(info);
	if (ret)
		PlaySound("Pain");

	return ret;
}

void CNPC_CustomActor::FireBullets(const FireBulletsInfo_t &info)
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	FireBulletsInfo_t modinfo = info;

	float flDamage = info.m_flDamage;
	if (pWeapon)
	{
		modinfo.m_vecFirstStartPos = GetLocalOrigin();
		modinfo.m_flDropOffDist = pWeapon->GetWpnData().m_flDropOffDistance;
	}

	flDamage += ((flDamage / 100.0f) * m_flDamageScaleValue);

	modinfo.m_flDamage = flDamage;
	modinfo.m_iPlayerDamage = (int)flDamage;

	BaseClass::FireBullets(modinfo);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::IsFollowingTarget()
{
	return (m_FollowBehavior.GetFollowTarget() != NULL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::CanFollowTarget()
{
	if (m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_PRONE)
		return false;

	if (HasSpawnFlags(SF_CUSTOM_ACTOR_NOT_COMMANDABLE))
		return false;

	if (IsInAScript())
		return false;

	// Don't bother people who don't want to be bothered
	if (!CanBeUsedAsAFriend())
		return false;

	if (!m_bIsAlly)
		return false;

	if (GetNearestMatchingPlayer(this) == NULL)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::CommanderUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_OnPlayerUse.FireOutput(pActivator, pCaller);

	if (!CanFollowTarget())
	{
		SimpleUse(pActivator, pCaller, useType, value);
		return;
	}

	// Follow anyone who 'presses' use...
	CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
	if (!pFollowTarget && (IRelationType(pActivator) == D_LI))
	{
		m_FollowBehavior.SetFollowTarget(pActivator);
		m_FollowBehavior.SetParameters(AIF_SIMPLE);
	}
	else if (pFollowTarget == pActivator)
	{
		m_FollowBehavior.SetFollowTarget(NULL);
		m_FollowBehavior.SetParameters(AIF_SIMPLE);
	}

	if (pActivator == GetNearestMatchingPlayer(this))
	{
		if (m_iTimesGreeted >= 6)
		{
			PlaySound("Stop");
			m_iTimesGreeted = 0;
		}
		if (m_iTimesGreeted >= 3)
			PlaySound("WhatNow");
		else
			PlaySound("Greetings");

		m_iTimesGreeted++;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::FValidateHintType(CAI_Hint *pHint)
{
	switch (pHint->HintType())
	{
	case HINT_WORLD_VISUALLY_INTERESTING:
		return true;
		break;

	default:
		break;
	}

	return BaseClass::FValidateHintType(pHint);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::InputStartPatrolling(inputdata_t &inputdata)
{
	m_bShouldPatrol = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::InputStopPatrolling(inputdata_t &inputdata)
{
	m_bShouldPatrol = false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::OnGivenWeapon(CBaseCombatWeapon *pNewWeapon)
{
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::InputSetCommandable(inputdata_t &inputdata)
{
	RemoveSpawnFlags(SF_CUSTOM_ACTOR_NOT_COMMANDABLE);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::DeathSound(const CTakeDamageInfo &info)
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	PlaySound("Death");
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::FearSound(void)
{
	PlaySound("Scared");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::UseSemaphore(void)
{
	// Ignore semaphore if we're told to work outside it
	if (HasSpawnFlags(SF_CUSTOM_ACTOR_IGNORE_SEMAPHORE))
		return false;

	return BaseClass::UseSemaphore();
}

void CNPC_CustomActor::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (!strcmp(type, "player_connection"))
		UpdateScaling();
}

void CNPC_CustomActor::UpdateScaling(void)
{
	if ((!bb2_enable_scaling.GetBool() && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA)) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION))
	{
		m_flDamageScaleValue = 0.0f;
		m_flHealthScaleValue = 0.0f;
		return;
	}

	float flDamageScaleAmount = 0.0f, flHealthScaleAmount = 0.0f;
	flDamageScaleAmount = (bb2_npc_scaling.GetInt() * ((float)GameBaseShared()->GetNumActivePlayers()) * m_flDamageScaleFactor);
	flHealthScaleAmount = (bb2_npc_scaling.GetInt() * ((float)GameBaseShared()->GetNumActivePlayers()) * m_flHealthScaleFactor);

	float defaultTotalHP = ((float)m_iTotalHP);
	float flTotal = (flHealthScaleAmount * (defaultTotalHP / 100.0f)) + defaultTotalHP;
	m_iTotalHP = ((int)flTotal);

	float newHP = 0.0f;
	float hpPercentLeft = (float)(((float)GetHealth()) / ((float)GetMaxHealth()));

	if (hpPercentLeft >= 1) // No HP lost.
		newHP = flTotal;
	else // HP lost
		newHP = ((((float)m_iTotalHP) / 100.0f) * (hpPercentLeft * 100.0f));

	newHP = round(newHP);
	if (newHP <= 0)
		newHP = 1;

	SetHealth((int)newHP);
	SetMaxHealth(m_iTotalHP);

	m_flDamageScaleValue = flDamageScaleAmount;
	m_flHealthScaleValue = flHealthScaleAmount;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_custom_actor, CNPC_CustomActor)

DECLARE_TASK(TASK_CUSTOM_ACTOR_SPEAK_MOURNING)

//=========================================================
// > SCHED_CUSTOM_ACTOR_PATROL
//=========================================================
DEFINE_SCHEDULE
(
SCHED_CUSTOM_ACTOR_PATROL,

"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_WANDER						901024"		// 90 to 1024 units
"		TASK_WALK_PATH					0"
"		TASK_WAIT_FOR_MOVEMENT			0"
"		TASK_STOP_MOVING				0"
"		TASK_WAIT						3"
"		TASK_WAIT_RANDOM				3"
"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_CUSTOM_ACTOR_PATROL" // keep doing it
""
"	Interrupts"
"		COND_ENEMY_DEAD"
"		COND_LIGHT_DAMAGE"
"		COND_HEAVY_DAMAGE"
"		COND_HEAR_DANGER"
"		COND_NEW_ENEMY"
)

DEFINE_SCHEDULE
(
SCHED_CUSTOM_ACTOR_MOURN_PLAYER,

"	Tasks"
"		TASK_GET_PATH_TO_PLAYER		0"
"		TASK_RUN_PATH_WITHIN_DIST	180"
"		TASK_WAIT_FOR_MOVEMENT		0"
"		TASK_STOP_MOVING			0"
"		TASK_TARGET_PLAYER			0"
"		TASK_FACE_TARGET			0"
"		TASK_CUSTOM_ACTOR_SPEAK_MOURNING			0"
"		TASK_SUGGEST_STATE			STATE:IDLE"
""
"	Interrupts"
"		COND_LIGHT_DAMAGE"
"		COND_HEAVY_DAMAGE"
"		COND_HEAR_DANGER"
"		COND_NEW_ENEMY"
)

AI_END_CUSTOM_NPC()
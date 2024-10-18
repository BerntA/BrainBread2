//=========       Copyright © Reperio Studios 2024 @ Bernt Andreas Eide!       ============//
//
// Purpose: Parasite - Jumper & Humper!
//
//========================================================================================//

#include "cbase.h"
#include "game.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_moveprobe.h"
#include "ai_memory.h"
#include "bitstring.h"
#include "npcevent.h"
#include "soundent.h"
#include "npc_parasite.h"
#include "ndebugoverlay.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "world.h"
#include "npc_bullseye.h"
#include "physics_npc_solver.h"
#include "decals.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PARASITE_IGNORE_WORLD_COLLISION_TIME 0.5f

const int PARASITE_MIN_JUMP_DIST = 48;
const int PARASITE_MAX_JUMP_DIST = 256;

//-----------------------------------------------------------------------------
// animation events
//-----------------------------------------------------------------------------
int AE_PARASITE_JUMPATTACK;
int AE_PARASITE_JUMP_TELEGRAPH;

//-----------------------------------------------------------------------------
// custom schedules
//-----------------------------------------------------------------------------
enum
{
	SCHED_PARASITE_RANGE_ATTACK1 = LAST_SHARED_SCHEDULE,
	SCHED_PARASITE_AMBUSH,
	SCHED_PARASITE_HOP_RANDOMLY,
	SCHED_PARASITE_HARASS_ENEMY,
	SCHED_PARASITE_FALL_TO_GROUND,
};

//=========================================================
// tasks
//=========================================================
enum
{
	TASK_PARASITE_HOP_ASIDE = LAST_SHARED_TASK,
	TASK_PARASITE_HOP_OFF_NPC,
	TASK_PARASITE_HARASS_HOP,
};

//=========================================================
// conditions 
//=========================================================
enum
{
	COND_PARASITE_ILLEGAL_GROUNDENT = LAST_SHARED_CONDITION,
};

BEGIN_DATADESC(CBaseParasite)

DEFINE_OUTPUT(m_OnLeap, "OnLeap"),

// Function Pointers
DEFINE_THINKFUNC(ThrowThink),
DEFINE_ENTITYFUNC(LeapTouch),

END_DATADESC()

void CBaseParasite::Spawn(void)
{
	if (ParseNPC(this))
	{
		SetHealth(m_iTotalHP);
		SetMaxHealth(m_iTotalHP);
		SetNPCName(GetNPCName());
		SetBoss(IsBoss());

		PrecacheModel(GetNPCModelName());
		SetModel(GetNPCModelName());

		m_nSkin = m_iModelSkin;
		UpdateNPCScaling();
		UpdateMeleeRange(NAI_Hull::Bounds(HULL_TINY, true));
	}
	else
	{
		UTIL_Remove(this);
		return;
	}

	SetHullType(HULL_TINY);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE | FSOLID_TRIGGER);
	SetMoveType(MOVETYPE_STEP);
	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE);

	SetViewOffset(Vector(6, 0, 11));		// Position of the eyes relative to NPC's origin.

	SetBloodColor(BLOOD_COLOR_RED);
	m_flFieldOfView = 0.5;
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_INNATE_RANGE_ATTACK1);
	CapabilitiesAdd(bits_CAP_SQUAD);

	GetEnemies()->SetFreeKnowledgeDuration(5.0);
	CollisionProp()->UseTriggerBounds(true, 15.0f);
}

void CBaseParasite::Precache(void)
{
	BaseClass::Precache();
}

void CBaseParasite::OnChangeActivity(Activity NewActivity)
{
	bool fRandomize = false;
	float flRandomRange = 0.0;

	// If this crab is starting to walk or idle, pick a random point within
	// the animation to begin. This prevents lots of crabs being in lockstep.
	if (NewActivity == ACT_IDLE)
	{
		flRandomRange = 0.75;
		fRandomize = true;
	}
	else if (NewActivity == ACT_RUN)
	{
		flRandomRange = 0.25;
		fRandomize = true;
	}

	BaseClass::OnChangeActivity(NewActivity);

	if (fRandomize)
	{
		SetCycle(random->RandomFloat(0.0, flRandomRange));
	}
}

Class_T	CBaseParasite::Classify(void)
{
	return CLASS_ZOMBIE;
}

Vector CBaseParasite::BodyTarget(const Vector& posSrc, bool bNoisy)
{
	Vector vecResult;
	vecResult = GetAbsOrigin();
	vecResult.z += 6;
	return vecResult;
}

float CBaseParasite::MaxYawSpeed(void)
{
	return BaseClass::MaxYawSpeed();
}

#define PARASITE_MAX_LEDGE_HEIGHT 12.0f

bool CBaseParasite::IsFirmlyOnGround()
{
	if (!(GetFlags() & FL_ONGROUND))
		return false;

	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() - Vector(0, 0, PARASITE_MAX_LEDGE_HEIGHT), MASK_NPCSOLID, this, GetCollisionGroup(), &tr);
	return tr.fraction != 1.0;
}

void CBaseParasite::MoveOrigin(const Vector& vecDelta)
{
	UTIL_SetOrigin(this, GetLocalOrigin() + vecDelta);
}

void CBaseParasite::ThrowAt(const Vector& vecPos)
{
	JumpAttack(false, vecPos, true);
}

void CBaseParasite::Leap(const Vector& vecVel)
{
	SetTouch(&CBaseParasite::LeapTouch);

	SetCondition(COND_FLOATING_OFF_GROUND);
	SetGroundEntity(NULL);

	m_flIgnoreWorldCollisionTime = gpGlobals->curtime + PARASITE_IGNORE_WORLD_COLLISION_TIME;

	if (HasHeadroom())
	{
		// Take him off ground so engine doesn't instantly reset FL_ONGROUND.
		MoveOrigin(Vector(0, 0, 1));
	}

	SetAbsVelocity(vecVel);

	m_bMidJump = true;
	SetThink(&CBaseParasite::ThrowThink);
	SetNextThink(gpGlobals->curtime);

	m_OnLeap.FireOutput(GetEnemy(), this);
}

void CBaseParasite::ThrowThink(void)
{
	if (gpGlobals->curtime > m_flNextNPCThink)
	{
		NPCThink();
		m_flNextNPCThink = gpGlobals->curtime + 0.1;
	}

	if (GetFlags() & FL_ONGROUND)
	{
		SetThink(&CBaseParasite::CallNPCThink);
		SetNextThink(gpGlobals->curtime + 0.1);
		return;
	}

	SetNextThink(gpGlobals->curtime);
}

void CBaseParasite::JumpAttack(bool bRandomJump, const Vector& vecPos, bool bThrown)
{
	Vector vecJumpVel;
	if (!bRandomJump)
	{
		float gravity = GetCurrentGravity();
		if (gravity <= 1)
		{
			gravity = 1;
		}

		// How fast does the parasite need to travel to reach the position given gravity?
		float flActualHeight = vecPos.z - GetAbsOrigin().z;
		float height = flActualHeight;
		if (height < 16)
		{
			height = 16;
		}
		else
		{
			float flMaxHeight = bThrown ? 400 : 120;
			if (height > flMaxHeight)
			{
				height = flMaxHeight;
			}
		}

		// overshoot the jump by an additional 8 inches
		// NOTE: This calculation jumps at a position INSIDE the box of the enemy (player)
		// so if you make the additional height too high, the crab can land on top of the
		// enemy's head.  If we want to jump high, we'll need to move vecPos to the surface/outside
		// of the enemy's box.

		float additionalHeight = 0;
		if (height < 32)
		{
			additionalHeight = 8;
		}

		height += additionalHeight;

		// NOTE: This equation here is from vf^2 = vi^2 + 2*a*d
		float speed = sqrt(2 * gravity * height);
		float time = speed / gravity;

		// add in the time it takes to fall the additional height
		// So the impact takes place on the downward slope at the original height
		time += sqrt((2 * additionalHeight) / gravity);

		// Scale the sideways velocity to get there at the right time
		VectorSubtract(vecPos, GetAbsOrigin(), vecJumpVel);
		vecJumpVel /= time;

		// Speed to offset gravity at the desired height.
		vecJumpVel.z = speed;

		// Don't jump too far/fast.
		float flJumpSpeed = vecJumpVel.Length();
		float flMaxSpeed = bThrown ? 1000.0f : 650.0f;
		if (flJumpSpeed > flMaxSpeed)
		{
			vecJumpVel *= flMaxSpeed / flJumpSpeed;
		}
	}
	else
	{
		//
		// Jump hop, don't care where.
		//
		Vector forward, up;
		AngleVectors(GetLocalAngles(), &forward, NULL, &up);
		vecJumpVel = Vector(forward.x, forward.y, up.z) * 350;
	}

	AttackSound();
	Leap(vecJumpVel);
}

void CBaseParasite::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->event == AE_PARASITE_JUMPATTACK)
	{
		// Ignore if we're in mid air
		if (m_bMidJump)
			return;

		CBaseEntity* pEnemy = GetEnemy();

		if (pEnemy)
		{
			if (m_bCommittedToJump)
			{
				JumpAttack(false, m_vecCommittedJumpPos);
			}
			else
			{
				// Jump at my enemy's eyes.
				JumpAttack(false, pEnemy->EyePosition());
			}

			m_bCommittedToJump = false;

		}
		else
		{
			// Jump hop, don't care where.
			JumpAttack(true);
		}

		return;
	}

	if (pEvent->event == AE_PARASITE_JUMP_TELEGRAPH)
	{
		TelegraphSound();

		CBaseEntity* pEnemy = GetEnemy();
		if (pEnemy)
		{
			// Once we telegraph, we MUST jump. This is also when commit to what point
			// we jump at. Jump at our enemy's eyes.
			m_vecCommittedJumpPos = pEnemy->EyePosition();
			m_bCommittedToJump = true;
		}

		return;
	}

	CAI_BaseNPC::HandleAnimEvent(pEvent);
}

void CBaseParasite::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{

	case TASK_PARASITE_HOP_OFF_NPC:
		if (GetFlags() & FL_ONGROUND)
		{
			TaskComplete();
		}
		else
		{
			// Face the direction I've been forced to jump.
			GetMotor()->SetIdealYawToTargetAndUpdate(GetAbsOrigin() + GetAbsVelocity());
		}
		break;

	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
	case TASK_PARASITE_HARASS_HOP:
	{
		if (IsActivityFinished())
		{
			TaskComplete();
			m_bMidJump = false;
			SetTouch(NULL);
			SetThink(&CBaseParasite::CallNPCThink);
			SetIdealActivity(ACT_IDLE);

			if (m_bAttackFailed)
			{
				// our attack failed because we just ran into something solid.
				// delay attacking for a while so we don't just repeatedly leap
				// at the enemy from a bad location.
				m_bAttackFailed = false;
				m_flNextAttack = gpGlobals->curtime + 1.2f;
			}
		}
		break;
	}

	default:
	{
		BaseClass::RunTask(pTask);
		break;
	}

	}
}

bool CBaseParasite::HasHeadroom()
{
	trace_t tr;
	UTIL_TraceEntity(this, GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, 1), MASK_NPCSOLID, this, GetCollisionGroup(), &tr);
	return (tr.fraction == 1.0);
}

void CBaseParasite::LeapTouch(CBaseEntity* pOther)
{
	m_bMidJump = false;

	if (IRelationType(pOther) <= D_FR)
	{
		// Don't hit if back on ground
		if (!(GetFlags() & FL_ONGROUND))
		{
			if (pOther->m_takedamage != DAMAGE_NO)
			{
				BiteSound();
				TouchDamage(pOther);

				// attack succeeded, so don't delay our next attack if we previously thought we failed
				m_bAttackFailed = false;
			}
			else
			{
				ImpactSound();
			}
		}
		else
		{
			ImpactSound();
		}
	}
	else if (!(GetFlags() & FL_ONGROUND))
	{
		// Still in the air...
		if (!pOther->IsSolid())
		{
			// Touching a trigger or something.
			return;
		}

		// just ran into something solid, so the attack probably failed.  make a note of it
		// so that when the attack is done, we'll delay attacking for a while so we don't
		// just repeatedly leap at the enemy from a bad location.
		m_bAttackFailed = true;

		if (gpGlobals->curtime < m_flIgnoreWorldCollisionTime)
		{
			// try to ignore the world, static props, and friends for a 
			// fraction of a second after they jump. This is because they often brush
			// doorframes or props as they leap, and touching those objects turns off
			// this touch function, which can cause them to hit the player and not bite.
			// A timer probably isn't the best way to fix this, but it's one of our 
			// safer options at this point (sjb).
			return;
		}
	}

	// Shut off the touch function.
	SetTouch(NULL);
	SetThink(&CBaseParasite::CallNPCThink);
}

int CBaseParasite::CalcDamageInfo(CTakeDamageInfo* pInfo)
{
	pInfo->Set(this, this, m_iDamageOneHand, DMG_SLASH);
	CalculateMeleeDamageForce(pInfo, GetAbsVelocity(), GetAbsOrigin());
	return pInfo->GetDamage();
}

void CBaseParasite::TouchDamage(CBaseEntity* pOther)
{
	CTakeDamageInfo info;
	CalcDamageInfo(&info);
	pOther->TakeDamage(info);
}

void CBaseParasite::GatherConditions(void)
{
	BaseClass::GatherConditions();

	// See if I've landed on an NPC or player or something else illegal
	ClearCondition(COND_PARASITE_ILLEGAL_GROUNDENT);
	CBaseEntity* ground = GetGroundEntity();

	if ((GetFlags() & FL_ONGROUND) && ground && !ground->IsWorld())
	{
		if ((ground->IsNPC() || ground->IsPlayer()))
		{
			SetCondition(COND_PARASITE_ILLEGAL_GROUNDENT);
		}
		else if (ground->VPhysicsGetObject() && (ground->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD))
		{
			SetCondition(COND_PARASITE_ILLEGAL_GROUNDENT);
		}
	}
}

void CBaseParasite::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	// Make the crab coo a little bit in combat state.
	if ((m_NPCState == NPC_STATE_COMBAT) && (random->RandomFloat(0, 5) < 0.1))
	{
		IdleSound();
	}
}

void CBaseParasite::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{

	case TASK_PARASITE_HARASS_HOP:
	{
		// Just pop up into the air like you're trying to get at the
		// enemy, even though it's known you can't reach them.
		if (GetEnemy())
		{
			Vector forward, up;

			GetVectors(&forward, NULL, &up);

			m_vecCommittedJumpPos = GetAbsOrigin();
			m_vecCommittedJumpPos += up * random->RandomFloat(80, 150);
			m_vecCommittedJumpPos += forward * random->RandomFloat(32, 80);

			m_bCommittedToJump = true;

			SetIdealActivity(ACT_RANGE_ATTACK1);
		}
		else
		{
			TaskFail("No enemy");
		}
	}
	break;

	case TASK_PARASITE_HOP_OFF_NPC:
	{
		CBaseEntity* ground = GetGroundEntity();
		if (ground)
		{
			// If jumping off of a physics object that the player is holding, create a 
			// solver to prevent the npc from colliding with that object for a 
			// short time.
			if (ground && ground->VPhysicsGetObject())
			{
				if (ground->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
				{
					NPCPhysics_CreateSolver(this, ground, true, 0.5);
				}
			}


			Vector vecJumpDir;

			// Jump in some random direction. This way if the person I'm standing on is
			// against a wall, I'll eventually get away.

			vecJumpDir.z = 0;
			vecJumpDir.x = 0;
			vecJumpDir.y = 0;

			while (vecJumpDir.x == 0 && vecJumpDir.y == 0)
			{
				vecJumpDir.x = random->RandomInt(-1, 1);
				vecJumpDir.y = random->RandomInt(-1, 1);
			}

			vecJumpDir.NormalizeInPlace();

			SetGroundEntity(NULL);

			if (HasHeadroom())
			{
				// Bump up
				MoveOrigin(Vector(0, 0, 1));
			}

			SetAbsVelocity(vecJumpDir * 200 + Vector(0, 0, 200));
		}
		else
		{
			// *shrug* I guess they're gone now. Or dead.
			TaskComplete();
		}
	}
	break;

	case TASK_RANGE_ATTACK1:
	{
		SetIdealActivity(ACT_RANGE_ATTACK1);
		break;
	}

	default:
	{
		BaseClass::StartTask(pTask);
		break;
	}

	}
}

float CBaseParasite::InnateRange1MinRange(void)
{
	return PARASITE_MIN_JUMP_DIST;
}

float CBaseParasite::InnateRange1MaxRange(void)
{
	return PARASITE_MAX_JUMP_DIST;
}

int CBaseParasite::RangeAttack1Conditions(float flDot, float flDist)
{
	if (gpGlobals->curtime < m_flNextAttack)
		return 0;

	if ((GetFlags() & FL_ONGROUND) == false)
		return 0;

	// When we're burrowed ignore facing, because when we unburrow we'll cheat and face our enemy.
	if (flDot < 0.65)
		return COND_NOT_FACING_ATTACK;

	// This code stops lots of parasites swarming you and blocking you
	// whilst jumping up and down in your face over and over. It forces
	// them to back up a bit. If this causes problems, consider using it
	// for the fast parasites only, rather than just removing it.(sjb)
	if (flDist < PARASITE_MIN_JUMP_DIST)
		return COND_TOO_CLOSE_TO_ATTACK;

	if (flDist > PARASITE_MAX_JUMP_DIST)
		return COND_TOO_FAR_TO_ATTACK;

	// Make sure the way is clear!
	CBaseEntity* pEnemy = GetEnemy();
	if (pEnemy)
	{
		bool bEnemyIsBullseye = (dynamic_cast<CNPC_Bullseye*>(pEnemy) != NULL);

		trace_t tr;
		AI_TraceLine(EyePosition(), pEnemy->EyePosition(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

		if (tr.m_pEnt != GetEnemy())
		{
			if (!bEnemyIsBullseye || tr.m_pEnt != NULL)
				return COND_NONE;
		}

		if (GetEnemy()->EyePosition().z - 36.0f > GetAbsOrigin().z)
		{
			// Only run this test if trying to jump at a player who is higher up than me, else this 
			// code will always prevent a parasite from jumping down at an enemy, and sometimes prevent it
			// jumping just slightly up at an enemy.
			Vector vStartHullTrace = GetAbsOrigin();
			vStartHullTrace.z += 1.0;

			Vector vEndHullTrace = GetEnemy()->EyePosition() - GetAbsOrigin();
			vEndHullTrace.NormalizeInPlace();
			vEndHullTrace *= 8.0;
			vEndHullTrace += GetAbsOrigin();

			AI_TraceHull(vStartHullTrace, vEndHullTrace, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, GetCollisionGroup(), &tr);

			if (tr.m_pEnt != NULL && tr.m_pEnt != GetEnemy())
			{
				return COND_TOO_CLOSE_TO_ATTACK;
			}
		}
	}

	return COND_CAN_RANGE_ATTACK1;
}

int CBaseParasite::OnTakeDamage_Alive(const CTakeDamageInfo& inputInfo)
{
	// No friendly fire from zombos!
	CBaseEntity* pAttacker = inputInfo.GetAttacker();
	if (pAttacker && (pAttacker->Classify() == CLASS_PLAYER_ZOMB))
		return 0;

	const float flCurrentHealth = ((float)m_iHealth.Get());

	CTakeDamageInfo info = inputInfo;

	// Don't take any acid damage.
	if (info.GetDamageType() & DMG_ACID)
		info.SetDamage(0);

	int ret = CAI_BaseNPC::OnTakeDamage_Alive(info);
	if (ret)
		OnTookDamage(info, flCurrentHealth);

	return ret;
}

void CBaseParasite::ClampRagdollForce(const Vector& vecForceIn, Vector* vecForceOut)
{
	// Assumes the parasite mass is 5kg (100 feet per second)
	float MAX_PARASITE_RAGDOLL_SPEED = 100.0f * 12.0f * 5.0f;

	Vector vecClampedForce;
	BaseClass::ClampRagdollForce(vecForceIn, &vecClampedForce);

	// Copy the force to vecForceOut, in case we don't change it.
	*vecForceOut = vecClampedForce;

	float speed = VectorNormalize(vecClampedForce);
	if (speed > MAX_PARASITE_RAGDOLL_SPEED)
	{
		// Don't let the ragdoll go as fast as it was going to.
		vecClampedForce *= MAX_PARASITE_RAGDOLL_SPEED;
		*vecForceOut = vecClampedForce;
	}
}

void CBaseParasite::Event_Killed(const CTakeDamageInfo& info)
{
	HL2MPRules()->DeathNotice(this, info);
	BaseClass::Event_Killed(info);
}

int CBaseParasite::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_FALL_TO_GROUND:
		return SCHED_PARASITE_FALL_TO_GROUND;

	case SCHED_RANGE_ATTACK1:
		return SCHED_PARASITE_RANGE_ATTACK1;

	case SCHED_FAIL_TAKE_COVER:
		return SCHED_ALERT_FACE;

	case SCHED_CHASE_ENEMY_FAILED:
	{
		if (!GetEnemy())
			break;

		if (!HasCondition(COND_SEE_ENEMY))
			break;

		return SCHED_PARASITE_HARASS_ENEMY;
	}
	break;
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

int CBaseParasite::SelectSchedule(void)
{
	if (HasCondition(COND_PARASITE_ILLEGAL_GROUNDENT))
	{
		// You're on an NPC's head. Get off.
		return SCHED_PARASITE_HOP_RANDOMLY;
	}

	switch (m_NPCState)
	{
	case NPC_STATE_ALERT:
	{
		if (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			if (fabs(GetMotor()->DeltaIdealYaw()) < (1.0 - m_flFieldOfView) * 60) // roughly in the correct direction
			{
				return SCHED_TAKE_COVER_FROM_ORIGIN;
			}
			else if (SelectWeightedSequence(ACT_SMALL_FLINCH) != -1)
			{
				m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat(1, 3);
				return SCHED_SMALL_FLINCH;
			}
		}
		else if (HasCondition(COND_HEAR_DANGER) ||
			HasCondition(COND_HEAR_PLAYER) ||
			HasCondition(COND_HEAR_WORLD) ||
			HasCondition(COND_HEAR_COMBAT))
		{
			return SCHED_ALERT_FACE_BESTSOUND;
		}
		else
		{
			return SCHED_IDLE_STAND; // todo ?? patrol??
		}
		break;
	}
	}

	if (HasCondition(COND_FLOATING_OFF_GROUND))
	{
		SetGravity(1.0);
		SetGroundEntity(NULL);
		return SCHED_FALL_TO_GROUND;
	}

	int nSchedule = BaseClass::SelectSchedule();
	if (nSchedule == SCHED_SMALL_FLINCH)
	{
		m_flNextFlinchTime = gpGlobals->curtime + random->RandomFloat(1, 3);
	}

	return nSchedule;
}

int CBaseParasite::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	if (failedSchedule == SCHED_BACK_AWAY_FROM_ENEMY && failedTask == TASK_FIND_BACKAWAY_FROM_SAVEPOSITION)
	{
		if (HasCondition(COND_SEE_ENEMY))
		{
			return SCHED_RANGE_ATTACK1;
		}
	}

	if (failedSchedule == SCHED_BACK_AWAY_FROM_ENEMY /*|| failedSchedule == SCHED_PATROL_WALK || failedSchedule == SCHED_COMBAT_PATROL*/)
	{
		if (!IsFirmlyOnGround())
		{
			return SCHED_PARASITE_HOP_RANDOMLY;
		}
	}

	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
}

void CTurtleLord::Precache(void)
{
	BaseClass::Precache();
}

void CTurtleLord::Spawn(void)
{
	Precache();
	BaseClass::Spawn();

	NPCInit();
	OnSetGibHealth();

	m_bMidJump = false;
	m_flDistTooFar = 500.0f;
	GetSenses()->SetDistLook(750.0f);
}

Activity CTurtleLord::NPC_TranslateActivity(Activity eNewActivity)
{
	if (eNewActivity == ACT_WALK)
		return ACT_RUN;

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}

void CTurtleLord::IdleSound(void)
{
	HL2MPRules()->EmitSoundToClient(this, "Idle", GetNPCType(), GetGender());
}

void CTurtleLord::AlertSound(void)
{
	HL2MPRules()->EmitSoundToClient(this, "Alert", GetNPCType(), GetGender());
}

void CTurtleLord::PainSound(const CTakeDamageInfo& info)
{
	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
}

void CTurtleLord::DeathSound(const CTakeDamageInfo& info)
{
	HL2MPRules()->EmitSoundToClient(this, "Death", GetNPCType(), GetGender());
}

void CTurtleLord::TelegraphSound(void)
{
	HL2MPRules()->EmitSoundToClient(this, "Alert", GetNPCType(), GetGender());
}

void CTurtleLord::AttackSound(void)
{
	HL2MPRules()->EmitSoundToClient(this, "Attack", GetNPCType(), GetGender());
}

void CTurtleLord::BiteSound(void)
{
	HL2MPRules()->EmitSoundToClient(this, "Bite", GetNPCType(), GetGender());
}

LINK_ENTITY_TO_CLASS(npc_turtle, CTurtleLord);

float CTurtleLord::MaxYawSpeed(void)
{
	switch (GetActivity())
	{
	case ACT_IDLE:
		return 30;

	case ACT_RUN:
	case ACT_WALK:
		return 20;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 15;

	default:
		return 30;
	}

	return BaseClass::MaxYawSpeed();
}

void CBaseParasite::BuildScheduleTestBits(void)
{
	if (!IsCurSchedule(SCHED_PARASITE_HOP_RANDOMLY))
	{
		SetCustomInterruptCondition(COND_PARASITE_ILLEGAL_GROUNDENT);
	}
	else
	{
		ClearCustomInterruptCondition(COND_PARASITE_ILLEGAL_GROUNDENT);
	}
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_turtle, CBaseParasite)

DECLARE_TASK(TASK_PARASITE_HOP_ASIDE)
DECLARE_TASK(TASK_PARASITE_HOP_OFF_NPC)
DECLARE_TASK(TASK_PARASITE_HARASS_HOP)

DECLARE_CONDITION(COND_PARASITE_ILLEGAL_GROUNDENT)

DECLARE_ANIMEVENT(AE_PARASITE_JUMPATTACK)
DECLARE_ANIMEVENT(AE_PARASITE_JUMP_TELEGRAPH)

DEFINE_SCHEDULE
(
	SCHED_PARASITE_RANGE_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_ENEMY				0"
	"		TASK_RANGE_ATTACK1			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_FACE_IDEAL				0"
	"		TASK_WAIT_RANDOM			0.5"
	""
	"	Interrupts"
	"		COND_ENEMY_OCCLUDED"
	"		COND_NO_PRIMARY_AMMO"
)

DEFINE_SCHEDULE
(
	SCHED_PARASITE_AMBUSH,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_WAIT_INDEFINITE		0"

	"	Interrupts"
	"		COND_SEE_ENEMY"
	"		COND_SEE_HATE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
)

DEFINE_SCHEDULE
(
	SCHED_PARASITE_HOP_RANDOMLY,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PARASITE_HOP_OFF_NPC	0"

	"	Interrupts"
)

DEFINE_SCHEDULE
(
	SCHED_PARASITE_HARASS_ENEMY,

	"	Tasks"
	"		TASK_FACE_ENEMY					0"
	"		TASK_PARASITE_HARASS_HOP		0"
	"		TASK_WAIT_FACE_ENEMY			1"
	"		TASK_SET_ROUTE_SEARCH_TIME		2"	// Spend 2 seconds trying to build a path if stuck
	//"		TASK_GET_PATH_TO_RANDOM_NODE	300"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"	Interrupts"
	"		COND_NEW_ENEMY"
)

DEFINE_SCHEDULE
(
	SCHED_PARASITE_FALL_TO_GROUND,

	"	Tasks"
	"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
	"		TASK_FALL_TO_GROUND				0"
	""
	"	Interrupts"
)

AI_END_CUSTOM_NPC()
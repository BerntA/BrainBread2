//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Stationary Soldier AI
//
//========================================================================================//

#include "cbase.h"
#include "npc_base_soldier_static.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_route.h"
#include "ai_interactions.h"
#include "ai_senses.h"
#include "ai_tacticalservices.h"
#include "soundent.h"
#include "npcevent.h"
#include "activitylist.h"
#include "player.h"
#include "engine/IEngineSound.h"
#include "grenade_frag.h"
#include "ndebugoverlay.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "ammodef.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_fSoldierQuestion;

#define SOLDIER_GRENADE_THROW_SPEED 650
#define SOLDIER_GRENADE_TIMER		3.5
#define SOLDIER_GRENADE_FLUSH_TIME	3.0		// Don't try to flush an enemy who has been out of sight for longer than this.
#define SOLDIER_GRENADE_FLUSH_DIST	256.0	// Don't try to flush an enemy who has moved farther than this distance from the last place I saw him.
#define	SOLDIER_MIN_GRENADE_CLEAR_DIST	250

#define SOLDIER_EYE_STANDING_POSITION	Vector( 0, 0, 66 )
#define SOLDIER_GUN_STANDING_POSITION	Vector( 0, 0, 57 )
#define SOLDIER_SHOTGUN_STANDING_POSITION	Vector( 0, 0, 36 )

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionSoldierBash = 0; // melee bash attack

//=========================================================
// Soldiers's Anim Events Go Here
//=========================================================
#define SOLDIER_AE_RELOAD			( 2 )
#define SOLDIER_AE_KICK				( 3 )
#define SOLDIER_AE_AIM				( 4 )
#define SOLDIER_AE_GREN_TOSS		( 7 )
#define SOLDIER_AE_GREN_LAUNCH		( 8 )
#define SOLDIER_AE_GREN_DROP		( 9 )
#define SOLDIER_AE_CAUGHT_ENEMY		( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.

//=========================================================
// Soldier activities
//=========================================================
extern Activity ACT_COMBINE_THROW_GRENADE;
extern Activity ACT_COMBINE_LAUNCH_GRENADE;

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{
	SQUAD_SLOT_GRENADE1 = LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_GRENADE2,
	SQUAD_SLOT_ATTACK_OCCLUDER,
	SQUAD_SLOT_OVERWATCH,
};

#define bits_MEMORY_PAIN_LIGHT_SOUND		bits_MEMORY_CUSTOM1
#define bits_MEMORY_PAIN_HEAVY_SOUND		bits_MEMORY_CUSTOM2
#define bits_MEMORY_PLAYER_HURT				bits_MEMORY_CUSTOM3

//---------------------------------------------------------
// NPC Hammer Input Info - Save / Restore
//---------------------------------------------------------
LINK_ENTITY_TO_CLASS(npc_soldier_base, CNPCBaseSoldierStatic);

BEGIN_DATADESC(CNPCBaseSoldierStatic)
END_DATADESC()

CNPCBaseSoldierStatic::CNPCBaseSoldierStatic()
{
	//UseClientSideAnimation(); // todo: KEEP?
	m_vecTossVelocity = vec3_origin;
	m_iNumGrenades = 1;
}

void CNPCBaseSoldierStatic::Precache()
{
	PrecacheModel("models/weapons/explosives/frag/w_frag_thrown.mdl");
	UTIL_PrecacheOther("npc_handgrenade");

	PrecacheScriptSound("NPC_Combine.GrenadeLaunch");
	PrecacheScriptSound("NPC_Combine.WeaponBash");
	PrecacheScriptSound("BaseEnemyHumanoid.WeaponBash");
	PrecacheScriptSound("Weapon_CombineGuard.Special1");

	BaseClass::Precache();
}

int CNPCBaseSoldierStatic::OnTakeDamage(const CTakeDamageInfo &info)
{
	int tookDamage = BaseClass::OnTakeDamage(info);
	return tookDamage;
}

void CNPCBaseSoldierStatic::FireBullets(const FireBulletsInfo_t &info)
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	FireBulletsInfo_t modinfo = info;

	float flDamage = 1.0f;
	if (GameBaseShared()->GetNPCData() && pWeapon)
	{
		flDamage = GameBaseShared()->GetNPCData()->GetFirearmDamage(GetNPCName(), pWeapon->GetClassname());
		modinfo.m_vecFirstStartPos = GetLocalOrigin();
		modinfo.m_flDropOffDist = pWeapon->GetWpnData().m_flDropOffDistance;
	}

	flDamage += ((flDamage / 100.0f) * m_flDamageScaleValue);

	modinfo.m_flDamage = flDamage;
	modinfo.m_iPlayerDamage = (int)flDamage;

	BaseClass::FireBullets(modinfo);
}

int CNPCBaseSoldierStatic::AllowEntityToBeGibbed(void)
{
	if (m_iHealth > 0)
		return GIB_NO_GIBS;

	return GIB_FULL_GIBS;
}

void CNPCBaseSoldierStatic::Spawn(void)
{
	if (ParseNPC(entindex()))
	{
		SetHealth(m_iTotalHP);
		SetMaxHealth(m_iTotalHP);
		SetKickDamage(m_iDamageKick);
		SetNPCName(GetNPCName());
		SetBoss(IsBoss());

		PrecacheModel(GetNPCModelName());
		SetModel(GetNPCModelName());

		m_nSkin = m_iModelSkin;
		UpdateNPCScaling();
	}
	else
	{
		UTIL_Remove(this);
		return;
	}

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_NONE);
	SetBloodColor(BLOOD_COLOR_RED);
	m_flFieldOfView = -0.2;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->curtime + 1;
	m_flNextPainSoundTime = 0;
	m_flNextAlertSoundTime = 0;

	CapabilitiesAdd(bits_CAP_TURN_HEAD);
	CapabilitiesAdd(bits_CAP_AIM_GUN);
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1);
	CapabilitiesAdd(bits_CAP_SQUAD);
	CapabilitiesAdd(bits_CAP_USE_WEAPONS);
	CapabilitiesAdd(bits_CAP_NO_HIT_SQUADMATES);
	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);

	m_bFirstEncounter = true; // this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	m_flNextLostSoundTime = 0;

	NPCInit();

	m_flDistTooFar = 512.0;
	GetSenses()->SetDistLook(1024.0);

	OnSetGibHealth();
}

void CNPCBaseSoldierStatic::GatherConditions()
{
	BaseClass::GatherConditions();

	ClearCondition(COND_SOLDIER_ATTACK_SLOT_AVAILABLE);

	if (GetState() == NPC_STATE_COMBAT)
	{
		// Soldiers that are standing around doing nothing poll for attack slots so
		// that they can respond quickly when one comes available. If they can 
		// occupy a vacant attack slot, they do so. This holds the slot until their
		// schedule breaks and schedule selection runs again, essentially reserving this
		// slot. If they do not select an attack schedule, then they'll release the slot.
		if (OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2))
		{
			SetCondition(COND_SOLDIER_ATTACK_SLOT_AVAILABLE);
		}
	}
}

void CNPCBaseSoldierStatic::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	if (IsOnFire())
		SetCondition(COND_SOLDIER_ON_FIRE);
	else
		ClearCondition(COND_SOLDIER_ON_FIRE);

	extern ConVar ai_debug_shoot_positions;
	if (ai_debug_shoot_positions.GetBool())
		NDebugOverlay::Cross3D(EyePosition(), 16, 0, 255, 0, false, 0.1);
}

float CNPCBaseSoldierStatic::MaxYawSpeed(void)
{
	switch (GetActivity())
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 45;
		break;
	case ACT_RUN:
	case ACT_RUN_HURT:
		return 15;
		break;
	case ACT_WALK:
	case ACT_WALK_CROUCH:
		return 25;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		return 35;
	default:
		return 35;
		break;
	}
}

Class_T	CNPCBaseSoldierStatic::Classify(void)
{
	return CLASS_COMBINE;
}

void CNPCBaseSoldierStatic::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SOLDIER_SIGNAL_BEST_SOUND:
		if (IsInSquad() && GetSquad()->NumMembers() > 1)
		{
			CBasePlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
			if (pPlayer && OccupyStrategySlot(SQUAD_SLOT_EXCLUSIVE_HANDSIGN) && pPlayer->FInViewCone(this))
			{
				CSound *pSound;
				pSound = GetBestSound();

				Assert(pSound != NULL);

				if (pSound)
				{
					Vector right, tosound;

					GetVectors(NULL, &right, NULL);

					tosound = pSound->GetSoundReactOrigin() - GetAbsOrigin();
					VectorNormalize(tosound);

					tosound.z = 0;
					right.z = 0;

					if (DotProduct(right, tosound) > 0)
					{
						// Right
						SetIdealActivity(ACT_SIGNAL_RIGHT);
					}
					else
					{
						// Left
						SetIdealActivity(ACT_SIGNAL_LEFT);
					}

					break;
				}
			}
		}

		// Otherwise, just skip it.
		TaskComplete();
		break;

	case TASK_ANNOUNCE_ATTACK:
	{
		// If Primary Attack
		if ((int)pTask->flTaskData == 1)
		{
			// -----------------------------------------------------------
			// If enemy isn't facing me and I haven't attacked in a while
			// annouce my attack before I start wailing away
			// -----------------------------------------------------------
			CBaseCombatCharacter *pBCC = GetEnemyCombatCharacterPointer();

			if (pBCC && pBCC->IsPlayer() && (!pBCC->FInViewCone(this)) &&
				(gpGlobals->curtime - m_flLastAttackTime > 3.0))
			{
				m_flLastAttackTime = gpGlobals->curtime;

				int iRand = random->RandomInt(1, 2);

				if (iRand == 1)
					HL2MPRules()->EmitSoundToClient(this, "Incoming", GetNPCType(), GetGender());
				else
					HL2MPRules()->EmitSoundToClient(this, "Contact", GetNPCType(), GetGender());

				// Wait two seconds
				SetWait(2.0);

				SetActivity(ACT_IDLE);
			}
			// -------------------------------------------------------------
			//  Otherwise move on
			// -------------------------------------------------------------
			else
			{
				TaskComplete();
			}
		}
		else
		{
			HL2MPRules()->EmitSoundToClient(this, "Grenade", GetNPCType(), GetGender());
			SetActivity(ACT_IDLE);
			SetWait(2.0);
		}
		break;
	}

	case TASK_SOLDIER_FACE_TOSS_DIR:
		break;

	case TASK_SOLDIER_IGNORE_ATTACKS:
		// must be in a squad
		if (m_pSquad && m_pSquad->NumMembers() > 2)
		{
			// the enemy must be far enough away
			if (GetEnemy() && (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).Length() > 512.0)
			{
				m_flNextAttack = gpGlobals->curtime + pTask->flTaskData;
			}
		}
		TaskComplete();
		break;

	case TASK_SOLDIER_DEFER_SQUAD_GRENADES:
	{
		if (m_pSquad)
		{
			// iterate my squad and stop everyone from throwing grenades for a little while.
			AISquadIter_t iter;

			CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember(&iter) : NULL;
			while (pSquadmate)
			{
				CNPCBaseSoldierStatic *pComrade = dynamic_cast<CNPCBaseSoldierStatic*>(pSquadmate);
				if (pComrade)
					pComrade->m_flNextGrenadeCheck = gpGlobals->curtime + 5;

				pSquadmate = m_pSquad->GetNextMember(&iter);
			}
		}

		TaskComplete();
		break;
	}

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
	{
		if (pTask->iTask == TASK_FACE_ENEMY && HasCondition(COND_CAN_RANGE_ATTACK1))
		{
			TaskComplete();
			return;
		}

		BaseClass::StartTask(pTask);
		bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
		if (bIsFlying)
		{
			SetIdealActivity(ACT_GLIDE);
		}

	}
	break;

	case TASK_RANGE_ATTACK1:
	{
		if (GetActiveWeapon())
		{
			m_nShots = GetActiveWeapon()->GetRandomBurst();
			m_flShotDelay = GetActiveWeapon()->GetFireRate();
		}

		m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
		ResetIdealActivity(ACT_RANGE_ATTACK1);
		m_flLastAttackTime = gpGlobals->curtime;
	}
	break;

	case TASK_SOLDIER_DIE_INSTANTLY:
	{
		CTakeDamageInfo info;

		info.SetAttacker(this);
		info.SetInflictor(this);
		info.SetDamage(m_iHealth);
		info.SetDamageType(pTask->flTaskData);
		info.SetDamageForce(Vector(0.1, 0.1, 0.1));

		TakeDamage(info);

		TaskComplete();
	}
	break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

void CNPCBaseSoldierStatic::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SOLDIER_SIGNAL_BEST_SOUND:
		AutoMovement();
		if (IsActivityFinished())
		{
			TaskComplete();
		}
		break;

	case TASK_ANNOUNCE_ATTACK:
	{
		// Stop waiting if enemy facing me or lost enemy
		CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();
		if (!pBCC || pBCC->FInViewCone(this))
		{
			TaskComplete();
		}

		if (IsWaitFinished())
		{
			TaskComplete();
		}
	}
	break;

	case TASK_SOLDIER_FACE_TOSS_DIR:
	{
		// project a point along the toss vector and turn to face that point.
		GetMotor()->SetIdealYawToTargetAndUpdate(GetLocalOrigin() + m_vecTossVelocity * 64, AI_KEEP_YAW_SPEED);

		if (FacingIdeal())
		{
			TaskComplete(true);
		}
		break;
	}

	case TASK_RANGE_ATTACK1:
	{
		AutoMovement();

		Vector vecEnemyLKP = GetEnemyLKP();
		if (!FInAimCone(vecEnemyLKP))
		{
			GetMotor()->SetIdealYawToTargetAndUpdate(vecEnemyLKP, AI_KEEP_YAW_SPEED);
		}
		else
		{
			GetMotor()->SetIdealYawAndUpdate(GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED);
		}

		if (gpGlobals->curtime >= m_flNextAttack)
		{
			if (IsActivityFinished())
			{
				if (--m_nShots > 0)
				{
					// DevMsg("ACT_RANGE_ATTACK1\n");
					ResetIdealActivity(ACT_RANGE_ATTACK1);
					m_flLastAttackTime = gpGlobals->curtime;
					m_flNextAttack = gpGlobals->curtime + m_flShotDelay - 0.1;
				}
				else
				{
					// DevMsg("TASK_RANGE_ATTACK1 complete\n");
					TaskComplete();
				}
			}
		}
		else
		{
			// DevMsg("Wait\n");
		}
	}
	break;

	default:
	{
		BaseClass::RunTask(pTask);
		break;
	}
	}
}

void CNPCBaseSoldierStatic::Event_Killed(const CTakeDamageInfo &info)
{
	HL2MPRules()->DeathNotice(this, info);

	// If I was killed before I could finish throwing my grenade, drop
	// a grenade item that the player can retrieve.
	if ((GetActivity() == ACT_RANGE_ATTACK2) && !HasSpawnFlags(SF_NPC_NO_WEAPON_DROP))
	{
		if (m_iLastAnimEventHandled != SOLDIER_AE_GREN_TOSS)
		{
			// Drop the grenade as an item.
			Vector vecStart;
			GetAttachment("lefthand", vecStart);

			CBaseEntity *pItem = DropItem("weapon_frag", vecStart, RandomAngle(0, 360));
			if (pItem)
			{
				pItem->AddSpawnFlags(SF_NORESPAWN);
				IPhysicsObject *pObj = pItem->VPhysicsGetObject();
				if (pObj)
				{
					Vector			vel;
					vel.x = random->RandomFloat(-100.0f, 100.0f);
					vel.y = random->RandomFloat(-100.0f, 100.0f);
					vel.z = random->RandomFloat(800.0f, 1200.0f);
					AngularImpulse	angImp = RandomAngularImpulse(-300.0f, 300.0f);

					vel[2] = 0.0f;
					pObj->AddVelocity(&vel, &angImp);
				}
			}
		}
	}

	BaseClass::Event_Killed(info);
}

void CNPCBaseSoldierStatic::BuildScheduleTestBits(void)
{
	BaseClass::BuildScheduleTestBits();

	if (gpGlobals->curtime < m_flNextAttack)
	{
		ClearCustomInterruptCondition(COND_CAN_RANGE_ATTACK1);
		ClearCustomInterruptCondition(COND_CAN_RANGE_ATTACK2);
	}

	if (!IsCurSchedule(SCHED_SOLDIER_BURNING_STAND))
	{
		SetCustomInterruptCondition(COND_SOLDIER_ON_FIRE);
	}
}

Activity CNPCBaseSoldierStatic::NPC_TranslateActivity(Activity eNewActivity)
{
	if (ai_show_active_military_activities.GetBool())
		Msg("Running Activity %i Act Name: %s\n", eNewActivity, GetActivityName(eNewActivity));

	if (HasCondition(COND_SOLDIER_ON_FIRE))
		return BaseClass::NPC_TranslateActivity(ACT_IDLE_ON_FIRE);

	if (eNewActivity == ACT_RANGE_ATTACK2)
	{
		return (Activity)ACT_COMBINE_THROW_GRENADE;
	}
	else if (eNewActivity == ACT_IDLE)
	{
		if (!IsCrouching() && (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT))
		{
			eNewActivity = ACT_IDLE_ANGRY;
		}
	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}

int CNPCBaseSoldierStatic::GetSoundInterests(void)
{
	return	SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER | SOUND_PHYSICS_DANGER | SOUND_BULLET_IMPACT | SOUND_MOVE_AWAY;
}

bool CNPCBaseSoldierStatic::QueryHearSound(CSound *pSound)
{
	if (pSound->SoundContext() & SOUND_CONTEXT_COMBINE_ONLY)
		return true;

	if (pSound->SoundContext() & SOUND_CONTEXT_EXCLUDE_COMBINE)
		return false;

	return BaseClass::QueryHearSound(pSound);
}

void CNPCBaseSoldierStatic::AnnounceEnemyType(CBaseEntity *pEnemy)
{
	const char *pSentenceName = "COMBINE_MONST";
	switch (pEnemy->Classify())
	{
	case CLASS_PLAYER:
		pSentenceName = "COMBINE_ALERT";
		break;

	case CLASS_MILITARY:
	case CLASS_MILITARY_VEHICLE:
		pSentenceName = "COMBINE_MONST_CITIZENS";
		break;

	case CLASS_ZOMBIE_BOSS:
	case CLASS_ZOMBIE:
		pSentenceName = "COMBINE_MONST_ZOMBIES";
		break;
	}

	HL2MPRules()->EmitSoundToClient(this, "Contact", GetNPCType(), GetGender());
}

void CNPCBaseSoldierStatic::AnnounceEnemyKill(CBaseEntity *pEnemy)
{
	if (!pEnemy)
		return;

	const char *pSentenceName = "COMBINE_KILL_MONST";
	switch (pEnemy->Classify())
	{
	case CLASS_PLAYER:
		pSentenceName = "COMBINE_PLAYER_DEAD";
		break;

		// no sentences for these guys yet
	case CLASS_MILITARY:
	case CLASS_MILITARY_VEHICLE:
		break;

	case CLASS_ZOMBIE_BOSS:
	case CLASS_ZOMBIE:
		break;
	}

	HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());
}

int CNPCBaseSoldierStatic::SelectCombatSchedule()
{
	// -----------
	// dead enemy
	// -----------
	if (HasCondition(COND_ENEMY_DEAD))
	{
		// call base class, all code to handle dead enemies is centralized there.
		return SCHED_NONE;
	}

	// -----------
	// new enemy
	// -----------
	if (HasCondition(COND_NEW_ENEMY))
	{
		CBaseEntity *pEnemy = GetEnemy();
		bool bFirstContact = false;
		float flTimeSinceFirstSeen = gpGlobals->curtime - GetEnemies()->FirstTimeSeen(pEnemy);

		if (flTimeSinceFirstSeen < 3.0f)
			bFirstContact = true;

		if (m_pSquad && pEnemy)
		{
			if (HasCondition(COND_SEE_ENEMY))
			{
				AnnounceEnemyType(pEnemy);
			}

			if (HasCondition(COND_CAN_RANGE_ATTACK1) && OccupyStrategySlot(SQUAD_SLOT_ATTACK1))
			{
				// Start suppressing if someone isn't firing already (SLOT_ATTACK1). This means
				// I'm the guy who spotted the enemy, I should react immediately.
				return SCHED_SOLDIER_SUPPRESS;
			}

			if (m_pSquad->IsLeader(this) || (m_pSquad->GetLeader() && m_pSquad->GetLeader()->GetEnemy() != pEnemy))
			{
				// I'm the leader, but I didn't get the job suppressing the enemy. We know this because
				// This code only runs if the code above didn't assign me SCHED_COMBINE_SUPPRESS.
				if (HasCondition(COND_CAN_RANGE_ATTACK1) && OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2))
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
			else
			{
				if (m_pSquad->GetLeader() && FOkToMakeSound(SENTENCE_PRIORITY_MEDIUM))
				{
					JustMadeSound(SENTENCE_PRIORITY_MEDIUM);	// squelch anything that isn't high priority so the leader can speak
				}

				// First contact, and I'm solo, or not the squad leader.
				if (HasCondition(COND_SEE_ENEMY) && CanGrenadeEnemy())
				{
					if (OccupyStrategySlot(SQUAD_SLOT_GRENADE1))
					{
						return SCHED_RANGE_ATTACK2;
					}
				}

				if (!bFirstContact && OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2))
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
		}
	}

	// ---------------------
	// no ammo
	// ---------------------
	if ((HasCondition(COND_NO_PRIMARY_AMMO) || HasCondition(COND_LOW_PRIMARY_AMMO)) && !HasCondition(COND_CAN_MELEE_ATTACK1))
	{
		return SCHED_RELOAD;
	}

	// ----------------------
	// LIGHT DAMAGE
	// ----------------------
	if (HasCondition(COND_LIGHT_DAMAGE))
	{
	}

	// If I'm scared of this enemy run away
	if (IRelationType(GetEnemy()) == D_FR)
	{
		// If I've seen the enemy recently, cower. Ignore the time for unforgettable enemies.
		AI_EnemyInfo_t *pMemory = GetEnemies()->Find(GetEnemy());
		if ((pMemory && pMemory->bUnforgettable) || (GetEnemyLastTimeSeen() > (gpGlobals->curtime - 5.0)))
		{
			// If we're facing him, just look ready. Otherwise, face him.
			if (FInAimCone(GetEnemy()->EyePosition()))
				return SCHED_COMBAT_STAND;

			return SCHED_FEAR_FACE;
		}
	}

	int attackSchedule = SelectScheduleAttack();
	if (attackSchedule != SCHED_NONE)
		return attackSchedule;

	if (HasCondition(COND_ENEMY_OCCLUDED))
	{
		// stand up, just in case
		Stand();
		DesireStand();
	}

	return SCHED_NONE;
}

int CNPCBaseSoldierStatic::SelectSchedule(void)
{
	if (HasCondition(COND_SOLDIER_ON_FIRE))
		return SCHED_SOLDIER_BURNING_STAND;

	int nSched = SelectFlinchSchedule();
	if (nSched != SCHED_NONE)
		return nSched;

	if (m_NPCState != NPC_STATE_SCRIPT)
	{
		// These things are done in any state but dead and prone
		if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE)
		{
			// Cower when physics objects are thrown at me
			if (HasCondition(COND_HEAR_PHYSICS_DANGER))
			{
				return SCHED_FLINCH_PHYSICS;
			}

			// grunts place HIGH priority on running away from danger sounds.
			if (HasCondition(COND_HEAR_DANGER))
			{
				CSound *pSound;
				pSound = GetBestSound();

				Assert(pSound != NULL);
				if (pSound)
				{
					if (pSound->m_iType & SOUND_DANGER)
					{
						// I hear something dangerous, probably need to take cover.
						// dangerous sound nearby!, call it out

						HL2MPRules()->EmitSoundToClient(this, "Incoming", GetNPCType(), GetGender());

						// If the sound is approaching danger, I have no enemy, and I don't see it, turn to face.
						if (!GetEnemy() && pSound->IsSoundType(SOUND_CONTEXT_DANGER_APPROACH) && pSound->m_hOwner && !FInViewCone(pSound->GetSoundReactOrigin()))
						{
							GetMotor()->SetIdealYawToTarget(pSound->GetSoundReactOrigin());
							return SCHED_SOLDIER_FACE_IDEAL_YAW;
						}
					}

					// JAY: This was disabled in HL1.  Test?
					if (!HasCondition(COND_SEE_ENEMY) && (pSound->m_iType & (SOUND_PLAYER | SOUND_COMBAT)))
					{
						GetMotor()->SetIdealYawToTarget(pSound->GetSoundReactOrigin());
					}
				}
			}
		}

		if (BehaviorSelectSchedule())
		{
			return BaseClass::SelectSchedule();
		}
	}

	switch (m_NPCState)
	{
	case NPC_STATE_IDLE:
	{
	}

	case NPC_STATE_ALERT:
	{
		if (HasCondition(COND_HEAR_COMBAT))
		{
			CSound *pSound = GetBestSound();

			if (pSound && pSound->IsSoundType(SOUND_COMBAT))
			{
				if (m_pSquad && m_pSquad->GetSquadMemberNearestTo(pSound->GetSoundReactOrigin()) == this && OccupyStrategySlot(SQUAD_SLOT_INVESTIGATE_SOUND))
				{
				}
			}
		}
	}
	break;

	case NPC_STATE_COMBAT:
	{
		int nSched = SelectCombatSchedule();
		if (nSched != SCHED_NONE)
			return nSched;
	}
	break;
	}

	// no special cases here, call the base class
	return BaseClass::SelectSchedule();
}

int CNPCBaseSoldierStatic::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
}

int CNPCBaseSoldierStatic::SelectScheduleAttack()
{
	// Drop a grenade?
	if (HasCondition(COND_SOLDIER_DROP_GRENADE))
		return SCHED_SOLDIER_DROP_GRENADE;

	// Kick attack?
	if (HasCondition(COND_CAN_MELEE_ATTACK1))
	{
		return SCHED_MELEE_ATTACK1;
	}

	// Can I shoot?
	if (HasCondition(COND_CAN_RANGE_ATTACK1))
	{
		// Engage if allowed
		if (OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2))
		{
			return SCHED_RANGE_ATTACK1;
		}

		// Throw a grenade if not allowed to engage with weapon.
		if (CanGrenadeEnemy())
		{
			if (OccupyStrategySlot(SQUAD_SLOT_GRENADE1))
			{
				return SCHED_RANGE_ATTACK2;
			}
		}
	}

	if (GetEnemy() && !HasCondition(COND_SEE_ENEMY))
	{
		// We don't see our enemy. If it hasn't been long since I last saw him,
		// and he's pretty close to the last place I saw him, throw a grenade in 
		// to flush him out. A wee bit of cheating here...

		float flTime;
		float flDist;

		flTime = gpGlobals->curtime - GetEnemies()->LastTimeSeen(GetEnemy());
		flDist = (GetEnemy()->GetAbsOrigin() - GetEnemies()->LastSeenPosition(GetEnemy())).Length();

		//Msg("Time: %f   Dist: %f\n", flTime, flDist );
		if (flTime <= SOLDIER_GRENADE_FLUSH_TIME && flDist <= SOLDIER_GRENADE_FLUSH_DIST && CanGrenadeEnemy(false) && OccupyStrategySlot(SQUAD_SLOT_GRENADE1))
		{
			return SCHED_RANGE_ATTACK2;
		}
	}

	if (HasCondition(COND_WEAPON_SIGHT_OCCLUDED))
	{
		// If they are hiding behind something that we can destroy, start shooting at it.
		CBaseEntity *pBlocker = GetEnemyOccluder();
		if (pBlocker && pBlocker->GetHealth() > 0 && OccupyStrategySlot(SQUAD_SLOT_ATTACK_OCCLUDER))
		{
			return SCHED_SHOOT_ENEMY_COVER;
		}
	}

	return SCHED_NONE;
}

int CNPCBaseSoldierStatic::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_RANGE_ATTACK1:
	{
		if (HasCondition(COND_NO_PRIMARY_AMMO) || HasCondition(COND_LOW_PRIMARY_AMMO))
		{
			// Ditch the strategy slot for attacking (which we just reserved!)
			VacateStrategySlot();
			return TranslateSchedule(SCHED_RELOAD);
		}

		// always assume standing
		Stand();

		return SCHED_SOLDIER_RANGE_ATTACK1;
	}
	case SCHED_RANGE_ATTACK2:
	{
		// If my weapon can range attack 2 use the weapon
		if (GetActiveWeapon() && GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK2)
		{
			return SCHED_RANGE_ATTACK2;
		}
		// Otherwise use innate attack
		else
		{
			return SCHED_SOLDIER_RANGE_ATTACK2;
		}
	}
	case SCHED_SOLDIER_SUPPRESS:
	{
#define MIN_SIGNAL_DIST	256
		if (GetEnemy() != NULL && GetEnemy()->IsPlayer() && m_bFirstEncounter)
		{
			float flDistToEnemy = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length();

			if (flDistToEnemy >= MIN_SIGNAL_DIST)
			{
				m_bFirstEncounter = false;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return SCHED_SOLDIER_SIGNAL_SUPPRESS;
			}
		}

		return SCHED_SOLDIER_SUPPRESS;
	}
	case SCHED_FAIL:
	{
		if (GetEnemy() != NULL)
		{
			return SCHED_SOLDIER_COMBAT_FAIL;
		}
		return SCHED_FAIL;
	}
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

void CNPCBaseSoldierStatic::OnStartSchedule(int scheduleType)
{
}

void CNPCBaseSoldierStatic::HandleAnimEvent(animevent_t *pEvent)
{
	Vector vecShootDir;
	Vector vecShootOrigin;
	bool handledEvent = false;

	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		BaseClass::HandleAnimEvent(pEvent);
	}
	else
	{
		switch (pEvent->event)
		{
		case SOLDIER_AE_AIM:
		{
			handledEvent = true;
			break;
		}
		case SOLDIER_AE_RELOAD:

			// We never actually run out of ammo, just need to refill the clip
			if (GetActiveWeapon())
			{
				GetActiveWeapon()->WeaponSound(RELOAD_NPC);
				GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1();
				GetActiveWeapon()->m_iClip2 = GetActiveWeapon()->GetMaxClip2();
			}
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
			handledEvent = true;
			break;

		case SOLDIER_AE_GREN_TOSS:
		{
			Vector vecSpin;
			vecSpin.x = random->RandomFloat(-1000.0, 1000.0);
			vecSpin.y = random->RandomFloat(-1000.0, 1000.0);
			vecSpin.z = random->RandomFloat(-1000.0, 1000.0);

			Vector vecStart;
			GetAttachment("lefthand", vecStart);

			if (m_NPCState == NPC_STATE_SCRIPT)
			{
				// Use a fixed velocity for grenades thrown in scripted state.
				// Grenades thrown from a script do not count against grenades remaining for the AI to use.
				Vector forward, up, vecThrow;

				GetVectors(&forward, NULL, &up);
				vecThrow = forward * 750 + up * 175;
				Fraggrenade_Create(vecStart, vec3_angle, vecThrow, vecSpin, this, SOLDIER_GRENADE_TIMER, true);
			}
			else
			{
				// Use the Velocity that AI gave us.
				Fraggrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, SOLDIER_GRENADE_TIMER, true);
				m_iNumGrenades--;
			}

			// wait six seconds before even looking again to see if a grenade can be thrown.
			m_flNextGrenadeCheck = gpGlobals->curtime + 6;
		}
		handledEvent = true;
		break;

		case SOLDIER_AE_GREN_LAUNCH:
		{
			EmitSound("NPC_Combine.GrenadeLaunch");

			CBaseEntity *pGrenade = CreateNoSpawn("npc_contactgrenade", Weapon_ShootPosition(), vec3_angle, this);
			pGrenade->KeyValue("velocity", m_vecTossVelocity);
			pGrenade->Spawn();

			if (g_pGameRules->IsSkillLevel(SKILL_HARD))
				m_flNextGrenadeCheck = gpGlobals->curtime + random->RandomFloat(2, 5);// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		handledEvent = true;
		break;

		case SOLDIER_AE_GREN_DROP:
		{
			Vector vecStart;
			GetAttachment("lefthand", vecStart);

			Fraggrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vec3_origin, this, SOLDIER_GRENADE_TIMER, true);
			m_iNumGrenades--;
		}
		handledEvent = true;
		break;

		case SOLDIER_AE_KICK:
		{
			// Does no damage, because damage is applied based upon whether the target can handle the interaction
			CBaseEntity *pHurt = CheckTraceHullAttack(70, -Vector(16, 16, 18), Vector(16, 16, 18), 0, DMG_CLUB);
			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(pHurt);
			if (pBCC)
			{
				Vector forward, up;
				AngleVectors(GetLocalAngles(), &forward, NULL, &up);

				if (!pBCC->DispatchInteraction(g_interactionSoldierBash, NULL, this))
				{
					if (pBCC->IsPlayer())
					{
						pBCC->ViewPunch(QAngle(-12, -7, 0));
						pHurt->ApplyAbsVelocityImpulse(forward * 100 + up * 50);
					}

					CTakeDamageInfo info(this, this, m_nKickDamage, DMG_CLUB);
					CalculateMeleeDamageForce(&info, forward, pBCC->GetAbsOrigin());
					pBCC->TakeDamage(info);

					EmitSound("BaseEnemyHumanoid.WeaponBash");
				}
			}

			// kick sound
			handledEvent = true;
			break;
		}

		case SOLDIER_AE_CAUGHT_ENEMY:
			HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());
			handledEvent = true;
			break;

		default:
			BaseClass::HandleAnimEvent(pEvent);
			break;
		}
	}

	if (handledEvent)
	{
		m_iLastAnimEventHandled = pEvent->event;
	}
}

Vector CNPCBaseSoldierStatic::Weapon_ShootPosition()
{
	Vector right;
	GetVectors(NULL, &right, NULL);
	if (HasShotgun())
	{
		return GetAbsOrigin() + SOLDIER_SHOTGUN_STANDING_POSITION + right * 8;
	}
	else
	{
		return GetAbsOrigin() + SOLDIER_GUN_STANDING_POSITION + right * 8;
	}
}

void CNPCBaseSoldierStatic::PainSound(void)
{
	// NOTE: The response system deals with this at the moment
	if (GetFlags() & FL_DISSOLVING)
		return;

	if (gpGlobals->curtime > m_flNextPainSoundTime)
	{
		const char *pSentenceName = "Taunt";
		float healthRatio = (float)GetHealth() / (float)GetMaxHealth();
		if (!HasMemory(bits_MEMORY_PAIN_LIGHT_SOUND) && healthRatio > 0.9)
		{
			Remember(bits_MEMORY_PAIN_LIGHT_SOUND);
			pSentenceName = "Contact";
		}
		else if (!HasMemory(bits_MEMORY_PAIN_HEAVY_SOUND) && healthRatio < 0.25)
		{
			Remember(bits_MEMORY_PAIN_HEAVY_SOUND);
			pSentenceName = "Pain";
		}

		HL2MPRules()->EmitSoundToClient(this, pSentenceName, GetNPCType(), GetGender());
		m_flNextPainSoundTime = gpGlobals->curtime + 2;
	}
}

void CNPCBaseSoldierStatic::LostEnemySound(void)
{
	if (gpGlobals->curtime <= m_flNextLostSoundTime)
		return;

	HL2MPRules()->EmitSoundToClient(this, "ContactLost", GetNPCType(), GetGender());
	m_flNextLostSoundTime = gpGlobals->curtime + random->RandomFloat(5.0, 15.0);
}

void CNPCBaseSoldierStatic::FoundEnemySound(void)
{
	HL2MPRules()->EmitSoundToClient(this, "Contact", GetNPCType(), GetGender());
}

void CNPCBaseSoldierStatic::AlertSound(void)
{
	if (gpGlobals->curtime > m_flNextAlertSoundTime)
	{
		// go alert?
		m_flNextAlertSoundTime = gpGlobals->curtime + 10.0f;
	}
}

void CNPCBaseSoldierStatic::NotifyDeadFriend(CBaseEntity* pFriend)
{
	if (GetSquad()->NumMembers() < 2)
	{
		JustMadeSound();
		return;
	}

	BaseClass::NotifyDeadFriend(pFriend);
}

void CNPCBaseSoldierStatic::DeathSound(void)
{
	// NOTE: The response system deals with this at the moment
	if (GetFlags() & FL_DISSOLVING)
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

void CNPCBaseSoldierStatic::IdleSound(void)
{
	if (g_fSoldierQuestion || random->RandomInt(0, 1))
	{
		if (!g_fSoldierQuestion)
		{
			// ask question or make statement
			switch (random->RandomInt(0, 2))
			{
			case 0: // check in
				HL2MPRules()->EmitSoundToClient(this, "Question2", GetNPCType(), GetGender());
				g_fSoldierQuestion = 1;
				break;

			case 1: // question
				HL2MPRules()->EmitSoundToClient(this, "Question2", GetNPCType(), GetGender());
				g_fSoldierQuestion = 2;
				break;

			case 2: // statement
				HL2MPRules()->EmitSoundToClient(this, "Idle", GetNPCType(), GetGender());
				break;
			}
		}
		else
		{
			switch (g_fSoldierQuestion)
			{
			case 1: // check in
				HL2MPRules()->EmitSoundToClient(this, "Answer1", GetNPCType(), GetGender());
				g_fSoldierQuestion = 0;
				break;
			case 2: // question 
				HL2MPRules()->EmitSoundToClient(this, "Answer2", GetNPCType(), GetGender());
				g_fSoldierQuestion = 0;
				break;
			}
		}
	}
}

int	CNPCBaseSoldierStatic::RangeAttack2Conditions(float flDot, float flDist)
{
	return COND_NONE;
}

bool CNPCBaseSoldierStatic::CanThrowGrenade(const Vector &vecTarget)
{
	if (m_iNumGrenades < 1)
	{
		// Out of grenades!
		return false;
	}

	if (gpGlobals->curtime < m_flNextGrenadeCheck)
	{
		// Not allowed to throw another grenade right now.
		return false;
	}

	float flDist;
	flDist = (vecTarget - GetAbsOrigin()).Length();

	if (flDist > 1024 || flDist < 128)
	{
		// Too close or too far!
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}

	// -----------------------
	// If moving, don't check.
	// -----------------------
	if (m_flGroundSpeed != 0)
		return false;

	// ---------------------------------------------------------------------
	// Are any of my squad members near the intended grenade impact area?
	// ---------------------------------------------------------------------
	if (m_pSquad)
	{
		if (m_pSquad->SquadMemberInRange(vecTarget, SOLDIER_MIN_GRENADE_CLEAR_DIST))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.

			// Tell my squad members to clear out so I can get a grenade in
			CSoundEnt::InsertSound(SOUND_MOVE_AWAY | SOUND_CONTEXT_COMBINE_ONLY, vecTarget, SOLDIER_MIN_GRENADE_CLEAR_DIST, 0.1);
			return false;
		}
	}

	return CheckCanThrowGrenade(vecTarget);
}

bool CNPCBaseSoldierStatic::CheckCanThrowGrenade(const Vector &vecTarget)
{
	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: this is only valid for hand grenades, not RPG's
	Vector vecToss;
	Vector vecMins = -Vector(4, 4, 4);
	Vector vecMaxs = Vector(4, 4, 4);
	if (FInViewCone(vecTarget) && CBaseEntity::FVisible(vecTarget))
	{
		vecToss = VecCheckThrow(this, EyePosition(), vecTarget, SOLDIER_GRENADE_THROW_SPEED, 1.0, &vecMins, &vecMaxs);
	}
	else
	{
		// Have to try a high toss. Do I have enough room?
		trace_t tr;
		AI_TraceLine(EyePosition(), EyePosition() + Vector(0, 0, 64), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			return false;
		}

		vecToss = VecCheckToss(this, EyePosition(), vecTarget, -1, 1.0, true, &vecMins, &vecMaxs);
	}

	if (vecToss != vec3_origin)
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // 1/3 second.
		return true;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return false;
	}
}

bool CNPCBaseSoldierStatic::CanGrenadeEnemy(bool bUseFreeKnowledge)
{
	CBaseEntity *pEnemy = GetEnemy();

	Assert(pEnemy != NULL);

	if (pEnemy)
	{
		if (bUseFreeKnowledge)
		{
			// throw to where we think they are.
			return CanThrowGrenade(GetEnemies()->LastKnownPosition(pEnemy));
		}
		else
		{
			// hafta throw to where we last saw them.
			return CanThrowGrenade(GetEnemies()->LastSeenPosition(pEnemy));
		}
	}

	return false;
}

int CNPCBaseSoldierStatic::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (flDist > 64)
	{
		return COND_NONE; // COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NONE; // COND_NOT_FACING_ATTACK;
	}

	// Check Z
	if (GetEnemy() && fabs(GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > 64)
		return COND_NONE;

	// Make sure not trying to kick through a window or something. 
	trace_t tr;
	Vector vecSrc, vecEnd;

	vecSrc = WorldSpaceCenter();
	vecEnd = GetEnemy()->WorldSpaceCenter();

	AI_TraceLine(vecSrc, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	if (tr.m_pEnt != GetEnemy())
	{
		return COND_NONE;
	}

	return COND_CAN_MELEE_ATTACK1;
}

Vector CNPCBaseSoldierStatic::EyePosition(void)
{
	return GetAbsOrigin() + SOLDIER_EYE_STANDING_POSITION;
}

Vector CNPCBaseSoldierStatic::EyeOffset(Activity nActivity)
{
	return SOLDIER_EYE_STANDING_POSITION;
}

void CNPCBaseSoldierStatic::SetActivity(Activity NewActivity)
{
	BaseClass::SetActivity(NewActivity);
	m_iLastAnimEventHandled = -1;
}

NPC_STATE CNPCBaseSoldierStatic::SelectIdealState(void)
{
	switch (m_NPCState)
	{
	case NPC_STATE_COMBAT:
	{
		if (GetEnemy() == NULL)
		{
			if (!HasCondition(COND_ENEMY_DEAD))
			{
				// todo: LOOK FOR NEW TARGET?
			}

			return NPC_STATE_ALERT;
		}
		else if (HasCondition(COND_ENEMY_DEAD))
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

WeaponProficiency_t CNPCBaseSoldierStatic::CalcWeaponProficiency(CBaseCombatWeapon *pWeapon)
{
	if (FClassnameIs(pWeapon, "weapon_ak47"))
	{
		return WEAPON_PROFICIENCY_GOOD;
	}
	else if (FClassnameIs(pWeapon, "weapon_beretta") || FClassnameIs(pWeapon, "weapon_famas"))
	{
		return WEAPON_PROFICIENCY_VERY_GOOD;
	}
	else if (FClassnameIs(pWeapon, "weapon_remington") || FClassnameIs(pWeapon, "weapon_sawedoff"))
	{
		return WEAPON_PROFICIENCY_PERFECT;
	}

	return BaseClass::CalcWeaponProficiency(pWeapon);
}

bool CNPCBaseSoldierStatic::HasShotgun()
{
	if (GetActiveWeapon() && (GetActiveWeapon()->GetPrimaryAmmoType() == GetAmmoDef()->Index("Buckshot")))
		return true;

	return false;
}

bool CNPCBaseSoldierStatic::ActiveWeaponIsFullyLoaded()
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if (!pWeapon)
		return false;

	if (!pWeapon->UsesClipsForAmmo1())
		return false;

	return (pWeapon->Clip1() >= pWeapon->GetMaxClip1());
}

AI_BEGIN_CUSTOM_NPC(npc_soldier_base, CNPCBaseSoldierStatic)

DECLARE_TASK(TASK_SOLDIER_FACE_TOSS_DIR)
DECLARE_TASK(TASK_SOLDIER_IGNORE_ATTACKS)
DECLARE_TASK(TASK_SOLDIER_SIGNAL_BEST_SOUND)
DECLARE_TASK(TASK_SOLDIER_DEFER_SQUAD_GRENADES)
DECLARE_TASK(TASK_SOLDIER_DIE_INSTANTLY)

//Activities
DECLARE_ACTIVITY(ACT_COMBINE_THROW_GRENADE)
DECLARE_ACTIVITY(ACT_COMBINE_LAUNCH_GRENADE)

DECLARE_SQUADSLOT(SQUAD_SLOT_GRENADE1)
DECLARE_SQUADSLOT(SQUAD_SLOT_GRENADE2)

DECLARE_CONDITION(COND_SOLDIER_NO_FIRE)
DECLARE_CONDITION(COND_SOLDIER_DEAD_FRIEND)
DECLARE_CONDITION(COND_SOLDIER_DROP_GRENADE)
DECLARE_CONDITION(COND_SOLDIER_ON_FIRE)
DECLARE_CONDITION(COND_SOLDIER_ATTACK_SLOT_AVAILABLE)

DECLARE_INTERACTION(g_interactionSoldierBash);

//=========================================================
//	SCHED_SOLDIER_COMBAT_FAIL
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_COMBAT_FAIL,

"	Tasks"
"		TASK_STOP_MOVING			0"
"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE "
"		TASK_WAIT_FACE_ENEMY		2"
"		TASK_WAIT_PVS				0"
""
"	Interrupts"
"		COND_CAN_RANGE_ATTACK1"
"		COND_CAN_RANGE_ATTACK2"
"		COND_CAN_MELEE_ATTACK1"
"		COND_CAN_MELEE_ATTACK2"
)

//=========================================================
// SCHED_SOLDIER_COMBAT_FACE
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_COMBAT_FACE,

"	Tasks"
"		TASK_STOP_MOVING			0"
"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
"		TASK_FACE_ENEMY				0"
"		TASK_WAIT					1.5"
""
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_CAN_RANGE_ATTACK1"
"		COND_CAN_RANGE_ATTACK2"
)

//=========================================================
// SCHED_SOLDIER_SIGNAL_SUPPRESS
//	don't stop shooting until the clip is
//	empty or SOLDIER gets hurt.
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_SIGNAL_SUPPRESS,

"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_FACE_IDEAL					0"
"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_SIGNAL_GROUP"
"		TASK_RANGE_ATTACK1				0"
""
"	Interrupts"
"		COND_ENEMY_DEAD"
"		COND_LIGHT_DAMAGE"
"		COND_HEAVY_DAMAGE"
"		COND_NO_PRIMARY_AMMO"
"		COND_WEAPON_BLOCKED_BY_FRIEND"
"		COND_WEAPON_SIGHT_OCCLUDED"
"		COND_HEAR_DANGER"
"		COND_HEAR_MOVE_AWAY"
"		COND_SOLDIER_NO_FIRE"
)

//=========================================================
// SCHED_SOLDIER_SUPPRESS
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_SUPPRESS,

"	Tasks"
"		TASK_STOP_MOVING			0"
"		TASK_FACE_ENEMY				0"
"		TASK_RANGE_ATTACK1			0"
""
"	Interrupts"
"		COND_ENEMY_DEAD"
"		COND_LIGHT_DAMAGE"
"		COND_HEAVY_DAMAGE"
"		COND_NO_PRIMARY_AMMO"
"		COND_HEAR_DANGER"
"		COND_HEAR_MOVE_AWAY"
"		COND_SOLDIER_NO_FIRE"
"		COND_WEAPON_BLOCKED_BY_FRIEND"
)

//=========================================================
// SCHED_SOLDIER_RANGE_ATTACK1
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_RANGE_ATTACK1,

"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_FACE_ENEMY					0"
"		TASK_ANNOUNCE_ATTACK			1"	// 1 = primary attack
"		TASK_WAIT_RANDOM				0.3"
"		TASK_RANGE_ATTACK1				0"
"		TASK_SOLDIER_IGNORE_ATTACKS		0.5"
""
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_HEAVY_DAMAGE"
"		COND_LIGHT_DAMAGE"
"		COND_LOW_PRIMARY_AMMO"
"		COND_NO_PRIMARY_AMMO"
"		COND_WEAPON_BLOCKED_BY_FRIEND"
"		COND_TOO_CLOSE_TO_ATTACK"
"		COND_GIVE_WAY"
"		COND_HEAR_DANGER"
"		COND_HEAR_MOVE_AWAY"
"		COND_SOLDIER_NO_FIRE"
""
// Enemy_Occluded				Don't interrupt on this.  Means
//								comibine will fire where player was after
//								he has moved for a little while.  Good effect!!
// WEAPON_SIGHT_OCCLUDED		Don't block on this! Looks better for railings, etc.
)

//=========================================================
// 	SCHED_SOLDIER_RANGE_ATTACK2	
//
//	secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
//	SOLDIERs's grenade toss requires the enemy be occluded.
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_RANGE_ATTACK2,

"	Tasks"
"		TASK_STOP_MOVING					0"
"		TASK_SOLDIER_FACE_TOSS_DIR			0"
"		TASK_ANNOUNCE_ATTACK				2"	// 2 = grenade
"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_RANGE_ATTACK2"
"		TASK_SOLDIER_DEFER_SQUAD_GRENADES	0"
"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_SOLDIER_RANGE_ATTACK1"	// spray while you can!
""
"	Interrupts"
)

//=========================================================
// SCHED_SOLDIER_DROP_GRENADE
//
//	Place a grenade at my feet
//=========================================================
DEFINE_SCHEDULE
(
SCHED_SOLDIER_DROP_GRENADE,

"	Tasks"
"		TASK_STOP_MOVING					0"
"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_SPECIAL_ATTACK2"
"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_SOLDIER_RANGE_ATTACK1"	// spray while you can!
""
"	Interrupts"
)

DEFINE_SCHEDULE
(
SCHED_SOLDIER_BURNING_STAND,

"	Tasks"
"		TASK_SET_ACTIVITY				ACTIVITY:ACT_COMBINE_BUGBAIT"
"		TASK_RANDOMIZE_FRAMERATE		20"
"		TASK_WAIT						2"
"		TASK_WAIT_RANDOM				3"
"		TASK_SOLDIER_DIE_INSTANTLY		DMG_BURN"
"		TASK_WAIT						1.0"
"	"
"	Interrupts"
)

DEFINE_SCHEDULE
(
SCHED_SOLDIER_FACE_IDEAL_YAW,

"	Tasks"
"		TASK_FACE_IDEAL				0"
"	"
"	Interrupts"
)

AI_END_CUSTOM_NPC()
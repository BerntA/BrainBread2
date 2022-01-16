//=========       Copyright © Reperio Studios 2013-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie NPC BaseClass
//
//==============================================================================================//

#include "cbase.h"
#include "npc_BaseZombie.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_memory.h"
#include "ai_senses.h"
#include "bitstring.h"
#include "EntityFlame.h"
#include "npcevent.h"
#include "activitylist.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "rope.h"
#include "rope_shared.h"
#include "igamesystem.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "props.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Server.h"
#include "random_extended.h"
#include "ZombieVolume.h"
#include "GameDefinitions_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// After taking damage, ignore further damage for n seconds. This keeps the zombie
// from being interrupted while.
#define ZOMBIE_FLINCH_DELAY			3
#define ZOMBIE_BURN_TIME		10 // If ignited, burn for this many seconds
#define ZOMBIE_BURN_TIME_NOISE	2  // Give or take this many seconds.
#define ZOMBIE_LIFETIME (gpGlobals->curtime + (((float)RandomDoubleNumber(bb2_zombie_lifespan_min.GetFloat(), bb2_zombie_lifespan_max.GetFloat())) * 60.0f))

#define ZOMBIE_BBOX Vector(3.0f, 3.0f, 3.0f)
#define ZOMBIE_MAX_REACH 100.0f

#define HEALTH_REGEN_SUSPEND 10.0f // How many seconds to suspend hp regen when taking dmg?

static ConVar sk_npc_zombie_speed_override("sk_npc_zombie_speed_override", "0", FCVAR_GAMEDLL, "Override Zombie Move Speed", true, 0.0f, true, 5.0f);
static CUtlVector<CNPC_BaseZombie*> zombieList;
int g_pZombiesInWorld = 0;

class CZombieMoanManager : public CAutoGameSystemPerFrame
{
public:
	CZombieMoanManager(char const *name) : CAutoGameSystemPerFrame(name)
	{
	}

	void LevelInitPreEntity()
	{
		m_flLastTime = 0.0f;
	}

	void FrameUpdatePreEntityThink()
	{
		if (m_flLastTime > gpGlobals->curtime)
			return;

		m_flLastTime = (gpGlobals->curtime + random->RandomFloat(1.5f, 3.0f));

		for (int i = 0; i < zombieList.Count(); i++)
		{
			CNPC_BaseZombie *pZombie = zombieList[i];
			if (!pZombie || pZombie->IsBoss() || !pZombie->CanPlayMoanSound() || pZombie->m_nMoanFlags)
				continue;
			pZombie->m_nMoanFlags = ZOMBIE_MOAN_ALLOWED;
			return;
		}

		// No one could moan, reset everyone this time --- moan next time!
		for (int i = 0; i < zombieList.Count(); i++)
		{
			CNPC_BaseZombie *pZombie = zombieList[i];
			if (!pZombie)
				continue;
			pZombie->m_nMoanFlags = 0;
		}

		m_flLastTime = (gpGlobals->curtime + 1.0f);
	}

private:
	float m_flLastTime;
};

static CZombieMoanManager ZombieMoanManager("CZombieMoanManager");

static const char *pMoanSounds[] =
{
	"Moan1",
	"Moan2",
	"Moan3",
	"Moan4",
};

int AE_ZOMBIE_ATTACK_RIGHT;
int AE_ZOMBIE_ATTACK_LEFT;
int AE_ZOMBIE_ATTACK_BOTH;
int AE_ZOMBIE_STEP_LEFT;
int AE_ZOMBIE_STEP_RIGHT;
int AE_ZOMBIE_SCUFF_LEFT;
int AE_ZOMBIE_SCUFF_RIGHT;
int AE_ZOMBIE_ATTACK_SCREAM;
int AE_ZOMBIE_GET_UP;
int AE_ZOMBIE_POUND;
int AE_ZOMBIE_ALERTSOUND;

IMPLEMENT_SERVERCLASS_ST(CNPC_BaseZombie, DT_AI_BaseZombie)
SendPropExclude("DT_BaseCombatCharacter", "m_hActiveWeapon"),
SendPropExclude("DT_BaseCombatCharacter", "m_hMyWeapons"),

SendPropExclude("DT_BaseFlex", "m_flexWeight"),
SendPropExclude("DT_BaseFlex", "m_blinktoggle"),
SendPropExclude("DT_BaseFlex", "m_viewtarget"),

SendPropExclude("DT_BaseFlex", "m_vecLean"),
SendPropExclude("DT_BaseFlex", "m_vecShift"),
END_SEND_TABLE()

CNPC_BaseZombie::CNPC_BaseZombie()
{
	m_hLastIgnitionSource = NULL;
	g_pZombiesInWorld++;
	m_bUseNormalSpeed = m_bLifeTimeOver = m_bMarkedForDeath = false;
	m_flHealthRegenSuspended = m_flHealthRegenValue = 0.0f;
	m_nMoanFlags = 0;
	zombieList.AddToTail(this);
}

CNPC_BaseZombie::~CNPC_BaseZombie()
{
	g_pZombiesInWorld--;
	m_hBlockingEntity = NULL;
	m_hLastIgnitionSource = NULL;
	zombieList.FindAndRemove(this);
	CZombieVolume::OnZombieRemoved(this->entindex());
}

//-----------------------------------------------------------------------------
// Purpose: Returns this monster's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseZombie::Classify( void )
{
	return CLASS_ZOMBIE;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the maximum yaw speed based on the monster's current activity.
//-----------------------------------------------------------------------------
float CNPC_BaseZombie::MaxYawSpeed( void )
{
	if (IsMoving() && HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
		return( 15 );
	else
	{
		switch( GetActivity() )
		{
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			return 100;
			break;
		case ACT_RUN:
			return 15;
			break;
		case ACT_WALK:
		case ACT_IDLE:
			return 25;
			break;
		case ACT_RANGE_ATTACK1:
		case ACT_RANGE_ATTACK2:
		case ACT_MELEE_ATTACK1:
		case ACT_MELEE_ATTACK2:
			return 120;
		default:
			return 90;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!HasPoseParameter( GetSequence(), m_poseMove_Yaw ))
		return BaseClass::OverrideMoveFacing( move, flInterval );

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );
	float idealYaw = UTIL_AngleMod( flMoveYaw );

	if (GetEnemy())
	{
		float flEDist = UTIL_DistApprox2D( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter() );

		if (flEDist < 256.0)
		{
			float flEYaw = UTIL_VecToYaw( GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter() );

			if (flEDist < 128.0)
				idealYaw = flEYaw;
			else
				idealYaw = flMoveYaw + UTIL_AngleDiff( flEYaw, flMoveYaw ) * (2 - flEDist / 128.0);
		}
	}

	GetMotor()->SetIdealYawAndUpdate( idealYaw );

	// find movement direction to compensate for not being turned far enough
	float fSequenceMoveYaw = GetSequenceMoveYaw( GetSequence() );
	float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y + fSequenceMoveYaw );
	SetPoseParameter( m_poseMove_Yaw, GetPoseParameter( m_poseMove_Yaw ) + flDiff );

	return true;
}

static bool IsEnemyOnTop(CNPC_BaseZombie *pOuter, CBaseEntity *pEnemy, const float &flDist)
{
	if (pOuter && pEnemy)
	{
		CBaseEntity *pEnemyGroundEntity = pEnemy->GetGroundEntity();

		// Enemy is on our head or vice versa.
		if ((pEnemyGroundEntity == pOuter) || (pOuter->GetGroundEntity() == pEnemy))
			return true;

		// If the enemy is standing on a nearby friend, help him.
		if ((flDist <= ZOMBIE_MAX_REACH) && pEnemyGroundEntity && pEnemyGroundEntity->IsZombie(true) && pOuter->FVisible(pEnemy))
			return true;
	}

	return false;
}

static bool IsAbleToHitEnemy(CNPC_BaseZombie *pOuter, CBaseEntity *pEnemy, trace_t &tr, const float &flDist)
{
	if (pEnemy && tr.m_pEnt && (tr.fraction != 1.0))
	{
		// I might be too far away but our target is standing on a static prop which i just traced against!
		if (tr.DidHitWorld() && (tr.hitbox > 0) && ((tr.hitbox - 1) == pEnemy->GetGroundStaticPropIndex()) && (flDist <= ZOMBIE_MAX_REACH))
			return true;

		// If we hit our enemy or hit smth that the enemy stands on (non world ent) then keep trying to hit!
		if ((tr.m_pEnt == pEnemy) || (!tr.m_pEnt->IsWorld() && (pEnemy->GetGroundEntity() == tr.m_pEnt)))
			return true;
	}

	return false;
}

static bool CanAttackEntity(CNPC_BaseZombie *pOuter, CBaseEntity *pEnemy, trace_t &tr)
{
	CBaseEntity *pHit = tr.m_pEnt;
	if (pOuter && pHit && (tr.fraction != 1.0))
	{
		if (
			pHit->IsNPC() || pHit->IsPlayer() || pHit->GetObstruction() || // These are valid.
			(pEnemy && pHit->IsWorld() && (tr.hitbox > 0) && ((tr.hitbox - 1) == pEnemy->GetGroundStaticPropIndex())) || // I can attack anything which my enemy stands on, in this case some static prop.
			(pEnemy && !pHit->IsWorld() && (pHit == pEnemy->GetGroundEntity())) || // I can attack anything which my enemy stands on.
			(pEnemy && ((pEnemy->GetGroundEntity() == pOuter) || (pOuter->GetGroundEntity() == pEnemy))) || // I can attack my enemys ground ent if it is me or him.
			(!pHit->IsWorld() && (pHit->GetGroundEntity() == pOuter)) // I can attack anything which stands on me.
			)
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BaseZombie::MeleeAttack1Conditions(float flDot, float flFullDist)
{
	CBaseEntity *pEnemy = GetEnemy();
	if (!pEnemy)
		return COND_TOO_FAR_TO_ATTACK;

	const bool bIsNotFacing = (flDot < 0.7);
	const float flRange = GetClawAttackRange();
	const float flNPCHeight = WorldAlignSize().z;
	float flHeightDiff = 0.0f;
	Vector vFeetPos = GetAbsOrigin();
	Vector vEnemyPos = pEnemy->GetAbsOrigin();
	Vector vEnemyEyes = pEnemy->EyePosition();
	const float flDist = (vEnemyPos - vFeetPos).Length2D();
	const bool bIsEnemyOnTop = IsEnemyOnTop(this, pEnemy, flDist);

	flHeightDiff = (vEnemyEyes.z - vFeetPos.z);
	if (flHeightDiff < 0.0f) // He is under me? there is no way...
		return COND_TOO_FAR_TO_ATTACK;

	const bool bIsEnemyVisible = FVisible(pEnemy);
	flHeightDiff = (vEnemyPos.z - vFeetPos.z);
	if ((flHeightDiff > (flNPCHeight + flRange + 5.0f)) && !bIsEnemyOnTop) // Enemy is above me! How far can we reach?	
		return COND_TOO_FAR_TO_ATTACK;

	if (bIsEnemyOnTop || (bIsEnemyVisible && (flDist <= flRange)))
		return (bIsNotFacing ? COND_NOT_FACING_ATTACK : COND_CAN_MELEE_ATTACK1);

	Vector vStomachPos = WorldSpaceCenter();
	Vector vEyePos = vFeetPos + Vector(0.0f, 0.0f, (flNPCHeight - 12.0f));

	Vector forward;
	GetVectors(&forward, NULL, NULL);

	trace_t	tr;
	CTraceFilterNav traceFilter(this, false, this, GetCollisionGroup(), true, true);

	AI_TraceHull(vEyePos, vEyePos + forward * flRange, -ZOMBIE_BBOX, ZOMBIE_BBOX, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr);
	if (IsAbleToHitEnemy(this, pEnemy, tr, flDist)) // Check if we're able to, or have the right to try and attack!
		return (bIsNotFacing ? COND_NOT_FACING_ATTACK : COND_CAN_MELEE_ATTACK1);

	AI_TraceHull(vStomachPos, vStomachPos + forward * flRange, -ZOMBIE_BBOX, ZOMBIE_BBOX, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr);
	if (IsAbleToHitEnemy(this, pEnemy, tr, flDist)) // Check if we're able to, or have the right to try and attack!
		return (bIsNotFacing ? COND_NOT_FACING_ATTACK : COND_CAN_MELEE_ATTACK1);

	vFeetPos.z += 6.0f;
	AI_TraceHull(vFeetPos, vFeetPos + forward * flRange, -ZOMBIE_BBOX, ZOMBIE_BBOX, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr);
	if (IsAbleToHitEnemy(this, pEnemy, tr, flDist)) // Check if we're able to, or have the right to try and attack!
		return (bIsNotFacing ? COND_NOT_FACING_ATTACK : COND_CAN_MELEE_ATTACK1);

	return COND_TOO_FAR_TO_ATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
#define ZOMBIE_SCORCH_RATE		8
#define ZOMBIE_MIN_RENDERCOLOR	50
int CNPC_BaseZombie::OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo)
{
	const float flCurrentHealth = ((float)m_iHealth.Get());

	// BB2 no dmg for zombie if player zombie attack us
	CBaseEntity *pAttacker = inputInfo.GetAttacker();
	if (pAttacker)
	{
		if (pAttacker->Classify() == CLASS_PLAYER_ZOMB)
			return 0;

		// Update lifetime if attacked by a non zomb plr.
		if (pAttacker->IsPlayer() && pAttacker->IsHuman())
			m_flSpawnTime = ZOMBIE_LIFETIME;
	}

	if ((inputInfo.GetDamageType() & DMG_BURN) && !(inputInfo.GetSkillFlags() & SKILL_FLAG_BLAZINGAMMO))
	{
		// If a zombie is on fire it only takes damage from the fire that's attached to it. (DMG_DIRECT)
		// This is to stop zombies from burning to death 10x faster when they're standing around
		// 10 fire entities.
		if (IsOnFire() && !(inputInfo.GetDamageType() & DMG_DIRECT))
			return 0;

		Scorch(ZOMBIE_SCORCH_RATE, ZOMBIE_MIN_RENDERCOLOR);
	}

	if (ShouldIgnite(inputInfo))
		Ignite(100.0f);

	int ret = BaseClass::OnTakeDamage_Alive(inputInfo);
	if (ret)
	{
		m_flHealthRegenSuspended = (gpGlobals->curtime + HEALTH_REGEN_SUSPEND);
		OnTookDamage(inputInfo, flCurrentHealth);
	}

	return ret;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::CanPlayMoanSound()
{
	// Don't play if gagged!
	// Burning zombies play their moan loop at full volume for as long as they're
	// burning. Don't let a moan envelope play cause it will turn the volume down when done.
	return !(HasSpawnFlags(SF_NPC_GAG) || IsOnFire());
}

//-----------------------------------------------------------------------------
// Purpose: Play a moan or idle sound.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::IdleSound(void)
{
	// Non-boss moan/idle infrequently in IDLE state.
	if (!IsBoss() && (GetState() == NPC_STATE_IDLE) && (random->RandomFloat(0, 1) == 0))
		return;

	const char *pSound = (TryTheLuck(0.35f) ? pMoanSounds[random->RandomInt(0, (_ARRAYSIZE(pMoanSounds) - 1))] : "Idle");
	HL2MPRules()->EmitSoundToClient(this, pSound, GetNPCType(), GetGender());
	CSoundEnt::InsertSound(SOUND_COMBAT, EyePosition(), 300.0f, 0.5f, this);
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this gibbing zombie should ignite its gibs
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgniteZombieGib( void )
{
	return IsOnFire();
}

//-----------------------------------------------------------------------------
// Purpose: damage has been done. Should the zombie ignite?
//-----------------------------------------------------------------------------
bool CNPC_BaseZombie::ShouldIgnite( const CTakeDamageInfo &info )
{
	if (!AllowedToIgnite())
		return false;

	if ( IsOnFire() )
	{
		// Already burning!
		return false;
	}

	if ( (info.GetDamageType() & DMG_BURN) && !(info.GetSkillFlags() & SKILL_FLAG_BLAZINGAMMO) )
	{
		//
		// If we take more than ten percent of our health in burn damage within a five
		// second interval, we should catch on fire.
		//
		m_flBurnDamage += info.GetDamage();
		m_flBurnDamageResetTime = gpGlobals->curtime + 5;

		if (m_flBurnDamage >= m_iMaxHealth * 0.1)
		{
			// remember who put us on fire.
			if (info.GetAttacker() && (info.GetAttacker()->IsPlayer() || info.GetAttacker()->IsNPC()))
				m_hLastIgnitionSource = info.GetAttacker();

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sufficient fire damage has been done. Zombie ignites!
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

	if ( GetEffectEntity() != NULL )
		GetEffectEntity()->AddEffects( EF_DIMLIGHT );

	// Set the zombie up to burn to death in about ten seconds.
	SetHealth(MIN(m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC * (ZOMBIE_BURN_TIME + random->RandomFloat(-ZOMBIE_BURN_TIME_NOISE, ZOMBIE_BURN_TIME_NOISE))));
}

void CNPC_BaseZombie::CopyRenderColorTo( CBaseEntity *pOther )
{
	color32 color = GetRenderColor();
	pOther->SetRenderColor( color.r, color.g, color.b, color.a );
}

//-----------------------------------------------------------------------------
// Purpose: Look in front and see if the claw hit anything.
//
// Input  :	flDist				distance to trace		
//			iDamage				damage to do if attack hits
//			vecViewPunch		camera punch (if attack hits player)
//			vecVelocityPunch	velocity punch (if attack hits player)
//
// Output : The entity hit by claws. NULL if nothing.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_BaseZombie::ClawAttack(float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin)
{
	CBaseEntity *pHurt = NULL;
	CBaseEntity *pObstruction = m_hBlockingEntity.Get();

	if (pObstruction && IsCurSchedule(SCHED_ZOMBIE_BASH_DOOR)) // Force a hit @obstruction when we engage.
	{
		const Vector &vecAttackerOrigin = WorldSpaceCenter();
		CTakeDamageInfo	damageInfo(this, this, iDamage, DMG_ZOMBIE);
		Vector attackDir = (pObstruction->WorldSpaceCenter() - vecAttackerOrigin);
		VectorNormalize(attackDir);
		CalculateMeleeDamageForce(&damageInfo, attackDir, vecAttackerOrigin);
		pObstruction->TakeDamage(damageInfo);
		pHurt = pObstruction;
	}
	else
	{
		// LoS check, kinda redundant, similar to attack conditions check, we need this to prevent runners from wall hacking... if you trigger this attack in a run anim etc..
		CBaseEntity *pEnemy = GetEnemy();
		bool bEnemyAlreadyInSight = (pEnemy && HasCondition(COND_CAN_MELEE_ATTACK1));
		if (bEnemyAlreadyInSight == false)
		{
			const Vector vEyePos = EyePosition();
			const Vector &vStomachPos = WorldSpaceCenter();
			const Vector vFeetPos = GetAbsOrigin() + Vector(0.0f, 0.0f, 6.0f);
			const float flRange = GetClawAttackRange();

			Vector forward;
			GetVectors(&forward, NULL, NULL);

			CTraceFilterNav traceFilter(this, false, this, (GetCollisionGroup() == COLLISION_GROUP_NPC_ZOMBIE_CRAWLER) ? COLLISION_GROUP_NPC_ZOMBIE : GetCollisionGroup());
			trace_t	tr1, tr2, tr3;

			AI_TraceHull(vEyePos, vEyePos + forward * flRange, -ZOMBIE_BBOX, ZOMBIE_BBOX, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr1);
			AI_TraceHull(vStomachPos, vStomachPos + forward * flRange, -ZOMBIE_BBOX, ZOMBIE_BBOX, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr2);
			AI_TraceHull(vFeetPos, vFeetPos + forward * flRange, -ZOMBIE_BBOX, ZOMBIE_BBOX, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr3);

			if (!CanAttackEntity(this, pEnemy, tr1) && !CanAttackEntity(this, pEnemy, tr2) && !CanAttackEntity(this, pEnemy, tr3))
				return NULL;
		}

		//
		// Trace out a cubic section of our hull and see what we hit.
		//
		Vector vecMins = GetHullMins();
		Vector vecMaxs = GetHullMaxs();
		float height = ((vecMaxs.z - vecMins.z) / 2.0f);
		height = ceil(height);
		vecMins.z = -height;
		vecMaxs.z = height;

		// Try to hit them with a trace
		pHurt = CheckTraceHullAttack(flDist, vecMins, vecMaxs, iDamage, DMG_ZOMBIE, 1.0f, false, bEnemyAlreadyInSight);
	}

	if (pHurt)
	{
		AttackHitSound();

		// Apply attack view kick for plr.
		CBasePlayer *pPlayer = ToBasePlayer(pHurt);
		if (pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE))
		{
			int iRandomChance = random->RandomInt(0, 100);
			if (iRandomChance <= 50)
			{
				pPlayer->ViewPunch((qaViewPunch / 4.0f));
				pPlayer->VelocityPunch(vecVelocityPunch);
			}
		}

		Vector vecBloodPos;

		switch (BloodOrigin)
		{
		case ZOMBIE_BLOOD_LEFT_HAND:
			if (GetAttachment("blood_left", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));
			break;

		case ZOMBIE_BLOOD_RIGHT_HAND:
			if (GetAttachment("blood_right", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));
			break;

		case ZOMBIE_BLOOD_BOTH_HANDS:
			if (GetAttachment("blood_left", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));

			if (GetAttachment("blood_right", vecBloodPos))
				SpawnBlood(vecBloodPos, g_vecAttackDir, pHurt->BloodColor(), MIN(iDamage, 30));
			break;

		case ZOMBIE_BLOOD_BITE:
			// No blood for these.
			break;
		}
	}
	else
		AttackMissSound();

	return pHurt;
}

//-----------------------------------------------------------------------------
// Purpose: The zombie is frustrated and pounding walls/doors. Make an appropriate noise
// Input  : 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::PoundSound()
{
	trace_t		tr;
	Vector		forward;

	GetVectors( &forward, NULL, NULL );

	AI_TraceLine( EyePosition(), EyePosition() + forward * 128, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if( tr.fraction == 1.0 )
	{
		// Didn't hit anything!
		return;
	}

	if( tr.fraction < 1.0 && tr.m_pEnt )
	{
		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if( psurf )
		{
			EmitSound( physprops->GetString(psurf->sounds.impactHard) );
			return;
		}
	}

	// Otherwise fall through to the default sound.
	CPASAttenuationFilter filter(this, "NPC_BaseZombie.PoundDoor");
	EmitSound(filter, entindex(), "NPC_BaseZombie.PoundDoor");
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific events that occur when tagged animation
//			frames are played.
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_NPC_ATTACK_BROADCAST )
	{
		// TODO ? 
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_POUND )
	{
		PoundSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ALERTSOUND )
	{
		AlertSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_STEP_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( false );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_STEP_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootstepSound( true );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_GET_UP )
	{
		MakeAIFootstepSound(180.0f, 3.0f);
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_LEFT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( false );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_SCUFF_RIGHT )
	{
		MakeAIFootstepSound( 180.0f );
		FootscuffSound( true );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_SCREAM )
	{
		AttackSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );

		right = right * 100;
		forward = forward * 200;

		QAngle qa( -15, -20, -10 );
		Vector vec = right + forward;
		ClawAttack( GetClawAttackRange(), m_iDamageOneHand, qa, vec, ZOMBIE_BLOOD_RIGHT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );

		right = right * -100;
		forward = forward * 200;

		QAngle qa( -15, 20, -10 );
		Vector vec = right + forward;
		ClawAttack(GetClawAttackRange(), m_iDamageOneHand, qa, vec, ZOMBIE_BLOOD_LEFT_HAND);
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_BOTH )
	{
		Vector forward;
		QAngle qaPunch( 45, random->RandomInt(-5,5), random->RandomInt(-5,5) );
		AngleVectors( GetLocalAngles(), &forward );
		forward = forward * 200;
		ClawAttack(GetClawAttackRange(), m_iDamageBothHands, qaPunch, forward, ZOMBIE_BLOOD_BOTH_HANDS);
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the base zombie.
//
// !!!IMPORTANT!!! YOUR DERIVED CLASS'S SPAWN() RESPONSIBILITIES:
//
//		Call Precache();
//		Set blood color
//		Set health
//		Set field of view
//		Call CapabilitiesClear() & then set relevant capabilities
//		THEN Call BaseClass::Spawn()
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Spawn( void )
{
	// Customized:
	if (ParseNPC(this))
	{
		SetHealth(m_iTotalHP);
		SetMaxHealth(m_iTotalHP);
		SetNPCName(GetNPCName());
		SetBoss(IsBoss());

		PrecacheModel(GetNPCModelName());

		Hull_t lastHull = GetHullType();

		SetModel(GetNPCModelName());
		SetHullType(HULL_HUMAN);
		SetHullSizeNormal(true);
		SetDefaultEyeOffset();

		if (lastHull != GetHullType())
		{
			if (VPhysicsGetObject())
				SetupVPhysicsHull();
		}

		m_nSkin = m_iModelSkin;
		UpdateNPCScaling();
		UpdateMeleeRange(NAI_Hull::Bounds(GetHullType(), IsUsingSmallHull()));
	}
	else
	{
		UTIL_Remove(this);
		return;
	}

	// If Pure Classic is enabled we will not spawn non Fred or Walker npcs...
	if (GameBaseServer()->IsClassicMode() && !FClassnameIs(this, "npc_fred") && !FClassnameIs(this, "npc_walker"))
	{
		UTIL_Remove(this);
		return;
	}

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	m_NPCState = NPC_STATE_NONE;

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_SQUAD );

	m_flNextMoanSound = gpGlobals->curtime + 30.0f;

	NPCInit();

	// Zombies get to cheat for 6 seconds (sjb)
	GetEnemies()->SetFreeKnowledgeDuration( 6.0 );

	SetCollisionGroup(COLLISION_GROUP_NPC_ZOMBIE);
	m_flSpawnTime = ZOMBIE_LIFETIME;

	if (!IsBoss() && !HL2MPRules()->CanSpawnZombie())
	{
		DevMsg(2, "Zombie limit has been reached, removing zombie!\n");
		UTIL_Remove(this);
	}

	OnSetGibHealth();
}


//-----------------------------------------------------------------------------
// Purpose: Pecaches all resources this NPC needs.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::Precache( void )
{
	PrecacheScriptSound( "NPC_BaseZombie.AttackHit" );
	PrecacheScriptSound( "NPC_BaseZombie.AttackMiss" );
	PrecacheScriptSound( "NPC_BaseZombie.FootstepRight" );
	PrecacheScriptSound( "NPC_BaseZombie.FootstepLeft" );
	PrecacheScriptSound( "NPC_BaseZombie.ScuffRight" );
	PrecacheScriptSound( "NPC_BaseZombie.ScuffLeft" );
	PrecacheScriptSound( "E3_Phystown.Slicer" );
	PrecacheScriptSound( "NPC_BaseZombie.PoundDoor" );
	PrecacheParticleSystem( "blood_impact_zombie_01" );

	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		// Normal NPCs will just wander around like muricans walking in the mall.
		if (HasCondition(COND_ENEMY_TOO_FAR) && !CanAlwaysSeePlayers())
		{
			if (GetEnemies() && (GetEnemies()->NumEnemies() > 0))
				GetEnemies()->ClearEntireMemory();		

			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}
		return SCHED_ZOMBIE_CHASE_ENEMY;

	case SCHED_STANDOFF:
		return SCHED_ZOMBIE_WANDER_STANDOFF;

	case SCHED_MELEE_ATTACK1:
		return SCHED_ZOMBIE_MELEE_ATTACK1;

		// Don't waste time!
	case SCHED_IDLE_WALK:
	case SCHED_IDLE_STAND:
	case SCHED_ALERT_STAND:
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return SCHED_ZOMBIE_WANDER_MEDIUM;

		// Always investigate sounds, even when idle. Zombies are curious beasts!
	case SCHED_ALERT_FACE_BESTSOUND:
		return SCHED_INVESTIGATE_SOUND;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::BuildScheduleTestBits( void )
{
	// Ignore damage if we were recently damaged or we're attacking.
	if ( GetActivity() == ACT_MELEE_ATTACK1 )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}
	else if ( m_flNextFlinch >= gpGlobals->curtime )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
	}

	BaseClass::BuildScheduleTestBits();

	if (IsCurSchedule(SCHED_ZOMBIE_CHASE_ENEMY) && !CanAlwaysSeePlayers())
		SetCustomInterruptCondition(COND_ENEMY_TOO_FAR);
}


//-----------------------------------------------------------------------------
// Purpose: Called when we change schedules.
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnScheduleChange( void )
{
	//
	// If we took damage and changed schedules, ignore further damage for a few seconds.
	//
	if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))
		m_flNextFlinch = gpGlobals->curtime + ZOMBIE_FLINCH_DELAY;

	BaseClass::OnScheduleChange();
}

// A new sched. is about to start!
void CNPC_BaseZombie::OnStartSchedule(int scheduleType)
{
	m_bUseNormalSpeed = ShouldUseNormalSpeedForSchedule(scheduleType);
	BaseClass::OnStartSchedule(scheduleType);
}

//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_BaseZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_ZOMBIE_WANDER_MEDIUM )	
		return SCHED_ZOMBIE_WANDER_FAIL;

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_BaseZombie::SelectSchedule ( void )
{
	if (HasCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT))
		return SCHED_ZOMBIE_BASH_DOOR;

	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
		{
			if (HasCondition(COND_ZOMBIE_ENEMY_IN_SIGHT) && HasCondition(COND_CAN_MELEE_ATTACK1))
				return SCHED_MELEE_ATTACK1;

			if (!HasCondition(COND_ENEMY_UNREACHABLE) && !HasCondition(COND_ENEMY_TOO_FAR))		
				return SCHED_ZOMBIE_CHASE_ENEMY;			
		}

		if (HasCondition(COND_LOST_ENEMY) || !HasCondition(COND_ZOMBIE_ENEMY_IN_SIGHT) || (HasCondition(COND_ENEMY_UNREACHABLE) && MustCloseToAttack()))		
			return SCHED_ZOMBIE_WANDER_MEDIUM;

		break;

	case NPC_STATE_ALERT:
		if (HasCondition(COND_LOST_ENEMY) || HasCondition(COND_ENEMY_DEAD) || !HasCondition(COND_ZOMBIE_ENEMY_IN_SIGHT) || (HasCondition(COND_ENEMY_UNREACHABLE) && MustCloseToAttack()))
		{
			ClearCondition( COND_LOST_ENEMY );
			ClearCondition( COND_ENEMY_UNREACHABLE );
			return SCHED_ZOMBIE_WANDER_MEDIUM;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}

void CNPC_BaseZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if (m_hBlockingEntity.Get())
		SetCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT);
	else
		ClearCondition(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT);

	if (HasCondition(COND_SEE_ENEMY) && (!HasCondition(COND_ENEMY_TOO_FAR) || CanAlwaysSeePlayers()))
		SetCondition(COND_ZOMBIE_ENEMY_IN_SIGHT);
	else
		ClearCondition(COND_ZOMBIE_ENEMY_IN_SIGHT);
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	//
	// Cool off if we aren't burned for five seconds or so. 
	//
	if ((m_flBurnDamageResetTime) && (gpGlobals->curtime >= m_flBurnDamageResetTime))
		m_flBurnDamage = 0;

	// Non boss zombies/monsters have a limited lifetime/lifeline:
	CBaseEntity *pEnemy = GetEnemy();
	bool bHasNearbyEnemy = (pEnemy && pEnemy->IsPlayer() && pEnemy->IsHuman() && (this->GetLocalOrigin().DistTo(pEnemy->GetLocalOrigin()) <= 700.0f) && ((gpGlobals->curtime - GetEnemies()->LastTimeSeen(pEnemy, false)) <= 10.0f));
	if (!IsBoss() && !GameBaseServer()->IsTutorialModeEnabled() && (GetLifeSpan() < gpGlobals->curtime) && !m_bLifeTimeOver && !bHasNearbyEnemy)
	{
		m_bLifeTimeOver = true;
		SetLastHitGroup(HITGROUP_GENERIC);
		KillMe();
		return;
	}

	// Health Regen:
	if (CanRegenHealth() && (GetHealth() < GetMaxHealth()) && (gpGlobals->curtime > m_flHealthRegenSuspended))
	{
		m_flHealthRegenValue += (GetHealthRegenRate() * gpGlobals->frametime);
		if (m_flHealthRegenValue >= 1.0f) // When we've recovered at least 1HP, regen it!
		{
			m_flHealthRegenValue = ceil(m_flHealthRegenValue);
			TakeHealth(((int)m_flHealthRegenValue), DMG_GENERIC);
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_YAW_TO_DOOR:
	{
		CBaseEntity *pObstruction = m_hBlockingEntity.Get();
		AssertMsg(pObstruction != NULL, "Expected condition handling to break schedule before landing here");
		if (pObstruction != NULL)
		{
			float idealYaw = UTIL_VecToYaw(pObstruction->WorldSpaceCenter() - WorldSpaceCenter());
			GetMotor()->SetIdealYaw(idealYaw);
		}

		TaskComplete();
		break;
	}

	case TASK_ZOMBIE_PATH_TO_OBSTRUCTION:
	{
		CBaseEntity *pObstruction = m_hBlockingEntity.Get();
		if (pObstruction)
		{
			if (GetNavigator()->SetVectorGoalFromTarget(pObstruction->WorldSpaceCenter()))
				TaskComplete();
			else
				TaskFail("NPC was unable to navigate to door obstruction!\n");
		}
		else
			TaskFail(FAIL_NO_TARGET);

		break;
	}

	case TASK_ZOMBIE_ATTACK_DOOR:
	{
		SetIdealActivity(SelectDoorBash());
		break;
	}

	case TASK_ZOMBIE_OBSTRUCTION_CLEARED:
	{
		m_hBlockingEntity = NULL;
		TaskComplete();
		break;
	}

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_ATTACK_DOOR:
	{
		if (IsActivityFinished())
		{
			CBaseEntity *pObstruction = m_hBlockingEntity.Get();
			if ((pObstruction == NULL) || (pObstruction->GetObstruction() == ENTITY_OBSTRUCTION_NONE))
				TaskComplete();
			else
				ResetIdealActivity(SelectDoorBash());
		}
		break;
	}

	case TASK_ZOMBIE_YAW_TO_DOOR:
	case TASK_ZOMBIE_PATH_TO_OBSTRUCTION:
	case TASK_ZOMBIE_OBSTRUCTION_CLEARED:	
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
Activity CNPC_BaseZombie::SelectDoorBash()
{
	return ACT_MELEE_ATTACK1;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_BaseZombie::ShouldUseNormalSpeedForSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_ZOMBIE_WANDER_MEDIUM:
	case SCHED_ZOMBIE_WANDER_STANDOFF:
	case SCHED_ZOMBIE_MELEE_ATTACK1:
	case SCHED_ZOMBIE_BASH_DOOR:
	case SCHED_MOVE_TO_TARGET:
	case SCHED_MOVE_TO_TARGET_VITAL:
		return true;
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseZombie::Event_Killed(const CTakeDamageInfo &info)
{
	CTakeDamageInfo pDamageCopy = info;

	CBaseEntity *pIgniter = m_hLastIgnitionSource.Get();
	if (IsOnFire() && pIgniter && (pDamageCopy.GetDamageType() & DMG_BURN))
		pDamageCopy.SetAttacker(pIgniter);

	HL2MPRules()->DeathNotice(this, pDamageCopy);

	BaseClass::Event_Killed(pDamageCopy);
	m_hLastIgnitionSource = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	switch( NewState )
	{
	case NPC_STATE_COMBAT:
		RemoveSpawnFlags(SF_NPC_GAG);
		break;
	}
}

// Called when the zombie volume wants us to die quickly...
void CNPC_BaseZombie::MarkForDeath(void)
{
	m_bMarkedForDeath = true; // Set regardless of success so we don't waste our time.
	if (IsBoss() || GameBaseServer()->IsTutorialModeEnabled() || m_bLifeTimeOver || (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE)))
		return;

	float minTimeToLive = ((bb2_zombie_lifespan_min.GetFloat() * 60.0f) / 6.0f);
	minTimeToLive = MAX(minTimeToLive, 10.0f);
	float duration = (m_flSpawnTime - gpGlobals->curtime);
	if (minTimeToLive >= duration)
		return;

	m_flSpawnTime = gpGlobals->curtime + minTimeToLive;
}

static ConVar bb2_zombie_lifetime_advanced("bb2_zombie_lifetime_advanced", "1", FCVAR_GAMEDLL, "Enable more aggressive zombie lifetime, whenever spawners are stopped due to zombie limit they will force an old zombie to die much sooner than originally intended.", true, 0, true, 1);

/*static*/ void CNPC_BaseZombie::MarkOldestNPCForDeath(void)
{
	// Allow maps to disable this.
	if (bb2_zombie_lifetime_advanced.GetBool() == false)
		return;

	for (int i = 0; i < zombieList.Count(); i++)
	{
		CNPC_BaseZombie* zmb = zombieList[i];
		Assert(zmb != NULL);
		if (!zmb || zmb->IsMarkedForDeath())
			continue;

		zmb->MarkForDeath();
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Refines a base activity into something more specific to our internal state.
//-----------------------------------------------------------------------------
Activity CNPC_BaseZombie::NPC_TranslateActivity( Activity baseAct )
{
	if (baseAct == ACT_RUN_AIM)
		return ACT_RUN;
	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_BaseZombie::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{ 	
	if( IsCurSchedule(SCHED_BIG_FLINCH) )
	{
		// This zombie is assumed to be standing up. 
		// Return a position that's centered over the absorigin,
		// halfway between the origin and the head. 
		Vector vecTarget = GetAbsOrigin();
		vecTarget.z = (vecTarget.z + ( CollisionProp()->OBBSize().z - 2.5f ) * 0.5f);
		return vecTarget;
	}

	return BaseClass::BodyTarget( posSrc, bNoisy );
}

bool CNPC_BaseZombie::OverrideShouldAddToLookList(CBaseEntity *pEntity)
{
	if (!pEntity->IsPlayer())
		return true;

	if ((pEntity->Classify() != CLASS_PLAYER) || !pEntity->IsAlive() || (pEntity->GetTeamNumber() != TEAM_HUMANS))
		return false;

	return true;
}

bool CNPC_BaseZombie::IsBreakingDownObstacle(void)
{
	return IsCurSchedule(SCHED_ZOMBIE_BASH_DOOR);
}

float CNPC_BaseZombie::GetIdealSpeed() const
{
	const float baseValue = BaseClass::GetIdealSpeed();
	if (sk_npc_zombie_speed_override.GetFloat() > 0.0f)
		return (baseValue * sk_npc_zombie_speed_override.GetFloat());

	return (m_bUseNormalSpeed ? baseValue : (baseValue * m_flSpeedFactorValue));
}

float CNPC_BaseZombie::GetIdealAccel() const
{
	const float baseValue = BaseClass::GetIdealAccel();
	if (sk_npc_zombie_speed_override.GetFloat() > 0.0f)
		return (baseValue * sk_npc_zombie_speed_override.GetFloat());

	return (m_bUseNormalSpeed ? baseValue : (baseValue * m_flSpeedFactorValue));
}

void CNPC_BaseZombie::HandleMovementObstruction(CBaseEntity *pEntity, int type)
{
	if (type == ENTITY_OBSTRUCTION_NPC_OBSTACLE)
		return;

	if ((GetEnemy() == NULL) || !IsAllowedToBreakDoors()) // If you cannot break doors you cannot break anything else, you also need an enemy!
		return;

	m_hBlockingEntity = pEntity;	
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( base_zombie, CNPC_BaseZombie )

DECLARE_TASK(TASK_ZOMBIE_YAW_TO_DOOR)
DECLARE_TASK(TASK_ZOMBIE_ATTACK_DOOR)
DECLARE_TASK(TASK_ZOMBIE_PATH_TO_OBSTRUCTION)
DECLARE_TASK(TASK_ZOMBIE_OBSTRUCTION_CLEARED)

DECLARE_CONDITION(COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT)
DECLARE_CONDITION(COND_ZOMBIE_ENEMY_IN_SIGHT)

DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_RIGHT )
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_LEFT )
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_BOTH )
DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_LEFT )
DECLARE_ANIMEVENT( AE_ZOMBIE_STEP_RIGHT )
DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_LEFT )
DECLARE_ANIMEVENT( AE_ZOMBIE_SCUFF_RIGHT )
DECLARE_ANIMEVENT( AE_ZOMBIE_ATTACK_SCREAM )
DECLARE_ANIMEVENT( AE_ZOMBIE_GET_UP )
DECLARE_ANIMEVENT( AE_ZOMBIE_POUND )
DECLARE_ANIMEVENT( AE_ZOMBIE_ALERTSOUND )

//=========================================================
// AttackDoor
//=========================================================
DEFINE_SCHEDULE
(
SCHED_ZOMBIE_BASH_DOOR,

"	Tasks"
"       TASK_ZOMBIE_PATH_TO_OBSTRUCTION 0"
"		TASK_WALK_PATH					0"
"		TASK_WAIT_FOR_MOVEMENT			0"
"		TASK_STOP_MOVING				0"
"		TASK_ZOMBIE_YAW_TO_DOOR			0"
"		TASK_FACE_IDEAL					0"
"		TASK_ZOMBIE_ATTACK_DOOR			0"
"		TASK_ZOMBIE_OBSTRUCTION_CLEARED	0"	
""
"	Interrupts"
"		COND_NEW_ENEMY"
"		COND_ENEMY_DEAD"
"		COND_HEAVY_DAMAGE"
"		COND_TASK_FAILED"
)

//=========================================================
// ChaseEnemy
//=========================================================
DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_CHASE_ENEMY,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
	"		 TASK_SET_TOLERANCE_DISTANCE	24"
	"		 TASK_GET_CHASE_PATH_TO_ENEMY	900"
	"		 TASK_RUN_PATH					0"
	"		 TASK_WAIT_FOR_MOVEMENT			0"
	"		 TASK_FACE_ENEMY				0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_UNREACHABLE"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_TOO_CLOSE_TO_ATTACK"
	"		COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT"
	"		COND_TASK_FAILED"
	)

	//=========================================================
	// Wander around for a while so we don't look stupid. 
	// this is done if we ever lose track of our enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_MEDIUM,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_WANDER						480384" // 4 feet to 32 feet
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM" // keep doing it
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ZOMBIE_ENEMY_IN_SIGHT"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ZOMBIE_OBSTRUCTED_BY_BREAKABLE_ENT"
	)

	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_STANDOFF,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_WANDER						480384" // 4 feet to 32 feet
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_STOP_MOVING				0"
	"		TASK_WAIT_PVS					0" // if the player left my PVS, just wait.
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	)

	//=========================================================
	// If you fail to wander, wait just a bit and try again.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_WANDER_FAIL,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_WAIT				1"
	"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_ZOMBIE_WANDER_MEDIUM"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_RANGE_ATTACK2"
	"		COND_CAN_MELEE_ATTACK2"
	)

	//=========================================================
	// Like the base class, only don't stop in the middle of 
	// swinging if the enemy is killed, hides, or new enemy.
	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ZOMBIE_MELEE_ATTACK1,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
	"		TASK_MELEE_ATTACK1		0"
	""
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	)

	AI_END_CUSTOM_NPC()
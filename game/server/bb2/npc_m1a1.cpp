//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Friendly Military Tank
//
//========================================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "hl2_shared_misc.h"
#include "baseanimating.h"
#include "particle_parse.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TANK_MODEL "models/military/m1a1.mdl"
#define TANK_THINK_FREQ 1.0f
#define TANK_RELOAD_WAIT_TIME 5.0f
#define TANK_TURN_360_TIME 10.0f
#define MACHINE_GUN_COOLDOWN_TIME 5.0f
#define MACHINE_GUN_BULLETS_OVERHEAT 30
#define IDLE_TIME 1.0f
#define DEFAULT_PITCH_YAW_CONE_FOV 0.707107f
#define EXTRA_OFFSET Vector(0, 0, 40)

ConVar bb2_m1a1_damage("bb2_m1a1_damage", "200", FCVAR_REPLICATED, "Set the damage of the M1A1 cannon.", true, 100.0f, true, 500.0f);
ConVar bb2_m1a1_radius("bb2_m1a1_radius", "500", FCVAR_REPLICATED, "Set the radius of the explosion.", true, 200.0f, true, 1200.0f);
ConVar bb2_m1a1_damage_machinegun("bb2_m1a1_damage_machinegun", "12", FCVAR_REPLICATED, "Set the damage of the M1A1 machinegun.", true, 10.0f, true, 125.0f);
ConVar bb2_m1a1_machinegun_firerate("bb2_m1a1_machinegun_firerate", "0.08", FCVAR_REPLICATED, "Set the fire rate of the machinegun", true, 0.025f, true, 0.5f);

enum TankStates
{
	TANK_IDLE = 0,
	TANK_FACING_ENEMY,
	TANK_WAITING,
	TANK_FIRE_PROJECTILE,
	TANK_RELOADING,
};

enum MachineGunStates
{
	GUN_IDLE = 0,
	GUN_FIRE,
};

enum TankAttachments
{
	TANK_ATTACHMENT_CANNON = 0,
	TANK_ATTACHMENT_MUZZLE,
	TANK_ATTACHMENT_EYES,
	TANK_ATTACHMENT_SMOKE_MIDDLE,
	TANK_ATTACHMENT_SMOKE_BACK_1,
	TANK_ATTACHMENT_SMOKE_BACK_2,
	TANK_ATTACHMENT_SMOKE_BACK_3,
	TANK_ATTACHMENT_SMOKE_HEAD,

	TANK_ATTACHMENT_COUNT
};

enum TankPoseParams
{
	TANK_HEAD_YAW = 0,
	TANK_CANNON_PITCH,
	TANK_GUN_YAW,
	TANK_GUN_PITCH,

	TANK_POSEPARAM_COUNT
};

class CNPCM1A1 : public CBaseAnimating
{
public:
	DECLARE_CLASS(CNPCM1A1, CBaseAnimating);
	DECLARE_DATADESC();

	CNPCM1A1();
	virtual ~CNPCM1A1();

	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void PlaySound(const char *soundScript);
	virtual int	GetTracerAttachment(void);
	virtual Class_T Classify(void);

protected:

	virtual CBaseCombatCharacter *GetEnemyWithinRange(bool bMachineGun = false, bool bDoConeCheck = false);
	virtual bool IsValidTarget(CBaseCombatCharacter *pTarget, bool bMachineGun, const Vector &origin, const Vector &coneOrigin, const QAngle &angles, bool bDoViewConeCheck = false);
	virtual void TankLogicThinkFrame(void);

	// Tank Handling
	int m_iTankState;
	float m_flTimeToLookForVictims;
	float m_flTimeToFaceVictim;
	float m_flTimeStartedRotation;

	// Tank Pose Params:
	float m_flCannonYawStart;
	float m_flCannonYawEnd;
	float m_flCannonPitchStart;
	float m_flCannonPitchEnd;

	// Machine Gun 
	int m_iMachineGunState;
	int m_iAmmoType;
	int m_iBulletsFired;
	float m_flCooldownTime;
	float m_flMachineGunFireRate;
	CHandle<CBaseCombatCharacter> m_pMachineGunTarget;

	// Cannon
	CHandle<CBaseCombatCharacter> m_pCannonTarget;

	// Sound 
	float m_flNextMoveSound;
	float m_flNextIdleSound;

	// Hammer
	float m_flRangeMax, m_flRangeMin;
	int m_iClassification;

	// Performance
	int m_nCachedAttachments[TANK_ATTACHMENT_COUNT];
	int m_nCachedPoseParams[TANK_POSEPARAM_COUNT];
};

LINK_ENTITY_TO_CLASS(npc_m1a1, CNPCM1A1)

BEGIN_DATADESC(CNPCM1A1)
DEFINE_THINKFUNC(TankLogicThinkFrame),
DEFINE_KEYFIELD(m_flRangeMin, FIELD_FLOAT, "minrange"),
DEFINE_KEYFIELD(m_flRangeMax, FIELD_FLOAT, "maxrange"),
DEFINE_KEYFIELD(m_iClassification, FIELD_INTEGER, "classification"),
END_DATADESC()

CNPCM1A1::CNPCM1A1()
{
	// Tank
	m_flCannonYawStart = 0;
	m_flCannonPitchStart = 0;
	m_iTankState = TANK_IDLE;

	// Shared
	m_flTimeToLookForVictims = 0.0f;
	m_flNextMoveSound = 0.0f;
	m_flNextIdleSound = 0.0f;

	m_flTimeToFaceVictim = 0.0f;
	m_flTimeStartedRotation = 0.0f;

	// Machine Gun
	m_iMachineGunState = GUN_IDLE;
	m_iBulletsFired = 0;
	m_flCooldownTime = 0.0f;
	m_flMachineGunFireRate = 0.0f;

	m_pMachineGunTarget = NULL;
	m_pCannonTarget = NULL;

	// Default Hammer Vars:
	m_flRangeMax = 1000.0f;
	m_flRangeMin = 250.0f;

	m_iClassification = CLASS_MILITARY_VEHICLE;
}

CNPCM1A1::~CNPCM1A1()
{
	m_pMachineGunTarget = NULL;
	m_pCannonTarget = NULL;
}

void CNPCM1A1::Spawn(void)
{
	Precache();
	SetModel(TANK_MODEL);

	SetSolid(SOLID_VPHYSICS);
	SetMoveType(MOVETYPE_NONE);
	VPhysicsInitShadow(false, false);
	//VPhysicsInitStatic(); <- will prevent pose params and such...

	BaseClass::Spawn();

	int iSequence = SelectHeaviestSequence(ACT_IDLE);
	if (iSequence != ACT_INVALID)
	{
		SetSequence(iSequence);
		ResetSequenceInfo();
		m_flPlaybackRate = 1.0f;
	}

	if (m_flRangeMax < m_flRangeMin)
		m_flRangeMax = m_flRangeMin * 2;
	else if (m_flRangeMax <= 0.0f)
		m_flRangeMax = 1000.0f;

	if (m_flRangeMin <= 0.0f)
		m_flRangeMin = 250.0f;

	// Cache stuff:
	const char *attachments[TANK_ATTACHMENT_COUNT] =
	{
		"cannon_muzzle",
		"machinegun_muzzle",
		"eyes",
		"heavy_smoke_middle",
		"medium_smoke_back_1",
		"medium_smoke_back_2",
		"small_smoke_back_3",
		"small_smoke_head",
	};

	const char *poseparameters[TANK_POSEPARAM_COUNT] =
	{
		"aim_yaw",
		"aim_pitch",
		"gun_yaw",
		"gun_pitch",
	};

	for (int i = 0; i < TANK_ATTACHMENT_COUNT; i++)
		m_nCachedAttachments[i] = LookupAttachment(attachments[i]);

	for (int i = 0; i < TANK_POSEPARAM_COUNT; i++)
		m_nCachedPoseParams[i] = LookupPoseParameter(poseparameters[i]);

	SetThink(&CNPCM1A1::TankLogicThinkFrame);
	SetNextThink(gpGlobals->curtime + 0.01f);
}

void CNPCM1A1::Precache(void)
{
	BaseClass::Precache();
	PrecacheModel(TANK_MODEL);
	PrecacheParticleSystem("dust_bombdrop");
	PrecacheParticleSystem("door_explosion_smoke");
	PrecacheScriptSound("M1A1.Fire");
	PrecacheScriptSound("M1A1.Reload");
	PrecacheScriptSound("M1A1.Turning");
	PrecacheScriptSound("M1A1.Idle");

	m_iAmmoType = GetAmmoDef()->Index("AK47");
}

Class_T CNPCM1A1::Classify(void)
{
	return ((Class_T)m_iClassification);
}

int	CNPCM1A1::GetTracerAttachment(void)
{
	return m_nCachedAttachments[TANK_ATTACHMENT_MUZZLE];
}

void CNPCM1A1::PlaySound(const char *soundScript)
{
	EmitSound(soundScript);
}

CBaseCombatCharacter *CNPCM1A1::GetEnemyWithinRange(bool bMachineGun, bool bDoConeCheck)
{
	trace_t tr;
	CTraceFilterWorldAndPropsOnly filter;
	
	Vector vecHullMin = Vector(-2, -2, -2);
	Vector vecHullMax = Vector(2, 2, 2);

	Vector vecTankOrigin = GetAbsOrigin();
	Vector vecTankEyes;
	Vector vecTurretPos;
	Vector vecTraceDir;
	QAngle angTurret;
	vecTankEyes.Init();
	vecTurretPos.Init();
	angTurret.Init();
	float dist = 0.0f;

	GetAttachment(m_nCachedAttachments[TANK_ATTACHMENT_EYES], vecTankEyes);

	if (bMachineGun)
		GetAttachment(m_nCachedAttachments[TANK_ATTACHMENT_MUZZLE], vecTurretPos, angTurret);

	CUtlVector<CBaseCombatCharacter*> enemyList;

	// Check through players:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer)
			continue;

		if (!IsValidTarget(pPlayer, bMachineGun, vecTankOrigin, vecTurretPos, angTurret, bDoConeCheck))
			continue;

		vecTraceDir = (pPlayer->GetLocalOrigin() + EXTRA_OFFSET) - vecTankEyes;
		dist = vecTraceDir.Length();
		VectorNormalize(vecTraceDir);
		UTIL_TraceHull(vecTankEyes, vecTankEyes + vecTraceDir * dist, vecHullMin, vecHullMax, MASK_SHOT_HULL, &filter, &tr);
		if (tr.DidHit())
			continue;

		enemyList.AddToTail(pPlayer);
	}

	// Check through npcs:
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();
	for (int i = 0; i < nAIs; i++)
	{
		CAI_BaseNPC *pNPC = ppAIs[i];
		if (!pNPC)
			continue;

		if (!IsValidTarget(pNPC, bMachineGun, vecTankOrigin, vecTurretPos, angTurret, bDoConeCheck))
			continue;

		vecTraceDir = (pNPC->GetLocalOrigin() + EXTRA_OFFSET) - vecTankEyes;
		dist = vecTraceDir.Length();
		VectorNormalize(vecTraceDir);
		UTIL_TraceHull(vecTankEyes, vecTankEyes + vecTraceDir * dist, vecHullMin, vecHullMax, MASK_SHOT_HULL, &filter, &tr);
		if (tr.DidHit())
			continue;

		enemyList.AddToTail(pNPC);
	}

	// Pick the closest target:
	int enemyCount = enemyList.Count();
	if (enemyCount)
	{
		CBaseCombatCharacter *pNearest = NULL;
		if (enemyCount == 1)
			pNearest = enemyList[0];
		else
		{
			float flShortestDist = FLT_MAX;
			for (int i = 0; i < enemyCount; i++)
			{
				CBaseCombatCharacter *pEnemy = enemyList[i];
				float flDist = pEnemy->GetLocalOrigin().DistTo(vecTankOrigin);
				if (flDist < flShortestDist)
				{
					flShortestDist = flDist;
					pNearest = pEnemy;
				}
			}
		}

		enemyList.RemoveAll();
		return pNearest;
	}

	return NULL;
}

bool CNPCM1A1::IsValidTarget(CBaseCombatCharacter *pTarget, bool bMachineGun, const Vector &origin, const Vector &coneOrigin, const QAngle &angles, bool bDoViewConeCheck)
{
	if (pTarget && pTarget->IsAlive() && (pTarget->IRelationType(this) != D_LI) && !pTarget->IsDormant())
	{
		if (pTarget->IsPlayer() && (pTarget->GetTeamNumber() <= TEAM_SPECTATOR))
			return false;

		if (pTarget->IsNPC() && pTarget->MyNPCPointer())
		{
			if ((pTarget->MyNPCPointer()->GetSleepState() != AISS_AWAKE) ||
				(pTarget->MyNPCPointer()->Classify() == CLASS_NONE) ||
				(pTarget->MyNPCPointer()->GetCollisionGroup() == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING))
				return false;
		}

		Vector targetPos = pTarget->GetLocalOrigin() + EXTRA_OFFSET;
		float flDist = targetPos.DistTo(origin);
		if (bMachineGun)
		{
			if (flDist > m_flRangeMin)
				return false;
		}
		else
		{
			if (flDist <= m_flRangeMin ||
				flDist > m_flRangeMax)
				return false;
		}

		// Sometimes we want to check if we're within a X deg. cone:
		if (bDoViewConeCheck)
		{
			Vector vForward;
			AngleVectors(angles, &vForward);
			Vector vDiff = targetPos - coneOrigin;
			VectorNormalize(vDiff);
			if (vForward.Dot(vDiff) < DEFAULT_PITCH_YAW_CONE_FOV) // Must be within 45 degree angle (cone)
				return false;
		}

		return true;
	}

	return false;
}

void CNPCM1A1::TankLogicThinkFrame(void)
{
	StudioFrameAdvance();
	DispatchAnimEvents(this);

	// Keep looking for someone to blast up:
	if ((m_flTimeToLookForVictims < gpGlobals->curtime))
	{
		if (m_iTankState == TANK_IDLE)
		{
			bool bSetCannonTarget = true;
			CBaseCombatCharacter *pVictim = GetEnemyWithinRange();
			if (!pVictim)
			{
				pVictim = GetEnemyWithinRange(true);
				if (pVictim)
				{
					m_pMachineGunTarget = pVictim;
					bSetCannonTarget = false;
				}
			}

			if (pVictim)
			{
				Vector victimPos = pVictim->WorldSpaceCenter();
				Vector tankCenter = WorldSpaceCenter();
				m_iTankState = TANK_FACING_ENEMY;

				// YAW
				Vector dir = victimPos - tankCenter;
				VectorNormalize(dir);
				float yaw = UTIL_VecToYaw(dir);

				// Pitch
				Vector vecDir;
				GetAttachment(m_nCachedAttachments[TANK_ATTACHMENT_CANNON], vecDir);
				dir = victimPos - vecDir;
				VectorNormalize(dir);
				float pitchDiff = UTIL_VecToPitch(dir);

				if (pitchDiff <= -90)
					pitchDiff += 90;
				else if (pitchDiff > 90)
					pitchDiff -= 90;

				float flPercent = (((pitchDiff < 0) ? (90 - pitchDiff) : pitchDiff) / 180);
				m_flCannonPitchEnd = (flPercent * 26) - 13;

				// Calculate the time we should use to move from A to B.
				m_flTimeToFaceVictim = gpGlobals->curtime + ((fabs((m_flCannonYawStart - yaw)) / 360.0f) * TANK_TURN_360_TIME); // Set the time.
				m_flTimeStartedRotation = gpGlobals->curtime;
				m_flCannonYawEnd = yaw; // Set the end yaw.

				if (bSetCannonTarget)
					m_pCannonTarget = pVictim;
			}
		}
		else if (m_iTankState == TANK_WAITING)
		{
			CBaseCombatCharacter *pVictim = m_pCannonTarget.Get();
			Vector conePos, tankPos;
			QAngle angPos;
			GetAttachment(m_nCachedAttachments[TANK_ATTACHMENT_EYES], tankPos);
			GetAttachment(m_nCachedAttachments[TANK_ATTACHMENT_CANNON], conePos, angPos);
			if (IsValidTarget(pVictim, false, tankPos, conePos, angPos, true))
				m_iTankState = TANK_FIRE_PROJECTILE; // There's something here, kill it!
			else
			{
				m_pCannonTarget = NULL;
				m_iTankState = TANK_IDLE; // Nothing here, wait for new target /find.
			}
		}
		else if (m_iTankState == TANK_RELOADING)
		{
			m_iTankState = TANK_IDLE;
		}

		m_flTimeToLookForVictims = gpGlobals->curtime + TANK_THINK_FREQ;
	}

	// We want to move towards the victim.
	if (m_iTankState == TANK_FACING_ENEMY)
	{
		float timeNow = gpGlobals->curtime;
		if (timeNow >= m_flTimeToFaceVictim) // Finished! Give us some time to check.
		{
			m_iTankState = TANK_WAITING;
			m_flTimeToLookForVictims = gpGlobals->curtime + TANK_THINK_FREQ;
			m_flCannonYawStart = m_flCannonYawEnd;
			m_flCannonPitchStart = m_flCannonPitchEnd;
			StopSound("M1A1.Turning");
			m_iMachineGunState = GUN_FIRE;
		}
		else
		{
			m_iMachineGunState = GUN_IDLE;
			float timeToTake = (m_flTimeToFaceVictim - m_flTimeStartedRotation);
			float flFraction = clamp(((timeNow - m_flTimeStartedRotation) / timeToTake), 0.0f, 1.0f);

			int m_iPoseParam = m_nCachedPoseParams[TANK_HEAD_YAW];
			if (m_iPoseParam != -1)
			{
				float calcYaw = m_flCannonYawStart + ((m_flCannonYawEnd - m_flCannonYawStart) * flFraction);

				// Scale it back to 180 to -180
				float newYaw = (360 * (calcYaw / 360)) - 180;

				if (newYaw > 0)
					newYaw = (180 - newYaw);
				else
					newYaw = -(180 + newYaw);

				SetPoseParameter(m_iPoseParam, newYaw);
			}

			m_iPoseParam = m_nCachedPoseParams[TANK_CANNON_PITCH];
			if (m_iPoseParam != -1)
			{
				float calcPitch = m_flCannonPitchStart + ((m_flCannonPitchEnd - m_flCannonPitchStart) * flFraction);

				if ((calcPitch + 4) < 13)
					calcPitch += 4;

				SetPoseParameter(m_iPoseParam, calcPitch);
			}

			if (gpGlobals->curtime > m_flNextMoveSound)
			{
				m_flNextMoveSound = gpGlobals->curtime + 5.0f;
				PlaySound("M1A1.Turning");
			}
		}
	}
	else if (m_iTankState == TANK_FIRE_PROJECTILE)
	{
		// Get our barrel attachment.
		int iAttachment = m_nCachedAttachments[TANK_ATTACHMENT_CANNON];
		Vector vecPos;
		QAngle angPos;

		// Find our absolute position in the world: 
		if (GetAttachment(iAttachment, vecPos, angPos))
		{
			DispatchParticleEffect("muzzleflash_smg", PATTACH_POINT_FOLLOW, this, iAttachment); // Muzzle + Smoke
			CMissile *pMissile = CMissile::Create(vecPos, angPos, edict(), (m_iClassification == CLASS_MILITARY_VEHICLE));
			pMissile->SetGracePeriod(0.5);
			pMissile->SetDamage(bb2_m1a1_damage.GetFloat());
			pMissile->SetRadius(bb2_m1a1_radius.GetFloat());

			int iSequence = SelectHeaviestSequence(ACT_TANK_PRIMARY_FIRE);
			if (iSequence != ACT_INVALID)
			{
				ResetSequence(iSequence);
			}

			UTIL_ScreenShake(GetAbsOrigin(), 25.0, 150.0, 1.0, 500.0, SHAKE_START);
		}

		// Smoke Effects
		iAttachment = m_nCachedAttachments[TANK_ATTACHMENT_SMOKE_MIDDLE];
		if (iAttachment != -1)
			DispatchParticleEffect("dust_bombdrop", PATTACH_POINT_FOLLOW, this, iAttachment);

		for (int i = TANK_ATTACHMENT_SMOKE_BACK_1; i < TANK_ATTACHMENT_COUNT; i++)
		{
			iAttachment = m_nCachedAttachments[i];
			if (iAttachment != -1)
				DispatchParticleEffect("door_explosion_smoke", PATTACH_POINT_FOLLOW, this, iAttachment);
		}

		PlaySound("M1A1.Fire");
		PlaySound("M1A1.Reload");

		m_iTankState = TANK_RELOADING;
		m_flTimeToLookForVictims = gpGlobals->curtime + TANK_RELOAD_WAIT_TIME;
	}

	// Find a machine gun target
	// Handle the machine gun here:
	if ((m_iMachineGunState == GUN_FIRE) && (m_flMachineGunFireRate < gpGlobals->curtime) && (m_flCooldownTime < gpGlobals->curtime))
	{
		bool bCanStartFiring = false;

		int iAttachment = m_nCachedAttachments[TANK_ATTACHMENT_MUZZLE];
		Vector vecSrc, tankPos;
		QAngle angles;
		GetAttachment(iAttachment, vecSrc, angles);
		GetAttachment(m_nCachedAttachments[TANK_ATTACHMENT_EYES], tankPos);

		CBaseCombatCharacter *pTarget = m_pMachineGunTarget.Get();
		if (!IsValidTarget(pTarget, true, tankPos, vecSrc, angles, true))
		{
			m_pMachineGunTarget = GetEnemyWithinRange(true, true);
			bCanStartFiring = (m_pMachineGunTarget.Get() != NULL);
			if (bCanStartFiring)
				pTarget = m_pMachineGunTarget.Get();
		}
		else
			bCanStartFiring = true;

		if (bCanStartFiring)
		{
			float yaw, pitch;

			Vector dir = pTarget->WorldSpaceCenter() - vecSrc;
			VectorNormalize(dir);

			yaw = UTIL_VecToYaw(dir);
			pitch = UTIL_VecToPitch(dir);

			yaw = 13.0f * ((yaw - 180.0f) / 180.0f);
			pitch = 13.0f * ((pitch - 90.0f) / 90.0f);

			int m_iPoseParam = m_nCachedPoseParams[TANK_GUN_PITCH];
			if (m_iPoseParam != -1)
				SetPoseParameter(m_iPoseParam, pitch);

			m_iPoseParam = m_nCachedPoseParams[TANK_GUN_YAW];
			if (m_iPoseParam != -1)
				SetPoseParameter(m_iPoseParam, yaw);

			PlaySound("Weapon_AK47.Single");

			FireBullets(1, vecSrc, dir, VECTOR_CONE_1DEGREES, MAX_TRACE_LENGTH, m_iAmmoType, 2, entindex(), iAttachment, bb2_m1a1_damage_machinegun.GetInt(), this);
			DispatchParticleEffect("muzzleflash_smg", PATTACH_POINT_FOLLOW, this, iAttachment); // Muzzle + Smoke

			m_iBulletsFired++;
			if (m_iBulletsFired > MACHINE_GUN_BULLETS_OVERHEAT)
			{
				m_iBulletsFired = 0;
				m_flCooldownTime = gpGlobals->curtime + MACHINE_GUN_COOLDOWN_TIME;
				DispatchParticleEffect("weapon_muzzle_smoke_b", PATTACH_POINT_FOLLOW, this, iAttachment); // Muzzle + Smoke
			}

			m_flMachineGunFireRate = gpGlobals->curtime + bb2_m1a1_machinegun_firerate.GetFloat();
		}
	}

	if (m_flNextIdleSound < gpGlobals->curtime)
	{
		m_flNextIdleSound = gpGlobals->curtime + IDLE_TIME;
		PlaySound("M1A1.Idle");
	}

	SetNextThink(gpGlobals->curtime + 0.01f);
}

#if 0
CON_COMMAND(m1a1_test_yaw, "Test")
{
	if (args.ArgC() != 2)
		return;

	float flVal = atof(args[1]);

	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "npc_m1a1");
	if (pEntity)
	{
		CNPCM1A1 *pTank = dynamic_cast<CNPCM1A1*> (pEntity);
		if (pTank)
		{
			int iPoseParam = pTank->LookupPoseParameter("aim_yaw");
			if (iPoseParam != -1)
				pTank->SetPoseParameter(iPoseParam, flVal);
		}
	}
};

CON_COMMAND(m1a1_test_calc_yaw, "Test")
{
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "npc_m1a1");
	if (pEntity)
	{
		CNPCM1A1 *pTank = dynamic_cast<CNPCM1A1*> (pEntity);
		if (pTank)
		{
			int iPoseParam = pTank->LookupPoseParameter("aim_yaw");
			if (iPoseParam != -1)
			{
				CBasePlayer *pEnt = UTIL_GetLocalPlayer();
				if (pEnt)
				{
					Vector dir = pEnt->GetAbsOrigin() - pTank->GetAbsOrigin();
					VectorNormalize(dir);

					float yaw = UTIL_VecToYaw(dir);
					float newYaw = (360 * (yaw / 360)) - 180;

					if (newYaw > 0)
						newYaw = (180 - newYaw);
					else
						newYaw = -(180 + newYaw);

					pTank->SetPoseParameter(iPoseParam, newYaw);
				}
			}

			iPoseParam = pTank->LookupPoseParameter("gun_yaw");
			if (iPoseParam != -1)
			{
				CBasePlayer *pEnt = UTIL_GetLocalPlayer();
				if (pEnt)
				{
					Vector vecDir;
					pTank->GetAttachment(pTank->LookupAttachment("machinegun_muzzle"), vecDir);
					Vector dir = pEnt->GetAbsOrigin() - vecDir;
					VectorNormalize(dir);

					float yaw = UTIL_VecToYaw(dir);
					float newYaw = (360 * (yaw / 360)) - 180;

					if (newYaw > 0)
						newYaw = (180 - newYaw);
					else
						newYaw = -(180 + newYaw);

					pTank->SetPoseParameter(iPoseParam, newYaw);
				}
			}
		}
	}
};

CON_COMMAND(m1a1_test_calc_pitch, "Test")
{
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "npc_m1a1");
	if (pEntity)
	{
		CNPCM1A1 *pTank = dynamic_cast<CNPCM1A1*> (pEntity);
		if (pTank)
		{
			int iPoseParam = pTank->LookupPoseParameter("aim_pitch");
			if (iPoseParam != -1)
			{
				CBasePlayer *pEnt = UTIL_GetLocalPlayer();
				if (pEnt)
				{
					Vector vecDir;
					pTank->GetAttachment(pTank->LookupAttachment("cannon_muzzle"), vecDir);

					Vector	enemyDir = pEnt->GetAbsOrigin() - vecDir;
					VectorNormalize(enemyDir);
					float pitch = UTIL_VecToPitch(enemyDir);

					if (pitch <= -90)
						pitch += 90;
					else if (pitch > 90)
						pitch -= 90;

					float flPercent = (((pitch < 0) ? (90 - pitch) : pitch) / 180);
					float flScaled = (flPercent * 26) - 13;

					pTank->SetPoseParameter(iPoseParam, flScaled);
				}
			}
		}
	}
};

CON_COMMAND(m1a1_test_calc_gun_yaw, "Test")
{
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "npc_m1a1");
	if (pEntity)
	{
		CNPCM1A1 *pTank = dynamic_cast<CNPCM1A1*> (pEntity);
		if (pTank)
		{
			int iPoseParam = pTank->LookupPoseParameter("gun_yaw");
			if (iPoseParam != -1)
			{
				CBasePlayer *pEnt = UTIL_GetLocalPlayer();
				if (pEnt)
				{
					Vector vecDir;
					pTank->GetAttachment(pTank->LookupAttachment("machinegun_muzzle"), vecDir);

					Vector enemyDir = pEnt->WorldSpaceCenter() - vecDir;
					VectorNormalize(enemyDir);
					float yaw = UTIL_VecToYaw(enemyDir);
					Msg("GunYAW: %f\n", yaw);
				}
			}
		}
	}
};

CON_COMMAND(m1a1_test_calc_gun_pitch, "Test")
{
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, "npc_m1a1");
	if (pEntity)
	{
		CNPCM1A1 *pTank = dynamic_cast<CNPCM1A1*> (pEntity);
		if (pTank)
		{
			int iPoseParam = pTank->LookupPoseParameter("gun_pitch");
			if (iPoseParam != -1)
			{
				CBasePlayer *pEnt = UTIL_GetLocalPlayer();
				if (pEnt)
				{
					Vector vecSrc, vecUP;
					QAngle angles;
					pTank->GetAttachment(pTank->LookupAttachment("machinegun_muzzle"), vecSrc, angles);

					AngleVectors(angles, NULL, NULL, &vecUP);

					Vector enemyDir = pEnt->WorldSpaceCenter() - vecSrc;
					VectorNormalize(enemyDir);
					VectorNormalize(vecUP);

					float pitch = RAD2DEG(acos(DotProduct(vecUP, enemyDir) / (vecUP.Length() * enemyDir.Length())));
					Msg("GunPitch: %f\n", pitch);
				}
			}
		}
	}
};
#endif
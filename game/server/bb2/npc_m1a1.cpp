//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Friendly Military Tank
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "npc_vehicledriver.h"
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
#define TANK_TURN_360_TIME 10000.0f
#define MACHINE_GUN_COOLDOWN_TIME 5.0f
#define MACHINE_GUN_BULLETS_OVERHEAT 30
#define IDLE_TIME 1.0f

ConVar bb2_m1a1_range_max("bb2_m1a1_range_max", "2000", FCVAR_REPLICATED, "Set the max range of the M1A1.", true, 500.0f, true, 10000.0f);
ConVar bb2_m1a1_range_min("bb2_m1a1_range_min", "400", FCVAR_REPLICATED, "Set the min range of the M1A1.", true, 250.0f, true, 5000.0f);
ConVar bb2_m1a1_damage("bb2_m1a1_damage", "3000", FCVAR_REPLICATED, "Set the damage of the M1A1 cannon.", true, 100.0f, true, 10000.0f);
ConVar bb2_m1a1_radius("bb2_m1a1_radius", "500", FCVAR_REPLICATED, "Set the radius of the explosion.", true, 200.0f, true, 10000.0f);
ConVar bb2_m1a1_friendly("bb2_m1a1_friendly", "1", FCVAR_REPLICATED, "Should the M1A1 not kill friendlies, for example other Military NPCs.", true, 0.0f, true, 1.0f);
ConVar bb2_m1a1_damage_machinegun("bb2_m1a1_damage_machinegun", "30", FCVAR_REPLICATED, "Set the damage of the M1A1 machinegun.", true, 25.0f, true, 1000.0f);
ConVar bb2_m1a1_machinegun_firerate("bb2_m1a1_machinegun_firerate", "0.08", FCVAR_REPLICATED, "Set the fire rate of the machinegun", true, 0.01f, true, 1.0f);

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

const char *pszSharedSmokeAttachments[] =
{
	"medium_smoke_back_1",
	"medium_smoke_back_2",
	"small_smoke_back_3",
	"small_smoke_head",
};

class CNPCM1A1 : public CBaseAnimating
{
public:

	DECLARE_CLASS(CNPCM1A1, CBaseAnimating);
	DECLARE_DATADESC();

	CNPCM1A1();
	void Spawn(void);
	void Precache(void);

	Class_T Classify(void);

	int	GetTracerAttachment(void);

	void PlaySound(const char *soundScript);

private:

	CBaseEntity *GetEnemyWithinRange(bool bMachineGun = false);
	void OnLookForVictim(void);

	// Tank Handling
	int m_iTankState;
	float m_flTimeToLookForVictims;
	float m_flTimeToFaceVictim;
	float m_flTimeToUsePerUnit;

	// Tank Pose Params:
	float m_flCannonYawStart;
	float m_flCannonYawEnd;
	float m_flCannonPitchEnd;
	float m_flCannonPitchStart;

	// Machine Gun 
	int m_iMachineGunState;
	int m_iAmmoType;
	int m_iBulletsFired;
	float m_flCooldownTime;
	float m_flMachineGunFireRate;
	EHANDLE m_pEnemy;

	// Sound 
	float m_flNextMoveSound;
	float m_flNextIdleSound;
};

LINK_ENTITY_TO_CLASS(npc_m1a1, CNPCM1A1)

BEGIN_DATADESC(CNPCM1A1)
DEFINE_THINKFUNC(OnLookForVictim),
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

	// Machine Gun
	m_iMachineGunState = GUN_IDLE;
	m_iBulletsFired = 0;
	m_flCooldownTime = 0.0f;
	m_flMachineGunFireRate = 0.0f;

	m_pEnemy = NULL;
}

void CNPCM1A1::Spawn(void)
{
	Precache();
	SetModel(TANK_MODEL);

	SetSolid(SOLID_VPHYSICS);
	SetMoveType(MOVETYPE_NONE);
	VPhysicsInitShadow(false, false);
	//VPhysicsInitStatic();

	BaseClass::Spawn();

	int iSequence = SelectHeaviestSequence(ACT_IDLE);
	if (iSequence != ACT_INVALID)
	{
		SetSequence(iSequence);
		ResetSequenceInfo();
		m_flPlaybackRate = 1.0f;
	}

	SetThink(&CNPCM1A1::OnLookForVictim);
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
	if (bb2_m1a1_friendly.GetBool())
		return CLASS_MILITARY_VEHICLE;

	return CLASS_NONE;
}

int	CNPCM1A1::GetTracerAttachment(void)
{
	int iAttachment = LookupAttachment("machinegun_muzzle");
	if (iAttachment != -1)
		return iAttachment;

	return 1;
}

void CNPCM1A1::PlaySound(const char *soundScript)
{
	EmitSound(soundScript);
}

CBaseEntity *CNPCM1A1::GetEnemyWithinRange(bool bMachineGun)
{
	CBaseEntity *pEntity = NULL;
	for (CEntitySphereQuery sphere(GetAbsOrigin(), bb2_m1a1_range_max.GetFloat()); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		if ((pEntity == this) || (!pEntity->IsNPC() && !pEntity->IsPlayer()) || (!pEntity->IsZombie(true) && !pEntity->IsMercenary()) || 
			!pEntity->FVisible(this, MASK_SHOT))
			continue;

		float flDistToEnt = pEntity->GetAbsOrigin().DistTo(GetAbsOrigin());
		if (bMachineGun && (flDistToEnt > bb2_m1a1_range_min.GetFloat()))
			continue;
		else if (!bMachineGun && ((flDistToEnt > bb2_m1a1_range_max.GetFloat()) || (flDistToEnt < bb2_m1a1_range_min.GetFloat())))
			continue;

		return pEntity;
	}

	return NULL;
}

void CNPCM1A1::OnLookForVictim(void)
{
	StudioFrameAdvance();
	DispatchAnimEvents(this);

	// Keep looking for someone to blast up:
	if ((m_flTimeToLookForVictims < gpGlobals->curtime))
	{
		if (m_iTankState == TANK_IDLE)
		{
			CBaseEntity *pVictim = GetEnemyWithinRange();
			if (pVictim)
			{
				m_iTankState = TANK_FACING_ENEMY;

				// YAW
				Vector dir = pVictim->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize(dir);

				float yaw = UTIL_VecToYaw(dir);

				// Pitch
				Vector vecDir;
				GetAttachment(LookupAttachment("cannon_muzzle"), vecDir);

				dir = pVictim->GetAbsOrigin() - vecDir;
				VectorNormalize(dir);
				float pitchDiff = UTIL_VecToPitch(dir);

				if (pitchDiff <= -90)
					pitchDiff += 90;
				else if (pitchDiff > 90)
					pitchDiff -= 90;

				float flPercent = (((pitchDiff < 0) ? (90 - pitchDiff) : pitchDiff) / 180);
				m_flCannonPitchEnd = (flPercent * 26) - 13;

				// Calculate the time we should use to move from A to B.
				float timeToMoveInMS = (fabs((m_flCannonYawStart - yaw)) / 360) * TANK_TURN_360_TIME; // Equation
				m_flTimeToUsePerUnit = m_flTimeToFaceVictim = timeToMoveInMS; // Set the time.
				m_flCannonYawEnd = yaw; // Set the end yaw.
			}
		}
		else if (m_iTankState == TANK_WAITING)
		{
			CBaseEntity *pVictim = GetEnemyWithinRange();
			if (pVictim)
				m_iTankState = TANK_FIRE_PROJECTILE; // There's something here, kill it!
			else
				m_iTankState = TANK_IDLE; // Nothing here, wait for new target /find.
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
		if (((m_flTimeToUsePerUnit - m_flTimeToFaceVictim) >= m_flTimeToUsePerUnit)) // Finished! Give us some time to check.
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
			float flFraction = (1.0f - SimpleSpline(m_flTimeToFaceVictim / m_flTimeToUsePerUnit));

			int m_iPoseParam = LookupPoseParameter("aim_yaw");
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

			m_iPoseParam = LookupPoseParameter("aim_pitch");
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
		int iAttachment = LookupAttachment("cannon_muzzle");
		Vector vecPos;
		QAngle angPos;

		// Find our absolute position in the world: 
		if (GetAttachment(iAttachment, vecPos, angPos))
		{
			DispatchParticleEffect("muzzleflash_smg", PATTACH_POINT_FOLLOW, this, iAttachment); // Muzzle + Smoke
			CMissile *pMissile = CMissile::Create(vecPos, angPos, edict(), bb2_m1a1_friendly.GetBool());
			pMissile->SetGracePeriod(0.5);
			pMissile->SetDamage(bb2_m1a1_damage.GetFloat());
			pMissile->SetRadius(bb2_m1a1_radius.GetFloat());

			int iSequence = SelectHeaviestSequence(ACT_TANK_PRIMARY_FIRE);
			if (iSequence != ACT_INVALID)
			{
				ResetSequence(iSequence);
			}
		}

		// Smoke Effects
		iAttachment = LookupAttachment("heavy_smoke_middle");
		if (iAttachment != -1)
			DispatchParticleEffect("dust_bombdrop", PATTACH_POINT_FOLLOW, this, iAttachment);

		for (int i = 0; i < _ARRAYSIZE(pszSharedSmokeAttachments); i++)
		{
			iAttachment = LookupAttachment(pszSharedSmokeAttachments[i]);
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
	if ((m_iMachineGunState == GUN_FIRE) && (m_flMachineGunFireRate < gpGlobals->curtime))
	{
		bool bCanStartFiring = false;

		CBaseEntity *pTarget = m_pEnemy.Get();
		if (!pTarget || (pTarget && !pTarget->IsAlive()))
		{
			m_pEnemy = GetEnemyWithinRange(true);
			bCanStartFiring = (m_pEnemy.Get() != NULL);
			if (bCanStartFiring)
				pTarget = m_pEnemy.Get();
		}
		else
			bCanStartFiring = true;

		if (m_flCooldownTime > gpGlobals->curtime)
			bCanStartFiring = false;

		if (bCanStartFiring)
		{
			int iAttachment = LookupAttachment("machinegun_muzzle");

			float yaw, pitch;
			Vector vecDir, vecShootDir;
			QAngle angles;
			GetAttachment(iAttachment, vecDir, angles);

			Vector dir = pTarget->GetAbsOrigin() - vecDir;
			VectorNormalize(dir);

			yaw = UTIL_VecToYaw(dir);
			pitch = UTIL_VecToPitch(dir);

			//Msg("GUN YAW %f PITCH %f\n", yaw, pitch);

			if (pitch <= -90)
				pitch += 90;
			else if (pitch > 90)
				pitch -= 90;

			if (yaw <= -90)
				yaw += 90;
			else if (yaw > 90)
				yaw -= 90;

			pitch = (((((pitch < 0) ? (90 - pitch) : pitch) / 180)) * 26) - 13;
			yaw = (((((yaw < 0) ? (90 - yaw) : yaw) / 180)) * 26) - 13;

			int m_iPoseParam = LookupPoseParameter("gun_pitch");
			if (m_iPoseParam != -1)
				SetPoseParameter(m_iPoseParam, pitch);

			m_iPoseParam = LookupPoseParameter("gun_yaw");
			if (m_iPoseParam != -1)
				SetPoseParameter(m_iPoseParam, yaw);

			PlaySound("Weapon_AK47.Single");

			AngleVectors(angles, &vecShootDir);
			FireBullets(1, vecDir, vecShootDir, VECTOR_CONE_1DEGREES, MAX_TRACE_LENGTH, m_iAmmoType, 2, entindex(), iAttachment, bb2_m1a1_damage_machinegun.GetInt());
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

	float frame_msec = 1000.0f * gpGlobals->frametime;

	if (m_flTimeToFaceVictim > 0)
	{
		m_flTimeToFaceVictim -= frame_msec;
		if (m_flTimeToFaceVictim < 0)
			m_flTimeToFaceVictim = 0;
	}

	if (m_flNextIdleSound < gpGlobals->curtime)
	{
		m_flNextIdleSound = gpGlobals->curtime + IDLE_TIME;
		PlaySound("M1A1.Idle");
	}

	SetNextThink(gpGlobals->curtime + 0.01f);
}

#ifdef debug
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
#endif
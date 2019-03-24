//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: 2 Zombie Spawner : Based on the origin around the center of the volume/trigger. The volume makes no difference in spawn pos/area etc...
// Will be replaced with a volume related one + zombie limits...
// 2014. Added zombie limits. However zombie limits are not changed if using ent_remove.. Only if dead it will increase limit. And on spawn decrease. Max 80 or less...
// Changelog : April 08, fixed it to now spawn random zombies in its volume, a tracer checks bounding box if it is possible to spawn the zombie!
// it also asks for permission to spawn each zombie, and it respects the new zombie limit defined in the bb2_zombie*** convar.
//
//========================================================================================//

#include "cbase.h"
#include "ZombieVolume.h"
#include "ai_basenpc.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "collisionutils.h"
#include "random_extended.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum ZombieClassTypes
{
	ZOMBIE_TYPE_WALKER = 0,
	ZOMBIE_TYPE_RUNNER,
	ZOMBIE_TYPE_RANDOM,
};

ConVar bb2_zombie_spawner_distance("bb2_zombie_spawner_distance", "5200", FCVAR_GAMEDLL, "If there is no players within this distance from the spawner it will not spawn any zombies.", true, 200.0f, false, 0.0f);

bool IsAllowedToSpawn(CBaseEntity *pEntity, float distance, float zDiff, bool bCheckVisible)
{
	if (!pEntity)
		return false;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient)
			continue;

		if ((pClient->Classify() != CLASS_PLAYER) || !pClient->IsAlive() || pClient->IsObserver())
			continue;

		if (pEntity->GetLocalOrigin().DistTo(pClient->GetLocalOrigin()) > distance)
			continue;

		if (pEntity->CollisionProp() && (
			IsBoxIntersectingBox(
			(pEntity->GetLocalOrigin() + pEntity->CollisionProp()->OBBMins()),
			(pEntity->GetLocalOrigin() + pEntity->CollisionProp()->OBBMaxs()),
			(pClient->GetLocalOrigin() + pClient->WorldAlignMins()),
			(pClient->GetLocalOrigin() + pClient->WorldAlignMaxs())) ||
			IsPointInBox(
			pClient->GetLocalOrigin(),
			(pEntity->GetLocalOrigin() + pEntity->CollisionProp()->OBBMins()),
			(pEntity->GetLocalOrigin() + pEntity->CollisionProp()->OBBMaxs()))
			))
		{
			return true;
		}

		float diff = abs((pEntity->GetLocalOrigin() - pClient->GetLocalOrigin()).z);
		if ((zDiff > 0.0f) && (diff > zDiff))
			continue;

		if (bCheckVisible && !pEntity->FVisible(pClient, MASK_BLOCKLOS))
			continue;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Zombie Volume / Zombie Spawner - Thinks and spawns X zombies after X seconds!
//-----------------------------------------------------------------------------

BEGIN_DATADESC(CZombieVolume)

// Think
DEFINE_THINKFUNC(VolumeThink),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "ForceSpawn", InputStartSpawn),
DEFINE_INPUTFUNC(FIELD_VOID, "StopSpawn", InputStopSpawn),

// Outs
DEFINE_OUTPUT(m_OnWaveSpawned, "NewWaveSpawned"),
DEFINE_OUTPUT(m_OnForceSpawned, "OnStartWaves"),
DEFINE_OUTPUT(m_OnForceStop, "OnStop"),

// Hammer Keyfields
DEFINE_KEYFIELD(m_iTypeToSpawn, FIELD_INTEGER, "ZombieType"),
DEFINE_KEYFIELD(m_iZombiesToSpawn, FIELD_INTEGER, "ZombieNumber"),
DEFINE_KEYFIELD(m_flMaxDistance, FIELD_FLOAT, "MaximumDistance"),
DEFINE_KEYFIELD(m_flMaxZDifference, FIELD_FLOAT, "MaximumZDifference"),
DEFINE_KEYFIELD(m_flSpawnInterval, FIELD_FLOAT, "SpawnInterval"),
DEFINE_KEYFIELD(m_flSpawnFrequency, FIELD_FLOAT, "SpawnFrequency"),
DEFINE_KEYFIELD(m_flRandomSpawnPercent, FIELD_FLOAT, "RandomSpawnChance"),
DEFINE_KEYFIELD(m_bSpawnNoMatterWhat, FIELD_BOOLEAN, "AutoSpawn"),

DEFINE_KEYFIELD(goalEntity, FIELD_STRING, "goal_target"),
DEFINE_KEYFIELD(goalActivity, FIELD_INTEGER, "goal_activity"),
DEFINE_KEYFIELD(goalType, FIELD_INTEGER, "goal_type"),
DEFINE_KEYFIELD(goalInterruptType, FIELD_INTEGER, "goal_interrupt_type"),

DEFINE_FIELD(m_flNextSpawnWave, FIELD_TIME),
DEFINE_FIELD(m_iSpawnNum, FIELD_INTEGER),

END_DATADESC()

LINK_ENTITY_TO_CLASS(zombie_volume, CZombieVolume);

CZombieVolume::CZombieVolume(void)
{
	m_flSpawnInterval = 10.0f;
	m_flRandomSpawnPercent = 20.0f;
	m_iZombiesToSpawn = 5;
	m_iSpawnNum = 0;
	m_iTypeToSpawn = 0;
	m_flNextSpawnWave = 0.0f;
	m_bSpawnNoMatterWhat = false;
	m_spawnflags = 0;

	goalEntity = NULL_STRING;
	goalActivity = ACT_WALK;
	goalType = 0;
	goalInterruptType = DAMAGEORDEATH_INTERRUPTABILITY;

	m_flMaxDistance = bb2_zombie_spawner_distance.GetFloat();
	m_flMaxZDifference = 0.0f;
	m_flSpawnFrequency = 0.0f;
}

void CZombieVolume::Spawn()
{
	Precache();
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	SetModel(STRING(GetModelName()));
	m_nRenderMode = kRenderEnvironmental;

	if (m_iZombiesToSpawn > 50)
		m_iZombiesToSpawn = 50;
	else if (m_iZombiesToSpawn <= 0)
		m_iZombiesToSpawn = 5;

	if (HasSpawnFlags(SF_STARTACTIVE))
	{
		m_flNextSpawnWave = 0.0f;
		SetThink(&CZombieVolume::VolumeThink);
		SetNextThink(gpGlobals->curtime + 0.01f);
	}
}

void CZombieVolume::VolumeThink()
{
	if (m_flNextSpawnWave <= gpGlobals->curtime)
	{
		m_flNextSpawnWave = gpGlobals->curtime + m_flSpawnInterval;
		m_OnWaveSpawned.FireOutput(this, this);
		m_iSpawnNum = 0;
	}

	// If the round has started + we haven't  reached any limits, try to spawn!
	if (HL2MPRules()->CanSpawnZombie() && (m_iSpawnNum < m_iZombiesToSpawn) && HL2MPRules()->m_bRoundStarted && !HL2MPRules()->IsGameoverOrScoresVisible())
	{
		if (m_bSpawnNoMatterWhat || IsAllowedToSpawn(this, m_flMaxDistance, m_flMaxZDifference, (HasSpawnFlags(SF_NOVISCHECK) == false)))
			SpawnWave();
	}

	SetNextThink(gpGlobals->curtime + GetSpawnFrequency());
}

void CZombieVolume::InputStartSpawn(inputdata_t &inputData)
{
	m_flNextSpawnWave = 0.0f;
	SetThink(&CZombieVolume::VolumeThink);
	SetNextThink(gpGlobals->curtime + 0.01f);
	m_OnForceSpawned.FireOutput(this, this);
}

void CZombieVolume::InputStopSpawn(inputdata_t &inputData)
{
	SetThink(NULL);
	m_OnForceStop.FireOutput(this, this);
}

float CZombieVolume::GetSpawnFrequency(void)
{
	if (m_flSpawnFrequency > 0.0f)
		return m_flSpawnFrequency;

	return bb2_spawn_frequency.GetFloat();
}

void CZombieVolume::TraceZombieBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm, CBaseEntity *pEntity)
{
	Vector ZombieMins = Vector(-25, -25, 0);
	Vector ZombieMax = Vector(25, 25, 74);

	// Here the zombies will spawn standing up, not lying down, use a smaller hull!
	if (HasSpawnFlags(SF_FASTSPAWN))
	{
		ZombieMins = Vector(-18, -18, 0);
		ZombieMax = Vector(18, 18, 74);
	}

	Ray_t ray;
	ray.Init(start, end, ZombieMins, ZombieMax);
	UTIL_TraceRay(ray, fMask, pEntity, collisionGroup, &pm);
}

void CZombieVolume::SpawnWave()
{
	Vector vecBoundsMaxs = CollisionProp()->OBBMaxs(),
		vecBoundsMins = CollisionProp()->OBBMins();

	Vector vecDown;
	AngleVectors(GetLocalAngles(), NULL, NULL, &vecDown);
	VectorNormalize(vecDown);
	vecDown *= -1;

	Vector newPos = GetLocalOrigin() +
		Vector(random->RandomFloat(vecBoundsMins.x, vecBoundsMaxs.x), random->RandomFloat(vecBoundsMins.y, vecBoundsMaxs.y), 0);

	trace_t tr;
	UTIL_TraceLine(newPos, newPos + vecDown * MAX_TRACE_LENGTH, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NPC_ZOMBIE, &tr);
	newPos.z = ceil(tr.endpos.z + 2.0f);

	TraceZombieBBox(newPos, newPos, MASK_NPCSOLID, COLLISION_GROUP_NPC_ZOMBIE, tr, this);

	// We hit an entity which means there is already something in this part of the volume! Ignore and continue to next part of the volume!
	if (tr.startsolid || tr.allsolid || tr.DidHitNonWorldEntity())
	{
		DevMsg(2, "A zombie couldn't spawn at %f %f %f! (StartSolid %i, AllSolid %i, HitNonWorld %i - %s)\n",
			tr.endpos.x, tr.endpos.y, tr.endpos.z,
			tr.startsolid, tr.allsolid, tr.DidHitNonWorldEntity(), tr.m_pEnt ? tr.m_pEnt->GetClassname() : "N/A"
			);
		return;
	}

	CAI_BaseNPC *npcZombie = (CAI_BaseNPC*)CreateEntityByName(GetZombieClassnameToSpawn());
	if (npcZombie)
	{
		QAngle randomAngle = QAngle(0, random->RandomFloat(-180.0f, 180.0f), 0);
		npcZombie->SetAbsOrigin(tr.endpos);
		npcZombie->SetAbsAngles(randomAngle);
		if (HasSpawnFlags(SF_FASTSPAWN))
			npcZombie->SpawnDirectly(); // Skip zombie fade-in + rise stuff, use fast spawn for npcs which spawn in a hidden place, will make them spawn faster + more efficient.

		npcZombie->Spawn();
		// UTIL_DropToFloor(npcZombie, MASK_NPCSOLID);

		if (goalEntity != NULL_STRING)
		{
			CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, STRING(goalEntity));
			if (!pTarget)
				pTarget = gEntList.FindEntityByClassname(NULL, STRING(goalEntity));

			if (pTarget)
				npcZombie->SpawnRunSchedule(pTarget, ((Activity)goalActivity), (goalType >= 1), goalInterruptType);
		}
	}

	m_iSpawnNum++;
}

const char *CZombieVolume::GetZombieClassnameToSpawn()
{
	const char *pszTypes[] =
	{
		"npc_runner",
	};

	switch (m_iTypeToSpawn)
	{
	case ZOMBIE_TYPE_WALKER:
		return "npc_walker";
	case ZOMBIE_TYPE_RUNNER:
		return "npc_runner";
	case ZOMBIE_TYPE_RANDOM:
	{
		double percent = (m_flRandomSpawnPercent / PERCENT_BASE);
		if (TryTheLuck(percent) && !GameBaseServer()->IsClassicMode())
			return (pszTypes[random->RandomInt(0, (_ARRAYSIZE(pszTypes) - 1))]);

		return "npc_walker";
	}
	default:
		return "npc_walker";
	}
}
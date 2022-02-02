//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
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
#include "npc_BaseZombie.h"
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

static ConVar bb2_zombie_spawner_distance("bb2_zombie_spawner_distance", "5200", FCVAR_GAMEDLL, "If there is no players within this distance from the spawner it will not spawn any zombies.", true, 200.0f, false, 0.0f);
static ConVar bb2_zombie_spawner_continuous("bb2_zombie_spawner_continuous", "1", FCVAR_GAMEDLL, "Waves can be spawned even if the zombies in a previous wave did not fully spawn yet, do not wait.", true, 0.0f, true, 1.0f);
static CUtlVector<CZombieVolume*> g_pZombieVolumes;

bool IsAllowedToSpawn(CBaseEntity *pEntity, float distance, float zDiff, bool bCheckVisible)
{
	if (!pEntity)
		return false;

	CTraceFilterWorldOnly traceFilter;
	trace_t tr;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient || (pClient->Classify() != CLASS_PLAYER) || !pClient->IsAlive() || pClient->IsObserver())
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

		if (bCheckVisible)
		{
			UTIL_TraceLine(pEntity->GetLocalOrigin(), pClient->EyePosition(), MASK_BLOCKLOS, &traceFilter, &tr);
			if (tr.fraction != 1.0 || tr.startsolid)
				continue;
		}

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
DEFINE_KEYFIELD(m_iZombiesMin, FIELD_INTEGER, "ZombieNumber"),
DEFINE_KEYFIELD(m_iZombiesMax, FIELD_INTEGER, "ZombieNumberMax"),
DEFINE_KEYFIELD(m_flMaxDistance, FIELD_FLOAT, "MaximumDistance"),
DEFINE_KEYFIELD(m_flMaxZDifference, FIELD_FLOAT, "MaximumZDifference"),
DEFINE_KEYFIELD(m_flSpawnInterval, FIELD_FLOAT, "SpawnInterval"),
DEFINE_KEYFIELD(m_flRandomSpawnPercent, FIELD_FLOAT, "RandomSpawnChance"),
DEFINE_KEYFIELD(m_bSpawnNoMatterWhat, FIELD_BOOLEAN, "AutoSpawn"),

DEFINE_KEYFIELD(goalEntity, FIELD_STRING, "goal_target"),
DEFINE_KEYFIELD(goalActivity, FIELD_INTEGER, "goal_activity"),
DEFINE_KEYFIELD(goalType, FIELD_INTEGER, "goal_type"),
DEFINE_KEYFIELD(goalInterruptType, FIELD_INTEGER, "goal_interrupt_type"),

END_DATADESC()

LINK_ENTITY_TO_CLASS(zombie_volume, CZombieVolume);

CZombieVolume::CZombieVolume()
{
	m_flSpawnInterval = 10.0f;
	m_flRandomSpawnPercent = 20.0f;

	m_iZombiesMin = m_iZombiesMax = m_iZombiesToSpawn = 0;

	m_iSpawnNum = 0;
	m_iTypeToSpawn = 0;
	m_flNextSpawnWave = 0.0f;
	m_bSpawnNoMatterWhat = false;
	m_spawnflags = 0;

	goalEntity = NULL_STRING;
	goalActivity = 1;
	goalType = 0;
	goalInterruptType = DAMAGEORDEATH_INTERRUPTABILITY;

	m_flMaxDistance = bb2_zombie_spawner_distance.GetFloat();
	m_flMaxZDifference = 0.0f;

	m_vZombieList.Purge();
	g_pZombieVolumes.AddToTail(this);
}

CZombieVolume::~CZombieVolume()
{
	m_vZombieList.Purge();
	g_pZombieVolumes.FindAndRemove(this);
}

void CZombieVolume::Spawn()
{
	Precache();
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	SetModel(STRING(GetModelName()));
	SetBlocksLOS(false);
	m_nRenderMode = kRenderEnvironmental;
	m_iZombiesMax = MAX(m_iZombiesMin, m_iZombiesMax);

	if (HasSpawnFlags(SF_STARTACTIVE))
	{
		m_flNextSpawnWave = 0.0f;
		SetThink(&CZombieVolume::VolumeThink);
		SetNextThink(gpGlobals->curtime + 0.01f);
	}
}

void CZombieVolume::VolumeThink()
{
	float flSpawnFreq = 0.1f;

	if (m_flNextSpawnWave <= gpGlobals->curtime)
	{
		m_flNextSpawnWave = gpGlobals->curtime + m_flSpawnInterval;
		m_OnWaveSpawned.FireOutput(this, this);
		m_iSpawnNum = 0;
		m_iZombiesToSpawn = ((int)GameBaseShared()->GetPlayerScaledValue(m_iZombiesMin, m_iZombiesMax));
		m_iZombiesToSpawn = clamp(m_iZombiesToSpawn, 1, 50);
	}

	// If the round has started + we haven't reached any limits, try to spawn!
	if ((m_iSpawnNum < m_iZombiesToSpawn) && HL2MPRules()->m_bRoundStarted && !HL2MPRules()->IsGameoverOrScoresVisible())
	{
		if (HL2MPRules()->CanSpawnZombie())
		{
			if (m_bSpawnNoMatterWhat || IsAllowedToSpawn(this, m_flMaxDistance, m_flMaxZDifference, (HasSpawnFlags(SF_NOVISCHECK) == false)))
			{
				SpawnZombie();
				flSpawnFreq = GetSpawnFrequency();
				if (!bb2_zombie_spawner_continuous.GetBool())
					m_flNextSpawnWave += flSpawnFreq;
			}
		}
		else // Not able to spawn, tell zomb class to mark some for quick death stuff!		
			CNPC_BaseZombie::MarkOldestNPCForDeath();
	}

	SetNextThink(gpGlobals->curtime + flSpawnFreq);
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
	return RandomDoubleNumber(bb2_zombie_spawn_freq_min.GetFloat(), bb2_zombie_spawn_freq_max.GetFloat());
}

void CZombieVolume::TraceZombieBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm, CBaseEntity *pEntity)
{
	Vector ZombieMins = (HasSpawnFlags(SF_FASTSPAWN) ? Vector(-18, -18, 0) : Vector(-25, -25, 0));
	Vector ZombieMax = (HasSpawnFlags(SF_FASTSPAWN) ? Vector(18, 18, 74) : Vector(25, 25, 74));
	Ray_t ray;
	ray.Init(start, end, ZombieMins, ZombieMax);
	UTIL_TraceRay(ray, fMask, pEntity, collisionGroup, &pm);
}

void CZombieVolume::SpawnZombie()
{
	// Do we only want to spawn new zombos if the old ones died?
	if (HasSpawnFlags(SF_RESPAWN_ONLY_IF_DEAD) && (m_vZombieList.Count() >= m_iZombiesToSpawn))
		return;

	Vector vecBoundsMaxs = CollisionProp()->OBBMaxs(),
		vecBoundsMins = CollisionProp()->OBBMins();

	Vector vecDown;
	AngleVectors(GetLocalAngles(), NULL, NULL, &vecDown);
	VectorNormalize(vecDown);
	vecDown *= -1;

	Vector newPos = GetLocalOrigin() +
		Vector(random->RandomFloat(vecBoundsMins.x, vecBoundsMaxs.x), random->RandomFloat(vecBoundsMins.y, vecBoundsMaxs.y), 0);

	trace_t tr;
	CTraceFilterWorldAndPropsOnly worldFilter;
	UTIL_TraceLine(newPos, newPos + vecDown * MAX_TRACE_LENGTH, MASK_SOLID, &worldFilter, &tr);
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
				npcZombie->SpawnRunSchedule(pTarget, ((goalActivity >= 1) ? ACT_RUN : ACT_WALK), (goalType >= 1), goalInterruptType);
		}

		if (HasSpawnFlags(SF_RESPAWN_ONLY_IF_DEAD))
			m_vZombieList.AddToTail(npcZombie->entindex());
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

/*static*/ void CZombieVolume::OnZombieRemoved(int index)
{
	for (int i = 0; i < g_pZombieVolumes.Count(); i++)
	{
		if ((g_pZombieVolumes[i] == NULL) || !g_pZombieVolumes[i]->HasSpawnFlags(SF_RESPAWN_ONLY_IF_DEAD))
			continue;

		if (g_pZombieVolumes[i]->m_vZombieList.FindAndRemove(index))
			break;
	}
}
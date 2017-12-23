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
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum ZombieClassTypes
{
	ZOMBIE_TYPE_WALKER = 0,
	ZOMBIE_TYPE_RUNNER,
	ZOMBIE_TYPE_RANDOM,
};

ConVar bb2_zombie_spawner_distance("bb2_zombie_spawner_distance", "5200", FCVAR_REPLICATED, "If there is no players within this distance from the spawner it will not spawn any zombies.", true, 200.0f, false, 0.0f);

bool IsAllowedToSpawn(CBaseEntity *pEntity)
{
	if (!pEntity)
		return false;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient)
			continue;

		if (pClient->Classify() != CLASS_PLAYER || !pClient->IsAlive() || pClient->IsObserver())
			continue;

		Vector vecDistance = pClient->GetAbsOrigin();
		//Msg("Dist %f\n", pEntity->GetAbsOrigin().DistTo(vecDistance));
		if ((pEntity->GetAbsOrigin().DistTo(vecDistance) < bb2_zombie_spawner_distance.GetFloat()) && pEntity->FVisible(pClient, MASK_VISIBLE))
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
DEFINE_KEYFIELD(ZombieSpawnNum, FIELD_INTEGER, "ZombieNumber"),
DEFINE_KEYFIELD(SpawnInterval, FIELD_FLOAT, "SpawnInterval"),
DEFINE_KEYFIELD(m_flRandomSpawnPercent, FIELD_FLOAT, "RandomSpawnChance"),
DEFINE_KEYFIELD(m_bSpawnNoMatterWhat, FIELD_BOOLEAN, "AutoSpawn"),

DEFINE_KEYFIELD(goalEntity, FIELD_STRING, "goal_target"),
DEFINE_KEYFIELD(goalActivity, FIELD_INTEGER, "goal_activity"),
DEFINE_KEYFIELD(goalType, FIELD_INTEGER, "goal_type"),

DEFINE_FIELD(m_flNextSpawnWave, FIELD_TIME),
DEFINE_FIELD(m_iSpawnNum, FIELD_INTEGER),

END_DATADESC()

LINK_ENTITY_TO_CLASS(zombie_volume, CZombieVolume);

CZombieVolume::CZombieVolume(void)
{
	SpawnInterval = 10.0f;
	m_flRandomSpawnPercent = 20.0f;
	ZombieSpawnNum = 5;
	m_iSpawnNum = 0;
	m_iTypeToSpawn = 0;
	m_flNextSpawnWave = 0.0f;
	m_bSpawnNoMatterWhat = false;
	m_spawnflags = 0;

	goalEntity = NULL_STRING;
	goalActivity = ACT_WALK;
	goalType = 0;
}

void CZombieVolume::Spawn()
{
	Precache();
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	SetModel(STRING(GetModelName()));
	m_nRenderMode = kRenderEnvironmental;

	if (ZombieSpawnNum > 50)
		ZombieSpawnNum = 50;
	else if (ZombieSpawnNum <= 0)
		ZombieSpawnNum = 5;

	if (HasSpawnFlags(SF_STARTACTIVE))
	{
		m_OnWaveSpawned.FireOutput(this, this);
		m_flNextSpawnWave = gpGlobals->curtime + SpawnInterval;
		m_iSpawnNum = 0;

		SetThink(&CZombieVolume::VolumeThink);
		SetNextThink(gpGlobals->curtime + bb2_spawn_frequency.GetFloat());
	}
}

void CZombieVolume::VolumeThink()
{
	if (m_flNextSpawnWave <= gpGlobals->curtime)
	{
		m_flNextSpawnWave = gpGlobals->curtime + SpawnInterval;
		m_OnWaveSpawned.FireOutput(this, this);
		m_iSpawnNum = 0;
	}

	if (m_bSpawnNoMatterWhat || IsAllowedToSpawn(this))
	{
		if ((HL2MPRules()->CanSpawnZombie()) && (m_iSpawnNum < ZombieSpawnNum))
			SpawnWave();
	}

	SetNextThink(gpGlobals->curtime + bb2_spawn_frequency.GetFloat());
}

void CZombieVolume::InputStartSpawn(inputdata_t &inputData)
{
	m_flNextSpawnWave = gpGlobals->curtime + SpawnInterval;
	m_iSpawnNum = 0;

	SetThink(&CZombieVolume::VolumeThink);
	SetNextThink(gpGlobals->curtime + bb2_spawn_frequency.GetFloat());

	m_OnForceSpawned.FireOutput(this, this);
}

void CZombieVolume::InputStopSpawn(inputdata_t &inputData)
{
	SetThink(NULL);
	m_OnForceStop.FireOutput(this, this);
}

void CZombieVolume::TraceZombieBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm, CBaseEntity *pEntity)
{
	Vector ZombieMins = Vector(-50, -50, 0);
	Vector ZombieMax = Vector(50, 50, 82);

	Ray_t ray;
	ray.Init(start, end, ZombieMins, ZombieMax);
	UTIL_TraceRay(ray, fMask, pEntity, collisionGroup, &pm);
}

void CZombieVolume::SpawnWave()
{
	// We wait until the round has begun.
	if (!HL2MPRules()->m_bRoundStarted)
		return;

	bool bCouldSpawn = true;

	float triggerWidth = CollisionProp()->OBBSize().x;
	float triggerHeight = CollisionProp()->OBBSize().y;

	float xPos = triggerWidth / 2.0f;
	float yPos = triggerHeight / 2.0f;

	// Start filling MULTIPOS
	Vector newPos;
	Vector vecVolumePos = GetAbsOrigin();
	int randomPlusOrMinus = random->RandomInt(1, 2);
	float RandomXPlus = random->RandomFloat(vecVolumePos.x, (vecVolumePos.x + xPos));
	float RandomYPlus = random->RandomFloat(vecVolumePos.y, (vecVolumePos.y + yPos));
	float RandomXMinus = random->RandomFloat((vecVolumePos.x - xPos), vecVolumePos.x);
	float RandomYMinus = random->RandomFloat((vecVolumePos.y - yPos), vecVolumePos.y);

	switch (randomPlusOrMinus)
	{
	case 1:
	{
		newPos = Vector(RandomXPlus, RandomYPlus, vecVolumePos.z);
		break;
	}
	case 2:
	{
		newPos = Vector(RandomXMinus, RandomYMinus, vecVolumePos.z);
		break;
	}
	default:
	{
		newPos = Vector(RandomXPlus, RandomYPlus, vecVolumePos.z);
		break;
	}
	}

	Vector vecDown;
	AngleVectors(GetAbsAngles(), NULL, NULL, &vecDown);
	VectorNormalize(vecDown);
	vecDown *= -1;

	trace_t tr;
	TraceZombieBBox(newPos, newPos + vecDown * MAX_TRACE_LENGTH, MASK_NPCSOLID, COLLISION_GROUP_NPC, tr, this);

	// We hit an entity which means there is already something in this part of the volume! Ignore and continue to next part of the volume!
	CBaseEntity *pEntity = tr.m_pEnt;
	if (pEntity)
	{
		if (!FClassnameIs(pEntity, "worldspawn"))
		{
			DevMsg(2, "A zombie couldn't spawn at %f %f %f!\n", tr.endpos.x, tr.endpos.y, tr.endpos.z);
			bCouldSpawn = false;
		}
	}

	if (tr.startsolid || tr.DidHitNonWorldEntity())
	{
		DevMsg(2, "A zombie couldn't spawn at %f %f %f!\n", tr.endpos.x, tr.endpos.y, tr.endpos.z);
		bCouldSpawn = false;
	}

	if (bCouldSpawn)
	{
		CAI_BaseNPC *npcZombie = (CAI_BaseNPC*)CreateEntityByName(GetZombieClassnameToSpawn());
		if (npcZombie)
		{
			QAngle randomAngle = QAngle(0, random->RandomFloat(-180.0f, 180.0f), 0);
			npcZombie->SetAbsOrigin(tr.endpos);
			npcZombie->SetAbsAngles(randomAngle);
			if (HasSpawnFlags(SF_FASTSPAWN))
				npcZombie->SpawnDirectly(); // Skip zombie fade-in + rise stuff, use fast spawn for npcs which spawn in a hidden place, will make them spawn faster + more efficient.

			npcZombie->Spawn();
			//UTIL_DropToFloor(npcZombie, MASK_NPCSOLID);

			if (goalEntity != NULL_STRING)
			{
				CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, STRING(goalEntity));
				if (!pTarget)
					pTarget = gEntList.FindEntityByClassname(NULL, STRING(goalEntity));

				if (pTarget)
					npcZombie->SpawnRunSchedule(pTarget, ((Activity)goalActivity), (goalType >= 1));
			}
		}

		m_iSpawnNum++;
	}
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
		float percent = random->RandomFloat(0, 100);
		if (m_flRandomSpawnPercent >= percent && !GameBaseServer()->IsClassicMode())
			return (pszTypes[random->RandomInt(0, (_ARRAYSIZE(pszTypes) - 1))]);

		return "npc_walker";
	}
	default:
		return "npc_walker";
	}
}
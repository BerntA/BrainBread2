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
#include "triggers.h"
#include "props.h"
#include "saverestore_utlvector.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "hl2_player.h"
#include "hl2mp_player_shared.h"
#include "GameBase_Server.h"
#include "items.h"
#include "player.h"

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
DEFINE_THINKFUNC(Think),

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
}

void CZombieVolume::Spawn()
{
	BaseClass::Spawn();

	if (ZombieSpawnNum > 50)
		ZombieSpawnNum = 50;
	else if (ZombieSpawnNum <= 0)
		ZombieSpawnNum = 5;

	if (HasSpawnFlags(SF_STARTACTIVE))
	{
		m_OnWaveSpawned.FireOutput(this, this);
		m_flNextSpawnWave = gpGlobals->curtime + SpawnInterval;
		m_iSpawnNum = 0;

		SetThink(&CZombieVolume::Think);
		SetNextThink(gpGlobals->curtime + bb2_spawn_frequency.GetFloat());
	}
}

void CZombieVolume::Think()
{
	if (m_flNextSpawnWave <= gpGlobals->curtime)
	{
		m_flNextSpawnWave = gpGlobals->curtime + SpawnInterval;
		m_OnWaveSpawned.FireOutput(this, this);
		m_iSpawnNum = 0;
	}

	bool bAllowedToSpawn = IsAllowedToSpawn(this);
	if (m_bSpawnNoMatterWhat)
		bAllowedToSpawn = true;

	if (bAllowedToSpawn)
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

	SetThink(&CZombieVolume::Think);
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

	float xPos = triggerWidth / 2;
	float yPos = triggerHeight / 2;

	// Start filling MULTIPOS
	Vector newPos;
	int randomPlusOrMinus = random->RandomInt(1, 2);
	float RandomXPlus = random->RandomFloat(GetAbsOrigin().x, (GetAbsOrigin().x + xPos));
	float RandomYPlus = random->RandomFloat(GetAbsOrigin().y, (GetAbsOrigin().y + yPos));
	float RandomXMinus = random->RandomFloat((GetAbsOrigin().x - xPos), GetAbsOrigin().x);
	float RandomYMinus = random->RandomFloat((GetAbsOrigin().y - yPos), GetAbsOrigin().y);

	switch (randomPlusOrMinus)
	{
	case 1:
	{
		newPos = Vector(RandomXPlus, RandomYPlus, GetAbsOrigin().z);
		break;
	}
	case 2:
	{
		newPos = Vector(RandomXMinus, RandomYMinus, GetAbsOrigin().z);
		break;
	}
	default:
	{
		newPos = Vector(RandomXPlus, RandomYPlus, GetAbsOrigin().z);
		break;
	}
	}

	Vector vecAbsStart, vecAbsEnd, vecDown;
	AngleVectors(GetAbsAngles(), NULL, NULL, &vecDown);

	vecAbsStart = newPos;
	vecAbsEnd = vecAbsStart - (vecDown * MAX_TRACE_LENGTH);
	trace_t tr;
	TraceZombieBBox(vecAbsStart, vecAbsEnd, MASK_NPCSOLID, COLLISION_GROUP_NPC, tr, this);

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
		CBaseEntity *npcZombie = CreateEntityByName(GetZombieClassnameToSpawn());
		if (npcZombie)
		{
			npcZombie->SetAbsOrigin(tr.endpos);
			npcZombie->SetAbsAngles(GetAbsAngles());
			npcZombie->Spawn();
			//UTIL_DropToFloor(npcZombie, MASK_NPCSOLID);
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
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: 2 Zombie Spawner : Based on the origin around the center of the volume/trigger. The volume makes no difference in spawn pos/area etc...
// Will be replaced with a volume related one + zombie limits...
// 2014. Added zombie limits. However zombie limits are not changed if using ent_remove.. Only if dead it will increase limit. And on spawn decrease. Max 80 or less...
// Changelog : April 08, fixed it to now spawn random zombies in its volume, a tracer checks bounding box if it is possible to spawn the zombie!
// it also asks for permission to spawn each zombie, and it respects the new zombie limit defined in the bb2_zombie*** convar.
//
//========================================================================================//

#ifndef ZOMBIE_VOLUME_H
#define ZOMBIE_VOLUME_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

//
// Spawnflags
//
enum
{
	SF_STARTACTIVE = 0x01, // Start active
	SF_FASTSPAWN = 0x02, // Skip fade phase
	SF_NOVISCHECK = 0x04, // Don't care about FVisible, spawn regardless of PVS / vis state
	SF_RESPAWN_ONLY_IF_DEAD = 0x08, // Don't spawn zombies blindly, only respawn the ones who died, keep track of live zombies.
};

class CZombieVolume : public CBaseEntity
{
public:

	DECLARE_CLASS(CZombieVolume, CBaseEntity);
	DECLARE_DATADESC();

	CZombieVolume();
	virtual ~CZombieVolume();

	void Spawn(void);
	void VolumeThink();

	void InputStartSpawn(inputdata_t &inputData);
	void InputStopSpawn(inputdata_t &inputData);

	static void OnZombieRemoved(int index);

private:

	// Hammer Variables...
	int m_iZombiesMin;
	int m_iZombiesMax;
	int m_iZombiesToSpawn;

	float m_flMaxDistance;
	float m_flMaxZDifference;
	float m_flSpawnInterval;
	float m_flRandomSpawnPercent;

	int m_iTypeToSpawn;

	// Current Spawned Ents:
	int m_iSpawnNum;
	bool m_bSpawnNoMatterWhat;

	// Timer
	float m_flNextSpawnWave;

	// Schedule logic
	string_t goalEntity;
	int goalActivity;
	int goalType;
	int goalInterruptType;

	COutputEvent m_OnWaveSpawned;
	COutputEvent m_OnForceSpawned;
	COutputEvent m_OnForceStop;

protected:

	float GetSpawnFrequency(void);
	void TraceZombieBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm, CBaseEntity *pEntity);
	void SpawnZombie();
	const char *GetZombieClassnameToSpawn();

	CUtlVector<int> m_vZombieList;
};

#endif // ZOMBIE_VOLUME_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: Boss Spawn Entity : Will spawn the actual entity out from random points (set classname link for both spawner and spawn points). Chooses one out of the X set points. If none is present he will spawn in his entity's origin (the spawner).
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "triggers.h"
#include "props.h"
#include "saverestore_utlvector.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "GameEventListener.h"

CUtlVector<EHANDLE> bossSpawnPoints;

class CBossSpawnPoint : public CLogicalEntity
{
public:
	DECLARE_CLASS(CBossSpawnPoint, CLogicalEntity);
	DECLARE_DATADESC();

	CBossSpawnPoint()
	{
	}

	~CBossSpawnPoint()
	{
		if (bossSpawnPoints.Find(this) != -1)
			bossSpawnPoints.FindAndRemove(this);
	}

	void Spawn()
	{
		BaseClass::Spawn();

		if (pszClassnameLink == NULL_STRING)
		{
			Warning("info_boss_point '%s' has no classname link!\nRemoving!\n", STRING(GetEntityName()));
			UTIL_Remove(this);
			return;
		}

		if (bossSpawnPoints.Find(this) == -1)
			bossSpawnPoints.AddToTail(this);
	}

	COutputEvent m_OnSpawnedInPoint;

	const char *GetClassnameLink(void)
	{
		return STRING(pszClassnameLink);
	}

private:
	string_t pszClassnameLink;
};

BEGIN_DATADESC(CBossSpawnPoint)
DEFINE_KEYFIELD(pszClassnameLink, FIELD_STRING, "ClassnameLink"),
DEFINE_OUTPUT(m_OnSpawnedInPoint, "OnSpawn"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(info_boss_point, CBossSpawnPoint);

CBaseEntity *GetBossSpawnPointEntity(const char *classnameLink)
{
	int maxIndex = (bossSpawnPoints.Count() - 1);
	bool bDoesExist = false;

	// Don't perform the while loop unless this link + ent exist:
	for (int i = 0; i < bossSpawnPoints.Count(); i++)
	{
		CBaseEntity *pEntity = bossSpawnPoints[i].Get();
		if (pEntity)
		{
			CBossSpawnPoint *pBossPoint = dynamic_cast<CBossSpawnPoint*> (pEntity);
			if (pBossPoint && (!strcmp(pBossPoint->GetClassnameLink(), classnameLink)))
				bDoesExist = true;
		}
	}

	CBaseEntity *pPoint = NULL;
	if (bDoesExist)
	{
		while (pPoint == NULL)
		{
			int index = random->RandomInt(0, maxIndex);
			CBaseEntity *pEntity = bossSpawnPoints[index].Get();
			if (pEntity)
			{
				CBossSpawnPoint *pBossPoint = dynamic_cast<CBossSpawnPoint*> (pEntity);
				if (pBossPoint && (!strcmp(pBossPoint->GetClassnameLink(), classnameLink)))
					pPoint = pEntity;
			}
		}
	}

	return pPoint;
}

class CBossSpawner : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_CLASS(CBossSpawner, CLogicalEntity);
	DECLARE_DATADESC();

	CBossSpawner();
	~CBossSpawner();

	void Spawn();
	void InputSpawn(inputdata_t &inputData);
	void FireGameEvent(IGameEvent *event);

	COutputEvent m_OnDeath;

private:

	EHANDLE m_pActiveBossEnt;
	string_t pszClassnameLink;
};

BEGIN_DATADESC(CBossSpawner)
DEFINE_KEYFIELD(pszClassnameLink, FIELD_STRING, "ClassnameLink"),
DEFINE_INPUTFUNC(FIELD_VOID, "SpawnBoss", InputSpawn),
DEFINE_OUTPUT(m_OnDeath, "OnDeath"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(logic_boss_spawner, CBossSpawner);

CBossSpawner::CBossSpawner()
{
	m_pActiveBossEnt = NULL;
	ListenForGameEvent("entity_killed");
}

CBossSpawner::~CBossSpawner()
{
	CBaseEntity *pEnt = m_pActiveBossEnt.Get();
	if (pEnt)
	{
		UTIL_Remove(pEnt);
		m_pActiveBossEnt = NULL;
	}
}

void CBossSpawner::Spawn()
{
	BaseClass::Spawn();

	if (pszClassnameLink == NULL_STRING)
	{
		Warning("logic_boss_spawner '%s' has no classname link!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
	}
}

void CBossSpawner::InputSpawn(inputdata_t &inputData)
{
	// If we've already spawned some boss then replace it this new one: (we only monitor one at a time per boss spawn ent)
	CBaseEntity *pActiveBoss = m_pActiveBossEnt.Get();
	if (pActiveBoss)
	{
		UTIL_Remove(pActiveBoss);
		m_pActiveBossEnt = NULL;
	}

	const char *classnameToSpawn = STRING(pszClassnameLink);
	CBaseEntity *pNewBossSpawnEnt = GetBossSpawnPointEntity(classnameToSpawn);

	// No point available? Spawn at this spawners origin then...
	if (!pNewBossSpawnEnt)
	{
		Vector vecOrigin = GetAbsOrigin();
		Warning("No info_boss_point were found in the map!\nSpawning boss %s at %s pos : %f %f %f!\n", classnameToSpawn, STRING(GetEntityName()), vecOrigin.x, vecOrigin.y, vecOrigin.z);
		CBaseEntity *pNewEntity = Create(classnameToSpawn, GetAbsOrigin(), GetAbsAngles(), NULL);
		if (pNewEntity)
			m_pActiveBossEnt = pNewEntity;

		return;
	}

	// Create our boss:
	CBaseEntity *pNewEntity = Create(classnameToSpawn, pNewBossSpawnEnt->GetAbsOrigin(), pNewBossSpawnEnt->GetAbsAngles(), NULL);
	if (pNewEntity)
		m_pActiveBossEnt = pNewEntity;

	// Announce the spawn...
	CBossSpawnPoint *pBossPoint = dynamic_cast<CBossSpawnPoint*> (pNewBossSpawnEnt);
	if (pBossPoint)
		pBossPoint->m_OnSpawnedInPoint.FireOutput(this, this);
}

void CBossSpawner::FireGameEvent(IGameEvent *event)
{
	const char * type = event->GetName();

	if (!strcmp(type, "entity_killed"))
	{
		CBaseEntity *pVictim = UTIL_EntityByIndex(event->GetInt("entindex_killed", 0));
		if (!pVictim)
			return;

		CBaseEntity *pActiveBoss = m_pActiveBossEnt.Get();
		if (pActiveBoss)
		{
			if (pActiveBoss->entindex() == pVictim->entindex())
			{
				m_OnDeath.FireOutput(this, this);
				m_pActiveBossEnt = NULL;
			}
		}
	}
}
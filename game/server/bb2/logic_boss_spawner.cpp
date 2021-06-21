//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: Boss Spawn Entity : Will spawn the actual entity out from random points (set classname link for both spawner and spawn points). Chooses one out of the X set points. If none is present he will spawn in his entity's origin (the spawner).
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "triggers.h"
#include "props.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "GameEventListener.h"

static CUtlVector<EHANDLE> bossSpawnPoints;

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

static CBaseEntity *GetBossSpawnPointEntity(const char *classnameLink)
{
	CUtlVector<CBaseEntity*> pSpawnPoints;
	for (int i = 0; i < bossSpawnPoints.Count(); i++)
	{
		CBaseEntity *pEntity = bossSpawnPoints[i].Get();
		if (!pEntity)
			continue;

		CBossSpawnPoint *pBossPoint = dynamic_cast<CBossSpawnPoint*> (pEntity);
		if (pBossPoint && !strcmp(pBossPoint->GetClassnameLink(), classnameLink))
			pSpawnPoints.AddToTail(pEntity);
	}

	if (pSpawnPoints.Count() == 0)
		return NULL;

	CBaseEntity *pPoint = pSpawnPoints[random->RandomInt(0, (pSpawnPoints.Count() - 1))];
	pSpawnPoints.RemoveAll();
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
		m_pActiveBossEnt = Create(classnameToSpawn, GetAbsOrigin(), GetAbsAngles(), NULL);
		return;
	}

	// Create our boss:
	m_pActiveBossEnt = Create(classnameToSpawn, pNewBossSpawnEnt->GetAbsOrigin(), pNewBossSpawnEnt->GetAbsAngles(), NULL);

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
		if (pActiveBoss && (pActiveBoss->entindex() == pVictim->entindex()))
		{
			m_OnDeath.FireOutput(this, this);
			m_pActiveBossEnt = NULL;
		}
	}
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base class for respawnable items.
//
//========================================================================================//

#ifndef SPAWNABLE_ENTITY_H
#define SPAWNABLE_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "baseentity.h"

class CSpawnableEntity : public CLogicalEntity
{
	DECLARE_CLASS(CSpawnableEntity, CLogicalEntity);
	DECLARE_DATADESC();

public:
	CSpawnableEntity();
	virtual ~CSpawnableEntity();

	virtual void Spawn();
	virtual void SpawnEntity();

	void CheckSpawnInterval(void);

	void InputForceSpawn(inputdata_t &data);

	void InputEnableEntity(inputdata_t &data);
	void InputDisableEntity(inputdata_t &data);

	void InputSetMaxRespawnDelay(inputdata_t &data);
	void InputSetMinRespawnDelay(inputdata_t &data);

protected:
	void OnEntityCheck(void);

	virtual bool ShouldRespawnEntity(CBaseEntity *pActiveEntity);
	virtual CBaseEntity *SpawnNewEntity(void) { return NULL; }

	EHANDLE m_pEntityToSpawn;

	bool m_bDisabled;
	bool m_bShouldCreate;
	float m_flMinRespawnDelay;
	float m_flMaxRespawnDelay;

	COutputEvent m_OnSpawnEntity;
};

#endif // SPAWNABLE_ENTITY_H
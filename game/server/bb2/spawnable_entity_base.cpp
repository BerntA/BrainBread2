//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base class for respawnable items.
//
//========================================================================================//

#include "cbase.h"
#include "spawnable_entity_base.h"
#include "inventory_item.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "objective_icon.h"
#include "basecombatweapon_shared.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CSpawnableEntity)
DEFINE_FIELD(m_bShouldCreate, FIELD_BOOLEAN),

DEFINE_KEYFIELD(m_flMinRespawnDelay, FIELD_FLOAT, "MinRespawnDelay"),
DEFINE_KEYFIELD(m_flMaxRespawnDelay, FIELD_FLOAT, "MaxRespawnDelay"),
DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),

DEFINE_INPUTFUNC(FIELD_VOID, "ForceSpawn", InputForceSpawn),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnableEntity),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisableEntity),

DEFINE_INPUTFUNC(FIELD_FLOAT, "SetMaxRespawnDelay", InputSetMaxRespawnDelay),
DEFINE_INPUTFUNC(FIELD_FLOAT, "SetMinRespawnDelay", InputSetMinRespawnDelay),

DEFINE_OUTPUT(m_OnSpawnEntity, "OnSpawn"),

DEFINE_THINKFUNC(OnEntityCheck),
END_DATADESC()

LINK_ENTITY_TO_CLASS(entity_spawner_base, CSpawnableEntity);

CSpawnableEntity::CSpawnableEntity()
{
	m_pEntityToSpawn = NULL;
	m_bDisabled = false;
	m_bShouldCreate = false;
	m_flMinRespawnDelay = 0.0f;
	m_flMaxRespawnDelay = 10.0f;
}

CSpawnableEntity::~CSpawnableEntity()
{
	CBaseEntity *pEntity = m_pEntityToSpawn.Get();
	if (pEntity)
	{
		UTIL_Remove(pEntity);
		m_pEntityToSpawn = NULL;
	}
}

void CSpawnableEntity::Spawn()
{
	BaseClass::Spawn();

	CheckSpawnInterval();

	if (!m_bDisabled)
		SpawnEntity();

	// Start thinking soon...
	SetThink(&CSpawnableEntity::OnEntityCheck);
	SetNextThink(gpGlobals->curtime + 1.0f);
}

void CSpawnableEntity::SpawnEntity()
{
	m_pEntityToSpawn = SpawnNewEntity();
	if (m_pEntityToSpawn != NULL)
	{
		m_OnSpawnEntity.FireOutput(this, this);

		// Respawn FX.
		if (!m_pEntityToSpawn->IsNPC())
			DispatchParticleEffect("bb2_item_spawn", GetAbsOrigin(), GetAbsAngles());
	}
}

void CSpawnableEntity::CheckSpawnInterval(void)
{
	if (m_flMinRespawnDelay < 0)
		m_flMinRespawnDelay = 0.0f;

	if (m_flMinRespawnDelay > m_flMaxRespawnDelay)
		m_flMinRespawnDelay = 0.0f;

	if (m_flMaxRespawnDelay < 5)
		m_flMaxRespawnDelay = 5.0f;
}

void CSpawnableEntity::OnEntityCheck(void)
{
	float flNextThink = 0.4f;

	if (m_bShouldCreate)
	{
		m_bShouldCreate = false;
		SpawnEntity();
	}

	if (ShouldRespawnEntity(m_pEntityToSpawn.Get()) && !m_bShouldCreate && !m_bDisabled)
	{
		m_pEntityToSpawn = NULL;
		m_bShouldCreate = true;
		flNextThink = random->RandomFloat(m_flMinRespawnDelay, m_flMaxRespawnDelay);
	}

	SetNextThink(gpGlobals->curtime + flNextThink);
}

bool CSpawnableEntity::ShouldRespawnEntity(CBaseEntity *pActiveEntity)
{
	if (!pActiveEntity)
		return true;

	return false;
}

void CSpawnableEntity::InputForceSpawn(inputdata_t &data)
{
	SetThink(NULL);

	CBaseEntity *pEntity = m_pEntityToSpawn.Get();
	if (pEntity)
	{
		UTIL_Remove(pEntity);
		m_pEntityToSpawn = NULL;
	}

	m_bDisabled = false;
	m_bShouldCreate = false;

	SpawnEntity();

	// Start thinking soon...
	SetThink(&CSpawnableEntity::OnEntityCheck);
	SetNextThink(gpGlobals->curtime + 1.0f);
}

void CSpawnableEntity::InputEnableEntity(inputdata_t &data)
{
	if (!m_bDisabled)
		return;

	m_bDisabled = false;
	SetNextThink(gpGlobals->curtime + 0.25f);
	SpawnEntity();
}

void CSpawnableEntity::InputDisableEntity(inputdata_t &data)
{
	if (m_bDisabled)
		return;

	m_bDisabled = true;
	SetNextThink(gpGlobals->curtime + 0.25f);
}

void CSpawnableEntity::InputSetMaxRespawnDelay(inputdata_t &data)
{
	m_flMaxRespawnDelay = data.value.Float();
	CheckSpawnInterval();
}

void CSpawnableEntity::InputSetMinRespawnDelay(inputdata_t &data)
{
	m_flMinRespawnDelay = data.value.Float();
	CheckSpawnInterval();
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Auto Spawn NPC, only one npc will be alive at once.
//
//========================================================================================//

#include "cbase.h"
#include "spawnable_entity_base.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "basecombatweapon_shared.h"
#include "npc_combine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int GetCollisionGroupForClassname(const char *classname)
{
	if (classname && classname[0])
	{
		if (!strcmp(classname, "npc_walker") || !strcmp(classname, "npc_runner"))
			return COLLISION_GROUP_NPC_ZOMBIE;
		else if (!strcmp(classname, "npc_fred"))
			return COLLISION_GROUP_NPC_ZOMBIE_BOSS;
		else if (!strcmp(classname, "npc_military") || !strcmp(classname, "npc_military_static"))
			return COLLISION_GROUP_NPC_MILITARY;
		else if (!strcmp(classname, "npc_bandit"))
			return COLLISION_GROUP_NPC_MERCENARY;
	}

	return COLLISION_GROUP_NPC;
}

class CNPCAutoSpawner : public CSpawnableEntity
{
public:
	DECLARE_CLASS(CNPCAutoSpawner, CSpawnableEntity);
	DECLARE_DATADESC();

	CNPCAutoSpawner();
	void Spawn();

protected:
	CBaseEntity *SpawnNewEntity(void);
	void OnEntityCheck(void);

private:
	string_t szClassname;
	string_t szWeapon;
	bool m_bCanDropWeapons;
};

BEGIN_DATADESC(CNPCAutoSpawner)
DEFINE_KEYFIELD(szClassname, FIELD_STRING, "npc_classname"),
DEFINE_KEYFIELD(szWeapon, FIELD_STRING, "weapon_classname"),
DEFINE_KEYFIELD(m_bCanDropWeapons, FIELD_BOOLEAN, "allow_weapondrop"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_auto_spawner, CNPCAutoSpawner);

CNPCAutoSpawner::CNPCAutoSpawner()
{
	szClassname = NULL_STRING;
	szWeapon = NULL_STRING;
}

void CNPCAutoSpawner::Spawn()
{
	BaseClass::Spawn();

	if (szClassname == NULL_STRING)
	{
		Warning("npc_auto_spawner '%s' has no classname set!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
	}
}

CBaseEntity *CNPCAutoSpawner::SpawnNewEntity(void)
{
	CBaseEntity *pEntity = CreateEntityByName(STRING(szClassname));
	if (!pEntity)
		return NULL;

	pEntity->SetAbsOrigin(GetAbsOrigin());
	pEntity->SetAbsAngles(GetAbsAngles());
	pEntity->Spawn();
	UTIL_DropToFloor(pEntity, MASK_SHOT, this);

	CAI_BaseActor *pSoldier = dynamic_cast<CAI_BaseActor*> (pEntity);
	if (pSoldier)
	{
		if (szWeapon != NULL_STRING)
			pSoldier->GiveWeapon(szWeapon);

		if (!m_bCanDropWeapons)
			pSoldier->AddSpawnFlags(SF_NPC_NO_WEAPON_DROP);
	}

	return pEntity;
}

void CNPCAutoSpawner::OnEntityCheck(void)
{
	float flNextThink = 0.4f;

	if (m_bShouldCreate)
	{
		bool bCanSpawnEnt = true;

		trace_t tr;
		CTraceFilterOnlyNPCsAndPlayer filter(NULL, GetCollisionGroupForClassname(STRING(szClassname)));

		Vector mins = Vector(-60, -60, 0);
		Vector maxs = Vector(60, 60, 80);

		UTIL_TraceHull(GetAbsOrigin(), GetAbsOrigin(), mins, maxs, MASK_NPCSOLID, &filter, &tr);
		if (tr.fraction != 1.0f)
		{
			bCanSpawnEnt = false;
			flNextThink = 1.25f;
		}

		if (bCanSpawnEnt)
		{
			m_bShouldCreate = false;
			SpawnEntity();
		}
	}

	if (ShouldRespawnEntity(m_pEntityToSpawn.Get()) && !m_bShouldCreate && !m_bDisabled)
	{
		m_pEntityToSpawn = NULL;
		m_bShouldCreate = true;
		flNextThink = random->RandomFloat(m_flMinRespawnDelay, m_flMaxRespawnDelay);
	}

	SetNextThink(gpGlobals->curtime + flNextThink);
}
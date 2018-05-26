//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Randomized powerup spawner.
//
//========================================================================================//

#include "cbase.h"
#include "spawnable_entity_base.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "inventory_item.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *pchPowerups[] =
{
	"Critical",
	"Cheetah",
	"Predator",
	"Painkiller",
	"Nanites",
};

class CItemPowerupRandomizer : public CSpawnableEntity
{
public:
	DECLARE_CLASS(CItemPowerupRandomizer, CSpawnableEntity);
	DECLARE_DATADESC();

	CItemPowerupRandomizer();

protected:
	CBaseEntity *SpawnNewEntity(void);
};

BEGIN_DATADESC(CItemPowerupRandomizer)
END_DATADESC()

LINK_ENTITY_TO_CLASS(item_powerup_randomizer, CItemPowerupRandomizer);

CItemPowerupRandomizer::CItemPowerupRandomizer()
{
}

CBaseEntity *CItemPowerupRandomizer::SpawnNewEntity(void)
{
	const char *powerup = pchPowerups[random->RandomInt(0, (_ARRAYSIZE(pchPowerups) - 1))];

	CItem *pPowerupEnt = (CItem*)CreateEntityByName("item_powerup");
	if (pPowerupEnt)
	{
		Vector vecOrigin = GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;
		pPowerupEnt->SetParam(powerup);
		pPowerupEnt->SetAbsOrigin(vecOrigin);
		pPowerupEnt->SetAbsAngles(QAngle(0, 0, 0));
		pPowerupEnt->Spawn();
		pPowerupEnt->EnableRotationEffect();
		UTIL_DropToFloor(pPowerupEnt, MASK_SOLID_BRUSHONLY, this);
	}

	return pPowerupEnt;
}
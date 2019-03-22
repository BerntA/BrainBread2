//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Randomized ammo spawner.
//
//========================================================================================//

#include "cbase.h"
#include "spawnable_entity_base.h"
#include "inventory_item.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *pszAmmoClassnames[] =
{
	"ammo_pistol",
	"ammo_rifle",
	"ammo_slugs",
	"ammo_revolver",
	"ammo_smg",
	"ammo_sniper",
	"ammo_trapper",
};

class CItemAmmoRandomizer : public CSpawnableEntity
{
public:
	DECLARE_CLASS(CItemAmmoRandomizer, CSpawnableEntity);
	DECLARE_DATADESC();

	CItemAmmoRandomizer();

protected:
	CBaseEntity *SpawnNewEntity(void);
};

BEGIN_DATADESC(CItemAmmoRandomizer)
END_DATADESC()

LINK_ENTITY_TO_CLASS(item_ammo_randomizer, CItemAmmoRandomizer);

CItemAmmoRandomizer::CItemAmmoRandomizer()
{
}

CBaseEntity *CItemAmmoRandomizer::SpawnNewEntity(void)
{
	const char *pszClassname = pszAmmoClassnames[random->RandomInt(0, (_ARRAYSIZE(pszAmmoClassnames) - 1))];
	if (!pszClassname || !pszClassname[0])
		return NULL;

	CItem *pAmmoEntity = (CItem*)CreateEntityByName(pszClassname);
	if (pAmmoEntity)
	{
		Vector vecOrigin = GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

		pAmmoEntity->SetAbsOrigin(vecOrigin);
		pAmmoEntity->SetAbsAngles(QAngle(0, 0, 0));
		pAmmoEntity->Spawn();
		pAmmoEntity->EnableRotationEffect();
		pAmmoEntity->AddSpawnFlags(SF_NORESPAWN);
		UTIL_DropToFloor(pAmmoEntity, MASK_SOLID_BRUSHONLY, this);
	}

	return pAmmoEntity;
}
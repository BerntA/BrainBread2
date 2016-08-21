//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Spawns a random consumable inventory item within its origin. When it gets picked up a new item will spawn within the min max delay field.
//
//========================================================================================//

#include "cbase.h"
#include "spawnable_entity_base.h"
#include "inventory_item.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "objective_icon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CInventoryItemRandomizer : public CSpawnableEntity
{
public:
	DECLARE_CLASS(CInventoryItemRandomizer, CSpawnableEntity);
	DECLARE_DATADESC();

	CInventoryItemRandomizer();

protected:
	CBaseEntity *SpawnNewEntity(void);

private:
	uint m_iOverrideID;
};

BEGIN_DATADESC(CInventoryItemRandomizer)
DEFINE_KEYFIELD(m_iOverrideID, FIELD_INTEGER, "OverrideID"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(inventory_item_randomizer, CInventoryItemRandomizer);

CInventoryItemRandomizer::CInventoryItemRandomizer()
{
	m_iOverrideID = 0;
}

CBaseEntity *CInventoryItemRandomizer::SpawnNewEntity(void)
{
	int itemIndex = -1;

	if (!m_iOverrideID)
	{
		bool bCanLoopThrough = false;

		for (int i = 0; i < GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList().Count(); i++)
		{
			if (GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i].bAutoConsume && !GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i].bIsMapItem)
			{
				bCanLoopThrough = true;
				break;
			}
		}

		if (!bCanLoopThrough)
			return NULL;

		while (itemIndex == -1)
		{
			int index = random->RandomInt(0, (GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList().Count() - 1));
			bool bAutoConsume = GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[index].bAutoConsume;
			bool bMapItemCheck = GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[index].bIsMapItem;
			if (bAutoConsume && !bMapItemCheck)
				itemIndex = index;
		}
	}
	else
	{
		for (int i = 0; i < GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList().Count(); i++)
		{
			if (GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i].bAutoConsume && (GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i].iItemID == m_iOverrideID)
				&& (!GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i].bIsMapItem))
			{
				itemIndex = i;
				break;
			}
		}

		if (itemIndex == -1)
			return NULL;
	}

	int iItemID = GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[itemIndex].iItemID;
	CInventoryItem *pItem = (CInventoryItem *)CreateEntityByName("inventory_item");
	if (pItem)
	{
		Vector vecOrigin = GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

		pItem->SetAbsOrigin(vecOrigin);
		pItem->SetAbsAngles(QAngle(0, 0, 0));
		pItem->SetItem(GameBaseShared()->GetSharedGameDetails()->GetInventoryItemModel(iItemID, false), iItemID, NULL, false);
		pItem->Spawn();
		UTIL_DropToFloor(pItem, MASK_SOLID_BRUSHONLY, this);
	}

	return pItem;
}
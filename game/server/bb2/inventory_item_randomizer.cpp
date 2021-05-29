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
	int itemCount = GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList().Count();
	if (itemCount <= 0)
		return NULL;

	if (!m_iOverrideID)
	{
		CUtlVector<int> indexList;
		for (int i = 0; i < itemCount; i++)
		{
			const DataInventoryItem_Base_t *data = &GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i];
			if (data->bAutoConsume && !data->bIsMapItem)
				indexList.AddToTail(i);
		}

		if (indexList.Count() == 0)
			return NULL;

		itemIndex = indexList[random->RandomInt(0, (indexList.Count() - 1))];
	}
	else
	{
		for (int i = 0; i < itemCount; i++)
		{
			const DataInventoryItem_Base_t *data = &GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[i];
			if (data->bAutoConsume && (data->iItemID == m_iOverrideID) && !data->bIsMapItem)
			{
				itemIndex = i;
				break;
			}
		}

		if (itemIndex == -1)
			return NULL;
	}

	Assert(itemIndex >= 0 && itemIndex < itemCount);
	const DataInventoryItem_Base_t *data = &GameBaseShared()->GetSharedGameDetails()->GetInventoryItemList()[itemIndex];
	CInventoryItem *pItem = (CInventoryItem *)CreateEntityByName("inventory_item");
	if (pItem)
	{
		Vector vecOrigin = GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

		pItem->SetAbsOrigin(vecOrigin);
		pItem->SetItem((*data), false);
		pItem->Spawn();
		UTIL_DropToFloor(pItem, MASK_SOLID_BRUSHONLY, this);
	}

	return pItem;
}
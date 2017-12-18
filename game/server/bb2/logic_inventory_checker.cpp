//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Check if the user who activated us has a certain inventory item.
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"

class CLogicInventoryChecker : public CLogicalEntity
{
public:

	DECLARE_CLASS(CLogicInventoryChecker, CLogicalEntity);
	DECLARE_DATADESC();

	CLogicInventoryChecker();
	void Spawn();
	void InputCheckForItem(inputdata_t &inData);

private:

	int m_iAction;
	int m_iFilter;
	uint m_iItemID;
	bool m_bIsMapItem;

	COutputEvent m_OnFoundItem;
	COutputEvent m_OnNotFoundItem;
};

LINK_ENTITY_TO_CLASS(logic_inventory_check, CLogicInventoryChecker);

BEGIN_DATADESC(CLogicInventoryChecker)
DEFINE_KEYFIELD(m_iAction, FIELD_INTEGER, "Action"),
DEFINE_KEYFIELD(m_iFilter, FIELD_INTEGER, "Team"),
DEFINE_KEYFIELD(m_iItemID, FIELD_INTEGER, "ItemID"),
DEFINE_KEYFIELD(m_bIsMapItem, FIELD_BOOLEAN, "MapItem"),

DEFINE_INPUTFUNC(FIELD_VOID, "CheckForItem", InputCheckForItem),
DEFINE_OUTPUT(m_OnFoundItem, "OnFound"),
DEFINE_OUTPUT(m_OnNotFoundItem, "OnNotFound"),
END_DATADESC()

CLogicInventoryChecker::CLogicInventoryChecker()
{
	m_iAction = 0;
	m_iFilter = 0;
	m_iItemID = 0;
	m_bIsMapItem = false;
}

void CLogicInventoryChecker::Spawn()
{
	if (!m_iItemID)
	{
		Warning("logic_inventory_check '%s' with invalid ItemID '%u'!\nRemoving!\n", STRING(GetEntityName()), m_iItemID);
		UTIL_Remove(this);
		return;
	}

	BaseClass::Spawn();
}

void CLogicInventoryChecker::InputCheckForItem(inputdata_t &inData)
{
	CHL2MP_Player *pActivator = ToHL2MPPlayer(inData.pActivator);
	if (!pActivator)
		return;

	if ((m_iFilter > 0) && (pActivator->GetTeamNumber() != m_iFilter))
		return;

	bool bFound = false;
	int itemIndex = GameBaseShared()->GetInventoryItemForPlayer(pActivator->entindex(), m_iItemID, m_bIsMapItem);
	if ((itemIndex != -1) && m_iItemID)
	{
		for (int i = (GameBaseShared()->GetServerInventory().Count() - 1); i >= 0; i--)
		{
			if (GameBaseShared()->GetServerInventory()[i].m_iPlayerIndex != pActivator->entindex())
				continue;

			if (GameBaseShared()->GetServerInventory()[i].m_iItemID != m_iItemID)
				continue;

			if (GameBaseShared()->GetServerInventory()[i].bIsMapItem != m_bIsMapItem)
				continue;

			// Auto-Use item.
			if (m_iAction == 1)
				GameBaseShared()->UseInventoryItem(pActivator->entindex(), m_iItemID, m_bIsMapItem, false, false, i);
			else if (m_iAction == 2) // Auto-Drop Item.
				GameBaseShared()->RemoveInventoryItem(pActivator->entindex(), pActivator->GetAbsOrigin(), (m_bIsMapItem ? 1 : 0), m_iItemID, false, i);
			else if (m_iAction == 3) // Delete item.
				GameBaseShared()->RemoveInventoryItem(pActivator->entindex(), pActivator->GetAbsOrigin(), (m_bIsMapItem ? 1 : 0), m_iItemID, true, i);

			bFound = true;
		}
	}

	if (bFound)
		m_OnFoundItem.FireOutput(pActivator, this);
	else
		m_OnNotFoundItem.FireOutput(pActivator, this);
}
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
	uint m_iExchangeItemID;
	bool m_bIsMapItem;
	bool m_bOneAtATime;

	COutputEvent m_OnFoundItem;
	COutputEvent m_OnNotFoundItem;
};

LINK_ENTITY_TO_CLASS(logic_inventory_check, CLogicInventoryChecker);

BEGIN_DATADESC(CLogicInventoryChecker)
DEFINE_KEYFIELD(m_iAction, FIELD_INTEGER, "Action"),
DEFINE_KEYFIELD(m_iFilter, FIELD_INTEGER, "Team"),
DEFINE_KEYFIELD(m_iItemID, FIELD_INTEGER, "ItemID"),
DEFINE_KEYFIELD(m_iExchangeItemID, FIELD_INTEGER, "ItemExchangeID"),
DEFINE_KEYFIELD(m_bIsMapItem, FIELD_BOOLEAN, "MapItem"),
DEFINE_KEYFIELD(m_bOneAtATime, FIELD_BOOLEAN, "OnePerCheck"),

DEFINE_INPUTFUNC(FIELD_VOID, "CheckForItem", InputCheckForItem),
DEFINE_OUTPUT(m_OnFoundItem, "OnFound"),
DEFINE_OUTPUT(m_OnNotFoundItem, "OnNotFound"),
END_DATADESC()

CLogicInventoryChecker::CLogicInventoryChecker()
{
	m_iAction = m_iFilter = 0;
	m_iItemID = m_iExchangeItemID = 0;
	m_bIsMapItem = m_bOneAtATime = false;
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
	const DataInventoryItem_Base_t *pInvItem = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(m_iItemID, m_bIsMapItem);
	const DataInventoryItem_Base_t *pExchangeItem = ((m_iExchangeItemID && m_bIsMapItem) ? GameBaseShared()->GetSharedGameDetails()->GetInventoryData(m_iExchangeItemID, m_bIsMapItem) : NULL);

	for (int i = (GameBaseShared()->GetServerInventory().Count() - 1); i >= 0; i--)
	{
		const InventoryItem_t *pItem = &GameBaseShared()->GetServerInventory()[i];
		if ((pItem->bIsMapItem != m_bIsMapItem) || (pItem->m_iPlayerIndex != pActivator->entindex()) || (pItem->m_iItemID != m_iItemID))
			continue;

		switch (m_iAction)
		{
		case INV_CHECK_USE: // Auto-Use
			GameBaseShared()->UseInventoryItem(pActivator->entindex(), m_iItemID, m_bIsMapItem, false, false, i);
			break;
		case INV_CHECK_DROP: // Drop
		case INV_CHECK_DELETE: // Delete
			GameBaseShared()->RemoveInventoryItem(pActivator->entindex(), pActivator->GetAbsOrigin(), (m_bIsMapItem ? 1 : 0), m_iItemID, (m_iAction == INV_CHECK_DELETE), i);
			break;
		}

		if (pExchangeItem && (m_iAction == INV_CHECK_DELETE)) // Exchange prev. item for this new item!
			GameBaseShared()->AddInventoryItem(pActivator->entindex(), pExchangeItem, m_bIsMapItem);

		bFound = true;
		if (m_bOneAtATime)
			break;
	}

	if (bFound && pInvItem && pExchangeItem && (m_iAction == INV_CHECK_DELETE))
	{
		CSingleUserRecipientFilter filter(pActivator);
		EmitSound(filter, pActivator->entindex(), pInvItem->szSoundScriptExchange);
	}

	if (bFound)
		m_OnFoundItem.FireOutput(pActivator, this);
	else
		m_OnNotFoundItem.FireOutput(pActivator, this);
}
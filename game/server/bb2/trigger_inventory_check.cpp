//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Check if the user who touched this volume has a certain inv. item, do something about it?
//
//========================================================================================//

#include "cbase.h"
#include "triggers.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"

class CTriggerInventoryCheck : public CTriggerMultiple
{
public:
	DECLARE_CLASS(CTriggerInventoryCheck, CTriggerMultiple);
	DECLARE_DATADESC();

	CTriggerInventoryCheck();
	void Spawn();
	void StartTouch(CBaseEntity *pOther);

private:

	int m_iAction;
	int m_iFilter;
	uint m_iItemID;
	bool m_bIsMapItem;

	COutputEvent m_OnFoundItem;
};

LINK_ENTITY_TO_CLASS(trigger_inventory_check, CTriggerInventoryCheck);

BEGIN_DATADESC(CTriggerInventoryCheck)
DEFINE_KEYFIELD(m_iAction, FIELD_INTEGER, "Action"),
DEFINE_KEYFIELD(m_iFilter, FIELD_INTEGER, "Team"),
DEFINE_KEYFIELD(m_iItemID, FIELD_INTEGER, "ItemID"),
DEFINE_KEYFIELD(m_bIsMapItem, FIELD_BOOLEAN, "MapItem"),
DEFINE_OUTPUT(m_OnFoundItem, "OnFound"),
END_DATADESC()

CTriggerInventoryCheck::CTriggerInventoryCheck()
{
	m_iAction = 0;
	m_iFilter = 0;
	m_iItemID = 0;
	m_bIsMapItem = false;
}

void CTriggerInventoryCheck::Spawn()
{
	if (!m_iItemID)
	{
		Warning("trigger_inventory_check '%s' with invalid ItemID '%u'!\nRemoving!\n", STRING(GetEntityName()), m_iItemID);
		UTIL_Remove(this);
		return;
	}

	BaseClass::Spawn();
}

void CTriggerInventoryCheck::StartTouch(CBaseEntity *pOther)
{
	if (m_bDisabled || !pOther || !pOther->IsPlayer() || ((m_iFilter > 0) && (pOther->GetTeamNumber() != m_iFilter)))
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pOther);
	if (!pPlayer)
		return;

	for (int i = (GameBaseShared()->GetServerInventory().Count() - 1); i >= 0; i--)
	{
		const InventoryItem_t *pItem = &GameBaseShared()->GetServerInventory()[i];
		if ((pItem->bIsMapItem != m_bIsMapItem) || (pItem->m_iPlayerIndex != pPlayer->entindex()) || (pItem->m_iItemID != m_iItemID))
			continue;

		switch (m_iAction)
		{
		case INV_CHECK_USE: // Auto-Use
			GameBaseShared()->UseInventoryItem(pPlayer->entindex(), m_iItemID, m_bIsMapItem, false, false, i);
			break;
		case INV_CHECK_DROP: // Drop
		case INV_CHECK_DELETE: // Delete
			GameBaseShared()->RemoveInventoryItem(pPlayer->entindex(), pPlayer->GetAbsOrigin(), (m_bIsMapItem ? 1 : 0), m_iItemID, (m_iAction == INV_CHECK_DELETE), i);
			break;
		}

		m_OnFoundItem.FireOutput(pPlayer, this);
	}
}
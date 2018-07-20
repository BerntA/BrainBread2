//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Inventory Item Object
//
//========================================================================================//

#ifndef INVENTORY_ITEMS_H
#define INVENTORY_ITEMS_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"
#include "props.h"
#include "items.h"
#include "GameEventListener.h"

#define OBJECTIVE_ICON_EXTRA_HEIGHT 30.0f

class CInventoryItem : public CItem, public CGameEventListener
{
public:

	DECLARE_CLASS(CInventoryItem, CItem);
	DECLARE_DATADESC();

	CInventoryItem();
	virtual ~CInventoryItem();

	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void DelayedUse(CBaseEntity *pActivator);
	void OnRotationEffect(void);

	CHL2MP_Player *GetHumanInteractor(CBaseEntity *pActivator);
	bool SetItem(const DataInventoryItem_Base_t &data, bool bMapItem);
	bool SetItem(uint itemID, bool bMapItem);

	const DataInventoryItem_Base_t* GetItemData(void) { return m_pData; }

private:

	const DataInventoryItem_Base_t *m_pData;

	bool m_bExcludeFromInventory;
	uint m_iItemID;
	bool m_bIsMapItem;
	bool m_bHasDoneLateUpdate;

	COutputEvent m_OnUse;

	EHANDLE m_pObjIcon;

protected:

	void FireGameEvent(IGameEvent *event);
};

#endif // INVENTORY_ITEMS_H
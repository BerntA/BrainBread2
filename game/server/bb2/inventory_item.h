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

#include "cbase.h"
#include "baseanimating.h"
#include "props.h"
#include "items.h"

#define OBJECTIVE_ICON_EXTRA_HEIGHT 30.0f

class CInventoryItem : public CItem
{
public:

	DECLARE_CLASS(CInventoryItem, CItem);
	DECLARE_DATADESC();

	CInventoryItem();
	~CInventoryItem();
	void Precache(void);
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void DelayedUse(CBaseEntity *pActivator);
	void OnRotationEffect(void);

	//virtual bool EnablePhysics() { return true; }
	virtual void SetItem(const char *model, uint iID, const char *entityLink, bool bMapItem);

private:

	bool m_bExcludeFromInventory;
	uint m_iItemID;
	bool m_bIsMapItem;

	string_t szModel;
	string_t szEntityLink;

	COutputEvent m_OnUse;

	EHANDLE m_pObjIcon;
};

#endif // INVENTORY_ITEMS_H
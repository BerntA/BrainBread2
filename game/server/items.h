//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEMS_H
#define ITEMS_H

#ifdef _WIN32
#pragma once
#endif

#include "entityoutput.h"
#include "player_pickup.h"

struct DataInventoryItem_Base_t;
class CItem : public CBaseAnimating, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS(CItem, CBaseAnimating);

	CItem();

	// Inventory accessor
	virtual bool SetItem(const DataInventoryItem_Base_t &data, bool bMapItem) { return false; }
	virtual bool SetItem(uint itemID, bool bMapItem) { return false; }
	virtual void SetParam(const char *param) { }
	virtual void SetParam(float param) { }
	virtual bool EnablePhysics() { return false; }
	virtual bool CanBeUsed() { return true; }
	virtual bool CanPickup() { return !(GetEffects() & EF_NODRAW); }
	virtual void Spawn(void);
	virtual void Precache();
	virtual int GetItemPrio(void) const { return ITEM_PRIORITY_GENERIC; }

	virtual unsigned int PhysicsSolidMaskForEntity(void) const;

	virtual CBaseEntity* Respawn(void);
	virtual void ItemTouch(CBaseEntity *pOther);
	virtual void Materialize(void);
	virtual bool MyTouch(CBasePlayer *pPlayer) { return false; }

	// Become touchable when we are at rest
	virtual void OnEntityEvent(EntityEvent_t event, void *pEventData);

	// Activate when at rest, but don't allow pickup until then
	void ActivateWhenAtRest(float flTime = 0.5f);

	// IPlayerPickupVPhysics
	virtual void OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON);
	virtual void OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t reason);

	virtual int	ObjectCaps()
	{
		return (m_bIsDisabled ? BaseClass::ObjectCaps() : (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE));
	}

	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual const Vector &GetOriginalSpawnOrigin(void) { return m_vOriginalSpawnOrigin; }
	virtual const QAngle &GetOriginalSpawnAngles(void) { return m_vOriginalSpawnAngles; }
	virtual void SetOriginalSpawnOrigin(const Vector& origin) { m_vOriginalSpawnOrigin = origin; }
	virtual void SetOriginalSpawnAngles(const QAngle& angles) { m_vOriginalSpawnAngles = angles; }
	virtual bool CreateItemVPhysicsObject(void);
	virtual bool ItemCanBeTouchedByPlayer(CBasePlayer *pPlayer);

	void EnableItem(inputdata_t &inputData);
	void DisableItem(inputdata_t &inputData);
	virtual void OnItemStateUpdated(bool bDisabled) { }

#if defined( HL2MP )
	void	FallThink(void);
#endif

	DECLARE_DATADESC();
protected:
	virtual void ComeToRest(void);

private:
	bool		m_bActivateWhenAtRest;

	COutputEvent m_OnPlayerTouch;
	COutputEvent m_OnCacheInteraction;

	Vector		m_vOriginalSpawnOrigin;
	QAngle		m_vOriginalSpawnAngles;

protected:
	int			m_iOldGlowMode;
	bool		m_bIsDisabled;
};

#endif // ITEMS_H
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
#include "vphysics/constraints.h"

#define SF_ITEM_START_CONSTRAINED	0x00000001

class CItem : public CBaseAnimating, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS(CItem, CBaseAnimating);

	CItem();

	// Inventory accessor
	virtual void SetItem(const char *model, uint iID, const char *entityLink, bool bMapItem) { }
	virtual void SetParam(const char *param) { }
	virtual void SetParam(float param) { }
	virtual bool EnablePhysics() { return false; }
	virtual bool CanBeUsed() { return true; }
	virtual bool CanPickup() { return !(GetEffects() & EF_NODRAW); }
	virtual void Spawn(void);
	virtual void Precache();

	unsigned int PhysicsSolidMaskForEntity(void) const;

	virtual CBaseEntity* Respawn(void);
	virtual void ItemTouch(CBaseEntity *pOther);
	virtual void Materialize(void);
	virtual bool MyTouch(CBasePlayer *pPlayer) { return false; };

	// Become touchable when we are at rest
	virtual void OnEntityEvent(EntityEvent_t event, void *pEventData);

	// Activate when at rest, but don't allow pickup until then
	void ActivateWhenAtRest(float flTime = 0.5f);

	// IPlayerPickupVPhysics
	virtual void OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON);
	virtual void OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t reason);

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_WCEDIT_POSITION; };
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	Vector	GetOriginalSpawnOrigin(void) { return m_vOriginalSpawnOrigin; }
	QAngle	GetOriginalSpawnAngles(void) { return m_vOriginalSpawnAngles; }
	void	SetOriginalSpawnOrigin(const Vector& origin) { m_vOriginalSpawnOrigin = origin; }
	void	SetOriginalSpawnAngles(const QAngle& angles) { m_vOriginalSpawnAngles = angles; }
	bool	CreateItemVPhysicsObject(void);
	virtual bool	ItemCanBeTouchedByPlayer(CBasePlayer *pPlayer);

#if defined( HL2MP )
	void	FallThink(void);
	float  m_flNextResetCheckTime;
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

	IPhysicsConstraint		*m_pConstraint;
};

#endif // ITEMS_H
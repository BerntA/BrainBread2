//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: area portal entity: toggles visibility areas on/off
//
// NOTE: These are not really brush entities.  They are brush entities from a 
// designer/worldcraft perspective, but by the time they reach the game, the 
// brush model is gone and this is, in effect, a point entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "func_areaportalbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CAreaPortal : public CFuncAreaPortalBase
{
public:
	DECLARE_CLASS(CAreaPortal, CFuncAreaPortalBase);
	DECLARE_DATADESC();

	CAreaPortal();

	virtual void	Spawn(void);
	virtual void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int		UpdateTransmitState();
	virtual bool	UpdateVisibility(const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient);
};

LINK_ENTITY_TO_CLASS(func_areaportal, CAreaPortal);

BEGIN_DATADESC(CAreaPortal)
DEFINE_KEYFIELD(m_portalNumber, FIELD_INTEGER, "portalnumber"),
END_DATADESC()

CAreaPortal::CAreaPortal()
{
	m_state = AREAPORTAL_OPEN;
}

void CAreaPortal::Spawn(void)
{
	AddEffects(EF_NORECEIVESHADOW | EF_NOSHADOW);
	Precache();
}

bool CAreaPortal::UpdateVisibility(const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient)
{
	if (m_state)
	{
		// We're not closed, so give the base class a chance to close it.
		return BaseClass::UpdateVisibility(vOrigin, fovDistanceAdjustFactor, bIsOpenOnClient);
	}
	else
	{
		bIsOpenOnClient = false;
		return false;
	}
}

void CAreaPortal::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_ON)
		SetState(AREAPORTAL_OPEN);
	else if (useType == USE_OFF)
		SetState(AREAPORTAL_CLOSED);
}

int CAreaPortal::UpdateTransmitState()
{
	// Our brushes are kept around so don't transmit anything since we don't want to draw them.
	return SetTransmitState(FL_EDICT_DONTSEND);
}
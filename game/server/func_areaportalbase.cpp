//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "func_areaportalbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// A sphere around the player used for backface culling of areaportals.
#define VIEWER_PADDING	80

CUtlLinkedList<CFuncAreaPortalBase*, unsigned short> g_AreaPortals;

BEGIN_DATADESC(CFuncAreaPortalBase)

DEFINE_KEYFIELD(m_iPortalVersion, FIELD_INTEGER, "PortalVersion"),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "Open", InputOpen),
DEFINE_INPUTFUNC(FIELD_VOID, "Close", InputClose),
DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),

// TODO: obsolete! remove	
DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputClose),
DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputOpen),

END_DATADESC()

CFuncAreaPortalBase::CFuncAreaPortalBase()
{
	m_state = AREAPORTAL_OPEN;
	m_portalNumber = -1;
	m_AreaPortalsElement = g_AreaPortals.AddToTail(this);
	m_iPortalVersion = 0;
}

CFuncAreaPortalBase::~CFuncAreaPortalBase()
{
	g_AreaPortals.Remove(m_AreaPortalsElement);
}

bool CFuncAreaPortalBase::UpdateVisibility(const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient)
{
	// NOTE: We leave bIsOpenOnClient alone on purpose here. See the header for a description of why.

	if (m_portalNumber == -1)
		return false;

	// See if the viewer is on the backside.
	VPlane plane;
	if (!engine->GetAreaPortalPlane(vOrigin, m_portalNumber, &plane))
		return true; // leave it open if there's an error here for some reason

	bool bOpen = false;
	if (plane.DistTo(vOrigin) + VIEWER_PADDING > 0)
		bOpen = true;

	return bOpen;
}

void CFuncAreaPortalBase::Precache(void)
{
	UpdateState();
}

bool CFuncAreaPortalBase::KeyValue(const char *szKeyName, const char *szValue)
{
	if (FStrEq(szKeyName, "StartOpen"))
	{
		m_state = (atoi(szValue) != 0) ? AREAPORTAL_OPEN : AREAPORTAL_CLOSED;
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

void CFuncAreaPortalBase::InputClose(inputdata_t &inputdata)
{
	SetState(AREAPORTAL_CLOSED);
}

void CFuncAreaPortalBase::InputOpen(inputdata_t &inputdata)
{
	SetState(AREAPORTAL_OPEN);
}

void CFuncAreaPortalBase::InputToggle(inputdata_t &inputdata)
{
	SetState(((m_state == AREAPORTAL_OPEN) ? AREAPORTAL_CLOSED : AREAPORTAL_OPEN));
}

void CFuncAreaPortalBase::UpdateState(void)
{
	engine->SetAreaPortalState(m_portalNumber, m_state);
}
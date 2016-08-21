//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Client Side Gib - TempEnt Definition.
//
//========================================================================================//

#include "cbase.h"
#include "gibs_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTEClientSideGib : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEClientSideGib, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEClientSideGib(const char *name);

	CNetworkVector(m_vecOrigin);
	CNetworkQAngle(m_angRotation);
	CNetworkVector(m_vecVelocity);
	CNetworkVar(int, m_nModelIndex);
	CNetworkVar(int, m_nBody);
	CNetworkVar(int, m_nSkin);
	CNetworkVar(int, m_nFlags);
	CNetworkVar(int, m_nEffects);
	CNetworkVar(int, m_iGibType);
	CNetworkVar(int, m_iPlayerIndex);
};

IMPLEMENT_SERVERCLASS_ST(CTEClientSideGib, DT_TEClientSideGib)
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD),
SendPropAngle(SENDINFO_VECTORELEM(m_angRotation, 0), 13),
SendPropAngle(SENDINFO_VECTORELEM(m_angRotation, 1), 13),
SendPropAngle(SENDINFO_VECTORELEM(m_angRotation, 2), 13),
SendPropVector(SENDINFO(m_vecVelocity), -1, SPROP_COORD),
SendPropModelIndex(SENDINFO(m_nModelIndex)),
SendPropInt(SENDINFO(m_nBody), ANIMATION_BODY_BITS),
SendPropInt(SENDINFO(m_nSkin), ANIMATION_SKIN_BITS),
SendPropInt(SENDINFO(m_nFlags), MAX_GIB_BITS, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_nEffects), EF_MAX_BITS, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iGibType), 2, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iPlayerIndex), 5, SPROP_UNSIGNED),
END_SEND_TABLE()

CTEClientSideGib::CTEClientSideGib(const char *name) : CBaseTempEntity(name)
{
	m_vecOrigin.Init();
	m_angRotation.Init();
	m_vecVelocity.Init();
	m_nModelIndex = 0;
	m_nBody = 0;
	m_nSkin = 0;
	m_nFlags = 0;
	m_nEffects = 0;
	m_iGibType = 0;
	m_iPlayerIndex = 0;
}

static CTEClientSideGib g_TEClientSideGib("clientsidegib");

void TE_ClientSideGib(IRecipientFilter& filter, float delay, int modelindex, int body, int skin,
	const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects, int gibType, int playerIndex)
{
	g_TEClientSideGib.m_vecOrigin = pos;
	g_TEClientSideGib.m_angRotation = angles;
	g_TEClientSideGib.m_vecVelocity = vel;
	g_TEClientSideGib.m_nModelIndex = modelindex;
	g_TEClientSideGib.m_nBody = body;
	g_TEClientSideGib.m_nSkin = skin;
	g_TEClientSideGib.m_nFlags = flags;
	g_TEClientSideGib.m_nEffects = effects;
	g_TEClientSideGib.m_iGibType = gibType;
	g_TEClientSideGib.m_iPlayerIndex = playerIndex;

	g_TEClientSideGib.Create(filter, delay);
}
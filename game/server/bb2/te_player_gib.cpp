//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: TempEnt : Player gib & ragdoll, the new client-side player models required a separate event for player related gibbings.
//
//========================================================================================//

#include "cbase.h"
#include "basetempentity.h"
#include "gibs_shared.h"

class CTEPlayerGib : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEPlayerGib, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEPlayerGib(const char *name);

	CNetworkVar(int, m_iIndex);
	CNetworkVar(int, m_iFlags);
	CNetworkVar(int, m_iType);
	CNetworkVector(m_vecOrigin);
	CNetworkVector(m_vecVelocity);
	CNetworkQAngle(m_angRotation);	
};

CTEPlayerGib::CTEPlayerGib(const char *name) : CBaseTempEntity(name)
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlayerGib, DT_TEPlayerGib)
SendPropInt(SENDINFO(m_iIndex), 4, SPROP_UNSIGNED), // Should correspond to a value within 1-MAX_PLAYERS!
SendPropInt(SENDINFO(m_iFlags), MAX_GIB_BITS, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iType), 3, SPROP_UNSIGNED),
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD),
SendPropAngle(SENDINFO_VECTORELEM(m_angRotation, 0), 13),
SendPropAngle(SENDINFO_VECTORELEM(m_angRotation, 1), 13),
SendPropAngle(SENDINFO_VECTORELEM(m_angRotation, 2), 13),
SendPropVector(SENDINFO(m_vecVelocity), -1, SPROP_COORD),
END_SEND_TABLE()

static CTEPlayerGib g_TEPlayerGib("PlayerGib");

void TE_PlayerGibRagdoll(
	int	index,
	int flags,
	int type,
	const Vector &filterOrigin,
	const Vector &actualOrigin,
	const Vector &velocity,
	const QAngle &angles
	)
{
	CPASFilter filter(filterOrigin);
	g_TEPlayerGib.m_iIndex = index;
	g_TEPlayerGib.m_iFlags = flags;
	g_TEPlayerGib.m_iType = type;
	g_TEPlayerGib.m_vecOrigin = actualOrigin;
	g_TEPlayerGib.m_vecVelocity = velocity;
	g_TEPlayerGib.m_angRotation = angles;
	g_TEPlayerGib.Create(filter, 0);
}
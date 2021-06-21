//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "animation.h"
#include "baseflex.h"
#include "filesystem.h"
#include "studio.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "tier1/strtools.h"
#include "KeyValues.h"
#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_moveprobe.h"
#include "ai_baseactor.h"
#include "datacache/imdlcache.h"
#include "tier1/byteswap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ---------------------------------------------------------------------
//
// CBaseFlex -- physically simulated brush rectangular solid
//
// ---------------------------------------------------------------------

// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CBaseFlex, DT_BaseFlex)
SendPropVector(SENDINFO(m_viewtarget), -1, SPROP_COORD),
#ifdef HL2_DLL
SendPropFloat(SENDINFO_VECTORELEM(m_vecViewOffset, 0), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO_VECTORELEM(m_vecViewOffset, 1), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO_VECTORELEM(m_vecViewOffset, 2), 0, SPROP_NOSCALE),

SendPropVector(SENDINFO(m_vecLean), -1, SPROP_COORD),
SendPropVector(SENDINFO(m_vecShift), -1, SPROP_COORD),
#endif
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(funCBaseFlex, CBaseFlex); // meaningless independant class!!

CBaseFlex::CBaseFlex(void)
{
#ifdef _DEBUG
	// default constructor sets the viewtarget to NAN
	m_viewtarget.Init();
#endif
}

CBaseFlex::~CBaseFlex(void)
{
}

void CBaseFlex::SetViewtarget(const Vector &viewtarget)
{
	m_viewtarget = viewtarget;	// bah
}

//-----------------------------------------------------------------------------
// Purpose: Clear out body lean states that are invalidated with Teleport
//-----------------------------------------------------------------------------
void CBaseFlex::Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity)
{
	BaseClass::Teleport(newPosition, newAngles, newVelocity);
#ifdef HL2_DLL

	// clear out Body Lean
	m_vecPrevOrigin = vec3_origin;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: keep track of accel/decal and lean the body
//-----------------------------------------------------------------------------
void CBaseFlex::DoBodyLean(void)
{
#ifdef HL2_DLL
	CAI_BaseNPC *myNpc = MyNPCPointer();
	if (myNpc)
	{
		Vector vecDelta;
		Vector vecPos;
		Vector vecOrigin = GetAbsOrigin();

		if (m_vecPrevOrigin == vec3_origin)
		{
			m_vecPrevOrigin = vecOrigin;
		}

		vecDelta = vecOrigin - m_vecPrevOrigin;
		vecDelta.x = clamp(vecDelta.x, -50, 50);
		vecDelta.y = clamp(vecDelta.y, -50, 50);
		vecDelta.z = clamp(vecDelta.z, -50, 50);

		float dt = gpGlobals->curtime - GetLastThink();
		bool bSkip = ((GetFlags() & (FL_FLY | FL_SWIM)) != 0) || (GetMoveParent() != NULL) || (GetGroundEntity() == NULL) || (GetGroundEntity()->IsMoving());
		bSkip |= myNpc->TaskRanAutomovement();

		if (!bSkip)
		{
			if (vecDelta.LengthSqr() > m_vecPrevVelocity.LengthSqr())
			{
				float decay = ExponentialDecay(0.6, 0.1, dt);
				m_vecPrevVelocity = m_vecPrevVelocity * (decay)+vecDelta * (1.f - decay);
			}
			else
			{
				float decay = ExponentialDecay(0.4, 0.1, dt);
				m_vecPrevVelocity = m_vecPrevVelocity * (decay)+vecDelta * (1.f - decay);
			}

			vecPos = m_vecPrevOrigin + m_vecPrevVelocity;

			float decay = ExponentialDecay(0.5, 0.1, dt);
			m_vecShift = m_vecShift * (decay)+(vecOrigin - vecPos) * (1.f - decay); // FIXME: Scale this
			m_vecLean = (vecOrigin - vecPos) * 1.0; // FIXME: Scale this
		}
		else
		{
			m_vecPrevVelocity = vecDelta;
			float decay = ExponentialDecay(0.5, 0.1, dt);
			m_vecShift = m_vecLean * decay;
			m_vecLean = m_vecShift * decay;
		}

		m_vecPrevOrigin = vecOrigin;
	}
#endif
}
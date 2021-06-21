//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEFLEX_H
#define BASEFLEX_H
#ifdef _WIN32
#pragma once
#endif

#include "BaseAnimatingOverlay.h"
#include "utlvector.h"
#include "utlrbtree.h"

struct flexsettinghdr_t;
struct flexsetting_t;

//-----------------------------------------------------------------------------
// Purpose: Animated characters who have vertex flex capability (e.g., facial expressions)
//-----------------------------------------------------------------------------
class CBaseFlex : public CBaseAnimatingOverlay
{
	DECLARE_CLASS(CBaseFlex, CBaseAnimatingOverlay);
public:
	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();

	CBaseFlex(void);
	~CBaseFlex(void);

	virtual	void		SetViewtarget(const Vector &viewtarget);
	const Vector		&GetViewtarget(void) const;

	void				SentenceStop(void) { EmitSound("AI_BaseNPC.SentenceStop"); }

private:

	// Vector from actor to eye target
	CNetworkVector(m_viewtarget);

public:
	void DoBodyLean(void);
	virtual void Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);

#ifdef HL2_DLL
	Vector m_vecPrevOrigin;
	Vector m_vecPrevVelocity;
	CNetworkVector(m_vecLean);
	CNetworkVector(m_vecShift);
#endif
};

//-----------------------------------------------------------------------------
// Other inlines
//-----------------------------------------------------------------------------
inline const Vector &CBaseFlex::GetViewtarget() const
{
	return m_viewtarget.Get();	// bah
}

EXTERN_SEND_TABLE(DT_BaseFlex);

#endif // BASEFLEX_H
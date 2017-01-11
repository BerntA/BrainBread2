//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: First Person Body
//
//========================================================================================//

#ifndef C_FIRSTPERSON_BODY_H
#define C_FIRSTPERSON_BODY_H

#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "c_baseanimatingoverlay.h"
#include "baseentity_shared.h"

class C_FirstpersonBody : public C_BaseAnimatingOverlay
{
	DECLARE_CLASS(C_FirstpersonBody, C_BaseAnimatingOverlay);
public:

	C_FirstpersonBody();
	virtual int ObjectCaps()
	{
		return FCAP_DONT_SAVE;
	}

	virtual void Spawn();
	virtual ShadowType_t ShadowCastType();
	virtual int DrawModel(int flags);
	virtual bool ShouldDraw();
	virtual bool ShouldReceiveProjectedTextures(int flags);
	virtual RenderGroup_t GetRenderGroup() { return RENDER_GROUP_OPAQUE_ENTITY; }
	virtual void GetRenderBounds(Vector &mins, Vector &maxs);
	virtual void StudioFrameAdvance();
};

extern ConVar bb2_render_body;

#endif // C_FIRSTPERSON_BODY_H
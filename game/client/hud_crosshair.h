//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_CROSSHAIR_H
#define HUD_CROSSHAIR_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

namespace vgui
{
	class IScheme;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCrosshair : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudCrosshair, vgui::Panel);
public:
	CHudCrosshair(const char *pElementName);
	virtual ~CHudCrosshair();

	virtual void	VidInit(void);
	virtual void	SetCrosshairAngle(const QAngle& angle);
	virtual void	SetCrosshair(void);
	virtual void	DrawCrosshair(void) {}
	virtual void	ProcessInput();
	virtual bool	HasCrosshair(void) { return (m_pCrosshair != NULL); }
	virtual bool	ShouldDraw();
	virtual CHudTexture *GetCrosshairIcon(void);

	// any UI element that wants to be at the aim point can use this to figure out where to draw
	static void	GetDrawPosition(float *pX, float *pY, bool *pbBehindCamera, QAngle angleCrosshairOffset = vec3_angle);
protected:
	virtual void	ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void	Paint();

	// Crosshair sprite and colors
	CHudTexture		*m_pCrosshair;
	CHudTexture		*m_pCursor;
	CHudTexture		*m_pDelayedUse;
	Color			m_clrCrosshair;
	QAngle			m_vecCrossHairOffsetAngle;
	bool m_bHoldingUseButton;
	float m_flTimePressedUse;

	CPanelAnimationVar(bool, m_bHideCrosshair, "never_draw", "false");

	CPanelAnimationVarAliasType(float, use_offset_x, "use_offset_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, use_offset_y, "use_offset_y", "0", "proportional_float");
};

// Enable/disable crosshair rendering.
extern ConVar crosshair;
extern ConVar crosshair_color_red;
extern ConVar crosshair_color_green;
extern ConVar crosshair_color_blue;
extern ConVar crosshair_color_alpha;

#endif // HUD_CROSSHAIR_H
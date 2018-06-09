//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: 3D NPC Health Bars, handles all health bars for npcs
//
//========================================================================================//

#ifndef HUD_NPC_HEALTH_BAR_H
#define HUD_NPC_HEALTH_BAR_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "vgui_controls/AnimationController.h"

struct HealthBarItem_t
{
	int index;
	float flTime;
	Vector vecMins;
	Vector vecMaxs;
};

class CHudNPCHealthBar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudNPCHealthBar, vgui::Panel);

public:

	CHudNPCHealthBar(const char * pElementName);
	virtual ~CHudNPCHealthBar();

	void Init(void);
	void VidInit(void);
	void Reset(void);
	bool ShouldDraw(void);
	void AddHealthBarItem(C_BaseEntity *pEntity, int index, bool bIsBoss);
	void Cleanup(void) { pszNPCHealthBarList.Purge(); }

protected:

	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint();

private:

	int m_nTextureBackground;
	int m_nTexture_Bar;

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "Default");

	CPanelAnimationVarAliasType(float, bg_x, "bg_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bg_y, "bg_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bg_w, "bg_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bg_h, "bg_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, bar_y, "bar_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bar_h, "bar_h", "0", "proportional_float");

	CUtlVector<HealthBarItem_t> pszNPCHealthBarList;
};

extern CHudNPCHealthBar *GetHealthBarHUD();

#endif // HUD_NPC_HEALTH_BAR_H
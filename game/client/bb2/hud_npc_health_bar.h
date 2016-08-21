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
	char szName[MAX_PLAYER_NAME_LENGTH];
	int m_iEntIndex;
	int m_iCurrentHealth;
	int m_iMaxHealth;
	bool m_bIsBoss;
	bool m_bShouldShow;
	bool m_bDisplaying;
	bool m_bFadingIn;
	bool m_bFadingOut;
	float m_flLerp;
};

//-----------------------------------------------------------------------------
// Purpose: Declarations
//-----------------------------------------------------------------------------
class CHudNPCHealthBar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudNPCHealthBar, vgui::Panel);

public:

	CHudNPCHealthBar(const char * pElementName);

	void Init(void);
	void Reset(void);
	void OnThink(void);
	void VidInit(void);
	void AddHealthBarItem(int index, bool bIsBoss, int currHealth, int maxHealth, const char *name);

protected:

	virtual void FireGameEvent(IGameEvent *event);
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint();

private:

	int GetItem(int index);
	int m_nTextureBackground;
	int m_nTexture_Bar;
	bool m_bShouldRender;

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "Default");

	CPanelAnimationVarAliasType(float, bg_x, "bg_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bg_y, "bg_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bg_w, "bg_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bg_h, "bg_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, bar_y, "bar_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bar_h, "bar_h", "0", "proportional_float");

	CUtlVector<HealthBarItem_t> pszNPCHealthBarList;
};

#endif // HUD_NPC_HEALTH_BAR_H
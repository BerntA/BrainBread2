//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective HUD - Gets data from the logic_objective & trigger_capturepoint entity.
//
//========================================================================================//

#ifndef HUD_OBJECTIVE_H
#define HUD_OBJECTIVE_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "vgui_controls/AnimationController.h"
#include "GameBase_Shared.h"

class CHudObjective : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudObjective, vgui::Panel);

public:

	CHudObjective(const char * pElementName);
	virtual void Init(void);
	virtual void Reset(void);
	virtual void VidInit(void);
	virtual bool ShouldDraw(void);
	virtual void FireGameEvent(IGameEvent *event);

	float GetYPosOffset(void) { return m_flYPos; }

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	float m_flYPos;
	bool HasThisItem(int index);
	CUtlVector<ObjectiveItem_t> pszObjectiveItems;

	CPanelAnimationVar(vgui::HFont, m_hObjFont, "ObjectiveFont", "BB2_PANEL");
	CPanelAnimationVar(vgui::HFont, m_hTimFont, "TimerFont", "BB2_PANEL");
	CPanelAnimationVar(Color, m_TextColorObj, "ObjectiveColor", "255 255 255 255");
	CPanelAnimationVar(Color, m_TextColorTim, "TimerColor", "255 255 255 255");

	bool HasObjectiveFlag(int index, int nFlag);
	void AddObjectiveFlag(int index, int nFlag);
};

class CHudCaptureProgressBar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudCaptureProgressBar, vgui::Panel);

public:

	CHudCaptureProgressBar(const char * pElementName);
	virtual void Init(void);
	virtual void Reset(void);
	virtual void VidInit(void);
	virtual bool ShouldDraw(void);

	void MsgFunc_CapturePointProgress(bf_read &msg);

protected:

	wchar_t unicodeMessage[128];
	bool m_bShouldDrawProgress;
	float m_flProgressPercent;

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	int m_nTextureBarBackground;
	int m_nTextureBarForeground;

	CPanelAnimationVar(vgui::HFont, m_hDefaultFont, "DefaultFont", "BB2_PANEL");
	CPanelAnimationVarAliasType(float, text_ypos_offset, "text_ypos_offset", "0", "proportional_float");
};

#endif // HUD_OBJECTIVE_H
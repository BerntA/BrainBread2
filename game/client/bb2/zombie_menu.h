//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie Skill Tree Panel
//
//========================================================================================//

#ifndef ZOMBIEMENU_H
#define ZOMBIEMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <game/client/iviewport.h>
#include "MouseInputPanel.h"
#include "SkillTreeIcon.h"

namespace vgui
{
	class TextEntry;
}

class CZombieMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CZombieMenu, vgui::Frame);

public:
	CZombieMenu(IViewPort *pViewPort);
	virtual ~CZombieMenu();

	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void PaintBackground();

	void OnThink();
	void OnClose() {} // unused
	const char *GetName(void) { return PANEL_ZOMBIE; }
	void SetData(KeyValues *data) {};
	void Update() {};
	void Reset();
	bool NeedsUpdate(void) { return false; }
	bool HasInputElements(void) { return true; }
	void ShowPanel(bool bShow);

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

protected:

	vgui::Panel *CreateControlByName(const char *controlName);
	void OnKeyCodeTyped(vgui::KeyCode code);
	void OnCommand(const char *command);
	void PerformLayout();

	vgui::ImagePanel *m_pImgBackground;
	vgui::Label *m_pCreditsInfo;
	vgui::Label *m_pToolTip;

	vgui::SkillTreeIcon *m_pSkillIcon[10];

	IViewPort	*m_pViewPort;
};

#endif // ZOMBIEMENU_H

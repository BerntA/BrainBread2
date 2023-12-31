//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Human Skill Tree Panel
//
//========================================================================================//

#ifndef SKILLTREE_H
#define SKILLTREE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <game/client/iviewport.h>
#include "SkillTreeIcon.h"
#include "ToolTipItem.h"

namespace vgui
{
	class TextEntry;
}

class CSkillTree : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CSkillTree, vgui::Frame);

public:

	CSkillTree(IViewPort *pViewPort);
	virtual ~CSkillTree();

	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void PaintBackground();

	void OnThink();
	void OnClose() {} // unused
	const char *GetName(void) { return PANEL_SKILL; }
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

private:

	vgui::ImagePanel *m_pBackground;

	vgui::Label *m_pLabelTalents;
	vgui::Label *m_pToolTip;

	vgui::SkillTreeIcon *m_pSkillIcon[30];

	vgui::ImagePanel *m_pSpecialSkill[3];

protected:

	vgui::Panel *CreateControlByName(const char *controlName);
	void OnKeyCodeTyped(vgui::KeyCode code);

	void OnCommand(const char *command);
	void PerformLayout();

	IViewPort	*m_pViewPort;
};

#endif // SKILLTREE_H
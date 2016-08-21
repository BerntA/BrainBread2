//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Team Selection Menu
//
//========================================================================================//

#ifndef TEAMMENU_H
#define TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <utlvector.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <game/client/iviewport.h>
#include "mouseoverpanelbutton.h"
#include "vgui/MouseCode.h"
#include <vgui_controls/RichText.h>
#include "MouseInputPanel.h"
#include "vgui_base_frame.h"
#include "basemodelpanel.h"
#include "CharacterPreviewPanel.h"

namespace vgui
{
	class TextEntry;
}

class CTeamMenu : public vgui::CVGUIBaseFrame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CTeamMenu, vgui::CVGUIBaseFrame);

public:
	CTeamMenu(IViewPort *pViewPort);
	virtual ~CTeamMenu();

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground();

	virtual void OnThink();
	const char *GetName(void) { return PANEL_TEAM; }
	void SetData(KeyValues *data) {};
	void Update() {};
	void Reset();
	bool NeedsUpdate(void) { return false; }
	bool HasInputElements(void) { return true; }
	void ShowPanel(bool bShow);
	void OnShowPanel(bool bShow);
	void ForceClose(void);

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

private:
	void OnSetCharacterPreview();

protected:

	virtual vgui::Panel *CreateControlByName(const char *controlName);
	void OnCommand(const char *command);
	void PerformLayout();

	vgui::ImagePanel *m_pImgJoin[2];
	vgui::Button *m_pButtonJoin[2];
	vgui::Label *m_pInfo;
	vgui::Label *m_pTeamInfo[2];

	vgui::CharacterPreviewPanel *m_pModelPreviews[2];

	bool m_bRollover[2];

	IViewPort	*m_pViewPort;
};

#endif // TEAMMENU_H
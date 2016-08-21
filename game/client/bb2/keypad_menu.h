//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: VGUI KeyPad Screen
//
//========================================================================================//

#ifndef KEYPADMENU_H
#define KEYPADMENU_H
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
#include "usermessages.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "hud.h"

namespace vgui
{
	class TextEntry;
}

class CKeyPadMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CKeyPadMenu, vgui::Frame);

public:
	CKeyPadMenu(IViewPort *pViewPort);
	virtual ~CKeyPadMenu();

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground();
	virtual void OnThink();
	virtual const char *GetName(void) { return PANEL_KEYPAD; }
	virtual void SetData(KeyValues *data);
	virtual void Reset();
	virtual void Update() {};
	virtual bool NeedsUpdate(void) { return false; }
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	void UpdateKeyPadCode(const char *szCode, int iEntIndex);

protected:

	virtual vgui::Panel *CreateControlByName(const char *controlName);

	void OnCommand(const char *command);
	void PerformLayout();
	void OnKeyCodeTyped(vgui::KeyCode code);

	// BB2 HARDCODED CONTROLS
	vgui::ImagePanel *m_pImgBackground;

	// Key Pad buttons.
	vgui::Button *m_pButtonKey[10];

	bool IsKeyPadCodeCorrect(const char *szCode);
	char szKeyPadCode[16];
	char szTempCode[5];
	int iEntityIndex;

	IViewPort	*m_pViewPort;
};

#endif // KEYPADMENU_H
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
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <game/client/iviewport.h>
#include "usermessages.h"
#include "hud.h"
#include "hud_macros.h"
#include "iclientmode.h"

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

	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void PaintBackground();
	const char *GetName(void) { return PANEL_KEYPAD; }
	void SetData(KeyValues *data);
	void Reset();
	void Update() {};
	bool NeedsUpdate(void) { return false; }
	bool HasInputElements(void) { return true; }
	void ShowPanel(bool bShow);

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

private:
	char szTempCode[16];
	int iEntityIndex;

protected:
	vgui::Panel *CreateControlByName(const char *controlName);

	void PerformLayout();
	void OnCommand(const char *command);
	void OnKeyCodeTyped(vgui::KeyCode code);

	vgui::ImagePanel *m_pImgBackground;
	vgui::Button *m_pButtonKey[10];

	IViewPort	*m_pViewPort;
};

#endif // KEYPADMENU_H
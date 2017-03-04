//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Keypad VGUI Screen.
//
//========================================================================================//

#ifndef KEYPAD_SCREEN_H
#define KEYPAD_SCREEN_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_vguiscreen.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"

class KeyPadScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS(KeyPadScreen, CVGuiScreenPanel);

public:
	KeyPadScreen(vgui::Panel *pParent, const char *pMetaClassName);
	virtual ~KeyPadScreen();

	bool Init(KeyValues *pKeyValues, VGuiScreenInitData_t *pInitData);
	void OnTick();
	void PerformLayout();

protected:
	void OnCommand(const char *command);
	void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	vgui::Button *m_pButtons[12];
	vgui::ImagePanel *m_pButtonBG[12];
};

#endif // KEYPAD_SCREEN_H
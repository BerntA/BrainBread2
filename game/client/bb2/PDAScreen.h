//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: PDA VGUI Screen.
//
//========================================================================================//

#ifndef PDA_SCREEN_H
#define PDA_SCREEN_H
#ifdef _WIN32
#pragma once
#endif

#include "c_vguiscreen.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"

class PDAScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS(PDAScreen, CVGuiScreenPanel);

public:
	PDAScreen(vgui::Panel *pParent, const char *pMetaClassName);
	virtual ~PDAScreen();

	bool Init(KeyValues *pKeyValues, VGuiScreenInitData_t *pInitData);
	void OnTick();

private:
	vgui::Label *m_pText;
};

#endif // PDA_SCREEN_H
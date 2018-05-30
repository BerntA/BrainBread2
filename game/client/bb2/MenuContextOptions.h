//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Options Menu 
//
//========================================================================================//

#ifndef MENU_CONTEXT_OPTIONS_H
#define MENU_CONTEXT_OPTIONS_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "InlineMenuButton.h"
#include "vgui_base_panel.h"

// Option Menus
#include "OptionMenuKeyboard.h"
#include "OptionMenuMouse.h"
#include "OptionMenuAudio.h"
#include "OptionMenuVideo.h"
#include "OptionMenuGraphics.h"
#include "OptionMenuPerformance.h"
#include "OptionMenuOther.h"

namespace vgui
{
	class MenuContextOptions;

	class MenuContextOptions : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(MenuContextOptions, vgui::CVGUIBasePanel);

	public:
		MenuContextOptions(vgui::Panel *parent, char const *panelName);
		~MenuContextOptions();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);

	private:
		vgui::Label *m_pLabelTitle;
		vgui::InlineMenuButton *m_pMenuButton[8];
		vgui::ImagePanel *m_pImgBackground;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // MENU_CONTEXT_OPTIONS_H
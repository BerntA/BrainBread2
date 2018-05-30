//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Main Menu Context Preview
//
//========================================================================================//

#ifndef MENU_CONTEXT_MAIN_H
#define MENU_CONTEXT_MAIN_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "InlineMenuButton.h"
#include "vgui_base_panel.h"

namespace vgui
{
	class MenuContextMain;

	class MenuContextMain : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(MenuContextMain, vgui::CVGUIBasePanel);

	public:
		MenuContextMain(vgui::Panel *parent, char const *panelName);
		~MenuContextMain();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);

	private:
		vgui::InlineMenuButton *m_pMenuButton[5];

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // MENU_CONTEXT_MAIN_H
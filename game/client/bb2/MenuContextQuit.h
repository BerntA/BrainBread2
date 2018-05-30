//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Exit/Disconnect Menu - If you're in-game it disconnects you when confirming to exit.
//
//========================================================================================//

#ifndef MENU_CONTEXT_QUIT_H
#define MENU_CONTEXT_QUIT_H

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
	class MenuContextQuit;

	class MenuContextQuit : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(MenuContextQuit, vgui::CVGUIBasePanel);

	public:
		MenuContextQuit(vgui::Panel *parent, char const *panelName);
		~MenuContextQuit();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);

	private:
		vgui::InlineMenuButton *m_pMenuButton[2];
		vgui::ImagePanel *m_pImgBackground;
		vgui::Label *m_pLabelMessage;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // MENU_CONTEXT_QUIT_H
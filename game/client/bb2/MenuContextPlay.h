//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Play Menu - Introduces the Server Browser, Create a Game and perhaps the global scoreboard (if we'll keep it) 
//
//========================================================================================//

#ifndef MENU_CONTEXT_PLAY_H
#define MENU_CONTEXT_PLAY_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include "InlineMenuButton.h"
#include "vgui_base_panel.h"

namespace vgui
{
	class MenuContextPlay;

	class MenuContextPlay : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(MenuContextPlay, vgui::CVGUIBasePanel);

	public:
		MenuContextPlay(vgui::Panel *parent, char const *panelName);
		~MenuContextPlay();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);

	private:
		vgui::Label *m_pLabelTitle;
		vgui::InlineMenuButton *m_pMenuButton[4];
		vgui::ImagePanel *m_pImgBackground;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // MENU_CONTEXT_PLAY_H
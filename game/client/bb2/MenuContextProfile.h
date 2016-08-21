//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Profile Menu - Includes the achievement panel and perhaps some model selection editor in the future???
//
//========================================================================================//

#ifndef MENU_CONTEXT_PROFILE_H
#define MENU_CONTEXT_PROFILE_H

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
	class MenuContextProfile;

	class MenuContextProfile : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(MenuContextProfile, vgui::CVGUIBasePanel);

	public:
		MenuContextProfile(vgui::Panel *parent, char const *panelName);
		~MenuContextProfile();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);

	private:
		vgui::Label *m_pLabelTitle;
		vgui::InlineMenuButton *m_pMenuButton[2];
		vgui::ImagePanel *m_pImgBackground;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // MENU_CONTEXT_PROFILE_H
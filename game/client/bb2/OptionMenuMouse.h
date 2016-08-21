//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Mouse Related Options
//
//========================================================================================//

#ifndef OPTION_MENU_MOUSE_H
#define OPTION_MENU_MOUSE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/SectionedListPanel.h>
#include "GraphicalCheckBox.h"
#include "GraphicalOverlayInset.h"
#include "vgui_base_panel.h"
#include "InlineMenuButton.h"

namespace vgui
{
	class OptionMenuMouse;

	class OptionMenuMouse : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuMouse, vgui::CVGUIBasePanel);

	public:
		OptionMenuMouse(vgui::Panel *parent, char const *panelName);
		~OptionMenuMouse();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);
		void ApplyChanges(void);

	private:
		vgui::ImagePanel *m_pDivider[2];
		vgui::Label *m_pTextTitle[2];
		vgui::GraphicalCheckBox *m_pCheckBoxVar[7];
		vgui::GraphicalOverlay *m_pSlider[4];

		vgui::InlineMenuButton *m_pApplyButton;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // OPTION_MENU_MOUSE_H
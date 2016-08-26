//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Performance Options: Gibs, Particles, Misc stuff....
//
//========================================================================================//

#ifndef OPTION_MENU_PERFORMANCE_H
#define OPTION_MENU_PERFORMANCE_H

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
#include <vgui_controls/ComboBox.h>
#include "vgui_base_panel.h"
#include "InlineMenuButton.h"

namespace vgui
{
	class OptionMenuPerformance;
	class OptionMenuPerformance : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuPerformance, vgui::CVGUIBasePanel);

	public:
		OptionMenuPerformance(vgui::Panel *parent, char const *panelName);
		~OptionMenuPerformance();

		void OnUpdate(bool bInGame);
		void ApplyChanges(void);
		void SetupLayout(void);

	private:
		vgui::ImagePanel *m_pDivider[2];
		vgui::Label *m_pTextTitle[2];

		vgui::GraphicalCheckBox *m_pCheckBoxVar[10];
		vgui::GraphicalOverlay *m_pSlider[3];
		vgui::InlineMenuButton *m_pApplyButton;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // OPTION_MENU_PERFORMANCE_H
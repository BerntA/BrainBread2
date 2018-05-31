//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Custom Options - BB2 Custom Options & Other interesting tweak options for better performance / graphics.
//
//========================================================================================//

#ifndef OPTION_MENU_OTHER_H
#define OPTION_MENU_OTHER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include "vgui_base_panel.h"
#include "GraphicalCheckBox.h"
#include "GraphicalOverlayInset.h"
#include "ComboList.h"
#include "ComboImageList.h"
#include "InlineMenuButton.h"

namespace vgui
{
	class OptionMenuOther;
	class OptionMenuOther : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuOther, vgui::CVGUIBasePanel);

	public:
		OptionMenuOther(vgui::Panel *parent, char const *panelName);
		~OptionMenuOther();

		void OnUpdate(bool bInGame);
		void ApplyChanges(void);
		void SetupLayout(void);

	private:
		vgui::ImagePanel *m_pDivider[2];
		vgui::Label *m_pTextTitle[2];

		vgui::GraphicalCheckBox *m_pCheckBoxVar[5];
		vgui::ComboList *m_pComboSoundSet[5];

		vgui::ComboImageList *m_pComboImgList[1];

		vgui::GraphicalOverlay *m_pCrosshairColorSlider[4];

		vgui::InlineMenuButton *m_pApplyButton;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // OPTION_MENU_OTHER_H
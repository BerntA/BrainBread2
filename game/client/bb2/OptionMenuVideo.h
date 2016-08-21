//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Video Related Options.
//
//========================================================================================//

#ifndef OPTION_MENU_VIDEO_H
#define OPTION_MENU_VIDEO_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/SectionedListPanel.h>
#include "GraphicalCheckBox.h"
#include "GraphicalOverlayInset.h"
#include "vgui_base_panel.h"
#include "ComboList.h"
#include "InlineMenuButton.h"

namespace vgui
{
	class OptionMenuVideo;

	class OptionMenuVideo : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuVideo, vgui::CVGUIBasePanel);

	public:
		OptionMenuVideo(vgui::Panel *parent, char const *panelName);
		~OptionMenuVideo();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);
		void ApplyChanges(void);

	private:
		vgui::ImagePanel *m_pDivider[2];
		vgui::Label *m_pTextTitle[2];
		vgui::GraphicalOverlay *m_pSensSlider[2];
		vgui::GraphicalCheckBox *m_pCheckBox[4];

		vgui::ComboList *m_pVideoCombo[2];

		vgui::InlineMenuButton *m_pApplyButton;

		void PrepareResolutionList();
		void SetCurrentResolutionComboItem();

		int m_nSelectedMode;

		MESSAGE_FUNC_PTR_CHARPTR(OnTextChanged, "TextChanged", panel, text);

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // OPTION_MENU_VIDEO_H
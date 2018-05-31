//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Audio Related Options
//
//========================================================================================//

#ifndef OPTION_MENU_AUDIO_H
#define OPTION_MENU_AUDIO_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include "GraphicalCheckBox.h"
#include "GraphicalOverlayInset.h"
#include "vgui_base_panel.h"
#include "ComboList.h"
#include "InlineMenuButton.h"

namespace vgui
{
	class OptionMenuAudio;

	class OptionMenuAudio : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuAudio, vgui::CVGUIBasePanel);

	public:
		OptionMenuAudio(vgui::Panel *parent, char const *panelName);
		~OptionMenuAudio();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);
		void ApplyChanges(void);

	private:
		vgui::Label *m_pTextTitle[3];
		vgui::ImagePanel *m_pDivider[3];
		vgui::GraphicalOverlay *m_pSensSlider[4];
		vgui::ComboList *m_pComboList[2];
		vgui::GraphicalCheckBox *m_pCheckBox[3];

		vgui::InlineMenuButton *m_pApplyButton;

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // OPTION_MENU_AUDIO_H
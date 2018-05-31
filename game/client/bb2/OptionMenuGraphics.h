//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Graphic Related Options
//
//========================================================================================//

#ifndef OPTION_MENU_GRAPHICS_H
#define OPTION_MENU_GRAPHICS_H

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
#include "InlineMenuButton.h"

namespace vgui
{
	class OptionMenuGraphics;

	class OptionMenuGraphics : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuGraphics, vgui::CVGUIBasePanel);

	public:
		OptionMenuGraphics(vgui::Panel *parent, char const *panelName);
		~OptionMenuGraphics();

		void ApplyChanges(void);
		void SetupLayout(void);
		void OnUpdate(bool bInGame);

	private:
		vgui::ImagePanel *m_pDivider[2];
		vgui::Label *m_pTextTitle[2];
		vgui::ComboList *m_pGraphicsCombo[8];
		vgui::GraphicalOverlay *m_pSlider[3];
		vgui::InlineMenuButton *m_pApplyButton;

		struct AAMode_t
		{
			int m_nNumSamples;
			int m_nQualityLevel;
		};

		AAMode_t m_nAAModes[16];
		int m_nNumAAModes;
		void SetComboItemAsRecommended(vgui::ComboBox *combo, int iItem);
		int FindMSAAMode(int nAASamples, int nAAQuality);
		void ApplyChangesToConVar(const char *pConVarName, int value);
		void MarkDefaultSettingsAsRecommended();

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // OPTION_MENU_GRAPHICS_H
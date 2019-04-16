//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Image Based CheckBox.
//
//========================================================================================//

#ifndef GRAPHICAL_CHECKBOX_H
#define GRAPHICAL_CHECKBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>

namespace vgui
{
	class GraphicalCheckBox;
	class GraphicalCheckBox : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(GraphicalCheckBox, vgui::Panel);

	public:
		GraphicalCheckBox(vgui::Panel *parent, char const *panelName, const char *text, const char *fontName = "MainMenuTextBig", bool bDisableInput = false);
		~GraphicalCheckBox();

		void SetEnabled(bool state);
		bool IsChecked() { return m_bIsChecked; }
		void SetCheckedStatus(bool bStatus);
		Label *GetTitleLabel() { return m_pLabelTitle; }

	private:

		bool m_bIsChecked;
		bool m_bDisableInput;
		char szFont[32];
		vgui::Label *m_pLabelTitle;
		vgui::ImagePanel *m_pCheckImg;
		vgui::Button *m_pButton;

		const char *GetCheckImage(int iValue);

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
		void OnCommand(const char* pcCommand);
	};
}

#endif // GRAPHICAL_CHECKBOX_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Combo List - Similar to combo box but more visually appealing. 
//
//========================================================================================//

#ifndef COMBO_LIST_H
#define COMBO_LIST_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/ComboBox.h>

namespace vgui
{
	class ComboList;

	class ComboList : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ComboList, vgui::Panel);

	public:
		ComboList(vgui::Panel *parent, char const *panelName, const char *text, int items);
		~ComboList();

		vgui::ComboBox *GetComboBox() { return m_pComboBox; }
		void SetEnabled(bool state);

	private:
		vgui::Button *m_pButton;
		vgui::Label *m_pLabel;
		vgui::ComboBox *m_pComboBox;

	protected:

		void OnCommand(const char *pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // COMBO_LIST_H
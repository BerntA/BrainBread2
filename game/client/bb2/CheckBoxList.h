//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Check Box List - A list of check box controls.
//
//========================================================================================//

#ifndef CHECK_BOX_LIST_H
#define CHECK_BOX_LIST_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/ScrollBar.h>
#include "GraphicalCheckBox.h"
#include <vgui/MouseCode.h>

namespace vgui
{
	class CheckBoxList;

	struct CheckBoxListItem
	{
		GraphicalCheckBox *boxGUI;
		int m_iPlayerIndex;
	};

	class CheckBoxList : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CheckBoxList, vgui::Panel);

	public:
		CheckBoxList(vgui::Panel *parent, char const *panelName);
		~CheckBoxList();

		void Reset(void);
		void AddItemToList(const char *szText, int index = -1);
		bool HasItems() { return (checkBoxList.Count()); }
		CUtlVector<CheckBoxListItem> checkBoxList;

	private:
		int m_iCurrentIndex;

	protected:

		void OnMouseWheeled(int delta);
		void OnThink();
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // CHECK_BOX_LIST_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Draw multiple labels in one control, properly positioning and so forth. Allowing multiple colors.
//
//========================================================================================//

#ifndef MULTILABEL_H
#define MULTILABEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui/ILocalize.h>

struct MultiLabel_Item_t
{
	vgui::Label *m_pLabel;
	Color fgColor;
};

namespace vgui
{
	class MultiLabel;
	class MultiLabel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(MultiLabel, vgui::Panel);

	public:
		MultiLabel(vgui::Panel *parent, char const *panelName, const char *fontName);
		~MultiLabel();

		// Apply split color, decide new pos, size, etc for newly split label.
		void SetTextColorSegmented(const char *szText[], Color textColor[], int iSize);

		// Apply Changes
		void UpdateText(const char *szText[]);

		// Refresh/Delete/Reset...
		void DeleteAll();

		vgui::HFont GetFont() { return m_hFont; }

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();

	private:
		char szFont[32];
		CUtlVector<MultiLabel_Item_t> pszTextSplits;
		vgui::HFont m_hFont;
	};
}
#endif //MULTILABEL
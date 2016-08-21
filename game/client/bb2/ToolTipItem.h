//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Tool Tip GUI Control
//
//========================================================================================//

#ifndef TOOLTIPITEM_H
#define TOOLTIPITEM_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>

namespace vgui
{
	class ToolTipItem;

	class ToolTipItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ToolTipItem, vgui::Panel);

	public:

		ToolTipItem(vgui::Panel *parent, char const *panelName);
		~ToolTipItem();

		virtual void SetSize(int wide, int tall);

		void SetToolTip(const char *szTitle, const char *szDescription);
		int GetSizeOfString(const char *szString);

	private:
		HFont defaultFont;

		vgui::ImagePanel *m_pBackground;
		vgui::Label *m_pInfoTitle;
		vgui::RichText *m_pInfoDesc;

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // TOOLTIPITEM_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Detail Panel - Draws the information for a quest in the quest list, such as objectives, finished objectives, etc...
//
//========================================================================================//

#ifndef QUEST_DETAIL_PANEL_H
#define QUEST_DETAIL_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/RichText.h>
#include "GraphicalCheckBox.h"

namespace vgui
{
	class QuestDetailPanel;
	class QuestDetailPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(QuestDetailPanel, vgui::Panel);

	public:
		QuestDetailPanel(vgui::Panel* parent, char const* panelName);
		virtual ~QuestDetailPanel();

		virtual void SetupLayout(void);
		virtual bool SelectID(int index);
		virtual void SetSize(int wide, int tall);
		virtual void Cleanup(void);

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
		virtual void PerformLayout();

	private:
		vgui::ImagePanel* m_pDividerTitle;
		vgui::ImagePanel* m_pDividerObjectives;

		vgui::Label* m_pLabelTitle;
		vgui::Label* m_pLabelObjectives;
		vgui::RichText* m_pTextDescription;

		Color m_colCheckBox;
		CUtlVector<GraphicalCheckBox*> pszObjectives;
	};
}

#endif // QUEST_DETAIL_PANEL_H
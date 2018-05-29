//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Detail Panel - Draws the information for a quest in the quest list, such as objectives, finished objectives, etc...
// We also draw the quests available in a list and side quests.
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
#include "QuestList.h"

namespace vgui
{
	class QuestDetailPanel;
	class QuestDetailPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(QuestDetailPanel, vgui::Panel);

	public:
		QuestDetailPanel(vgui::Panel *parent, char const *panelName);
		virtual ~QuestDetailPanel();

		virtual void SetupLayout(void);
		virtual void ChooseQuestItem(int index);
		virtual void SetSize(int wide, int tall);
		virtual void Cleanup(void);

	protected:

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();

	private:

		vgui::ImagePanel *m_pImagePreview;
		vgui::ImagePanel *m_pImageDivider[2];
		vgui::ImagePanel *m_pQuestDivider[2];
		vgui::Label *m_pQuestListLabel[2];
		vgui::Divider *m_pDivider;

		vgui::Label *m_pLabelTitle; // Main title for image preview.
		vgui::Label *m_pLabelHeader; // Main header.
		vgui::RichText *m_pTextDescription;
		vgui::Label *m_pLabelObjectives;

		vgui::QuestList *m_pQuestLists[2];

		CUtlVector<GraphicalCheckBox*> pszObjectives;
	};
}

#endif // QUEST_DETAIL_PANEL_H
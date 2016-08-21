//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest List - Keeps track of all Quest Item objects. 
//
//========================================================================================//

#ifndef QUEST_LIST_H
#define QUEST_LIST_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include "QuestItem.h"

namespace vgui
{
	class QuestList;
	class QuestList : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(QuestList, vgui::Panel);

	public:
		QuestList(vgui::Panel *parent, char const *panelName, bool bSideQuest);
		virtual ~QuestList();

		void Cleanup(void);
		void CreateList(void);
		void UpdateLayout(void);
		void SetActiveIndex(int index);
		int GetFirstItem(void);

	protected:

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void OnMouseWheeled(int delta);

	private:

		bool m_bSideQuest;
		int m_iScrollOffset;
		CUtlVector<QuestItem*> pszQuestItems;
	};
}

#endif // QUEST_LIST_H
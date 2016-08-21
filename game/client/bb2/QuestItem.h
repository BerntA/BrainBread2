//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Item - This controls indicates the progress of a quest (by color) and also sets the quest detail panel's active quest index when this item is clicked. 
// This item is linked to a quest index.
//
//========================================================================================//

#ifndef QUEST_ITEM_H
#define QUEST_ITEM_H

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
	class QuestItem;
	class QuestItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(QuestItem, vgui::Panel);

	public:
		QuestItem(vgui::Panel *parent, char const *panelName, const char *title, int iStatus, int iIndex);
		virtual ~QuestItem();

		int GetIndex(void) { return m_iIndex; }
		void SetActive(bool bValue) { m_bIsActive = bValue; }

		virtual void SetSize(int wide, int tall);

	protected:

		virtual void OnThink();
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void OnCommand(const char *pcCommand);

	private:

		int m_iIndex;
		bool m_bIsActive;
		vgui::Button *m_pButton;
		vgui::ImagePanel *m_pBackground;
		vgui::Label *m_pLabelTitle;
	};
}

#endif // QUEST_ITEM_H
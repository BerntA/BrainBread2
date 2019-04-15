//=========       Copyright © Reperio Studios 2015-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the extra objectives, also known as quests. Completing quests give more XP than completing base objectives.
//
//=============================================================================================//

#ifndef BASE_PANEL_H
#define BASE_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_base_frame.h"
#include "QuestDetailPanel.h"
#include "QuestList.h"
#include <vgui_controls/Divider.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>

class CQuestPanel : public vgui::CVGUIBaseFrame
{
	DECLARE_CLASS_SIMPLE(CQuestPanel, vgui::CVGUIBaseFrame);

public:
	CQuestPanel(vgui::VPANEL parent);
	~CQuestPanel();

	virtual void OnShowPanel(bool bShow);
	virtual void OnSelectQuest(int index);
	virtual void UpdateSelectedQuest(void);

protected:

	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void UpdateLayout(void);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void PaintBackground();

private:

	MESSAGE_FUNC_PARAMS(OnDropInvItem, "DropItem", data);

	vgui::ImagePanel* m_pPageDivider;
	vgui::Label* m_pPageTitle;
	vgui::Label* m_pLabelWarning;
	vgui::QuestDetailPanel* m_pQuestDetailList;
	vgui::QuestList* m_pQuestLists;

	int m_iSelectedQuestIdx;
};

extern CQuestPanel* g_pQuestPanel;

#endif // BASE_PANEL_H
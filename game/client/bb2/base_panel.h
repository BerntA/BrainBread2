//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: The base vgui for humans and zombie, this panel will contain the quest, char overview and inventory. However we might add the skill tree here as well.
//
//========================================================================================//

#ifndef BASE_PANEL_H
#define BASE_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_base_frame.h"
#include "InventoryItem.h"
#include "QuestDetailPanel.h"
#include "CharacterStatPreview.h"
#include <vgui_controls/Divider.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>

class CBaseGamePanel : public vgui::CVGUIBaseFrame
{
	DECLARE_CLASS_SIMPLE(CBaseGamePanel, vgui::CVGUIBaseFrame);

public:
	CBaseGamePanel(vgui::VPANEL parent);
	~CBaseGamePanel();

	virtual void OnShowPanel(bool bShow);
	void OnSelectQuest(int index);

protected:

	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void UpdateLayout(void);
	virtual void OnCommand(const char *pcCommand);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);

private:

	MESSAGE_FUNC_PARAMS(OnDropInvItem, "DropItem", data);

	int m_iCurrentPage;
	void SetPage();

	vgui::ImagePanel *m_pBackground;
	vgui::Label *m_pPageTitle;
	vgui::Label *m_pLabelWarning;
	vgui::Divider *m_pTitleDivider;
	vgui::Divider *m_pNavigationDivider;

	vgui::Button *m_pButton[2];
	vgui::ImagePanel *m_pImage[2];
	vgui::Label *m_pButtonText[2];

	// Inventory
	vgui::InventoryItem *m_pInventoryItem[12];

	// Quest
	vgui::QuestDetailPanel *m_pQuestDetailList;
};

#endif // BASE_PANEL_H
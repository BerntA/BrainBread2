//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays Global Scores - Stat Tracking for all players. Global Scores...
//
//========================================================================================//

#ifndef PLAY_MENU_SCOREBOARD_H
#define PLAY_MENU_SCOREBOARD_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/SectionedListPanel.h>
#include "GraphicalCheckBox.h"
#include "GraphicalOverlayInset.h"
#include <vgui_controls/ListPanel.h>
#include "LeaderboardItem.h"
#include "vgui_base_panel.h"

namespace vgui
{
	class PlayMenuScoreboard;
	class PlayMenuScoreboard : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(PlayMenuScoreboard, vgui::CVGUIBasePanel);

	public:
		PlayMenuScoreboard(vgui::Panel *parent, char const *panelName);
		~PlayMenuScoreboard();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);

		void RefreshScores(void);
		void RefreshCallback(int iItems);

		void AddScoreItem(const char *pszNickName, const char *pszSteamID, int32 plLevel, int32 plKills, int32 plDeaths, int iIndex);

		int GetMaxPages() { return m_iPageNum; }
		int GetCurrentPage() { return m_iCurrPage; }

	private:
		vgui::ImagePanel *m_pImgArrowBox[2];
		vgui::Button *m_pBtnArrowBox[2];

		vgui::Label *m_pGridDetail[5];
		vgui::LeaderboardItem *m_pScoreItem[5];

		int m_iPageNum;
		int m_iCurrPage;

	protected:
		virtual void OnKeyCodeTyped(KeyCode code);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void OnCommand(const char* pcCommand);
	};
}

#endif // PLAY_MENU_SCOREBOARD_H
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Scoreboard Team Section Handler
//
//========================================================================================//

#ifndef SCOREBOARD_SECTION_PANEL_H
#define SCOREBOARD_SECTION_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include "ScoreboardItem.h"

namespace vgui
{
	class CScoreBoardSectionPanel;
	class CScoreBoardSectionPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CScoreBoardSectionPanel, vgui::Panel);

	public:
		CScoreBoardSectionPanel(vgui::Panel *parent, char const *panelName, int targetTeam);
		~CScoreBoardSectionPanel();

		void OnThink();
		void PerformLayout();
		void Cleanup();
		void UpdateScoreInfo();
		void GetScoreInfo(int playerIndex, KeyValues *kv);

	private:
		float m_flLastUpdate;
		int m_iTeamLink;

		vgui::ImagePanel *m_pBackground;
		vgui::ImagePanel *m_pBanner;

		vgui::Label *m_pSectionInfo[5];

		CUtlVector<vgui::ScoreboardItem*> m_pItems;
		int GetIndexFromScoreItemList(int playerIndex);

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // SCOREBOARD_SECTION_PANEL_H
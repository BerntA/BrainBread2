//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Scoreboard Item - Adds to the scoreboard. Contains player score, stats, etc...
//
//========================================================================================//

#ifndef SCOREBOARD_ITEM_H
#define SCOREBOARD_ITEM_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "vgui_avatarimage.h"
#include "MultiLabel.h"

namespace vgui
{
	class ScoreboardItem;
	class ScoreboardItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ScoreboardItem, vgui::Panel);

	public:
		ScoreboardItem(vgui::Panel *parent, char const *panelName, KeyValues *pkvPlayerData);
		~ScoreboardItem();

		int GetPlayerIndex() { return m_iPlayerIndex; }
		int GetPlayerScore() { return m_iTotalScore; }

		void UpdateItem(KeyValues *pkvPlayerData);
		void SetSize(int wide, int tall);

	private:

		int m_iPlayerIndex;
		int m_iTotalScore;
		int m_iSectionTeam;

		bool m_bRollover;
		bool m_bIsLocalPlayer;
		bool m_bIsBot;

		Color GetPingColorState(int latency);		

		CAvatarImage *m_pAvatarIMG;
		vgui::Button *m_pButton;
		vgui::ImagePanel *m_pImgBackground;
		vgui::ImagePanel *m_pSteamAvatar;
		vgui::ImagePanel *m_pSkullIcon;

		vgui::MultiLabel *m_pUserName;
		vgui::Label *m_pLevel;
		vgui::Label *m_pKills;
		vgui::Label *m_pDeaths;
		vgui::Label *m_pPing;

		vgui::Button *m_pMuteButton;
		vgui::ImagePanel *m_pImgMuteStatus;

		char szSteamID[80];

		float m_flSteamImgThink;

	protected:

		virtual void OnCommand(const char* pcCommand);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void OnThink();
	};
}

#endif // SCOREBOARD_ITEM_H
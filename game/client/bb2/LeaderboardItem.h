//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Global Scoreboard Item
//
//========================================================================================//

#ifndef LEADERBOARD_ITEM_H
#define LEADERBOARD_ITEM_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "vgui_avatarimage.h"

namespace vgui
{
	class LeaderboardItem;
	class LeaderboardItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(LeaderboardItem, vgui::Panel);

	public:
		LeaderboardItem(vgui::Panel *parent, char const *panelName, const char *pszPlayerName, const char *pszSteamID, int32 plLevel, int32 plKills, int32 plDeaths);
		~LeaderboardItem();

		virtual void SetSize(int wide, int tall);
		const char *GetSteamID(void) { return szSteamID; }

	private:

		CAvatarImage *m_pAvatarIMG;
		vgui::Button *m_pButton;
		vgui::ImagePanel *m_pImgBackground;
		vgui::ImagePanel *m_pSteamAvatar;

		vgui::Label *m_pLevel;
		vgui::Label *m_pUserName;
		vgui::Label *m_pKills;
		vgui::Label *m_pDeaths;

		char szSteamID[80];
		float m_flSteamImgThink;

	protected:

		virtual void OnCommand(const char* pcCommand);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void OnThink();
	};
}

#endif // LEADERBOARD_ITEM_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Show top 4 players GUI! Finds the 4 top scorers and theirs steam avatars.
//
//========================================================================================//

#ifndef TOPPLAYERS_PANEL_H
#define TOPPLAYERS_PANEL_H

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
#include "vgui_avatarimage.h"

namespace vgui
{
	class TopPlayersPanel;

	class TopPlayersPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(TopPlayersPanel, vgui::Panel);

	public:
		TopPlayersPanel(vgui::Panel *parent, char const *panelName);
		~TopPlayersPanel();

		bool FindTopPlayers(void);

		bool FindTopPlayersElimination(void);
		bool FindTopPlayersObjective(void);
		bool FindTopPlayersArena(void);
		bool FindTopPlayersDeathmatch(void);

		void Reset();

	private:
		vgui::ImagePanel *m_pImageAvatars[4];
		vgui::ImagePanel *m_pImageStars[4];

		vgui::Label *m_pLabelInfo[2];
		vgui::Label *m_pLabelNames[4];

		CAvatarImage *m_pSteamAvatars[4];

	protected:
		void OnThink();
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // TOPPLAYERS_PANEL_H
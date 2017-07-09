//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles displaying available maps to play, showing info about them and so forth.
//
//========================================================================================//

#ifndef PLAY_MENU_CREATE_GAME_H
#define PLAY_MENU_CREATE_GAME_H

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
#include "ExoticImageButton.h"
#include "MapSelectionPanel.h"
#include "ServerSettingsPanel.h"
#include "vgui_base_panel.h"
#include "AnimatedMenuButton.h"
#include <vgui_controls/Tooltip.h>

namespace vgui
{
	class PlayMenuCreateGame;

	class PlayMenuCreateGame : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(PlayMenuCreateGame, vgui::CVGUIBasePanel);

	public:
		PlayMenuCreateGame(vgui::Panel *parent, char const *panelName);
		~PlayMenuCreateGame();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);
		void OnShowPanel(bool bShow);

		// Controls
		vgui::MapSelectionPanel *m_pMapPanel;
		vgui::ServerSettingsPanel *m_pServerPanel;

		vgui::ImagePanel *m_pDividers[2];
		vgui::Label *m_pTitles[2];
		vgui::Label *m_pExtraInfo;
		vgui::RichText *m_pMapDescription;

		vgui::AnimatedMenuButton *m_pPlayButton;

		vgui::TextTooltip *m_pGamemodeToolTip;

	private:
		CPanelAnimationVar(float, m_flLoadMapTimer, "LoadMapTimer", "0.0f");

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void OnCommand(const char* pcCommand);
	};
}

#endif // PLAY_MENU_CREATE_GAME_H
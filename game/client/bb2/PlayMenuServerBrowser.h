//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Server Browser : Displays all available servers for BB2 - allows custom filtering as well.
//
//========================================================================================//

#ifndef PLAY_MENU_SERVER_BROWSER_H
#define PLAY_MENU_SERVER_BROWSER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Tooltip.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/CheckButton.h>
#include "vgui_base_panel.h"
#include "ExoticImageButton.h"
#include "InlineMenuButton.h"
#include "OverlayButton.h"
#include <steam/steam_api.h>
#include <steam/isteamapps.h>

namespace vgui
{
	class PlayMenuServerBrowser;
	class PlayMenuServerBrowser : public vgui::CVGUIBasePanel, public ISteamMatchmakingPlayersResponse
	{
		DECLARE_CLASS_SIMPLE(PlayMenuServerBrowser, vgui::CVGUIBasePanel);

	public:
		PlayMenuServerBrowser(vgui::Panel *parent, char const *panelName);
		~PlayMenuServerBrowser();

		void OnUpdate(bool bInGame);
		void OnShowPanel(bool bShow);
		void SetupLayout(void);
		void RefreshServerList(int type);
		void HandleFavoriteStateOfServer(void);

		void ResetFilters();
		void ToggleFilters();
		bool IsFiltersVisible() { return m_pButtons[(_ARRAYSIZE(m_pButtons) - 1)]->IsSelected(); }
		void ShouldLaunchWithPassword(bool bValue);
		void AddServerToList(gameserveritem_t *pGameServerItem);
		void RefreshComplete(void);
		bool IsServerInList(const char *address);

		// Player List Fetching:
		void AddPlayerToList(const char *pchName, int nScore, float flTimePlayed);
		void PlayersFailedToRespond();
		void PlayersRefreshComplete();

	private:

		int m_iImageListIndexes[6];

		bool m_bFilterNoEmpty;
		bool m_bFilterNoFull;
		bool m_bFilterNoPassword;

		int m_iFilterGamemode;
		int m_iFilterPlayerProfile;
		int m_iFilterPing;

		int m_iCurrentSearchMode;

		vgui::Label *m_pPasswordContextInfo;
		vgui::TextEntry *m_pPasswordText;
		vgui::ImagePanel *m_pPasswordContextBG;
		vgui::InlineMenuButton *m_pPasswordContextButton[2];

		vgui::ImagePanel *m_pProfileIcon;
		vgui::ImagePanel *m_pBanListIcon;
		vgui::ImagePanel *m_pPasswordIcon;
		vgui::ImagePanel *m_pRefreshIcon;
		vgui::Label *m_pServerInfo;
		vgui::ImagePanel *m_pImgBG;
		vgui::ImagePanel *m_pImgServerBG;

		vgui::OverlayButton *m_pButtons[9];

		vgui::Button *m_pServerBrowserSectionInfo[5];
		vgui::SectionedListPanel *m_pServerList;
		vgui::SectionedListPanel *m_pPlayerList;
		vgui::ImageList *m_pImageList;
		vgui::TextTooltip *m_pTooltipItem;
		vgui::TextTooltip *m_pTooltipItemLong;

		// Filtering GUI
		vgui::ImagePanel *m_pFilterBG;
		vgui::ComboBox *m_pFilterComboChoices[3];
		vgui::Label *m_pFilterComboInfo[3];
		vgui::CheckButton *m_pFilterCheckButtons[3];
		vgui::TextEntry *m_pFilterMap;
		vgui::Label *m_pFilterMapInfo;

		// Handle Data:
		int m_nServers;
		int m_nPlayers;

		MESSAGE_FUNC_PARAMS(OnSelectItem, "ItemSelected", data);
		MESSAGE_FUNC_PARAMS(OnItemDoubleClicked, "ItemDoubleLeftClick", data);

		HServerQuery pPlayerInfoRequest;

	protected:
		virtual void OnCommand(const char* pcCommand);
		virtual void OnKeyCodeTyped(KeyCode code);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

		static bool StaticServerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);
		static bool StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);
	};
}

#endif // PLAY_MENU_SERVER_BROWSER_H
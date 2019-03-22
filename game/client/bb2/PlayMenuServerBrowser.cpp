//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Server Browser : Displays all available servers for BB2 - allows custom filtering as well.
//
//========================================================================================//

#include "cbase.h"
#include "PlayMenuServerBrowser.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Client.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define SERVER_NAV_WIDTH scheme()->GetProportionalScaledValue(90)
#define SERVER_NAV_HEIGHT scheme()->GetProportionalScaledValue(24)
#define SERVER_NAV_HEIGHT_BROWSER scheme()->GetProportionalScaledValue(34)
#define SERVER_FILTERS_HEIGHT scheme()->GetProportionalScaledValue(70)
#define SERVER_PLAYER_LIST_WIDE scheme()->GetProportionalScaledValue(150)

static int m_iCurrentSortMethod = 0;

enum serverSortMethods_t
{
	SORT_NONE = 0,

	SORT_HOSTNAME_DESCENDING,
	SORT_HOSTNAME_ASCENDING,

	SORT_GAMEMODE_DESCENDING,
	SORT_GAMEMODE_ASCENDING,

	SORT_PLAYER_COUNT_DESCENDING,
	SORT_PLAYER_COUNT_ASCENDING,

	SORT_PING_DESCENDING,
	SORT_PING_ASCENDING,

	SORT_MAP_DESCENDING,
	SORT_MAP_ASCENDING,
};

PlayMenuServerBrowser::PlayMenuServerBrowser(vgui::Panel *parent, char const *panelName)
	: BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pTooltipItem = new vgui::TextTooltip(this, "");
	m_pTooltipItem->SetTooltipDelay(200);
	m_pTooltipItem->SetEnabled(true);
	m_pTooltipItem->SetTooltipFormatToSingleLine();

	m_pTooltipItemLong = new vgui::TextTooltip(this, "");
	m_pTooltipItemLong->SetTooltipDelay(200);
	m_pTooltipItemLong->SetEnabled(true);
	m_pTooltipItemLong->SetTooltipFormatToMultiLine();

	m_pServerList = vgui::SETUP_PANEL(new vgui::SectionedListPanel(this, "ServerBrowser"));
	m_pServerList->SetZPos(250);
	m_pServerList->SetVisible(true);
	m_pServerList->AddActionSignalTarget(this);
	m_pServerList->SetBorder(NULL);
	m_pServerList->SetPaintBorderEnabled(false);
	m_pServerList->SetDrawHeaders(false);
	m_pServerList->AddSection(0, "", StaticServerSortFunc);
	m_pServerList->SetSectionAlwaysVisible(0);

	const char *pchSectionInfoForBrowser[] =
	{
		"#GameUI_ServerBrowser_Hostname",
		"#GameUI_ServerBrowser_Gamemode",
		"#GameUI_ServerBrowser_Players",
		"#GameUI_ServerBrowser_Ping",
		"#GameUI_ServerBrowser_Map",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pServerBrowserSectionInfo); i++)
	{
		m_pServerBrowserSectionInfo[i] = vgui::SETUP_PANEL(new vgui::Button(this, "ServerSectionInfo", pchSectionInfoForBrowser[i], this, VarArgs("SortList%i", (i + 1))));
		m_pServerBrowserSectionInfo[i]->SetPaintBorderEnabled(false);
		m_pServerBrowserSectionInfo[i]->SetZPos(395);
	}

	m_pServerBrowserSectionInfo[1]->SetTooltip(m_pTooltipItemLong, "#GameUI_ServerBrowser_InfoGamemode");

	m_pPlayerList = vgui::SETUP_PANEL(new vgui::SectionedListPanel(this, "PlayerList"));
	m_pPlayerList->SetZPos(300);
	m_pPlayerList->SetVisible(true);
	m_pPlayerList->SetBorder(NULL);
	m_pPlayerList->SetPaintBorderEnabled(false);
	m_pPlayerList->AddSection(0, "", StaticPlayerSortFunc);
	m_pPlayerList->SetSectionAlwaysVisible(0);

	m_pRefreshIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "RefreshIco"));
	m_pRefreshIcon->SetShouldScaleImage(true);
	m_pRefreshIcon->SetImage("server/loading");
	m_pRefreshIcon->SetZPos(60);

	m_pPasswordIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "PasswordIco"));
	m_pPasswordIcon->SetShouldScaleImage(true);
	m_pPasswordIcon->SetImage("server/padlock");
	m_pPasswordIcon->SetZPos(400);
	m_pPasswordIcon->SetTooltip(m_pTooltipItem, "#GameUI_ServerBrowser_IconPadlock");

	m_pProfileIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ProfileIco"));
	m_pProfileIcon->SetShouldScaleImage(true);
	m_pProfileIcon->SetImage("server/stats_global");
	m_pProfileIcon->SetZPos(400);
	m_pProfileIcon->SetTooltip(m_pTooltipItem, "#GameUI_ServerBrowser_IconStats");

	m_pBanListIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "BanListIco"));
	m_pBanListIcon->SetShouldScaleImage(true);
	m_pBanListIcon->SetImage("server/shield");
	m_pBanListIcon->SetZPos(400);
	m_pBanListIcon->SetTooltip(m_pTooltipItem, "#GameUI_ServerBrowser_IconShield");

	m_pServerInfo = vgui::SETUP_PANEL(new vgui::Label(this, "InfoServers", ""));
	m_pServerInfo->SetContentAlignment(vgui::Label::a_center);
	m_pServerInfo->SetZPos(50);

	m_pImgBG = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ContextBG"));
	m_pImgBG->SetShouldScaleImage(true);
	m_pImgBG->SetImage("server/mainbg");
	m_pImgBG->SetZPos(5);

	const char *szTitles[] =
	{
		"#GameUI_ServerBrowser_Internet",
		"#GameUI_ServerBrowser_Lan",
		"#GameUI_ServerBrowser_History",
		"#GameUI_ServerBrowser_Favorites",
		"#GameUI_ServerBrowser_Friends",
		"#GameUI_ServerBrowser_Refresh",
		"",
		"#GameUI_ServerBrowser_Connect",
		"#GameUI_ServerBrowser_Filters",
	};

	const char *szCommands[] =
	{
		"Internet",
		"LAN",
		"History",
		"Favorites",
		"Friends",
		"Refresh",
		"FavoriteState",
		"Connect",
		"Filters",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
	{
		m_pButtons[i] = vgui::SETUP_PANEL(new vgui::OverlayButton(this, "OverlayButton", szTitles[i], szCommands[i], "OptionTextSmall"));
		m_pButtons[i]->SetZPos(15);
	}

	m_pButtons[(_ARRAYSIZE(m_pButtons) - 1)]->SetToggleMode(true);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 2)]->SetToggleMode(true);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 3)]->SetToggleMode(true);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 4)]->SetToggleMode(true);

	m_pImgServerBG = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ServerBG"));
	m_pImgServerBG->SetShouldScaleImage(true);
	m_pImgServerBG->SetImage("server/serverbg");
	m_pImgServerBG->SetZPos(10);

	// Setup Image List
	m_pImageList = new ImageList(true);
	m_iImageListIndexes[0] = m_pImageList->AddImage(scheme()->GetImage("server/padlock", false));
	m_iImageListIndexes[1] = m_pImageList->AddImage(scheme()->GetImage("server/stats_global", false));
	m_iImageListIndexes[2] = m_pImageList->AddImage(scheme()->GetImage("server/stats_local", false));
	m_iImageListIndexes[3] = m_pImageList->AddImage(scheme()->GetImage("server/vac", false));
	m_iImageListIndexes[4] = m_pImageList->AddImage(scheme()->GetImage("server/shield", false));
	m_iImageListIndexes[5] = m_pImageList->AddImage(scheme()->GetImage("server/shield_vac", false));

	m_pServerList->AddColumnToSection(0, "protected", "", SectionedListPanel::COLUMN_IMAGE | 0, scheme()->GetProportionalScaledValue(20));
	m_pServerList->AddColumnToSection(0, "profile", "", SectionedListPanel::COLUMN_IMAGE | 0, scheme()->GetProportionalScaledValue(20));
	m_pServerList->AddColumnToSection(0, "banlist", "", SectionedListPanel::COLUMN_IMAGE | 0, scheme()->GetProportionalScaledValue(20));
	m_pServerList->AddColumnToSection(0, "hostname", "", 0, scheme()->GetProportionalScaledValue(300));
	m_pServerList->AddColumnToSection(0, "game", "", 0, scheme()->GetProportionalScaledValue(72));
	m_pServerList->AddColumnToSection(0, "players", "", 0, scheme()->GetProportionalScaledValue(34));
	m_pServerList->AddColumnToSection(0, "ping", "", 0, scheme()->GetProportionalScaledValue(34));
	m_pServerList->AddColumnToSection(0, "map", "", 0, scheme()->GetProportionalScaledValue(140));
	m_pPlayerList->AddColumnToSection(0, "name", "#GameUI_ServerBrowser_PlayerName", 0, scheme()->GetProportionalScaledValue(110));
	m_pPlayerList->AddColumnToSection(0, "score", "#GameUI_ServerBrowser_PlayerScore", 0, scheme()->GetProportionalScaledValue(40));

	// Filter Panel:
	m_pFilterBG = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "FilterBG"));
	m_pFilterBG->SetShouldScaleImage(true);
	m_pFilterBG->SetImage("server/filterbg");
	m_pFilterBG->SetZPos(70);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 1)]->SetZPos(80);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 2)]->SetZPos(80);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 3)]->SetZPos(80);
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 4)]->SetZPos(80);

	const char *szText1[] =
	{
		"#GameUI_ServerBrowser_ProfileSaving",
		"#GameUI_ServerBrowser_Gamemode",
		"#GameUI_ServerBrowser_Ping",
	};

	const char *szText2[] =
	{
		"#BROWSER_EMPTY",
		"#BROWSER_FULL",
		"#BROWSER_PASSWORD",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pFilterComboChoices); i++)
	{
		m_pFilterComboChoices[i] = vgui::SETUP_PANEL(new vgui::ComboBox(this, "FilterCombo", 4, false));
		m_pFilterComboInfo[i] = vgui::SETUP_PANEL(new vgui::Label(this, "FilterComboText", ""));
		m_pFilterComboChoices[i]->SetZPos(90);
		m_pFilterComboInfo[i]->SetZPos(80);
		m_pFilterComboInfo[i]->SetContentAlignment(Label::a_center);
		m_pFilterComboInfo[i]->SetText(szText1[i]);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pFilterCheckButtons); i++)
	{
		m_pFilterCheckButtons[i] = vgui::SETUP_PANEL(new vgui::CheckButton(this, "FilterCheck", ""));
		m_pFilterCheckButtons[i]->SetZPos(90);
		m_pFilterCheckButtons[i]->SetText(g_pVGuiLocalize->Find(szText2[i]));
	}

	m_pFilterComboChoices[0]->AddItem("Any", NULL);
	m_pFilterComboChoices[0]->AddItem("Global", NULL);
	m_pFilterComboChoices[0]->AddItem("Local", NULL);
	m_pFilterComboChoices[0]->AddItem("No", NULL);

	m_pFilterComboChoices[1]->AddItem("Any", NULL);
	m_pFilterComboChoices[1]->AddItem("Story", NULL);
	m_pFilterComboChoices[1]->AddItem("Objective", NULL);
	m_pFilterComboChoices[1]->AddItem("Elimination", NULL);
	m_pFilterComboChoices[1]->AddItem("Arena", NULL);
	m_pFilterComboChoices[1]->AddItem("Deathmatch", NULL);

	m_pFilterComboChoices[2]->AddItem("Any", NULL);
	m_pFilterComboChoices[2]->AddItem("< 50", NULL);
	m_pFilterComboChoices[2]->AddItem("< 100", NULL);
	m_pFilterComboChoices[2]->AddItem("< 150", NULL);
	m_pFilterComboChoices[2]->AddItem("< 250", NULL);
	m_pFilterComboChoices[2]->AddItem("< 350", NULL);
	m_pFilterComboChoices[2]->AddItem("< 600", NULL);

	m_pFilterMap = vgui::SETUP_PANEL(new vgui::TextEntry(this, "FilterMap"));
	m_pFilterMap->SetZPos(90);
	m_pFilterMap->SetEditable(true);

	m_pFilterMapInfo = vgui::SETUP_PANEL(new vgui::Label(this, "FilterMapText", ""));
	m_pFilterMapInfo->SetZPos(80);
	m_pFilterMapInfo->SetContentAlignment(Label::a_center);
	m_pFilterMapInfo->SetText("#GameUI_ServerBrowser_Map");

	// Password Panel:
	m_pPasswordContextBG = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "PWBG"));
	m_pPasswordContextBG->SetShouldScaleImage(true);
	m_pPasswordContextBG->SetImage("server/filterbg");
	m_pPasswordContextBG->SetZPos(260);

	m_pPasswordContextInfo = vgui::SETUP_PANEL(new vgui::Label(this, "PassInfo", ""));
	m_pPasswordContextInfo->SetZPos(270);
	m_pPasswordContextInfo->SetContentAlignment(Label::a_center);
	m_pPasswordContextInfo->SetText("#GameUI_ServerBrowser_PasswordRequired");

	m_pPasswordText = vgui::SETUP_PANEL(new vgui::TextEntry(this, "PasswordEntry"));
	m_pPasswordText->SetZPos(280);
	m_pPasswordText->SetEditable(true);

	const char *szPasswordOptions[] =
	{
		"#GameUI_ServerBrowser_Ok",
		"#GameUI_ServerBrowser_Cancel",
	};

	int iCommandsPw[] =
	{
		COMMAND_PASSWORD_OK,
		COMMAND_PASSWORD_CANCEL,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pPasswordContextButton); i++)
	{
		m_pPasswordContextButton[i] = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "PasswordBtn", iCommandsPw[i], szPasswordOptions[i], "BB2_PANEL_SMALL", 0));
		m_pPasswordContextButton[i]->SetZPos(280);
		m_pPasswordContextButton[i]->SetVisible(true);
		m_pPasswordContextButton[i]->AddActionSignalTarget(this);
	}

	m_pServerList->SetImageList(m_pImageList, false);

	// Generic
	pPlayerInfoRequest = NULL;
	m_nServers = 0;
	m_nPlayers = 0;
	m_iCurrentSortMethod = SORT_PLAYER_COUNT_DESCENDING;
	InvalidateLayout();
	PerformLayout();

	ResetFilters();
}

PlayMenuServerBrowser::~PlayMenuServerBrowser()
{
	if (NULL != m_pImageList)
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}

void PlayMenuServerBrowser::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		// Filter GUI
		m_pFilterBG->SetVisible(IsFiltersVisible());
		m_pFilterMap->SetVisible(IsFiltersVisible());
		m_pFilterMapInfo->SetVisible(IsFiltersVisible());
		m_pFilterBG->SetVisible(IsFiltersVisible());

		for (int i = 0; i < _ARRAYSIZE(m_pFilterCheckButtons); i++)
		{
			m_pFilterCheckButtons[i]->SetVisible(IsFiltersVisible());

			m_pFilterComboInfo[i]->SetVisible(IsFiltersVisible());
			m_pFilterComboChoices[i]->SetVisible(IsFiltersVisible());
		}

		for (int i = 0; i < _ARRAYSIZE(m_pPasswordContextButton); i++)
			m_pPasswordContextButton[i]->OnUpdate();

		m_pRefreshIcon->SetVisible(GameBaseShared()->GetSteamServerManager()->IsBusy());

		for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
			m_pButtons[i]->OnUpdate();

		if (GameBaseShared()->GetSteamServerManager()->IsBusy())
			m_pServerInfo->SetText("");
		else if (m_nServers > 0)
		{
			wchar_t message[128];
			wchar_t arg1[10], arg2[10];

			V_swprintf_safe(arg1, L"%i", m_nServers);
			V_swprintf_safe(arg2, L"%i", m_nPlayers);

			g_pVGuiLocalize->ConstructString(message, sizeof(message), g_pVGuiLocalize->Find("#GameUI_ServerBrowser_Found"), 2, arg1, arg2);

			m_pServerInfo->SetText(message);
		}
		else
			m_pServerInfo->SetText("#GameUI_ServerBrowser_Empty");

		m_pServerInfo->SetContentAlignment(vgui::Label::a_center);

		if (m_pImageList)
		{
			for (int i = 0; i < m_pImageList->GetImageCount(); i++)
			{
				m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValue(12), scheme()->GetProportionalScaledValue(12));
				m_pImageList->GetImage(i)->SetPos(0, scheme()->GetProportionalScaledValue(2));
			}
		}

		// Set Filter Values:
		m_bFilterNoEmpty = m_pFilterCheckButtons[0]->IsSelected();
		m_bFilterNoFull = m_pFilterCheckButtons[1]->IsSelected();
		m_bFilterNoPassword = m_pFilterCheckButtons[2]->IsSelected();
		m_iFilterGamemode = m_pFilterComboChoices[1]->GetActiveItem();
		m_iFilterPlayerProfile = m_pFilterComboChoices[0]->GetActiveItem();
		m_iFilterPing = m_pFilterComboChoices[2]->GetActiveItem();
	}
}

void PlayMenuServerBrowser::ResetFilters()
{
	for (int i = 0; i < _ARRAYSIZE(m_pFilterCheckButtons); i++)
		m_pFilterCheckButtons[i]->SetSelected(false);

	for (int i = 0; i < _ARRAYSIZE(m_pFilterComboChoices); i++)
		m_pFilterComboChoices[i]->ActivateItem(0);

	m_bFilterNoEmpty = false;
	m_bFilterNoFull = false;
	m_bFilterNoPassword = false;
	m_iFilterGamemode = 0;
	m_iFilterPlayerProfile = 0;
	m_iFilterPing = 0;
}

void PlayMenuServerBrowser::ToggleFilters()
{
	if (IsFiltersVisible())
		m_pButtons[(_ARRAYSIZE(m_pButtons) - 1)]->Reset();
	else
		m_pButtons[(_ARRAYSIZE(m_pButtons) - 1)]->SetSelection(true);

	int w, h;
	GetSize(w, h);

	if (IsFiltersVisible())
	{
		m_pServerList->SetSize(w - SERVER_PLAYER_LIST_WIDE, h - SERVER_FILTERS_HEIGHT - SERVER_NAV_HEIGHT_BROWSER);
		m_pServerList->SetPos(0, SERVER_NAV_HEIGHT_BROWSER);

		m_pPlayerList->SetSize(SERVER_PLAYER_LIST_WIDE, h - SERVER_FILTERS_HEIGHT - SERVER_NAV_HEIGHT);
		m_pPlayerList->SetPos(w - SERVER_PLAYER_LIST_WIDE, SERVER_NAV_HEIGHT);

		m_pImgServerBG->SetSize(w, h - SERVER_FILTERS_HEIGHT - SERVER_NAV_HEIGHT);
		m_pImgServerBG->SetPos(0, SERVER_NAV_HEIGHT);
	}
	else
	{
		m_pServerList->SetSize(w - SERVER_PLAYER_LIST_WIDE, h - SERVER_NAV_HEIGHT - SERVER_NAV_HEIGHT_BROWSER);
		m_pServerList->SetPos(0, SERVER_NAV_HEIGHT_BROWSER);

		m_pPlayerList->SetSize(SERVER_PLAYER_LIST_WIDE, h - (SERVER_NAV_HEIGHT * 2));
		m_pPlayerList->SetPos(w - SERVER_PLAYER_LIST_WIDE, SERVER_NAV_HEIGHT);

		m_pImgServerBG->SetSize(w, h - (SERVER_NAV_HEIGHT * 2));
		m_pImgServerBG->SetPos(0, SERVER_NAV_HEIGHT);
	}
}

void PlayMenuServerBrowser::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	if (bShow)
		RefreshServerList(0);
}

void PlayMenuServerBrowser::SetupLayout(void)
{
	BaseClass::SetupLayout();

	m_pPasswordContextInfo->SetVisible(false);
	m_pPasswordText->SetVisible(false);
	m_pPasswordContextBG->SetVisible(false);
	m_pPasswordContextButton[0]->SetVisible(false);
	m_pPasswordContextButton[1]->SetVisible(false);

	if (!IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
			m_pButtons[i]->Reset();

		m_pButtons[0]->SetSelection(true);
	}

	int w, h, fx, fy, fw, fh;
	GetSize(w, h);

	m_pServerInfo->SetSize(scheme()->GetProportionalScaledValue(120), scheme()->GetProportionalScaledValue(24));
	m_pServerInfo->SetPos(w - scheme()->GetProportionalScaledValue(170), 0);

	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
	{
		m_pButtons[i]->SetSize(SERVER_NAV_WIDTH, SERVER_NAV_HEIGHT);
		if (i < (_ARRAYSIZE(m_pButtons) - 1))
			m_pButtons[i]->SetPos((i * SERVER_NAV_WIDTH), 0);
		else
			m_pButtons[i]->SetPos(0, h - SERVER_NAV_HEIGHT);
	}

	m_pButtons[_ARRAYSIZE(m_pButtons) - 2]->SetPos(w - SERVER_NAV_WIDTH, h - SERVER_NAV_HEIGHT);
	m_pButtons[_ARRAYSIZE(m_pButtons) - 3]->SetPos(w - (SERVER_NAV_WIDTH * 2), h - SERVER_NAV_HEIGHT);
	m_pButtons[_ARRAYSIZE(m_pButtons) - 4]->SetPos(w - (SERVER_NAV_WIDTH * 3), h - SERVER_NAV_HEIGHT);

	m_pFilterBG->SetSize(w, scheme()->GetProportionalScaledValue(70));
	m_pFilterBG->SetPos(0, h - scheme()->GetProportionalScaledValue(70));
	m_pFilterBG->GetPos(fx, fy);
	fx += SERVER_NAV_WIDTH;
	m_pFilterBG->GetSize(fw, fh);
	fw -= SERVER_NAV_WIDTH;

	m_pFilterMapInfo->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(16));
	m_pFilterMapInfo->SetPos(fx + scheme()->GetProportionalScaledValue(130), fy + scheme()->GetProportionalScaledValue(42));
	m_pFilterMap->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(16));
	m_pFilterMap->SetPos(fx + scheme()->GetProportionalScaledValue(230), fy + scheme()->GetProportionalScaledValue(42));
	m_pFilterMap->SetPaintBorderEnabled(false);

	for (int i = 0; i < _ARRAYSIZE(m_pFilterCheckButtons); i++)
	{
		m_pFilterCheckButtons[i]->SetSize(scheme()->GetProportionalScaledValue(120), scheme()->GetProportionalScaledValue(17));
		m_pFilterCheckButtons[i]->SetPos(fx + scheme()->GetProportionalScaledValue(4), fy + scheme()->GetProportionalScaledValue(5) + (scheme()->GetProportionalScaledValue(19) * i));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pFilterComboInfo); i++)
	{
		m_pFilterComboInfo[i]->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(17));
		m_pFilterComboInfo[i]->SetPos(fx + scheme()->GetProportionalScaledValue(130), fy + scheme()->GetProportionalScaledValue(5) + (scheme()->GetProportionalScaledValue(19) * i));
		m_pFilterComboChoices[i]->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(17));
		m_pFilterComboChoices[i]->SetPos(fx + scheme()->GetProportionalScaledValue(230), fy + scheme()->GetProportionalScaledValue(5) + (scheme()->GetProportionalScaledValue(19) * i));

		if (m_pFilterComboChoices[i]->GetMenu())
			m_pFilterComboChoices[i]->GetMenu()->SetPaintBorderEnabled(false);
	}

	m_pFilterComboInfo[2]->SetPos(fx + scheme()->GetProportionalScaledValue(320), fy + scheme()->GetProportionalScaledValue(5));
	m_pFilterComboChoices[2]->SetPos(fx + scheme()->GetProportionalScaledValue(420), fy + scheme()->GetProportionalScaledValue(5));

	m_pServerList->SetSize(w - SERVER_PLAYER_LIST_WIDE, h - SERVER_NAV_HEIGHT - SERVER_NAV_HEIGHT_BROWSER);
	m_pServerList->SetPos(0, SERVER_NAV_HEIGHT_BROWSER);

	m_pPlayerList->SetSize(SERVER_PLAYER_LIST_WIDE, h - (SERVER_NAV_HEIGHT * 2));
	m_pPlayerList->SetPos(w - SERVER_PLAYER_LIST_WIDE, SERVER_NAV_HEIGHT);

	m_pImgServerBG->SetSize(w, h - (SERVER_NAV_HEIGHT * 2));
	m_pImgServerBG->SetPos(0, SERVER_NAV_HEIGHT);

	m_pRefreshIcon->SetSize(scheme()->GetProportionalScaledValue(12), scheme()->GetProportionalScaledValue(12));
	m_pRefreshIcon->SetPos(w - scheme()->GetProportionalScaledValue(14), 6);

	m_pImgBG->SetPos(0, 0);
	m_pImgBG->SetSize(w, h);

	int sw, sh, sx, sy;
	m_pServerList->GetPos(sw, sh);
	m_pServerList->GetSize(sx, sy);

	m_pPasswordContextBG->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(70));
	m_pPasswordContextBG->SetPos(sw + (sx / 2) - (scheme()->GetProportionalScaledValue(150) / 2), (h / 2) - (scheme()->GetProportionalScaledValue(70) / 2));

	int pwx, pwy, pww, pwh;
	m_pPasswordContextBG->GetSize(pww, pwh);
	m_pPasswordContextBG->GetPos(pwx, pwy);

	for (int i = 0; i < _ARRAYSIZE(m_pPasswordContextButton); i++)
		m_pPasswordContextButton[i]->SetSize(scheme()->GetProportionalScaledValue(32), scheme()->GetProportionalScaledValue(40));

	m_pPasswordContextButton[0]->SetPos(pwx + (pww / 2) - scheme()->GetProportionalScaledValue(32), pwy + scheme()->GetProportionalScaledValue(36));
	m_pPasswordContextButton[1]->SetPos(pwx + (pww / 2), pwy + scheme()->GetProportionalScaledValue(36));

	m_pPasswordContextInfo->SetSize(pww, scheme()->GetProportionalScaledValue(18));
	m_pPasswordContextInfo->SetPos(pwx, pwy);

	m_pPasswordText->SetTextHidden(true);
	m_pPasswordText->SetMaximumCharCount(-1);
	m_pPasswordText->SetSize(scheme()->GetProportionalScaledValue(120), scheme()->GetProportionalScaledValue(16));
	m_pPasswordText->SetPos(pwx + (pww / 2) - scheme()->GetProportionalScaledValue(60), pwy + scheme()->GetProportionalScaledValue(18));
	m_pPasswordText->SetPaintBorderEnabled(false);

	// Icons
	m_pPasswordIcon->SetSize(scheme()->GetProportionalScaledValue(10), scheme()->GetProportionalScaledValue(10));
	m_pPasswordIcon->SetPos(sw + scheme()->GetProportionalScaledValue(5), SERVER_NAV_HEIGHT);

	m_pProfileIcon->SetSize(scheme()->GetProportionalScaledValue(10), scheme()->GetProportionalScaledValue(10));
	m_pProfileIcon->SetPos(sw + scheme()->GetProportionalScaledValue(22), SERVER_NAV_HEIGHT);

	m_pBanListIcon->SetSize(scheme()->GetProportionalScaledValue(10), scheme()->GetProportionalScaledValue(10));
	m_pBanListIcon->SetPos(sw + scheme()->GetProportionalScaledValue(43), SERVER_NAV_HEIGHT);

	m_pServerBrowserSectionInfo[0]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(10));
	m_pServerBrowserSectionInfo[1]->SetSize(scheme()->GetProportionalScaledValue(72), scheme()->GetProportionalScaledValue(10));
	m_pServerBrowserSectionInfo[2]->SetSize(scheme()->GetProportionalScaledValue(34), scheme()->GetProportionalScaledValue(10));
	m_pServerBrowserSectionInfo[3]->SetSize(scheme()->GetProportionalScaledValue(34), scheme()->GetProportionalScaledValue(10));
	m_pServerBrowserSectionInfo[4]->SetSize(scheme()->GetProportionalScaledValue(140), scheme()->GetProportionalScaledValue(10));

	m_pServerBrowserSectionInfo[0]->SetPos(sw + scheme()->GetProportionalScaledValue(60), SERVER_NAV_HEIGHT);
	m_pServerBrowserSectionInfo[1]->SetPos(sw + scheme()->GetProportionalScaledValue(360), SERVER_NAV_HEIGHT);
	m_pServerBrowserSectionInfo[2]->SetPos(sw + scheme()->GetProportionalScaledValue(432), SERVER_NAV_HEIGHT);
	m_pServerBrowserSectionInfo[3]->SetPos(sw + scheme()->GetProportionalScaledValue(466), SERVER_NAV_HEIGHT);
	m_pServerBrowserSectionInfo[4]->SetPos(sw + scheme()->GetProportionalScaledValue(500), SERVER_NAV_HEIGHT);
}

void PlayMenuServerBrowser::ShouldLaunchWithPassword(bool bValue)
{
	m_pPasswordContextInfo->SetVisible(false);
	m_pPasswordText->SetVisible(false);
	m_pPasswordContextBG->SetVisible(false);
	m_pPasswordContextButton[0]->SetVisible(false);
	m_pPasswordContextButton[1]->SetVisible(false);

	if (bValue)
	{
		if ((m_pServerList->GetSelectedItem() != -1))
		{
			KeyValues *pkvData = m_pServerList->GetItemData(m_pServerList->GetSelectedItem());

			char szPassword[256];
			m_pPasswordText->GetText(szPassword, 256);

			const char *szMap = pkvData->GetString("map");
			const char *szConnection = pkvData->GetString("ip");

			GameBaseClient->RunMap(szMap, szConnection, szPassword);
		}
	}
}

void PlayMenuServerBrowser::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	m_pServerList->SetBorder(NULL);

	for (int i = 0; i <= m_pServerList->GetHighestItemID(); i++)
	{
		if (m_pServerList->GetSelectedItem() != i)
			m_pServerList->SetItemFgColor(i, pScheme->GetColor("SeverListItemSelectedColor", Color(255, 255, 255, 255)));
		else
			m_pServerList->SetItemFgColor(i, pScheme->GetColor("SeverListItemColor", Color(0, 0, 0, 255)));

		m_pServerList->SetItemFont(i, pScheme->GetFont("ServerBrowser"));
	}

	m_pServerList->SetSectionFgColor(0, Color(255, 255, 255, 255));
	m_pServerList->SetSectionDividerColor(0, Color(255, 255, 255, 0));
	m_pServerList->SetBgColor(pScheme->GetColor("ServerListBgColor", Color(30, 32, 29, 0)));
	m_pServerList->SetFontSection(0, pScheme->GetFont("ServerBrowser"));
	m_pServerList->SetFgColor(Color(255, 255, 255, 0));

	// Player List:
	m_pPlayerList->SetBorder(NULL);

	for (int i = 0; i <= m_pPlayerList->GetHighestItemID(); i++)
	{
		if (m_pPlayerList->GetSelectedItem() != i)
			m_pPlayerList->SetItemFgColor(i, pScheme->GetColor("SeverListItemSelectedColor", Color(255, 255, 255, 255)));
		else
			m_pPlayerList->SetItemFgColor(i, pScheme->GetColor("SeverListItemColor", Color(0, 0, 0, 255)));

		m_pPlayerList->SetItemFont(i, pScheme->GetFont("ServerBrowser"));
	}

	m_pPlayerList->SetSectionFgColor(0, Color(255, 255, 255, 255));
	m_pPlayerList->SetSectionDividerColor(0, Color(255, 255, 255, 0));
	m_pPlayerList->SetBgColor(pScheme->GetColor("ServerListPlayerListBgColor", Color(30, 32, 29, 200)));
	m_pPlayerList->SetFontSection(0, pScheme->GetFont("ServerBrowser"));
	m_pPlayerList->SetFgColor(Color(255, 255, 255, 0));

	m_pServerInfo->SetFgColor(pScheme->GetColor("ServerListInfoTextColor", Color(255, 255, 255, 255)));
	m_pServerInfo->SetFont(pScheme->GetFont("ServerBrowser"));

	m_pFilterMapInfo->SetFgColor(pScheme->GetColor("ServerListFilterTextColor", Color(255, 255, 255, 255)));
	m_pFilterMapInfo->SetFont(pScheme->GetFont("OptionTextSmall"));

	m_pPasswordContextInfo->SetFgColor(pScheme->GetColor("ServerListPasswordInfoTextColor", Color(255, 255, 255, 255)));
	m_pPasswordContextInfo->SetFont(pScheme->GetFont("OptionTextSmall"));

	m_pPasswordText->SetFgColor(pScheme->GetColor("ServerListPasswordTextColor", Color(255, 255, 255, 255)));
	m_pPasswordText->SetBgColor(Color(23, 25, 27, 200));
	m_pPasswordText->SetFont(pScheme->GetFont("OptionTextSmall"));

	m_pFilterMap->SetBgColor(Color(23, 25, 27, 200));

	for (int i = 0; i < _ARRAYSIZE(m_pFilterCheckButtons); i++)
	{
		m_pFilterCheckButtons[i]->SetFont(pScheme->GetFont("OptionTextSmall"));
		m_pFilterCheckButtons[i]->SetFgColor(pScheme->GetColor("ServerListFilterTextColor", Color(255, 255, 255, 255)));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pFilterComboChoices); i++)
	{
		m_pFilterComboInfo[i]->SetFgColor(pScheme->GetColor("ServerListFilterTextColor", Color(255, 255, 255, 255)));
		m_pFilterComboInfo[i]->SetFont(pScheme->GetFont("OptionTextSmall"));

		m_pFilterComboChoices[i]->SetBgColor(pScheme->GetColor("ServerListComboBgColor", Color(55, 55, 55, 150)));
		m_pFilterComboChoices[i]->SetAlpha(180);
		m_pFilterComboChoices[i]->SetFgColor(pScheme->GetColor("ServerListFilterTextColor", Color(255, 255, 255, 255)));
		m_pFilterComboChoices[i]->SetFont(pScheme->GetFont("OptionTextSmall"));
		m_pFilterComboChoices[i]->SetBorder(NULL);
		m_pFilterComboChoices[i]->SetPaintBorderEnabled(false);
		m_pFilterComboChoices[i]->SetDisabledBgColor(pScheme->GetColor("ServerListComboBgColor", Color(55, 55, 55, 150)));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pServerBrowserSectionInfo); i++)
	{
		m_pServerBrowserSectionInfo[i]->SetFont(pScheme->GetFont("ServerBrowser"));
		m_pServerBrowserSectionInfo[i]->SetFgColor(pScheme->GetColor("ServerListInfoTextColor", Color(255, 255, 255, 255)));
		m_pServerBrowserSectionInfo[i]->SetBgColor(Color(0, 0, 0, 0));
		m_pServerBrowserSectionInfo[i]->SetBorder(NULL);
	}
}

void PlayMenuServerBrowser::AddServerToList(gameserveritem_t *pGameServerItem)
{
	// Prevent duplicates:
	const char *connectionAddress = pGameServerItem->m_NetAdr.GetConnectionAddressString();
	if (IsServerInList(connectionAddress))
		return;

	// Filtrate:
	char szMapTextFilter[256];
	m_pFilterMap->GetText(szMapTextFilter, 256);

	if (szMapTextFilter && szMapTextFilter[0] && (!Q_stristr(pGameServerItem->m_szMap, szMapTextFilter)))
		return;

	if (m_bFilterNoEmpty && (pGameServerItem->m_nPlayers <= 0))
		return;

	if (m_bFilterNoFull && (pGameServerItem->m_nPlayers == pGameServerItem->m_nMaxPlayers))
		return;

	if (m_bFilterNoPassword && (pGameServerItem->m_bPassword))
		return;

	if ((m_iFilterGamemode == 1) && (Q_stristr(pGameServerItem->m_szMap, "bba_") || Q_stristr(pGameServerItem->m_szMap, "bbe_") || Q_stristr(pGameServerItem->m_szMap, "bbd_") || Q_stristr(pGameServerItem->m_szMap, "bbc_")))
		return;

	if ((m_iFilterGamemode == 2) && (!Q_stristr(pGameServerItem->m_szMap, "bbc_")))
		return;

	if ((m_iFilterGamemode == 3) && (!Q_stristr(pGameServerItem->m_szMap, "bbe_")))
		return;

	if ((m_iFilterGamemode == 4) && (!Q_stristr(pGameServerItem->m_szMap, "bba_")))
		return;

	if ((m_iFilterGamemode == 5) && (!Q_stristr(pGameServerItem->m_szMap, "bbd_")))
		return;

	if ((m_iFilterPlayerProfile == 1) && (!Q_stristr(pGameServerItem->m_szGameTags, "savedata 1")))
		return;

	if ((m_iFilterPlayerProfile == 2) && (!Q_stristr(pGameServerItem->m_szGameTags, "savedata 2")))
		return;

	if ((m_iFilterPlayerProfile == 3) && (Q_stristr(pGameServerItem->m_szGameTags, "savedata")))
		return;

	int iServerPing = pGameServerItem->m_nPing;
	if (
		((m_iFilterPing == 1) && (iServerPing >= 50)) ||
		((m_iFilterPing == 2) && (iServerPing >= 100)) ||
		((m_iFilterPing == 3) && (iServerPing >= 150)) ||
		((m_iFilterPing == 4) && (iServerPing >= 250)) ||
		((m_iFilterPing == 5) && (iServerPing >= 350)) ||
		((m_iFilterPing == 6) && (iServerPing >= 600))
		)
		return;

	KeyValues *serverData = new KeyValues("data");
	bool IsSecured = pGameServerItem->m_bPassword;
	bool HasProfileSys = (Q_stristr(pGameServerItem->m_szGameTags, "savedata")) ? true : false;
	bool HasBanListEnabled = (Q_stristr(pGameServerItem->m_szGameTags, "banlist")) ? true : false;
	bool IsOutdated = (!Q_stristr(pGameServerItem->m_szGameTags, GameBaseShared()->GetGameVersion())) ? true : false;
	bool VACEnabled = pGameServerItem->m_bSecure;

	const char *pszGameDescription = "Story";
	if (Q_stristr(pGameServerItem->m_szMap, "bbe_"))
		pszGameDescription = "Elimination";
	else if (Q_stristr(pGameServerItem->m_szMap, "bba_"))
		pszGameDescription = "Arena";
	else if (Q_stristr(pGameServerItem->m_szMap, "bbd_"))
		pszGameDescription = "Deathmatch";
	else if (Q_stristr(pGameServerItem->m_szMap, "bbc_"))
		pszGameDescription = "Objective";

	char pchHostname[128];
	Q_snprintf(pchHostname, 128, "%s%s", (IsOutdated ? "[OUTDATED] " : ""), pGameServerItem->GetName());

	int iconPasswordIndex = (IsSecured ? m_iImageListIndexes[0] : -1);

	int iconProfileIndex = -1;
	if (HasProfileSys)
	{
		if (Q_stristr(pGameServerItem->m_szGameTags, "savedata 1"))
			iconProfileIndex = m_iImageListIndexes[1];
		else
			iconProfileIndex = m_iImageListIndexes[2];
	}

	int iconBanListIndex = -1;
	if (HasBanListEnabled && VACEnabled)
		iconBanListIndex = m_iImageListIndexes[5];
	else if (HasBanListEnabled)
		iconBanListIndex = m_iImageListIndexes[4];
	else if (VACEnabled)
		iconBanListIndex = m_iImageListIndexes[3];

	serverData->SetInt("protected", iconPasswordIndex);
	serverData->SetInt("profile", iconProfileIndex);
	serverData->SetInt("banlist", iconBanListIndex);
	serverData->SetString("hostname", pchHostname);
	serverData->SetString("ip", connectionAddress);
	serverData->SetString("game", pszGameDescription);
	serverData->SetString("players", VarArgs("%i/%i", pGameServerItem->m_nPlayers, pGameServerItem->m_nMaxPlayers));
	serverData->SetInt("ping", iServerPing);
	serverData->SetInt("activePlayers", pGameServerItem->m_nPlayers);
	serverData->SetInt("rawIP", pGameServerItem->m_NetAdr.GetIP());
	serverData->SetInt("rawPort", pGameServerItem->m_NetAdr.GetConnectionPort());
	serverData->SetInt("rawQueryPort", pGameServerItem->m_NetAdr.GetQueryPort());
	serverData->SetInt("timePlayedOnServer", pGameServerItem->m_ulTimeLastPlayed);
	serverData->SetInt("appID", pGameServerItem->m_nAppID);
	serverData->SetString("map", pGameServerItem->m_szMap);

	m_pServerList->AddItem(0, serverData);

	m_nServers++;
	m_nPlayers += pGameServerItem->m_nPlayers;

	serverData->deleteThis();
}

void PlayMenuServerBrowser::RefreshServerList(int type)
{
	// If we are still finishing the previous refresh, then release it.
	if (GameBaseShared()->GetSteamServerManager()->IsBusy())
	{
		GameBaseShared()->GetSteamServerManager()->StopCurrentRequest();
	}

	for (int i = 0; i < (_ARRAYSIZE(m_pButtons) - 1); i++)
	{
		if (i == type)
			continue;

		m_pButtons[i]->Reset();
	}

	// Refresh our list.
	m_pServerList->RemoveAll();
	m_pPlayerList->RemoveAll();

	// Reset our server count
	m_nServers = 0;
	m_nPlayers = 0;

	m_iCurrentSearchMode = type;
	m_pButtons[(_ARRAYSIZE(m_pButtons) - 3)]->SetText("#GameUI_ServerBrowser_AddToFavorites");
	if (m_iCurrentSearchMode == 3)
		m_pButtons[(_ARRAYSIZE(m_pButtons) - 3)]->SetText("#GameUI_ServerBrowser_RemoveFromFavorites");

	GameBaseShared()->GetSteamServerManager()->RequestServersByType(type);
}

void PlayMenuServerBrowser::HandleFavoriteStateOfServer(void)
{
	if (!steamapicontext || (steamapicontext && !steamapicontext->SteamMatchmaking()))
		return;

	int itemID = m_pServerList->GetSelectedItem();
	if (itemID == -1)
		return;

	KeyValues *itemData = m_pServerList->GetItemData(itemID);
	if (!itemData)
		return;

	uint32 ipAddress = itemData->GetInt("rawIP");
	uint16 port = itemData->GetInt("rawPort");
	uint16 queryPort = itemData->GetInt("rawQueryPort");
	uint32 lastTimePlayed = itemData->GetInt("timePlayedOnServer");
	uint32 appID = itemData->GetInt("appID");

	if (m_iCurrentSearchMode == 3)
	{
		steamapicontext->SteamMatchmaking()->RemoveFavoriteGame(appID, ipAddress, port, queryPort, 1);
		RefreshServerList(m_iCurrentSearchMode);
		return;
	}

	steamapicontext->SteamMatchmaking()->AddFavoriteGame(appID, ipAddress, port, queryPort, 1, lastTimePlayed);
}

//-----------------------------------------------------------------------------
// Purpose: Callback from c_steam_server_lister class
//-----------------------------------------------------------------------------
void PlayMenuServerBrowser::RefreshComplete(void)
{
	for (int i = 0; i < (_ARRAYSIZE(m_pButtons) - 1); i++)
		m_pButtons[i]->SetEnabled(true);
}

bool PlayMenuServerBrowser::IsServerInList(const char *address)
{
	for (int i = 0; i < m_pServerList->GetItemCount(); i++)
	{
		KeyValues *pkvData = m_pServerList->GetItemData(i);
		if (!pkvData)
			continue;

		const char *itemAddress = pkvData->GetString("ip");
		if (!strcmp(address, itemAddress))
			return true;
	}

	return false;
}

void PlayMenuServerBrowser::AddPlayerToList(const char *pchName, int nScore, float flTimePlayed)
{
	KeyValues *playerData = new KeyValues("data");
	playerData->SetString("name", pchName);
	playerData->SetInt("score", nScore);
	m_pPlayerList->AddItem(0, playerData);
	playerData->deleteThis();
}

void PlayMenuServerBrowser::PlayersFailedToRespond()
{
}

void PlayMenuServerBrowser::PlayersRefreshComplete()
{
}

void PlayMenuServerBrowser::OnSelectItem(KeyValues *data)
{
	if (!steamapicontext || (steamapicontext && !steamapicontext->SteamMatchmakingServers()))
		return;

	int itemID = m_pServerList->GetSelectedItem();
	if (itemID == -1)
		return;

	KeyValues *itemData = m_pServerList->GetItemData(itemID);
	if (!itemData)
		return;

	m_pPlayerList->RemoveAll();

	uint32 ip = itemData->GetInt("rawIP");
	uint16 port = itemData->GetInt("rawPort");

	if (pPlayerInfoRequest)
	{
		steamapicontext->SteamMatchmakingServers()->CancelServerQuery(pPlayerInfoRequest);
		pPlayerInfoRequest = NULL;
	}

	pPlayerInfoRequest = steamapicontext->SteamMatchmakingServers()->PlayerDetails(ip, port, this);
}

void PlayMenuServerBrowser::OnItemDoubleClicked(KeyValues *data)
{
	int itemID = m_pServerList->GetSelectedItem();
	if (itemID == -1)
		return;

	KeyValues *itemData = m_pServerList->GetItemData(itemID);
	if (!itemData)
		return;

	int iProtected = itemData->GetInt("protected");
	if (iProtected == -1)
	{
		const char *szMap = itemData->GetString("map");
		const char *szConnection = itemData->GetString("ip");
		GameBaseClient->RunMap(szMap, szConnection);
	}
	else
	{
		m_pPasswordContextInfo->SetVisible(true);
		m_pPasswordText->SetVisible(true);
		m_pPasswordContextBG->SetVisible(true);
		m_pPasswordContextButton[0]->SetVisible(true);
		m_pPasswordContextButton[1]->SetVisible(true);
	}
}

void PlayMenuServerBrowser::OnCommand(const char* pcCommand)
{
	if (!strcmp(pcCommand, "Filters"))
		ToggleFilters();
	else if (!strcmp(pcCommand, "Internet"))
		RefreshServerList(0);
	else if (!strcmp(pcCommand, "LAN"))
		RefreshServerList(1);
	else if (!strcmp(pcCommand, "History"))
		RefreshServerList(2);
	else if (!strcmp(pcCommand, "Favorites"))
		RefreshServerList(3);
	else if (!strcmp(pcCommand, "Friends"))
		RefreshServerList(4);
	else if (!strcmp(pcCommand, "Connect"))
		OnItemDoubleClicked(NULL);
	else if (!strcmp(pcCommand, "Refresh"))
		RefreshServerList(m_iCurrentSearchMode);
	else if (!strcmp(pcCommand, "FavoriteState"))
		HandleFavoriteStateOfServer();

	int sortIndex = 1;
	for (int i = 0; i < _ARRAYSIZE(m_pServerBrowserSectionInfo); i++)
	{
		if (!strcmp(pcCommand, VarArgs("SortList%i", (i + 1))))
		{
			if (m_iCurrentSortMethod == sortIndex)
				sortIndex += 1;

			m_iCurrentSortMethod = sortIndex;
			m_pServerList->RefreshList();
			break;
		}

		sortIndex += 2;
	}
}

void PlayMenuServerBrowser::OnKeyCodeTyped(KeyCode code)
{
	if (code == KEY_F5)
	{
		for (int i = 0; i < (_ARRAYSIZE(m_pButtons) - 4); i++)
		{
			if (m_pButtons[i]->IsSelected())
			{
				RefreshServerList(i);
				break;
			}
		}
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}

bool PlayMenuServerBrowser::StaticServerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	const char *keyToSearch = "hostname";
	if ((m_iCurrentSortMethod == SORT_GAMEMODE_DESCENDING) || (m_iCurrentSortMethod == SORT_GAMEMODE_ASCENDING))
		keyToSearch = "game";
	else if ((m_iCurrentSortMethod == SORT_MAP_DESCENDING) || (m_iCurrentSortMethod == SORT_MAP_ASCENDING))
		keyToSearch = "map";
	else if ((m_iCurrentSortMethod == SORT_PLAYER_COUNT_DESCENDING) || (m_iCurrentSortMethod == SORT_PLAYER_COUNT_ASCENDING))
		keyToSearch = "activePlayers";
	else if ((m_iCurrentSortMethod == SORT_PING_DESCENDING) || (m_iCurrentSortMethod == SORT_PING_ASCENDING))
		keyToSearch = "ping";

	// Sort the list by plr count or ping.
	if ((m_iCurrentSortMethod >= SORT_PLAYER_COUNT_DESCENDING && m_iCurrentSortMethod <= SORT_PLAYER_COUNT_ASCENDING) ||
		(m_iCurrentSortMethod >= SORT_PING_DESCENDING && m_iCurrentSortMethod <= SORT_PING_ASCENDING))
	{
		int v1 = it1->GetInt(keyToSearch);
		int v2 = it2->GetInt(keyToSearch);

		if ((m_iCurrentSortMethod == SORT_PLAYER_COUNT_DESCENDING) || (m_iCurrentSortMethod == SORT_PING_DESCENDING))
		{
			if (v1 > v2)
				return true;
			else if (v1 < v2)
				return false;
		}
		else if ((m_iCurrentSortMethod == SORT_PLAYER_COUNT_ASCENDING) || (m_iCurrentSortMethod == SORT_PING_ASCENDING))
		{
			if (v1 > v2)
				return false;
			else if (v1 < v2)
				return true;
		}
	}
	else // Sort by hostname, gamemode or map.
	{
		char n1[128], n2[128];
		Q_strncpy(n1, it1->GetString(keyToSearch), 128);
		Q_strncpy(n2, it2->GetString(keyToSearch), 128);
		Q_strlower(n1); Q_strlower(n2);

		if ((m_iCurrentSortMethod == SORT_HOSTNAME_DESCENDING) || (m_iCurrentSortMethod == SORT_GAMEMODE_DESCENDING) || (m_iCurrentSortMethod == SORT_MAP_DESCENDING))
		{
			for (uint i = 0; i < strlen(n1); i++)
			{
				if (i >= strlen(n2))
					break;

				if (((uint)n1[i]) > ((uint)n2[i]))
					return true;
				else if (((uint)n1[i]) < ((uint)n2[i]))
					return false;
			}
		}
		else if ((m_iCurrentSortMethod == SORT_HOSTNAME_ASCENDING) || (m_iCurrentSortMethod == SORT_GAMEMODE_ASCENDING) || (m_iCurrentSortMethod == SORT_MAP_ASCENDING))
		{
			for (uint i = 0; i < strlen(n1); i++)
			{
				if (i >= strlen(n2))
					break;

				if (((uint)n1[i]) > ((uint)n2[i]))
					return false;
				else if (((uint)n1[i]) < ((uint)n2[i]))
					return true;
			}
		}
	}

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return (itemID1 < itemID2);
}

bool PlayMenuServerBrowser::StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	int v1 = it1->GetInt("score");
	int v2 = it2->GetInt("score");
	if (v1 > v2)
		return true;
	else if (v1 < v2)
		return false;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return (itemID1 < itemID2);
}
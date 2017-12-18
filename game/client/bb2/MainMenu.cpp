//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Main Menu Master, this panel is the very main parent for all the other main menu panels.
//
//========================================================================================//

#include "cbase.h"
#include "GameBase_Client.h"
#include "MainMenu.h"
#include "vgui_controls/Frame.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "IGameUIFuncs.h"
#include "GameBase_Shared.h"
#include "vgui_base_panel.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Console Helpers:
extern IGameUIFuncs *gameuifuncs; // for key binding details

void CMainMenu::OnRetrievedActivePlayerCount(NumberOfCurrentPlayers_t *pCallback, bool bIOFailure)
{
	if (pCallback->m_bSuccess != 1 || bIOFailure)
		return;

	int32 count = pCallback->m_cPlayers;
	if (count <= 0)
	{
		m_pLabelPlayerCount->SetText("");
		return;
	}

	wchar_t wszArg1[10], wszUnicodeString[128];
	V_swprintf_safe(wszArg1, L"%i", count);
	g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#GameUI_MainMenu_ActivePlayers"), 1, wszArg1);
	m_pLabelPlayerCount->SetText(wszUnicodeString);
}

void CMainMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelVersion->SetFgColor(Color(pScheme->GetColor("VersionTextColor", Color(255, 255, 255, 255))));
	m_pLabelVersion->SetFont(pScheme->GetFont("ServerBrowser"));

	m_pLabelPlayerCount->SetFgColor(Color(pScheme->GetColor("VersionTextColor", Color(255, 255, 255, 255))));
	m_pLabelPlayerCount->SetFont(pScheme->GetFont("ServerBrowser"));
}

// The panel background image should be square, not rounded.
void CMainMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBorderEnabled(false);
	BaseClass::PaintBackground();
}

void CMainMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	SetSize(ScreenWidth(), ScreenHeight());
	SetPos(0, 0);

	int w, h;
	GetSize(w, h);

	m_pContextMenuMain->SetSize(scheme()->GetProportionalScaledValue(625), scheme()->GetProportionalScaledValue(60));
	m_pLabelVersion->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(11));
	m_pLabelPlayerCount->SetSize(scheme()->GetProportionalScaledValue(140), scheme()->GetProportionalScaledValue(11));
	m_pGameBanner->SetSize(scheme()->GetProportionalScaledValue(240), scheme()->GetProportionalScaledValue(70));
	m_pPasswordDialog->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(100));

	m_pAdvertisementPanel->SetSize(scheme()->GetProportionalScaledValue(192), scheme()->GetProportionalScaledValue(70));
	m_pAdvertisementPanel->SetPos((w / 2) - scheme()->GetProportionalScaledValue(96), scheme()->GetProportionalScaledValue(72));

	m_pLabelVersion->SetPos(w - scheme()->GetProportionalScaledValue(105), h - scheme()->GetProportionalScaledValue(11));
	m_pLabelVersion->SetContentAlignment(vgui::Label::a_east);

	m_pLabelPlayerCount->SetPos(w - scheme()->GetProportionalScaledValue(145), h - scheme()->GetProportionalScaledValue(22));
	m_pLabelPlayerCount->SetContentAlignment(vgui::Label::a_east);

	m_pOverlayImage->SetSize(ScreenWidth(), ScreenHeight());
	m_pOverlayImage->SetPos(0, 0);
	m_pOverlayImage->SetZPos(-1);

	int wz, hz;
	m_pContextMenuMain->GetSize(wz, hz);
	m_pContextMenuMain->SetPos((w / 2) - (wz / 2), h - hz - scheme()->GetProportionalScaledValue(19));

	// Main Controls
	m_pContextMenuOptions->SetSize(scheme()->GetProportionalScaledValue(125), scheme()->GetProportionalScaledValue(270));
	m_pContextMenuOptions->SetPos((w / 2) - (wz / 2) + scheme()->GetProportionalScaledValue(375), h - hz - scheme()->GetProportionalScaledValue(290));

	m_pContextMenuPlay->SetSize(scheme()->GetProportionalScaledValue(125), scheme()->GetProportionalScaledValue(150));
	m_pContextMenuPlay->SetPos((w / 2) - (wz / 2), h - hz - scheme()->GetProportionalScaledValue(170));

	m_pContextMenuProfile->SetSize(scheme()->GetProportionalScaledValue(125), scheme()->GetProportionalScaledValue(120));
	m_pContextMenuProfile->SetPos((w / 2) - (wz / 2) + scheme()->GetProportionalScaledValue(125), h - hz - scheme()->GetProportionalScaledValue(140));

	m_pContextMenuCredits->SetSize(ScreenWidth(), scheme()->GetProportionalScaledValue(375));
	m_pContextMenuCredits->SetPos(0, 0);

	m_pContextMenuQuit->SetSize(ScreenWidth(), scheme()->GetProportionalScaledValue(200));
	m_pContextMenuQuit->SetPos(0, (ScreenHeight() / 2) - scheme()->GetProportionalScaledValue(100));

	m_pGameBanner->SetPos((w / 2) - (scheme()->GetProportionalScaledValue(240) / 2), 0);

	m_pPasswordDialog->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(72));
	m_pPasswordDialog->GetSize(wz, hz);
	m_pPasswordDialog->SetPos((w / 2) - (wz / 2), (h / 2) - (hz / 2));
	m_pPasswordDialog->PerformLayout();

	if (pMenuContextHandler)
		pMenuContextHandler->SetupLayout(w, h);
}

void CMainMenu::SetMenuOverlayState(int value)
{
	m_pOverlayImage->OnSetupLayout(value);
	if (value == VGUI_OVERLAY_MAINMENU || value == VGUI_OVERLAY_INGAME_FINISH)
	{
		m_pAdvertisementPanel->SetAlpha(255);
		m_pContextMenuMain->SetAlpha(255);
		m_pAdvertisementPanel->SetEnabled(true);
	}
	else if (value == VGUI_OVERLAY_INGAME_OPEN)
	{
		CloseMainMenu(0.0f);
		m_pAdvertisementPanel->SetAlpha(0);
		m_pContextMenuMain->SetAlpha(0);
		ActivateMainMenu();
	}
	else if (value == VGUI_OVERLAY_INGAME_CLOSE)
	{
		CloseMainMenu(0.0f);
		ActivateMainMenu(false);
	}
	else if (value == VGUI_OVERLAY_INGAME_FINISH_CLOSE)
	{
		engine->ClientCmd_Unrestricted("gameui_allowescape\n");
		engine->ClientCmd_Unrestricted("escape\n");
		CloseMainMenu(0.25f);
	}
}

void CMainMenu::ActivateMainMenu(bool bShow, bool bAdvertsOnly)
{
	if (steamapicontext && steamapicontext->SteamUserStats())
	{
		SteamAPICall_t hSteamAPICall = steamapicontext->SteamUserStats()->GetNumberOfCurrentPlayers();
		m_callResultGetNumberOfCurrentPlayers.Set(hSteamAPICall, this, &CMainMenu::OnRetrievedActivePlayerCount);
	}

	GetAnimationController()->RunAnimationCommand(m_pAdvertisementPanel, "alpha", (bShow ? 256.0f : 0.0f), 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
	m_pAdvertisementPanel->SetEnabled(bShow);

	if (!bAdvertsOnly)
		m_pContextMenuMain->OnShowPanel(bShow);
}

// Check if any of our children are active:
bool CMainMenu::HasActivePanel(void)
{
	bool bFoundActivePanel = false;

	for (int i = 0; i < GetChildCount(); ++i)
	{
		CVGUIBasePanel *pPanel = ToBasePanel(GetChild(i));
		if (pPanel)
		{
			if (pPanel == m_pContextMenuMain)
				continue;

			if (pPanel->IsBusy())
			{
				bFoundActivePanel = true;
				break;
			}
		}
	}

	if (ConsoleDialog->IsVisible() || m_pPasswordDialog->IsVisible())
		bFoundActivePanel = true;

	return bFoundActivePanel;
}

// Return one step back:
void CMainMenu::DoReturn(bool bOptionUpdate, bool bForceOff)
{
	// We only want to reset our options!
	if (bOptionUpdate && !bForceOff && GetContextHandler())
	{
		GetContextHandler()->ResetOptions();
		return;
	}

	// Close all visible panels:
	for (int i = 0; i < GetChildCount(); ++i)
	{
		CVGUIBasePanel *pPanel = ToBasePanel(GetChild(i));
		if (pPanel)
		{
			if (pPanel == m_pContextMenuMain)
				continue;

			if (pPanel->IsVisible() && pPanel->IsBusy() && !bForceOff)
				pPanel->OnShowPanel(false);
			else if (bForceOff)
				pPanel->ForceClose();
		}
	}

	CloseGameUIContextMenus(NULL, bForceOff);

	if (m_pPasswordDialog->IsVisible())
		m_pPasswordDialog->ActivateUs(false);

	// Activate our main panel now:
	if (!m_pContextMenuMain->IsVisible())
		ActivateMainMenu();

	if (m_pAdvertisementPanel->GetAlpha() < 255)
	{
		GetAnimationController()->RunAnimationCommand(m_pAdvertisementPanel, "alpha", 256.0f, 0.0f, 0.5f, AnimationController::INTERPOLATOR_LINEAR);
		m_pAdvertisementPanel->SetEnabled(true);
	}
}

void CMainMenu::ApplyChanges(void)
{
	if (GetContextHandler()->m_pMouseOptions->IsVisible())
		GetContextHandler()->m_pMouseOptions->ApplyChanges();

	if (GetContextHandler()->m_pAudioOptions->IsVisible())
		GetContextHandler()->m_pAudioOptions->ApplyChanges();

	if (GetContextHandler()->m_pVideoOptions->IsVisible())
		GetContextHandler()->m_pVideoOptions->ApplyChanges();

	if (GetContextHandler()->m_pGraphicOptions->IsVisible())
		GetContextHandler()->m_pGraphicOptions->ApplyChanges();

	if (GetContextHandler()->m_pPerformanceOptions->IsVisible())
		GetContextHandler()->m_pPerformanceOptions->ApplyChanges();

	if (GetContextHandler()->m_pOtherOptions->IsVisible())
		GetContextHandler()->m_pOtherOptions->ApplyChanges();

	if (GetContextHandler()->m_pCharacterPanel->IsVisible())
		GetContextHandler()->m_pCharacterPanel->ApplyChanges();
}

void CMainMenu::OnThink()
{
	bool bIsInGame = GameBaseClient->IsInGame();
	for (int i = 0; i < GetChildCount(); ++i)
	{
		CVGUIBasePanel *pChild = ToBasePanel(GetChild(i));
		if (pChild)
			pChild->OnUpdate(bIsInGame);
	}

	m_pOverlayImage->OnUpdate();
	m_pGameBanner->SetAlpha(m_pAdvertisementPanel->GetAlpha());

	if (bIsInGame)
	{
		if (HasActivePanel() || !m_pContextMenuMain->IsVisible())
		{
			if (!m_bPreventEscapeToGame)
			{
				//engine->ClientCmd_Unrestricted("gameui_preventescape\n");
				m_bPreventEscapeToGame = true;
			}
		}
		else
		{
			if (m_bPreventEscapeToGame)
			{
				//engine->ClientCmd_Unrestricted("gameui_allowescape\n");
				m_bPreventEscapeToGame = false;
			}
		}
	}

	if (!HasActivePanel() && (!m_pContextMenuMain->IsVisible() || !m_pContextMenuMain->IsBusy()))
		ActivateMainMenu();

	engine->ClientCmd_Unrestricted("hideconsole\n");

	if (ConsoleDialog->IsVisible())
		ConsoleDialog->MoveToFront();

	if (m_pPasswordDialog->IsVisible())
		m_pPasswordDialog->MoveToFront();

	m_pLabelVersion->SetAlpha(m_pContextMenuMain->GetAlpha());
	m_pLabelPlayerCount->SetAlpha(m_pContextMenuMain->GetAlpha());
	m_pPasswordDialog->OnUpdate(bIsInGame);

	if (m_flMainMenuCloseTimer > 0 && (m_flMainMenuCloseTimer < engine->Time()))
	{
		m_flMainMenuCloseTimer = 0.0f;
		SetMenuOverlayState(VGUI_OVERLAY_INGAME_FINISH);
	}

	BaseClass::OnThink();
}

CMainMenu::CMainMenu(vgui::VPANEL parent) : BaseClass(NULL, "MainMenu")
{
	SetParent(parent);
	SetName("MainMenu");
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	SetScheme("BaseScheme");
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetProportional(true);
	SetVisible(true);

	SetSize(ScreenWidth(), ScreenHeight());
	SetZPos(100);

	m_pLabelVersion = vgui::SETUP_PANEL(new vgui::Label(this, "InfoVersion", ""));
	m_pLabelVersion->SetContentAlignment(vgui::Label::a_east);
	m_pLabelVersion->SetZPos(130);

	m_pLabelPlayerCount = vgui::SETUP_PANEL(new vgui::Label(this, "InfoPlayerCount", ""));
	m_pLabelPlayerCount->SetContentAlignment(vgui::Label::a_east);
	m_pLabelPlayerCount->SetZPos(130);

	m_pContextMenuMain = vgui::SETUP_PANEL(new vgui::MenuContextMain(this, "ContextMain"));
	m_pContextMenuMain->SetZPos(120);

	m_pContextMenuPlay = vgui::SETUP_PANEL(new vgui::MenuContextPlay(this, "ContextPlay"));
	m_pContextMenuPlay->SetZPos(130);

	m_pContextMenuProfile = vgui::SETUP_PANEL(new vgui::MenuContextProfile(this, "ContextProfile"));
	m_pContextMenuProfile->SetZPos(130);

	m_pContextMenuQuit = vgui::SETUP_PANEL(new vgui::MenuContextQuit(this, "ContextQuit"));
	m_pContextMenuQuit->SetZPos(130);

	m_pContextMenuCredits = vgui::SETUP_PANEL(new vgui::MenuContextCredits(this, "ContextCredits"));
	m_pContextMenuCredits->SetZPos(130);

	m_pContextMenuOptions = vgui::SETUP_PANEL(new vgui::MenuContextOptions(this, "ContextOptions"));
	m_pContextMenuOptions->SetZPos(130);

	m_pAdvertisementPanel = vgui::SETUP_PANEL(new vgui::AdvertisementPanel(this, "Advertisements"));
	m_pAdvertisementPanel->SetZPos(80);
	m_pAdvertisementPanel->SetVisible(true);

	m_pContextMenuQuit->ForceClose();
	m_pContextMenuPlay->ForceClose();
	m_pContextMenuProfile->ForceClose();
	m_pContextMenuMain->ForceClose();
	m_pContextMenuCredits->ForceClose();
	m_pContextMenuOptions->ForceClose();

	m_pGameBanner = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "BannerIMG"));
	m_pGameBanner->SetShouldScaleImage(true);
	m_pGameBanner->SetImage("bb2_logo");
	m_pGameBanner->SetZPos(90);

	m_pOverlayImage = vgui::SETUP_PANEL(new vgui::CMenuOverlay(this, "MenuFadingOverlay"));
	m_pOverlayImage->SetZPos(-1);
	m_pOverlayImage->SetVisible(true);

	ConsoleDialog = new CGameConsoleDialog();
	ConsoleDialog->SetVisible(false);
	ConsoleDialog->ToggleConsole(false, true);

	m_pPasswordDialog = new CPasswordDialog(this, "PasswordDialog");
	m_pPasswordDialog->SetZPos(200);
	m_pPasswordDialog->SetVisible(false);

	pMenuContextHandler = new CMenuContextHandler();
	pMenuContextHandler->CreateMenuContext(this, 110);

	m_flMainMenuCloseTimer = 0.0f;

	InvalidateLayout();

	PerformLayout();

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	Activate();
	MoveToFront();
	RequestFocus();

	m_pLabelVersion->SetText(VarArgs("Version: %s", GameBaseShared()->GetGameVersion()));
	m_pLabelVersion->SetContentAlignment(vgui::Label::a_east);
}

CMainMenu::~CMainMenu()
{
	if (pMenuContextHandler)
		delete pMenuContextHandler;
}

void CMainMenu::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (gameuifuncs->GetButtonCodeForBind("OpenGameConsole") == code)
		ConsoleDialog->ToggleConsole(!ConsoleDialog->IsVisible());
	else if ((code == KEY_ESCAPE) || (code == KEY_BACKSPACE))
	{
		DoReturn();

		if (!m_bPreventEscapeToGame && GameBaseClient->IsMainMenuVisibleWhileInGame())
			SetMenuOverlayState(VGUI_OVERLAY_INGAME_CLOSE);
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}

CVGUIBasePanel *CMainMenu::GetGameUIContextObject(int iMenuCommand)
{
	switch (iMenuCommand)
	{
	case COMMAND_PLAY:
		return ToBasePanel(m_pContextMenuPlay);
	case COMMAND_CREDITS:
		return ToBasePanel(m_pContextMenuCredits);
	case COMMAND_PROFILE:
		return ToBasePanel(m_pContextMenuProfile);
	case COMMAND_OPTIONS:
		return ToBasePanel(m_pContextMenuOptions);
	case COMMAND_QUITCONFIRM:
		return ToBasePanel(m_pContextMenuQuit);
		// Play Menu
	case COMMAND_SERVERBROWSER:
		return ToBasePanel(GetContextHandler()->m_pServerMenu);
	case COMMAND_CREATEGAME:
		return ToBasePanel(GetContextHandler()->m_pCreateGameMenu);
	case COMMAND_SHOW_SCOREBOARD:
		return ToBasePanel(GetContextHandler()->m_pScoreboard);
		// Option Menu
	case COMMAND_KEYBOARD:
		return ToBasePanel(GetContextHandler()->m_pKeyboardOptions);
	case COMMAND_MOUSE:
		return ToBasePanel(GetContextHandler()->m_pMouseOptions);
	case COMMAND_AUDIO:
		return ToBasePanel(GetContextHandler()->m_pAudioOptions);
	case COMMAND_VIDEO:
		return ToBasePanel(GetContextHandler()->m_pVideoOptions);
	case COMMAND_GRAPHICS:
		return ToBasePanel(GetContextHandler()->m_pGraphicOptions);
	case COMMAND_PERFORMANCE:
		return ToBasePanel(GetContextHandler()->m_pPerformanceOptions);
	case COMMAND_OTHER:
		return ToBasePanel(GetContextHandler()->m_pOtherOptions);
		// Profile Menu
	case COMMAND_ACHIEVEMENT_PANEL:
		return ToBasePanel(GetContextHandler()->m_pAchievementPanel);
	case COMMAND_CHARACTER_PANEL:
		return ToBasePanel(GetContextHandler()->m_pCharacterPanel);
	}

	return NULL;
}

void CMainMenu::CloseGameUIContextMenus(CVGUIBasePanel *pContextObjectOther, bool bForceOff)
{
	vgui::Panel *pContextObject = this;
	if (pContextObjectOther)
		pContextObject = pContextObjectOther;

	if (pContextObject)
	{
		for (int i = 0; i < pContextObject->GetChildCount(); ++i)
		{
			CVGUIBasePanel *pChild = ToBasePanel(pContextObject->GetChild(i));
			if (pChild)
			{
				if (m_pContextMenuMain == pChild)
					continue;

				if (pChild->IsVisible() && !bForceOff)
					pChild->OnShowPanel(false);

				if (bForceOff)
					pChild->ForceClose();

				CloseGameUIContextMenus(pChild);
			}
		}
	}
}
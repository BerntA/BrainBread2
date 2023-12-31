//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Main Menu Master, this panel is the very main parent for all the other main menu panels.
//
//========================================================================================//

#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/KeyCode.h>

// BB2 Context & Standard Menus:
#include "MenuContextMain.h"
#include "MenuContextPlay.h"
#include "MenuContextQuit.h"
#include "MenuContextCredits.h"
#include "MenuContextOptions.h"
#include "MenuContextProfile.h"
#include "console_dialog.h"
#include "MenuOverlay.h"
#include "AdvertisementPanel.h"
#include "PasswordDialog.h"
#include "vgui_base_panel.h"
#include "clientmode_shared.h"

// Context Handler

// Play Menus
#include "PlayMenuCreateGame.h"
#include "PlayMenuServerBrowser.h"
#include "PlayMenuScoreboard.h"

// Option Menus
#include "OptionMenuKeyboard.h"
#include "OptionMenuMouse.h"
#include "OptionMenuAudio.h"
#include "OptionMenuVideo.h"
#include "OptionMenuGraphics.h"
#include "OptionMenuPerformance.h"
#include "OptionMenuOther.h"

// Profile Menus
#include "ProfileMenuAchievementPanel.h"
#include "ProfileMenuCharacterPanel.h"

class CMainMenu;

class CMenuContextHandler
{
public:
	CMenuContextHandler();
	~CMenuContextHandler();

	void CreateMenuContext(CMainMenu *pParent, int zpos = 1);

	// Shared
	void SetupLayout(int w, int h);
	void ResetOptions(void);

	// Option Panels:
	vgui::OptionMenuKeyboard *m_pKeyboardOptions;
	vgui::OptionMenuMouse *m_pMouseOptions;
	vgui::OptionMenuAudio *m_pAudioOptions;
	vgui::OptionMenuVideo *m_pVideoOptions;
	vgui::OptionMenuGraphics *m_pGraphicOptions;
	vgui::OptionMenuPerformance *m_pPerformanceOptions;
	vgui::OptionMenuOther *m_pOtherOptions;

	// Play Panels
	vgui::PlayMenuCreateGame *m_pCreateGameMenu;
	vgui::PlayMenuServerBrowser *m_pServerMenu;
	vgui::PlayMenuScoreboard *m_pScoreboard;

	// Profile Panels
	vgui::ProfileMenuAchievementPanel *m_pAchievementPanel;
	vgui::ProfileMenuCharacterPanel *m_pCharacterPanel;

private:

	bool m_bInitialized;
};

class CMainMenu : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CMainMenu, vgui::Frame);

public:

	CMainMenu(vgui::VPANEL parent);
	~CMainMenu();

	// Activate Changes @main menu overlay
	void SetMenuOverlayState(int value);
	// Activate Warn or norm menu screen on startup of game:
	void ActivateMainMenu(bool bShow = true, bool bAdvertsOnly = false);
	// Return to the main menu context:
	void DoReturn(bool bOptionUpdate = false, bool bForceOff = false);
	// Apply Options in the current active option panel.
	void ApplyChanges(void);
	// Do we have any active children?: (vis)
	bool HasActivePanel(void);
	// Close all panels in this object's children and their children and so forth.
	void CloseGameUIContextMenus(CVGUIBasePanel *pContextObjectOther = NULL, bool bForceOff = false);
	// Get the current base class which we want to use for a certain menu command. When opening the credits for example.
	CVGUIBasePanel *GetGameUIContextObject(int iMenuCommand);
	// Return a proper cast to CVGUIBasePanel
	inline CVGUIBasePanel *ToBasePanel(Panel *pPanel)
	{
		return dynamic_cast<CVGUIBasePanel *> (pPanel);
	}

	// Global Panels:
	CGameConsoleDialog *ConsoleDialog;
	CPasswordDialog *m_pPasswordDialog;
	vgui::CMenuOverlay *m_pOverlayImage;
	vgui::MenuContextMain *m_pContextMenuMain;

	// Other:
	vgui::MenuContextPlay *m_pContextMenuPlay;
	vgui::MenuContextProfile *m_pContextMenuProfile;
	vgui::MenuContextCredits *m_pContextMenuCredits;
	vgui::MenuContextOptions *m_pContextMenuOptions;
	vgui::MenuContextQuit *m_pContextMenuQuit;

	// Misc
	vgui::ImagePanel *m_pGameBanner;
	vgui::AdvertisementPanel *m_pAdvertisementPanel;

	bool m_bPreventEscapeToGame;

	vgui::Label *m_pLabelVersion;
	vgui::Label *m_pLabelPlayerCount;

	virtual void PerformLayout();

	CMenuContextHandler *GetContextHandler(void) { return pMenuContextHandler; }

protected:

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground();
	virtual void OnThink();
	virtual void OnClose() {} // unused
	virtual void OnKeyCodeTyped(vgui::KeyCode code);

private:
	CMenuContextHandler *pMenuContextHandler;

	// In-Game MainMenu handling...
	void CloseMainMenu(float delay) 
	{ 
		if (delay <= 0.0f)
			m_flMainMenuCloseTimer = 0.0f;
		else
			m_flMainMenuCloseTimer = engine->Time() + delay;
	}
	float m_flMainMenuCloseTimer;

	void OnRetrievedActivePlayerCount(NumberOfCurrentPlayers_t *pCallback, bool bIOFailure);
	CCallResult<CMainMenu, NumberOfCurrentPlayers_t> m_callResultGetNumberOfCurrentPlayers;
};

#endif // MAIN_MENU_H
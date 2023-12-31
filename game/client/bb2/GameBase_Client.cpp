//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: 2 Main Game Interface for BB2 Client Side
//
//========================================================================================//

#include "cbase.h"
#include "GameBase_Client.h"
#include "vgui_controls/Frame.h"
#include "weapon_parse.h"
#include "ienginevgui.h"
#include <GameUI/IGameUI.h>
#include "c_hl2mp_player.h"

// ADD INCLUDES FOR OTHER MENUS: (NON-BASEVIEWPORT/INTERFACE)
#include "LoadingPanel.h"
#include "AddonInstallerPanel.h"
#include "MainMenu.h"
#include "fmod_manager.h"
#include "NotePanel.h"
#include "vote_menu.h"
#include "ivoicetweak.h"
#include "quest_panel.h"

// Other helpers
#include "clientmode_shared.h"
#include <vgui_controls/Frame.h>
#include "vgui_base_frame.h"
#include "hud_objective.h"
#include "tier0/icommandline.h"
#include "c_leaderboard_handler.h"
#include "GameBase_Shared.h"
#include "iviewrender.h"
#include "view.h"
#include "c_playerresource.h"
#include "voice_status.h"
#include "GlobalRenderEffects.h"
#include "c_bb2_player_shared.h"
#include "music_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void OnUpdateMirrorRenderingState(IConVar *pConVar, char const *pOldString, float flOldValue)
{
	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	pLocal->UpdateVisibility();

	BB2PlayerGlobals->BodyUpdateVisibility();

	C_BaseCombatWeapon *pLocalWeapon = pLocal->GetActiveWeapon();
	if (!pLocalWeapon)
		return;

	pLocalWeapon->UpdateVisibility();
}

// ConVars Shared Client-Side:
ConVar bb2_loading_image("bb2_loading_image", NULL, FCVAR_CLIENTDLL | FCVAR_HIDDEN, "Current Loading Image");
ConVar bb2_show_details("bb2_show_details", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Show or Hide HUD details for the main HUD such as text above the bars.", true, 0, true, 1);
ConVar bb2_render_client_in_mirrors("bb2_render_client_in_mirrors", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Render the player in mirrors.", true, 0, true, 1, OnUpdateMirrorRenderingState);

void OnUpdateMulticoreState(IConVar *pConVar, char const *pOldString, float flOldValue)
{
	ConVar *var = (ConVar*)pConVar;
	if (var && var->GetBool())
		engine->ClientCmd_Unrestricted("exec multicore.cfg\n");
	else
		engine->ClientCmd_Unrestricted("mat_queue_mode 0\n");
}

ConVar bb2_enable_multicore("bb2_enable_multicore", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Enable or Disable multicore rendering, this feature is unstable but could increase performance!", true, 0, true, 1, OnUpdateMulticoreState);

void EnableFilmGrain(IConVar *pConVar, char const *pOldString, float flOldValue)
{
	ConVar *var = (ConVar*)pConVar;
	if (view && var)
		view->SetScreenOverlayMaterial((var->GetBool() ? GlobalRenderEffects->GetFilmGrainOverlay() : NULL));
}

ConVar bb2_fx_filmgrain("bb2_fx_filmgrain", "0", FCVAR_ARCHIVE, "Enable or Disable film grain.", true, 0, true, 1, EnableFilmGrain);

static void VoiceThresholdChange(IConVar *pConVar, char const *pOldString, float flOldValue)
{
	ConVar *var = (ConVar*)pConVar;
	if (GameBaseClient->m_pVoiceTweak && var)
		GameBaseClient->m_pVoiceTweak->SetControlFloat(MicrophoneVolume, var->GetFloat());
}

static void VoiceBoostChange(IConVar *pConVar, char const *pOldString, float flOldValue)
{
	ConVar *var = (ConVar*)pConVar;
	if (GameBaseClient->m_pVoiceTweak && var)
		GameBaseClient->m_pVoiceTweak->SetControlFloat(MicBoost, (var->GetBool() ? 1.0f : 0.0f));
}

static ConVar voice_threshold("voice_threshold", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Voice Send Volume", true, 0.0f, true, 1.0f, VoiceThresholdChange);
static ConVar voice_boost("voice_boost", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Voice Boost Gain", true, 0.0f, true, 1.0f, VoiceBoostChange);

float m_flLastBloodParticleDispatchTime = 0.0f;
static bool g_bHasLoadedSteamStats = false;

// GameUI
static CDllDemandLoader g_GameUIDLL("GameUI");

class CGameBaseClient : public IGameBaseClient
{
private:

	// VGUI Panel Members
	CQuestPanel *QuestPanel;
	CNotePanel *NotePanel;
	CVotePanel *VotePanel;
	CMainMenu *MainMenu;
	CLoadingPanel *LoadingPanel;
	CAddonInstallerPanel *ClientWorkshopInstallerPanel;
	// GameUI accessor
	IGameUI *GameUI;

	STEAM_CALLBACK(CGameBaseClient, Steam_OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived);

public:

	CGameBaseClient(void);

	// Initialize, called when the game launches. (only once)
	void Initialize(void);
	// Init/Create in-game panels.
	void CreateInGamePanels(vgui::VPANEL parent);
	// Cleanup
	void Destroy(void);

	// Get the loading image for the choosen map / the map running on the server you've connected to. If you don't have this image under vgui/loading/ it will choose the default image.
	const char *GetLoadingImage();
	// Are we in-game?
	bool IsInGame(void);

	// Connection, Changelevel and Map commands.
	void ResetMapConVar(void);
	void RunMap(const char *szMap);
	void RunMap(const char *szMap, const char *szConnectionString);
	void RunMap(const char *szMap, const char *szConnectionString, const char *szPassword);
	void Changelevel(const char *szMap);

	// Update all of our VGUI stuff here.
	void RefreshVGUI(void);
	// Prevents issues where you'd be able to open up new panels as they were fading out. In some cases this would crash the client.
	bool CanOpenPanel(void);
	// Due to the fade out function not all panels get fully closed when we rush out of a game, that's why we have to handle proper forcing here!
	void CloseGamePanels(bool bInGamePanelsOnly = false);
	// Swap between quest previews within the Game Panel GUI.
	void SelectQuestPreview(int index);
	// Check if a baseviewport panel is visible:.
	bool IsViewPortPanelVisible(const char *panel);
	// Handle in-game commands to the main menu.
	void RunMainMenuCommand(int cmd);
	// Open game ui / in-game panels...
	void RunCommand(int iCommand);
	// Run Client Specific commands.
	void RunClientEffect(int iEffect, int iState);
	// Display in-game note. New screenoverlay logic.
	void ShowNote(const char *szHeader, const char *szFile);
	// Display the default voting panel.
	void ShowVotePanel(bool bForceOff = false);

	// Post Init - Late Init - Starts up the main menu.
	void PostInit(void);
	// Shown when joining a password protected server.
	void ShowPasswordDialog(bool bShow);
	// If we're not using direct connection then we'll redirect our items to the server browser GUI:
	bool AddServerBrowserItem(gameserveritem_t *pGameServerItem);
	// We tell our server browser GUI that we're done searchin.
	void ServerRefreshCompleted(void);

	// Load game localization data.
	void LoadGameLocalization(void);

	// Handle per-frame thinking...
	void OnUpdate(void);

	// Scoreboard Handling
	void AddScoreboardItem(const char *pszSteamID, const char *playerName, int32 plLevel, int32 plKills, int32 plDeaths, int index);
	void ScoreboardRefreshComplete(int maxEntries);

	// Console
	void ToggleConsole(void);
	void CloseConsole(void);
	void ClearConsole(void);

	// In-Game Stuff
	void OnLocalPlayerExternalRendering(void);
	bool IsMainMenuVisibleWhileInGame(void);
	bool m_bIsMenuVisibleAndInGame;

	// Voice
	bool IsPlayerGameVoiceMuted(int playerIndex);
	void MutePlayerGameVoice(int playerIndex, bool value);
};

static CGameBaseClient g_GameBaseClient;
IGameBaseClient *GameBaseClient = (IGameBaseClient *)&g_GameBaseClient;

// Constructor
CGameBaseClient::CGameBaseClient(void) : m_CallbackUserStatsReceived(this, &CGameBaseClient::Steam_OnUserStatsReceived)
{
	LoadingPanel = NULL;
	ClientWorkshopInstallerPanel = NULL;
	MainMenu = NULL;
	NotePanel = NULL;
	VotePanel = NULL;
	QuestPanel = NULL;
	GameUI = NULL;

	m_bIsMenuVisibleAndInGame = false;
}

// Early Initialization
void CGameBaseClient::Initialize(void)
{
	LoadGameLocalization();

	// Init Game UI
	CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
	if (gameUIFactory)
	{
		GameUI = (IGameUI*)gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
		if (!GameUI)
			Error("Couldn't load GameUI!\n");
	}
	else
		Error("Couldn't load GameUI!\n");

	// Create our MainMenu
	MainMenu = new CMainMenu(NULL);
	GameUI->SetMainMenuOverride(MainMenu->GetVPanel());

	// Create our Loading Panel
	LoadingPanel = new CLoadingPanel(NULL);
	LoadingPanel->SetVisible(false);
	GameUI->SetLoadingBackgroundDialog(LoadingPanel->GetVPanel());

	VPANEL GameUiDll = enginevgui->GetPanel(PANEL_GAMEUIDLL);
	ClientWorkshopInstallerPanel = new CAddonInstallerPanel(GameUiDll);
	ClientWorkshopInstallerPanel->SetVisible(false);

	if (steamapicontext && steamapicontext->SteamRemoteStorage())
		steamapicontext->SteamRemoteStorage()->SetCloudEnabledForApp(false);

	PostInit();
}

// Generate the in-game panels.
void CGameBaseClient::CreateInGamePanels(vgui::VPANEL parent)
{
	NotePanel = new CNotePanel(parent);
	VotePanel = new CVotePanel(parent);
	QuestPanel = new CQuestPanel(parent);
}

// Cleanup - called in vgui_int.cpp
void CGameBaseClient::Destroy(void)
{
	g_GameUIDLL.Unload();
	GameUI = NULL;

	if (LoadingPanel)
	{
		LoadingPanel->SetParent((vgui::Panel *)NULL);
		delete LoadingPanel;
	}

	if (ClientWorkshopInstallerPanel)
	{
		ClientWorkshopInstallerPanel->SetParent((vgui::Panel *)NULL);
		delete ClientWorkshopInstallerPanel;
	}

	if (NotePanel)
	{
		NotePanel->SetParent((vgui::Panel *)NULL);
		delete NotePanel;
	}

	if (VotePanel)
	{
		VotePanel->SetParent((vgui::Panel *)NULL);
		delete VotePanel;
	}

	if (QuestPanel)
	{
		QuestPanel->SetParent((vgui::Panel *)NULL);
		delete QuestPanel;
	}

	GlobalRenderEffects->Shutdown();
}

// Display in-game note. New screenoverlay logic.
void CGameBaseClient::ShowNote(const char *szHeader, const char *szFile)
{
	if (NotePanel)
		NotePanel->SetupNote(szHeader, szFile);
}

// Display the voting panel/menu.
void CGameBaseClient::ShowVotePanel(bool bForceOff)
{
	if (VotePanel)
	{
		if (bForceOff)
		{
			if (!VotePanel->IsVisible())
				return;

			VotePanel->OnShowPanel(false);
			return;
		}

		C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
		if (!pClient)
			return;

		VotePanel->OnShowPanel(!VotePanel->IsVisible());
	}
}

// Swap between quest previews within the Game Panel GUI.
void CGameBaseClient::SelectQuestPreview(int index)
{
	if (QuestPanel)
		QuestPanel->OnSelectQuest(index);
}

// Is this viewport panel visible?
bool CGameBaseClient::IsViewPortPanelVisible(const char *panel)
{
	IViewPortPanel *viewportPanel = (gViewPortInterface ? gViewPortInterface->FindPanelByName(panel) : NULL);
	if (viewportPanel)
	{
		Panel *panel = dynamic_cast<Panel*> (viewportPanel);
		if (panel)
			return panel->IsVisible();
	}
	return false;
}

// Run main menu commands.
void CGameBaseClient::RunMainMenuCommand(int cmd)
{
	if (MainMenu)
		MainMenu->SetMenuOverlayState(cmd);
}

// Open game ui / in-game panels...
void CGameBaseClient::RunCommand(int iCommand)
{
	// If we're trying to open up a gameUI context menu then do so:
	CVGUIBasePanel *pGameUIContext = MainMenu->GetGameUIContextObject(iCommand);
	if (pGameUIContext && MainMenu)
	{
		if (pGameUIContext->IsVisible())
			return;

		if ((iCommand == COMMAND_CREATEGAME) && GameBaseShared()->GetSharedMapData() && !GameBaseShared()->GetSharedMapData()->pszGameMaps.Count())
		{
			Warning("You don't have any maps available!\n");
			return;
		}

		MainMenu->CloseGameUIContextMenus();

		// Close other panels
		CVGUIBasePanel *pBaseParent = MainMenu->ToBasePanel(pGameUIContext->GetParent());
		if (!pBaseParent) // We're the main main menu:	
		{
			// Hide the main menu if these cases are true.
			if ((iCommand == COMMAND_QUITCONFIRM) || (iCommand == COMMAND_CREDITS))
				MainMenu->ActivateMainMenu(false);

			if ((iCommand != COMMAND_PLAY) && (iCommand != COMMAND_PROFILE) && (iCommand != COMMAND_CREDITS) && (iCommand != COMMAND_OPTIONS)
				&& (iCommand != COMMAND_QUITCONFIRM))
				MainMenu->ActivateMainMenu(false, true);
		}
		else // We're some other base panel:
			MainMenu->CloseGameUIContextMenus(pBaseParent);

		// Open our lucky panel!!
		pGameUIContext->OnShowPanel(true);
		return;
	}

	switch (iCommand)
	{
	case COMMAND_QUIT:
		engine->ClientCmd_Unrestricted("mat_savechanges\n");
		engine->ClientCmd_Unrestricted("host_writeconfig\n");
		CloseConsole();
		engine->ClientCmd_Unrestricted("gamemenucommand quitnoconfirm\n");

		break;
	case COMMAND_RETURN:
		if (MainMenu)
			MainMenu->DoReturn();

		break;
	case COMMAND_OPTIONS_APPLY:
		if (MainMenu)
			MainMenu->ApplyChanges();

		break;
	case COMMAND_DISCONNECT:
		CloseConsole();
		ClearConsole();

		if (MainMenu)
			MainMenu->DoReturn();

		engine->ClientCmd_Unrestricted("disconnect\n");
		FMODManager()->SetSoundVolume(1.0f);
		FMODManager()->TransitionAmbientSound("ui/mainmenu_theme.mp3");
		break;
	case COMMAND_RESET:
		engine->ClientCmd_Unrestricted("exec config_default.cfg\n");

		if (MainMenu)
			MainMenu->DoReturn(true);

		break;
	case COMMAND_PASSWORD_OK:
		if (MainMenu)
			MainMenu->GetContextHandler()->m_pServerMenu->ShouldLaunchWithPassword(true);

		break;
	case COMMAND_PASSWORD_CANCEL:
		if (MainMenu)
			MainMenu->GetContextHandler()->m_pServerMenu->ShouldLaunchWithPassword(false);

		break;
	case COMMAND_START_TUTORIAL:
		if (MainMenu)
			MainMenu->DoReturn();

		RunMap("tutorial");
		break;
	default:
		Warning("Invalid Menu Command was sent, ignoring...!\n");
		break;
	}
}

// Execute client specific commands.
void CGameBaseClient::RunClientEffect(int iEffect, int iState)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (pClient && view)
	{
		CHudCaptureProgressBar *pCaptureProgressHUD = GET_HUDELEMENT(CHudCaptureProgressBar);

		switch (iEffect)
		{
		case PLAYER_EFFECT_ZOMBIE_FLASHLIGHT:
		{
			view->SetScreenOverlayMaterial(NULL);
			if (pClient->GetTeamNumber() == TEAM_DECEASED)
			{
				if (iState >= 1)
					view->SetScreenOverlayMaterial(GlobalRenderEffects->GetZombieVision());

				pClient->SetZombieVision((iState >= 1));
			}

			if (iState <= 0)
			{
				if (bb2_fx_filmgrain.GetBool())
					view->SetScreenOverlayMaterial(GlobalRenderEffects->GetFilmGrainOverlay());
			}

			break;
		}
		case PLAYER_EFFECT_ENTERED_GAME:
		case PLAYER_EFFECT_FULLCHECK:
		{
			view->SetScreenOverlayMaterial(NULL);
			if (bb2_fx_filmgrain.GetBool())
				view->SetScreenOverlayMaterial(GlobalRenderEffects->GetFilmGrainOverlay());

			if (pCaptureProgressHUD)
				pCaptureProgressHUD->Reset();

			m_pPlayerRagdoll = NULL;

			pClient->SetZombieVision(false);
			m_flLastBloodParticleDispatchTime = 0.0f;
			break;
		}
		case PLAYER_EFFECT_BECOME_ZOMBIE:
		{
			if (pCaptureProgressHUD)
				pCaptureProgressHUD->Reset();

			break;
		}
		case PLAYER_EFFECT_DEATH:
		{
			CloseGamePanels(true);
			pClient->SetZombieVision(false);
			break;
		}
		}
	}
}

// Get the loading image for the map you're starting / server you're connecting to.
const char *CGameBaseClient::GetLoadingImage()
{
	return bb2_loading_image.GetString();
}

// Resets the loading image string.
void CGameBaseClient::ResetMapConVar(void)
{
	bb2_loading_image.SetValue(NULL);
	bb2_active_workshop_item.SetValue(0);
}

// Are we in-game?
bool CGameBaseClient::IsInGame(void)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();

	return ((pClient != NULL) && !engine->IsLevelMainMenuBackground());
}

// Prevents issues where you'd be able to open up new panels as they were fading out. In some cases this would crash the client.
bool CGameBaseClient::CanOpenPanel(void)
{
	// Prevents a big issue which will make the scoreboard "stuck"/"toggled" (not looking for key input) 
	// Wait for it to be fully removed:
	vgui::Frame* pBasePanel = dynamic_cast<vgui::Frame*> (gViewPortInterface ? gViewPortInterface->GetActivePanel() : NULL);
	if ((pBasePanel && pBasePanel->IsVisible()) || (QuestPanel && QuestPanel->IsVisible()) || (NotePanel && NotePanel->IsVisible()) || (VotePanel && VotePanel->IsVisible()))
		return false;

	return true;
}

// Due to the fade out function not all panels get fully closed when we rush out of a game, that's why we have to handle proper forcing here!
void CGameBaseClient::CloseGamePanels(bool bInGamePanelsOnly)
{
	if (QuestPanel)
		QuestPanel->ForceClose();

	if (NotePanel)
		NotePanel->ForceClose();

	if (VotePanel)
		VotePanel->ForceClose();

	CHudCaptureProgressBar *pCaptureProgressHUD = GET_HUDELEMENT(CHudCaptureProgressBar);
	if (pCaptureProgressHUD)
		pCaptureProgressHUD->Reset();

	IViewPortPanel *pBasePanel = (gViewPortInterface ? gViewPortInterface->GetActivePanel() : NULL);
	if (pBasePanel)
	{
		vgui::CVGUIBaseFrame *pBaseClassFrame = dynamic_cast<vgui::CVGUIBaseFrame*> (pBasePanel);
		if (pBaseClassFrame)
			pBaseClassFrame->ForceClose();
		else
			gViewPortInterface->ShowPanel(pBasePanel->GetName(), false);
	}

	// Refresh GameUI / MainMenu components:
	if (!bInGamePanelsOnly)
		RefreshVGUI();
}

// Run a map directly, called when you write map *** or create a game.
void CGameBaseClient::RunMap(const char *szMap)
{
	char pszMapPath[80];
	Q_snprintf(pszMapPath, 80, "maps/%s.bsp", szMap);

	if (!filesystem->FileExists(pszMapPath, "MOD"))
	{
		Warning("Couldn't find map %s.bsp!\n", szMap);
		return;
	}

	CloseConsole();
	bb2_loading_image.SetValue(szMap);
	RefreshVGUI();
	GetMusicSystem->RunLoadingSoundTrack(szMap);
	engine->ClientCmd_Unrestricted("progress_enable\n");
	engine->ClientCmd_Unrestricted(VarArgs("map %s\n", szMap));
}

// Called when you join a server.
void CGameBaseClient::RunMap(const char *szMap, const char *szConnectionString)
{
	CloseConsole();
	bb2_loading_image.SetValue(szMap);
	RefreshVGUI();
	GetMusicSystem->RunLoadingSoundTrack(szMap);
	engine->ClientCmd_Unrestricted("progress_enable\n");
	engine->ClientCmd_Unrestricted(VarArgs("connect %s\n", szConnectionString));
}

// Called when you join a server + adding the password for the server if it promts you to.
void CGameBaseClient::RunMap(const char *szMap, const char *szConnectionString, const char *szPassword)
{
	engine->ClientCmd_Unrestricted(VarArgs("password %s\n", szPassword));
	CloseConsole();
	bb2_loading_image.SetValue(szMap);
	RefreshVGUI();
	GetMusicSystem->RunLoadingSoundTrack(szMap);
	engine->ClientCmd_Unrestricted("progress_enable\n");
	engine->ClientCmd_Unrestricted(VarArgs("connect %s\n", szConnectionString));
}

// Called on level 'transit' changelevel to the next map.
void CGameBaseClient::Changelevel(const char *szMap)
{
	CloseConsole();
	bb2_loading_image.SetValue(szMap);
	RefreshVGUI();
	GetMusicSystem->RunLoadingSoundTrack(szMap);
	engine->ClientCmd_Unrestricted("progress_enable\n");
}

// update layout on all vgui stuff here.
void CGameBaseClient::RefreshVGUI(void)
{
	if (MainMenu)
	{
		MainMenu->PerformLayout();
		CloseConsole();
		MainMenu->DoReturn();

		RunMainMenuCommand(VGUI_OVERLAY_MAINMENU);
		m_bIsMenuVisibleAndInGame = false;
		engine->ClientCmd_Unrestricted("gameui_allowescape\n");
	}
}

// Post Init - Late Init : Loads the main menu
void CGameBaseClient::PostInit(void)
{
	if (MainMenu)
	{
		m_bIsMenuVisibleAndInGame = false;
		MainMenu->ActivateMainMenu();

		// Do we want to start with the console?
		if (CommandLine()->FindParm("-console") && !CommandLine()->FindParm("-tools") && !CommandLine()->FindParm("+connect")
			&& !CommandLine()->FindParm("+map"))
			ToggleConsole();

		// FORCE Base Stuff:
		FMODManager()->SetSoundVolume(1.0f);
		FMODManager()->TransitionAmbientSound("ui/mainmenu_theme.mp3");

		engine->ClientCmd_Unrestricted("mat_colorcorrection 1\n");

		if (bb2_enable_multicore.GetBool())
			engine->ClientCmd_Unrestricted("exec multicore.cfg\n");
		else
			engine->ClientCmd_Unrestricted("mat_queue_mode 0\n");

		m_pVoiceTweak = engine->GetVoiceTweakAPI();
		if (m_pVoiceTweak)
		{
			m_pVoiceTweak->SetControlFloat(MicBoost, (voice_boost.GetFloat() >= 1.0f) ? 1.0f : 0.0f);
			m_pVoiceTweak->SetControlFloat(MicrophoneVolume, voice_threshold.GetFloat());
			DevMsg("Initialized voice api\n");
		}
	}

	GlobalRenderEffects->Initialize();
	BB2PlayerGlobals->Initialize();

	if (steamapicontext && steamapicontext->SteamUserStats())
		steamapicontext->SteamUserStats()->RequestCurrentStats();

	// Bernt: If we want to connect to a server then let's interrupt it, get the IP:PORT and find the server first and get its map name so we can show off a proper loading screen.
	if (CommandLine()->FindParm("+connect"))
	{
		engine->ClientCmd_Unrestricted("disconnect\n");
		GameBaseShared()->GetSteamServerManager()->DirectConnect(CommandLine()->ParmValue("+connect", ""));
	}
}

// Displays a dialog which promts you to type in a password @ server.
void CGameBaseClient::ShowPasswordDialog(bool bShow)
{
	if (MainMenu)
	{
		if (bShow)
			CloseConsole();

		MainMenu->m_pPasswordDialog->ActivateUs(bShow);
	}
}

// If we're not using direct connection then we'll redirect our items to the server browser GUI:
bool CGameBaseClient::AddServerBrowserItem(gameserveritem_t *pGameServerItem)
{
	if (!MainMenu)
		return false;

	CVGUIBasePanel *pServerPanelBase = MainMenu->GetGameUIContextObject(COMMAND_SERVERBROWSER);
	if (pServerPanelBase)
	{
		if (!pServerPanelBase->IsBusy() || !pServerPanelBase->IsVisible())
			return false;

		// Add our item...
		MainMenu->GetContextHandler()->m_pServerMenu->AddServerToList(pGameServerItem);
	}

	return true;
}

// Tell our server browser GUI that we're done searching for the requested search.
void CGameBaseClient::ServerRefreshCompleted(void)
{
	if (!MainMenu)
		return;

	CVGUIBasePanel *pServerPanelBase = MainMenu->GetGameUIContextObject(COMMAND_SERVERBROWSER);
	if (pServerPanelBase)
	{
		if (!pServerPanelBase->IsBusy() || !pServerPanelBase->IsVisible())
			return;

		// Add our item...
		MainMenu->GetContextHandler()->m_pServerMenu->RefreshComplete();
	}
}

void CGameBaseClient::AddScoreboardItem(const char *pszSteamID, const char *playerName, int32 plLevel, int32 plKills, int32 plDeaths, int index)
{
	if (MainMenu && MainMenu->GetContextHandler())
		MainMenu->GetContextHandler()->m_pScoreboard->AddScoreItem(playerName, pszSteamID, plLevel, plKills, plDeaths, index);
}

void CGameBaseClient::ScoreboardRefreshComplete(int maxEntries)
{
	if (MainMenu && MainMenu->GetContextHandler())
		MainMenu->GetContextHandler()->m_pScoreboard->RefreshCallback(maxEntries);
}

void CGameBaseClient::LoadGameLocalization(void)
{
	if (!steamapicontext || !steamapicontext->SteamApps())
		return;

	const char *currentSelectedLanguage = steamapicontext->SteamApps()->GetCurrentGameLanguage();
	char pchPathToLocalizedFile[80];

	// NOT NEEDED ANYMORE, LATEST SDK 2013 UPD, FIXED THIS ISSUE:
	// CLOSED CAPTS. STILL NEED A HACK HACK THOUGH.

	//// Load default localization:
	//const char *localizationFiles[] = { "resource/chat_", "resource/gameui_", "resource/hl2_", "resource/replay_", "resource/valve_", "resource/youtube_" };
	//for (int i = 0; i < _ARRAYSIZE(localizationFiles); ++i)
	//{
	//	Q_snprintf(pchPathToLocalizedFile, 80, "%s%s.txt", localizationFiles[i], currentSelectedLanguage);
	//	g_pVGuiLocalize->AddFile(pchPathToLocalizedFile, "GAME");
	//}

	//// Load game localization:
	//Q_snprintf(pchPathToLocalizedFile, 80, "resource/brainbread2_%s.txt", currentSelectedLanguage);
	//if (filesystem->FileExists(pchPathToLocalizedFile, "MOD"))
	//	g_pVGuiLocalize->AddFile(pchPathToLocalizedFile, "MOD");
	//else
	//	g_pVGuiLocalize->AddFile("resource/brainbread2_english.txt", "MOD");

	// Load subtitle localization:
	Q_snprintf(pchPathToLocalizedFile, 80, "resource/closecaption_%s.dat", currentSelectedLanguage);
	if (filesystem->FileExists(pchPathToLocalizedFile, "MOD"))
		engine->ClientCmd_Unrestricted(VarArgs("cc_lang %s\n", currentSelectedLanguage));
	else
		engine->ClientCmd_Unrestricted("cc_lang english\n");
}

void CGameBaseClient::OnUpdate(void)
{
	CLeaderboardHandler::Update();

	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
	{
		m_bIsMenuVisibleAndInGame = false;
		return;
	}

	if (!engine->IsLevelMainMenuBackground() && engine->IsInGame())
	{
		if (!m_bIsMenuVisibleAndInGame)
		{
			if (enginevgui->IsGameUIVisible())
			{
				m_bIsMenuVisibleAndInGame = true;
				RunMainMenuCommand(VGUI_OVERLAY_INGAME_OPEN);
			}
		}
		else if (m_bIsMenuVisibleAndInGame)
		{
			if (!enginevgui->IsGameUIVisible())
			{
				m_bIsMenuVisibleAndInGame = false;
				RunMainMenuCommand(VGUI_OVERLAY_INGAME_CLOSE);
			}
		}
	}

	BB2PlayerGlobals->OnUpdate();
}

void CGameBaseClient::OnLocalPlayerExternalRendering(void)
{
	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	if (bb2_render_client_in_mirrors.GetBool())
	{
		pLocal->ThirdPersonSwitch(g_bShouldRenderLocalPlayerExternally);
		BB2PlayerGlobals->BodyUpdateVisibility();
		C_BaseCombatWeapon *pWeapon = pLocal->GetActiveWeapon();
		if (pWeapon)
			pWeapon->UpdateVisibility();
	}
}

// Is the main menu up while in-game?
bool CGameBaseClient::IsMainMenuVisibleWhileInGame(void)
{
	return m_bIsMenuVisibleAndInGame;
}

bool CGameBaseClient::IsPlayerGameVoiceMuted(int playerIndex)
{
	if (!engine->IsInGame())
		return false;

	return GetClientVoiceMgr()->IsPlayerBlocked(playerIndex);
}

void CGameBaseClient::MutePlayerGameVoice(int playerIndex, bool value)
{
	if (!engine->IsInGame())
		return;

	GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, value);
}

void CGameBaseClient::Steam_OnUserStatsReceived(UserStatsReceived_t *pUserStatsReceived)
{
	if (g_bHasLoadedSteamStats || !steamapicontext || !steamapicontext->SteamUserStats())
		return;

	DevMsg("Load SteamStats: EResult %d\n", pUserStatsReceived->m_eResult);

	if (pUserStatsReceived->m_eResult != k_EResultOK)
		return;

	IGameEvent *event = gameeventmanager->CreateEvent("user_data_downloaded");
	if (event)
		gameeventmanager->FireEventClientSide(event);

	// Update achievement panel!
	if (MainMenu && MainMenu->GetContextHandler() && MainMenu->GetContextHandler()->m_pAchievementPanel)
	{
		MainMenu->GetContextHandler()->m_pAchievementPanel->Cleanup();
		MainMenu->GetContextHandler()->m_pAchievementPanel->SetupLayout();
	}

	CLeaderboardHandler::InitHandle();
	g_bHasLoadedSteamStats = true;
}

// Toggle Console On or Off.
void CGameBaseClient::ToggleConsole(void)
{
	if (MainMenu)
		MainMenu->ConsoleDialog->ToggleConsole(!MainMenu->ConsoleDialog->IsVisible());
}

// Close the Console.
void CGameBaseClient::CloseConsole(void)
{
	if (MainMenu)
		MainMenu->ConsoleDialog->ToggleConsole(false, true);
}

// Clear / Reset the text/log in the console.
void CGameBaseClient::ClearConsole(void)
{
	if (MainMenu)
		MainMenu->ConsoleDialog->Clear();
}

// Console Commands
CON_COMMAND(OpenGameConsole, "Toggle the Console ON or OFF...")
{
	GameBaseClient->ToggleConsole();
};

CON_COMMAND(CloseGameConsole, "Force the Console to close!")
{
	GameBaseClient->CloseConsole();
};

CON_COMMAND(ClearGameConsole, "Reset Console/Clear all history text.")
{
	GameBaseClient->ClearConsole();
};

CON_COMMAND(vote_menu, "Open Vote Menu")
{
	bool bForce = false;

	if (args.ArgC() >= 2)
		bForce = ((atoi(args[1]) >= 1) ? true : false);

	GameBaseClient->ShowVotePanel(bForce);
};

CON_COMMAND(dev_reset_achievements, "Reset Achievements")
{
	if (!steamapicontext || !steamapicontext->SteamUserStats())
		return;

	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient || !g_PR)
	{
		Warning("You need to be in-game to use this command!\n");
		return;
	}

	if (g_PR->IsGroupIDFlagActive(pClient->entindex(), GROUPID_IS_DEVELOPER) || g_PR->IsGroupIDFlagActive(pClient->entindex(), GROUPID_IS_TESTER))
	{
		steamapicontext->SteamUserStats()->ResetAllStats(true);
		Warning("You've reset your achievements!\n");
	}
	else
		Warning("You must be a developer or tester to use this command!\n");
};
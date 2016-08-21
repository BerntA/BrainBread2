//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: 2 Main Game Interface for BB2 Client Side
//
//========================================================================================//

#ifndef GAME_BASE_CLIENT_H
#define GAME_BASE_CLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include "c_hl2mp_player.h"
#include <steam/steam_api.h>
#include "steam/isteamapps.h"

extern ConVar bb2_render_client_in_mirrors;
extern ConVar bb2_show_details;
extern ConVar bb2_extreme_gore;
extern ConVar bb2_enable_healthbar_for_all;
extern ConVar bb2_enable_multicore;

typedef struct IVoiceTweak_s IVoiceTweak;

// These are the available GameUI commands.
enum AvailableGameMenuCommands
{
	COMMAND_QUIT = 0,
	COMMAND_OPTIONS,
	COMMAND_CREDITS,
	COMMAND_PLAY,
	COMMAND_SERVERBROWSER,
	COMMAND_CREATEGAME,
	COMMAND_KEYBOARD,
	COMMAND_MOUSE,
	COMMAND_AUDIO,
	COMMAND_VIDEO,
	COMMAND_GRAPHICS,
	COMMAND_PERFORMANCE,
	COMMAND_OTHER,
	COMMAND_RETURN,
	COMMAND_DISCONNECT,
	COMMAND_QUITCONFIRM,
	COMMAND_RESET,
	COMMAND_PASSWORD_OK,
	COMMAND_PASSWORD_CANCEL,
	COMMAND_SHOW_SCOREBOARD ,
	COMMAND_PROFILE,
	COMMAND_ACHIEVEMENT_PANEL,
	COMMAND_CHARACTER_PANEL,
	COMMAND_START_TUTORIAL,
	COMMAND_OPTIONS_APPLY,
	COMMAND_LAST,
};

class IGameBaseClient
{
public:

	virtual void        Initialize(void) = 0;
	virtual void        CreateInGamePanels(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;

	// GameUI Accessors
	virtual void RunMainMenuCommand(int cmd) = 0;
	virtual void RunCommand(int iCommand) = 0;
	virtual void RunClientEffect(int iEffect, int iState) = 0;
	virtual void PostInit(void) = 0;
	virtual void ShowPasswordDialog(bool bShow) = 0;
	virtual bool AddServerBrowserItem(gameserveritem_t *pGameServerItem) = 0;
	virtual void ServerRefreshCompleted(void) = 0;

	// Scoreboard Handling
	virtual void RefreshScoreboard(int iOffset = 0) = 0;
	virtual void AddScoreboardItem(const char *pszSteamID, const char *playerName, int32 plLevel, int32 plKills, int32 plDeaths, int index) = 0;
	virtual void ScoreboardRefreshComplete(int maxEntries) = 0;

	// Shared
	virtual void LoadGameLocalization(void) = 0;
	virtual void OnUpdate(void) = 0;
	virtual bool CanOpenPanel(void) = 0;
	virtual void CloseGamePanels(void) = 0;
	virtual bool IsInGame(void) = 0;
	bool IsExtremeGore() { return bb2_extreme_gore.GetBool(); }
	virtual void ResetMapConVar(void) = 0;
	virtual void RunMap(const char *szMap) = 0;
	virtual void RunMap(const char *szMap, const char *szConnectionString) = 0;
	virtual void RunMap(const char *szMap, const char *szConnectionString, const char *szPassword) = 0;
	virtual void Changelevel(const char *szMap) = 0;
	virtual void RefreshVGUI(void) = 0;
	virtual const char *GetLoadingImage() = 0;
	IVoiceTweak				*m_pVoiceTweak;		// Engine voice tweak API.

	// In-Game Panel Accessors
	virtual void ShowNote(const char *szHeader, const char *szFile) = 0;
	virtual void ShowGamePanel(bool bForceOff = false) = 0;
	virtual void ShowVotePanel(bool bForceOff = false) = 0;
	virtual void SelectQuestPreview(int index) = 0;
	virtual bool IsViewPortPanelVisible(const char *panel) = 0;

	// Custom Console
	virtual void ToggleConsole(void) = 0;
	virtual void CloseConsole(void) = 0;
	virtual void ClearConsole(void) = 0;

	// In-Game 
	virtual void OnLocalPlayerExternalRendering(void) = 0;
	virtual bool IsMainMenuVisibleWhileInGame(void) = 0;

	// Voice
	virtual bool IsPlayerGameVoiceMuted(int playerIndex) = 0;
	virtual void MutePlayerGameVoice(int playerIndex, bool value) = 0;
};

extern IGameBaseClient *GameBaseClient;

#endif // GAME_BASE_CLIENT_H
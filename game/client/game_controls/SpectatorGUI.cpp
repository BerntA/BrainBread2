//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>
#include "spectatorgui.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "c_team.h"
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/TextImage.h>
#include "hl2mp_gamerules.h"
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include <stdio.h> // _snprintf define
#include <game/client/iviewport.h>
#include "commandmenu.h"
#include "GameBase_Client.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#endif

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include <igameresources.h>
#include "c_world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

CSpectatorGUI *g_pSpectatorGUI = NULL;

using namespace vgui;

ConVar cl_spec_mode("cl_spec_mode", "1", FCVAR_ARCHIVE | FCVAR_USERINFO | FCVAR_SERVER_CAN_EXECUTE, "spectator mode");

//-----------------------------------------------------------------------------
// main spectator panel
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorGUI::CSpectatorGUI(IViewPort *pViewPort) : EditablePanel(NULL, PANEL_SPECGUI)
{
	m_pViewPort = pViewPort;
	g_pSpectatorGUI = this;

	// initialize dialog
	SetVisible(false);
	SetProportional(true);

	// load the new scheme early!!
	SetScheme("BaseScheme");
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);

	m_pBackground = new Divider(this, "background");
	m_pPlayerLabel = new Label(this, "playerlabel", "");
	m_pGameInfo = new Label(this, "gamelabel", "");
	m_pGameInfo->SetVisible(true);

	m_pSharedInfo = new Label(this, "sharedlabel", "");

	Q_strncpy(pszMapName, "", 128);

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	m_pBackground->SetVisible(true);

	m_bShouldHide = false;

	InvalidateLayout();
	PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpectatorGUI::~CSpectatorGUI()
{
	g_pSpectatorGUI = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CSpectatorGUI::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(Color(0, 0, 0, 0)); // make the background transparent
	SetPaintBorderEnabled(false);

	SetBorder(NULL);

	// BB2
	m_pSharedInfo->SetFont(pScheme->GetFont("SpecStatus"));
	m_pGameInfo->SetFont(pScheme->GetFont("SpecStatus"));
	m_pPlayerLabel->SetFont(pScheme->GetFont("SpecText"));

	m_pSharedInfo->SetFgColor(pScheme->GetColor("SpectatorGUI.TextColor", Color(255, 255, 255, 255)));
	m_pGameInfo->SetFgColor(pScheme->GetColor("SpectatorGUI.TextColor", Color(255, 255, 255, 255)));
	m_pPlayerLabel->SetFgColor(pScheme->GetColor("SpectatorGUI.TextColor", Color(255, 255, 255, 255)));

	m_pBackground->SetBgColor(pScheme->GetColor("SpectatorGUI.BgColor", Color(15, 22, 20, 125)));
	m_pBackground->SetFgColor(pScheme->GetColor("SpectatorGUI.FgColor", Color(19, 17, 20, 255)));
	m_pBackground->SetBorder(NULL);
	m_pBackground->SetPaintBorderEnabled(false);

	SetZPos(-1);	// guarantee it shows above the scope
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorGUI::PerformLayout()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CSpectatorGUI::OnThink()
{
	BaseClass::OnThink();

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !IsVisible())
		return;

	SetSize(ScreenWidth(), ScreenHeight());
	m_pBackground->SetSize(ScreenWidth(), scheme()->GetProportionalScaledValue(40));

	int w, h;
	GetSize(w, h);

	UpdateSpecInfo();

	m_bShouldHide = !(GameBaseClient->IsViewPortPanelVisible(PANEL_ENDSCORE) || GameBaseClient->IsViewPortPanelVisible(PANEL_SCOREBOARD) || GameBaseClient->IsViewPortPanelVisible(PANEL_INFO) || GameBaseClient->IsViewPortPanelVisible(PANEL_TEAM));

	m_pPlayerLabel->SetContentAlignment(Label::Alignment::a_center);
	m_pPlayerLabel->SetPos(0, h - scheme()->GetProportionalScaledValue(24));
	m_pPlayerLabel->SetSize(w, scheme()->GetProportionalScaledValue(24));
	m_pGameInfo->SetSize(w, scheme()->GetProportionalScaledValue(40));
	m_pSharedInfo->SetSize(scheme()->GetProportionalScaledValue(140), scheme()->GetProportionalScaledValue(40));

	m_pBackground->SetVisible(m_bShouldHide);
	m_pGameInfo->SetVisible(m_bShouldHide);
	m_pSharedInfo->SetVisible(m_bShouldHide);
	m_pPlayerLabel->SetVisible(m_bShouldHide);
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CSpectatorGUI::ShowPanel(bool bShow)
{
	SetVisible(bShow);
}

bool CSpectatorGUI::ShouldShowPlayerLabel(int specmode)
{
	return ((specmode == OBS_MODE_IN_EYE) || (specmode == OBS_MODE_CHASE));
}
//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearranges elements
//-----------------------------------------------------------------------------
void CSpectatorGUI::Update()
{
	IGameResources *gr = GameResources();
	if (!gr)
		return;

	int specmode = GetSpectatorMode();
	int playernum = GetSpectatorTarget();

	m_pPlayerLabel->SetVisible(ShouldShowPlayerLabel(specmode));

	// update player name filed, text & color
	if (playernum > 0 && playernum <= gpGlobals->maxClients)
	{
		int m_iMaxHP = 100;

		C_BasePlayer *pClient = UTIL_PlayerByIndex(playernum);
		if (pClient)
			m_iMaxHP = pClient->GetMaxHealth();

		m_pPlayerLabel->SetText(VarArgs("%s [%i|%i]", gr->GetPlayerName(playernum), gr->GetHealth(playernum), m_iMaxHP));
	}
	else
		m_pPlayerLabel->SetText(L"");

	Q_FileBase(engine->GetLevelName(), pszMapName, 128);
}

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
void CSpectatorGUI::UpdateSpecInfo()
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !g_PR)
		return;

	int w, h;
	GetSize(w, h);

	int timer = (int)HL2MPRules()->GetTimeLeft();
	wchar_t wszUnicodeString[128];

	m_pSharedInfo->SetContentAlignment(Label::Alignment::a_east);
	m_pSharedInfo->SetPos(w - scheme()->GetProportionalScaledValue(140), 0);
	m_pSharedInfo->SetText(VarArgs("Map: %s\nTime: %d:%02d", pszMapName, (timer / 60), (timer % 60)));

	if (HL2MPRules()->GetCurrentGamemode() == MODE_ARENA)
	{
		if (HL2MPRules()->m_bRoundStarted)
		{
			if (HL2MPRules()->m_iNumReinforcements > 0)
			{
				wchar_t wszArg1[10], wszArg2[10];
				V_swprintf_safe(wszArg1, L"%i", (int)HL2MPRules()->GetReinforcementRespawnTime());
				V_swprintf_safe(wszArg2, L"%i", HL2MPRules()->m_iNumReinforcements);
				g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_RespawnTimeArena"), 2, wszArg1, wszArg2);
				m_pGameInfo->SetText(wszUnicodeString);
			}
			else
				m_pGameInfo->SetText("#HUD_NoReinforcementsLeft");
		}
		else
			m_pGameInfo->SetText("#HUD_WaitRoundStart");

		m_pGameInfo->SetContentAlignment(Label::Alignment::a_west);
		m_pGameInfo->SetPos(scheme()->GetProportionalScaledValue(4), 0);
		return;
	}
	else if (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE)
	{
		if (GetClientWorldEntity() && GetClientWorldEntity()->m_bIsStoryMap)
		{
			if (!pPlayer->m_BB2Local.m_bHasPlayerEscaped)
			{
				bool bDisplayRespawnTime = false;

				float flRespawnTime = pPlayer->GetRespawnTime() - gpGlobals->curtime;
				if (g_PR->GetSelectedTeam(pPlayer->entindex()) > 0)
					bDisplayRespawnTime = true;

				if (flRespawnTime < 0)
					flRespawnTime = 0.0f;

				if (bDisplayRespawnTime)
				{
					wchar_t wszArg1[10];
					V_swprintf_safe(wszArg1, L"%i", (int)flRespawnTime);
					g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_RespawnTimeDeathmatch"), 1, wszArg1);
					m_pGameInfo->SetText(wszUnicodeString);

					m_pGameInfo->SetContentAlignment(Label::Alignment::a_west);
					m_pGameInfo->SetPos(scheme()->GetProportionalScaledValue(4), 0);
					return;
				}
			}
		}
		else
		{
			if (pPlayer->m_BB2Local.m_bHasPlayerEscaped)
			{
				const char *key = engine->Key_LookupBinding("classic_rejoin_zombie");
				if (key)
				{
					char pszKey[32];
					Q_strncpy(pszKey, key, 32);
					Q_strupr(pszKey);

					wchar_t wszKey[32];
					g_pVGuiLocalize->ConvertANSIToUnicode(pszKey, wszKey, sizeof(wszKey));
					g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_RejoinAsZombie"), 1, wszKey);

					m_pGameInfo->SetText(wszUnicodeString);
					m_pGameInfo->SetPos(0, 0);
					m_pGameInfo->SetContentAlignment(Label::Alignment::a_center);
					return;
				}

				m_pGameInfo->SetText("#HUD_RejoinAsZombieTip");
				m_pGameInfo->SetPos(0, 0);
				m_pGameInfo->SetContentAlignment(Label::Alignment::a_center);
				return;
			}
		}
	}
	else if (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)
	{
		if (HL2MPRules()->m_bRoundStarted)
		{
			float flRespawnTime = 0.0f;
			bool bDisplayRespawnTime = false;

			flRespawnTime = pPlayer->GetRespawnTime() - gpGlobals->curtime;
			if (g_PR->GetSelectedTeam(pPlayer->entindex()) > 0)
				bDisplayRespawnTime = true;

			if (flRespawnTime < 0)
				flRespawnTime = 0.0f;

			C_Team *pHumanTeam = GetGlobalTeam(TEAM_HUMANS);
			C_Team *pZombieTeam = GetGlobalTeam(TEAM_DECEASED);

			if (bDisplayRespawnTime)
			{
				wchar_t wszArg1[10], wszArg2[10], wszArg3[10];
				V_swprintf_safe(wszArg1, L"%i", pHumanTeam->Get_Score());
				V_swprintf_safe(wszArg2, L"%i", (int)flRespawnTime);
				V_swprintf_safe(wszArg3, L"%i", pZombieTeam->Get_Score());
				g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_RespawnTimeElimination"), 3, wszArg1, wszArg2, wszArg3);
				m_pGameInfo->SetText(wszUnicodeString);
			}
			else
			{
				wchar_t wszArg1[10], wszArg2[10];
				V_swprintf_safe(wszArg1, L"%i", pHumanTeam->Get_Score());
				V_swprintf_safe(wszArg2, L"%i", pZombieTeam->Get_Score());
				g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_Elimination"), 2, wszArg1, wszArg2);
				m_pGameInfo->SetText(wszUnicodeString);
			}
		}
		else
			m_pGameInfo->SetText("#HUD_WaitRoundStart");

		m_pGameInfo->SetContentAlignment(Label::Alignment::a_west);
		m_pGameInfo->SetPos(scheme()->GetProportionalScaledValue(4), 0);
		return;
	}
	else if (HL2MPRules()->GetCurrentGamemode() == MODE_DEATHMATCH)
	{
		if (HL2MPRules()->m_bRoundStarted)
		{
			float flRespawnTime = (pPlayer->GetRespawnTime() - gpGlobals->curtime);
			if (flRespawnTime < 0)
				flRespawnTime = 0.0f;

			wchar_t wszArg1[10];
			V_swprintf_safe(wszArg1, L"%i", (int)flRespawnTime);
			g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_RespawnTimeDeathmatch"), 1, wszArg1);
			m_pGameInfo->SetText(wszUnicodeString);
		}
		else
			m_pGameInfo->SetText("#HUD_WaitRoundStart");

		m_pGameInfo->SetContentAlignment(Label::Alignment::a_west);
		m_pGameInfo->SetPos(scheme()->GetProportionalScaledValue(4), 0);
		return;
	}

	m_pGameInfo->SetText(L"");
}

static void ForwardSpecCmdToServer(const CCommand &args)
{
	if (engine->IsPlayingDemo())
		return;

	if (args.ArgC() == 1)
	{
		// just forward the command without parameters
		engine->ServerCmd(args[0]);
	}
	else if (args.ArgC() == 2)
	{
		// forward the command with parameter
		char command[128];
		Q_snprintf(command, sizeof(command), "%s \"%s\"", args[0], args[1]);
		engine->ServerCmd(command);
	}
}

CON_COMMAND_F(spec_next, "Spectate next player", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	if (engine->IsHLTV())
	{
		// handle the command clientside
		if (!HLTVCamera()->IsPVSLocked())
		{
			HLTVCamera()->SpecNextPlayer(false);
		}
	}
	else
	{
		ForwardSpecCmdToServer(args);
	}
}

CON_COMMAND_F(spec_prev, "Spectate previous player", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	if (engine->IsHLTV())
	{
		// handle the command clientside
		if (!HLTVCamera()->IsPVSLocked())
		{
			HLTVCamera()->SpecNextPlayer(true);
		}
	}
	else
	{
		ForwardSpecCmdToServer(args);
	}
}

CON_COMMAND_F(spec_mode, "Set spectator mode", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	if (engine->IsHLTV())
	{
		if (HLTVCamera()->IsPVSLocked())
		{
			// in locked mode we can only switch between first and 3rd person
			HLTVCamera()->ToggleChaseAsFirstPerson();
		}
		else
		{
			// we can choose any mode, not loked to PVS
			int mode;

			if (args.ArgC() == 2)
			{
				// set specifc mode
				mode = Q_atoi(args[1]);
			}
			else
			{
				// set next mode 
				mode = HLTVCamera()->GetMode() + 1;

				if (mode > LAST_PLAYER_OBSERVERMODE)
					mode = OBS_MODE_IN_EYE;
			}

			// handle the command clientside
			HLTVCamera()->SetMode(mode);
		}

		// turn off auto director once user tried to change view settings
		HLTVCamera()->SetAutoDirector(false);
	}
	else
	{
		// we spectate on a game server, forward command
		ForwardSpecCmdToServer(args);
	}
}

CON_COMMAND_F(spec_player, "Spectate player by name", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer || !pPlayer->IsObserver())
		return;

	if (args.ArgC() != 2)
		return;

	if (engine->IsHLTV())
	{
		// we can only switch primary spectator targets is PVS isnt locked by auto-director
		if (!HLTVCamera()->IsPVSLocked())
		{
			HLTVCamera()->SpecNamedPlayer(args[1]);
		}
	}
	else
	{
		ForwardSpecCmdToServer(args);
	}
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Scoreboard VGUI
//
//========================================================================================//

#include "cbase.h"
#include "ScoreBoardPanel.h"
#include <voice_status.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>
#include <vgui_controls/Label.h>
#include <igameresources.h>
#include "c_team.h"
#include "c_world.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CScoreBoardPanel::CScoreBoardPanel(IViewPort *pViewPort) : CVGUIBaseFrame(NULL, PANEL_SCOREBOARD, false, 0.2f)
{
	m_pViewPort = pViewPort;

	SetZPos(10);
	SetProportional(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetMoveable(false);
	SetSizeable(false);
	SetTitleBarVisible(false);
	SetCloseButtonVisible(false);

	SetScheme("BaseScheme");

	ListenForGameEvent("server_spawn");

	serverLabel = vgui::SETUP_PANEL(new vgui::Label(this, "ServerName", ""));
	serverLabel->SetZPos(80);
	serverLabel->SetContentAlignment(Label::Alignment::a_center);

	int teams[] =
	{
		TEAM_HUMANS,
		TEAM_DECEASED,
		TEAM_SPECTATOR,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pScoreSections); i++)
	{
		m_pScoreSections[i] = vgui::SETUP_PANEL(new vgui::CScoreBoardSectionPanel(this, "Section", teams[i]));
		m_pScoreSections[i]->SetZPos(50);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pInfoLabel); i++)
	{
		m_pInfoLabel[i] = vgui::SETUP_PANEL(new vgui::Label(this, "Info", ""));
		m_pInfoLabel[i]->SetZPos(60);
	}

	m_pInfoLabel[2]->SetContentAlignment(Label::Alignment::a_center);
	m_pInfoLabel[3]->SetContentAlignment(Label::Alignment::a_center);

	PerformLayout();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CScoreBoardPanel::~CScoreBoardPanel()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Call every frame
//-----------------------------------------------------------------------------
void CScoreBoardPanel::OnThink()
{
	BaseClass::OnThink();

	SetSize(ScreenWidth(), ScreenHeight());
	SetPos(0, 0);

	int w, h;
	GetSize(w, h);

	serverLabel->SetSize(w, scheme()->GetProportionalScaledValue(20));
	serverLabel->SetPos(0, 0);

	m_pInfoLabel[2]->SetSize(w, scheme()->GetProportionalScaledValue(10));
	m_pInfoLabel[2]->SetPos(0, scheme()->GetProportionalScaledValue(15));

	m_pInfoLabel[3]->SetSize(w, scheme()->GetProportionalScaledValue(10));
	m_pInfoLabel[3]->SetPos(0, scheme()->GetProportionalScaledValue(25));

	if (HL2MPRules() && HL2MPRules()->m_bRoundStarted)
	{
		int timer = (int)HL2MPRules()->GetTimeLeft();
		m_pInfoLabel[2]->SetText(VarArgs("Time: %d:%02d", (timer / 60), (timer % 60)));
		m_pInfoLabel[3]->SetText(VarArgs("Gamemode: %s", GetGamemodeName(HL2MPRules()->GetCurrentGamemode())));

		wchar_t wszUnicodeString[128];

		switch (HL2MPRules()->GetCurrentGamemode())
		{

		case MODE_DEATHMATCH:
		{
			int fraglimit = bb2_deathmatch_fraglimit.GetInt();
			if (fraglimit > 0)
			{
				wchar_t wszArg1[10];
				V_swprintf_safe(wszArg1, L"%i", fraglimit);
				g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_Fraglimit"), 1, wszArg1);
				m_pInfoLabel[0]->SetText(wszUnicodeString);
			}
			break;
		}

		case MODE_ARENA:
		{
			if (HL2MPRules()->m_iNumReinforcements > 0)
			{
				wchar_t wszArg1[10];
				V_swprintf_safe(wszArg1, L"%i", HL2MPRules()->m_iNumReinforcements);
				g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_ReinforcementsLeft"), 1, wszArg1);
				m_pInfoLabel[0]->SetText(wszUnicodeString);
			}
			else
				m_pInfoLabel[0]->SetText("#HUD_NoReinforcementsLeft");
			break;
		}

		case MODE_ELIMINATION:
		{
			C_Team *pHumanTeam = GetGlobalTeam(TEAM_HUMANS);
			C_Team *pZombieTeam = GetGlobalTeam(TEAM_DECEASED);

			wchar_t wszArg1[10], wszArg2[10];
			V_swprintf_safe(wszArg1, L"%i", pHumanTeam->Get_Score());
			V_swprintf_safe(wszArg2, L"%i", pZombieTeam->Get_Score());

			g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_TeamScore"), 1, wszArg1);
			m_pInfoLabel[0]->SetText(wszUnicodeString);

			g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#HUD_TeamScore"), 1, wszArg2);
			m_pInfoLabel[1]->SetText(wszUnicodeString);
			break;
		}

		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: clears everything in the scoreboard and all it's state
//-----------------------------------------------------------------------------
void CScoreBoardPanel::Reset()
{
	for (int i = 0; i < _ARRAYSIZE(m_pScoreSections); i++)	
		m_pScoreSections[i]->Cleanup();	
}

//-----------------------------------------------------------------------------
// Purpose: sets up screen
//-----------------------------------------------------------------------------
void CScoreBoardPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBgColor(Color(0, 0, 0, 0));
	SetBorder(NULL);

	serverLabel->SetFont(pScheme->GetFont("BB2_Scoreboard"));
	serverLabel->SetFgColor(Color(255, 255, 255, 255));

	for (int i = 0; i < 2; i++)
	{
		m_pInfoLabel[i]->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
		m_pInfoLabel[i]->SetFont(pScheme->GetFont("OptionTextMedium"));
	}

	m_pInfoLabel[2]->SetFont(pScheme->GetFont("OptionTextSmall"));
	m_pInfoLabel[3]->SetFont(pScheme->GetFont("OptionTextSmall"));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScoreBoardPanel::ShowPanel(bool bShow)
{
	PerformLayout();

	if (bShow)
	{
		Reset();
		MoveToFront();
	}

	BaseClass::OnShowPanel(bShow);
}

void CScoreBoardPanel::FireGameEvent(IGameEvent *event)
{
	const char * type = event->GetName();
	if (Q_strcmp(type, "server_spawn") == 0)
	{
		const char *hostname = event->GetString("hostname");
		serverLabel->SetText(hostname);
	}
}

void CScoreBoardPanel::PerformLayout()
{
	int w, h, wz, hz;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pScoreSections); i++)
		m_pScoreSections[i]->SetSize((w / 2) - scheme()->GetProportionalScaledValue(24), scheme()->GetProportionalScaledValue(258));

	for (int i = 0; i < _ARRAYSIZE(m_pInfoLabel); i++)
	{
		m_pInfoLabel[i]->SetText("");
		m_pInfoLabel[i]->SetSize((w / 2) - scheme()->GetProportionalScaledValue(24), scheme()->GetProportionalScaledValue(20));
	}

	m_pScoreSections[0]->SetPos(scheme()->GetProportionalScaledValue(16), scheme()->GetProportionalScaledValue(47));
	m_pScoreSections[1]->SetPos((w / 2) + scheme()->GetProportionalScaledValue(8), scheme()->GetProportionalScaledValue(47));

	m_pInfoLabel[0]->SetPos(scheme()->GetProportionalScaledValue(16), scheme()->GetProportionalScaledValue(30));
	m_pInfoLabel[1]->SetPos((w / 2) + scheme()->GetProportionalScaledValue(8), scheme()->GetProportionalScaledValue(30));

	for (int i = 0; i < _ARRAYSIZE(m_pScoreSections); i++)
		m_pScoreSections[i]->PerformLayout();

	if (HL2MPRules())
	{
		bool bSingleScoreboard = ((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) || (HL2MPRules()->GetCurrentGamemode() == MODE_DEATHMATCH) || ((HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE) && GetClientWorldEntity() && GetClientWorldEntity()->m_bIsStoryMap));

		m_pScoreSections[1]->SetVisible(!bSingleScoreboard);
		m_pScoreSections[1]->SetEnabled(!bSingleScoreboard);

		if (bSingleScoreboard)
		{
			m_pScoreSections[0]->GetSize(wz, hz);
			m_pScoreSections[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(47));
			m_pInfoLabel[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(30));

			m_pScoreSections[2]->SetSize(wz, scheme()->GetProportionalScaledValue(160));
			m_pScoreSections[2]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(313));
			m_pScoreSections[2]->PerformLayout();
		}
		else
		{
			m_pInfoLabel[0]->SetPos(scheme()->GetProportionalScaledValue(16), scheme()->GetProportionalScaledValue(30));
			m_pScoreSections[0]->SetPos(scheme()->GetProportionalScaledValue(16), scheme()->GetProportionalScaledValue(47));

			m_pScoreSections[2]->SetSize(w - scheme()->GetProportionalScaledValue(32), scheme()->GetProportionalScaledValue(160));
			m_pScoreSections[2]->SetPos(scheme()->GetProportionalScaledValue(16), scheme()->GetProportionalScaledValue(313));
			m_pScoreSections[2]->PerformLayout();
		}
	}
}
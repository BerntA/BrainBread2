//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Team Selection Menu
//
//========================================================================================//

#include "cbase.h"
#include <stdio.h>
#include <cdll_client_int.h>
#include "team_menu.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <filesystem.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include "vgui_controls/AnimationController.h"
#include <vgui/IInput.h>
#include "vgui_controls/ImagePanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include "c_hl2mp_player.h"
#include "vgui/MouseCode.h"
#include "GameBase_Client.h"
#include "GameBase_Shared.h"
#include "cdll_util.h"
#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

void CTeamMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pImgJoin); i++)
	{
		m_pImgJoin[i]->SetSize(scheme()->GetProportionalScaledValue(200), scheme()->GetProportionalScaledValue(200));
		m_pButtonJoin[i]->SetSize(scheme()->GetProportionalScaledValue(200), scheme()->GetProportionalScaledValue(200));

		m_pButtonJoin[i]->SetMouseInputEnabled(true);
		m_pButtonJoin[i]->SetKeyBoardInputEnabled(true);

		m_pTeamInfo[i]->SetSize(scheme()->GetProportionalScaledValue(200), scheme()->GetProportionalScaledValue(20));
	}

	m_pInfo->SetSize(w, scheme()->GetProportionalScaledValue(24));
	m_pInfo->SetPos(0, scheme()->GetProportionalScaledValue(40));

	m_pImgJoin[0]->SetPos(scheme()->GetProportionalScaledValue(100), h - scheme()->GetProportionalScaledValue(200));
	m_pButtonJoin[0]->SetPos(scheme()->GetProportionalScaledValue(100), h - scheme()->GetProportionalScaledValue(200));
	m_pButtonJoin[0]->MoveToFront();
	m_pTeamInfo[0]->SetPos(scheme()->GetProportionalScaledValue(100), h - scheme()->GetProportionalScaledValue(65));

	m_pImgJoin[1]->SetPos(w - scheme()->GetProportionalScaledValue(300), h - scheme()->GetProportionalScaledValue(200));
	m_pButtonJoin[1]->SetPos(w - scheme()->GetProportionalScaledValue(300), h - scheme()->GetProportionalScaledValue(200));
	m_pButtonJoin[1]->MoveToFront();
	m_pTeamInfo[1]->SetPos(w - scheme()->GetProportionalScaledValue(300), h - scheme()->GetProportionalScaledValue(65));

	for (int i = 0; i < _ARRAYSIZE(m_pModelPreviews); i++)
	{
		m_pModelPreviews[i]->SetSize(scheme()->GetProportionalScaledValue(200), h - scheme()->GetProportionalScaledValue(200));
	}

	m_pModelPreviews[0]->SetPos(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(50));
	m_pModelPreviews[1]->SetPos(w - scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(50));

	m_pImgJoin[0]->SetImage("team/survivor_team");
	m_pImgJoin[1]->SetImage("team/zombie_team");

	m_bRollover[0] = false;
	m_bRollover[1] = false;
}

void CTeamMenu::OnThink()
{
	BaseClass::OnThink();

	SetSize(ScreenWidth(), ScreenHeight());

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (IsVisible() && engine->IsInGame())
	{
		int deceasedSize = HL2MPRules()->GetTeamSize(TEAM_DECEASED);
		int humanSize = HL2MPRules()->GetTeamSize(TEAM_HUMANS);

		wchar_t wszArg1[10], wszArg2[10], wszUnicodeString[128];
		V_swprintf_safe(wszArg1, L"%i", humanSize);
		V_swprintf_safe(wszArg2, L"%i", deceasedSize);

		g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#VGUI_TeamMenuPlayers"), 1, wszArg1);
		m_pTeamInfo[0]->SetText(wszUnicodeString);

		g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#VGUI_TeamMenuPlayers"), 1, wszArg2);
		m_pTeamInfo[1]->SetText(wszUnicodeString);
	}

	if (m_pButtonJoin[0]->IsWithin(x, y) && !m_bRollover[0])
	{
		m_bRollover[0] = true;
		m_pImgJoin[0]->SetImage("team/survivor_team_over");
	}
	else if (!m_pButtonJoin[0]->IsWithin(x, y) && m_bRollover[0])
	{
		m_bRollover[0] = false;
		m_pImgJoin[0]->SetImage("team/survivor_team");
	}

	if (m_pButtonJoin[1]->IsWithin(x, y) && !m_bRollover[1])
	{
		m_bRollover[1] = true;
		m_pImgJoin[1]->SetImage("team/zombie_team_over");
	}
	else if (!m_pButtonJoin[1]->IsWithin(x, y) && m_bRollover[1])
	{
		m_bRollover[1] = false;
		m_pImgJoin[1]->SetImage("team/zombie_team");
	}
}

CTeamMenu::CTeamMenu(IViewPort *pViewPort) : BaseClass(NULL, PANEL_TEAM, false, 0.5f)
{
	m_pViewPort = pViewPort;

	SetZPos(10);

	// initialize dialog
	SetTitle("", false);

	// load the new scheme early!!
	SetScheme("BaseScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetCloseButtonVisible(false);
	SetProportional(true);
	SetVisible(false);

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);

	// Init Default 
	m_pInfo = vgui::SETUP_PANEL(new vgui::Label(this, "info", ""));

	for (int i = 0; i < _ARRAYSIZE(m_pButtonJoin); i++)
	{
		m_pButtonJoin[i] = vgui::SETUP_PANEL(new vgui::Button(this, VarArgs("btnJoin%i", (i + 1)), ""));
		m_pImgJoin[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, VarArgs("imgOption%i", (i + 1))));

		m_pButtonJoin[i]->SetPaintBorderEnabled(false);
		m_pButtonJoin[i]->SetPaintEnabled(false);
		m_pButtonJoin[i]->SetZPos(50);
		m_pButtonJoin[i]->SetArmedSound("ui/button_over.wav");
		m_pButtonJoin[i]->SetReleasedSound("ui/buttonclick.wav");

		m_pImgJoin[i]->SetZPos(25);
		m_pImgJoin[i]->SetShouldScaleImage(true);
	}

	m_pButtonJoin[0]->AddActionSignalTarget(this);
	m_pButtonJoin[1]->AddActionSignalTarget(this);

	m_pButtonJoin[0]->SetCommand("Humans");
	m_pButtonJoin[1]->SetCommand("Zombies");

	for (int i = 0; i < _ARRAYSIZE(m_pTeamInfo); i++)
	{
		m_pTeamInfo[i] = vgui::SETUP_PANEL(new vgui::Label(this, "teamInfo", ""));
		m_pTeamInfo[i]->SetZPos(35);
		m_pTeamInfo[i]->SetContentAlignment(Label::a_center);
	}

	m_pInfo->SetZPos(25);
	m_pInfo->SetContentAlignment(Label::a_center);
	m_pInfo->SetText("#VGUI_TeamMenuInfo");

	for (int i = 0; i < _ARRAYSIZE(m_pModelPreviews); i++)
	{
		m_pModelPreviews[i] = new vgui::CharacterPreviewPanel(this, "ModelPanel");
		m_pModelPreviews[i]->SetZPos(30);
		m_pModelPreviews[i]->SetKeyBoardInputEnabled(false);
		m_pModelPreviews[i]->SetMouseInputEnabled(false);
	}

	PerformLayout();

	//LoadControlSettings("resource/ui/teammenu.res");

	InvalidateLayout(false, true);
}

CTeamMenu::~CTeamMenu()
{
}

void CTeamMenu::OnCommand(const char *command)
{
	if (!Q_stricmp(command, "Humans"))
	{
		engine->ClientCmd_Unrestricted("jointeam 0\n");
	}
	else if (!Q_stricmp(command, "Zombies"))
	{
		engine->ClientCmd_Unrestricted("jointeam 1\n");
	}

	BaseClass::OnCommand(command);
}

void CTeamMenu::ShowPanel(bool bShow)
{
	OnShowPanel(bShow);
}

void CTeamMenu::OnShowPanel(bool bShow)
{
	if (bShow && GameBaseClient->IsViewPortPanelVisible(PANEL_SCOREBOARD))
		return;

	vgui::surface()->PlaySound("common/wpn_hudoff.wav");
	GameBaseClient->CloseConsole();

	BaseClass::OnShowPanel(bShow);

	PerformLayout();

	if (bShow)
	{
		m_pViewPort->ShowBackGround(bShow);
		gViewPortInterface->ShowBackGround(bShow);
		SetVisible(bShow);

		OnSetCharacterPreview();
	}
}

void CTeamMenu::ForceClose(void)
{
	BaseClass::ForceClose();

	for (int i = 0; i < _ARRAYSIZE(m_pModelPreviews); i++)
	{
		m_pModelPreviews[i]->DeleteModelData();
	}

	m_pViewPort->ShowBackGround(false);
	gViewPortInterface->ShowBackGround(false);
	SetVisible(false);
}

void CTeamMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pInfo->SetFgColor(pScheme->GetColor("TeamMenu.TextColor", Color(255, 255, 255, 255)));
	m_pInfo->SetFont(pScheme->GetFont("ScoreMenuBig"));

	for (int i = 0; i < _ARRAYSIZE(m_pTeamInfo); i++)
	{
		m_pTeamInfo[i]->SetFgColor(pScheme->GetColor("TeamMenu.TextColor", Color(255, 255, 255, 255)));
		m_pTeamInfo[i]->SetFont(pScheme->GetFont("SkillOtherText"));
	}
}

void CTeamMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

Panel *CTeamMenu::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CTeamMenu::Reset()
{
}

void CTeamMenu::OnSetCharacterPreview()
{
	if (!GameBaseShared()->GetSharedGameDetails())
		return;

	ConVarRef bb2_survivor_choice("bb2_survivor_choice");
	const char *survivor = bb2_survivor_choice.GetString();
	if (!survivor)
		survivor = "";
	
	m_pModelPreviews[0]->LoadModel(survivor, TEAM_HUMANS);
	m_pModelPreviews[1]->LoadModel(survivor, TEAM_DECEASED);

	ConVarRef extra_skin("bb2_survivor_choice_skin");
	ConVarRef extra_head("bb2_survivor_choice_extra_head");
	ConVarRef extra_body("bb2_survivor_choice_extra_body");
	ConVarRef extra_rightleg("bb2_survivor_choice_extra_leg_right");
	ConVarRef extra_leftleg("bb2_survivor_choice_extra_leg_left");

	for (int i = 0; i < _ARRAYSIZE(m_pModelPreviews); i++)
	{
		m_pModelPreviews[i]->SetProperties(extra_skin.GetInt(), extra_head.GetInt(), extra_body.GetInt(), extra_rightleg.GetInt(), extra_leftleg.GetInt());
	}
}
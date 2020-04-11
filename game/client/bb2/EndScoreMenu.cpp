//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Round End Score / End Game Score Preview, displaying best players, who won, etc in an animated and fancy manner.
//
//========================================================================================//

#include "cbase.h"
#include "EndScoreMenu.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/ImagePanel.h>
#include <game/client/iviewport.h>
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CEndScoreMenu::CEndScoreMenu(IViewPort *pViewPort) : BaseClass(NULL, PANEL_ENDSCORE, false, 0.5f, true)
{
	m_pViewPort = pViewPort;

	SetZPos(-1);

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

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);

	//LoadControlSettings("resource/ui/endscores.res");
	m_pLabelWinner = new Label(this, "LabelWinner", "");
	m_pLabelWinner->SetZPos(20);
	m_pLabelWinner->SetContentAlignment(Label::a_center);

	m_pTopPlayerList = new vgui::TopPlayersPanel(this, "TopPlayers");
	m_pTopPlayerList->AddActionSignalTarget(this);
	m_pTopPlayerList->SetZPos(30);

	m_pCharacterStatsPreview = vgui::SETUP_PANEL(new vgui::CharacterStatPreview(this, "CharacterStats"));
	m_pCharacterStatsPreview->AddActionSignalTarget(this);
	m_pCharacterStatsPreview->SetZPos(40);

	PerformLayout();
	InvalidateLayout(false, true);
	SetupLayout(true);

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
}

CEndScoreMenu::~CEndScoreMenu()
{
}

void CEndScoreMenu::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CEndScoreMenu::Update()
{
	if (GetBackground())
		GetBackground()->SetVisible(false);
}

void CEndScoreMenu::OnThink()
{
	BaseClass::OnThink();

	SetSize(ScreenWidth(), ScreenHeight());
	SetPos(0, 0);

	int w, h;
	GetSize(w, h);

	m_pTopPlayerList->SetAlpha(m_pLabelWinner->GetAlpha());

	m_pTopPlayerList->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(300));
	m_pTopPlayerList->SetPos((w / 2) - (scheme()->GetProportionalScaledValue(300) / 2), (h / 2) - (scheme()->GetProportionalScaledValue(200) / 2));

	int height = scheme()->GetProportionalScaledValue(300);
	int width = scheme()->GetProportionalScaledValue(240);
	m_pCharacterStatsPreview->SetSize(width, height);
	m_pCharacterStatsPreview->SetPos(w - width, (h / 2) - (height / 2));
}

void CEndScoreMenu::ShowPanel(bool bShow)
{
	if (bShow)
	{
		SetupLayout(true);
		m_pTopPlayerList->SetVisible(m_pTopPlayerList->FindTopPlayers());
	}

	GameBaseClient->CloseConsole();
	OnShowPanel(bShow);

	g_pVGuiPanel->MoveToBack(GetVPanel());
}

void CEndScoreMenu::SetupLayout(bool bReset, int iWinner, bool bTimeRanOut)
{
	if (bReset)
	{
		m_pTopPlayerList->SetAlpha(0);
		m_pTopPlayerList->SetVisible(false);
		m_pCharacterStatsPreview->SetAlpha(0);
		//m_pCharacterStatsPreview->SetVisible(true);
		m_pLabelWinner->SetAlpha(0);
		m_pLabelWinner->SetSize(ScreenWidth(), scheme()->GetProportionalScaledValue(30));
		m_pLabelWinner->SetPos(0, -scheme()->GetProportionalScaledValue(30));
		return;
	}

	int gamemode = MODE_OBJECTIVE;
	if (g_pGameRules != NULL)
		gamemode = HL2MPRules()->GetCurrentGamemode();

	const char *labelText = "#VGUI_GameOver_Draw";
	switch (iWinner)
	{
	case TEAM_DECEASED:
	{
		labelText = "#VGUI_GameOver_ZombiesDefault";
		if (gamemode == MODE_ARENA)
			labelText = "#VGUI_GameOver_ZombiesArena";
		break;
	}
	case TEAM_HUMANS:
	{
		labelText = "#VGUI_GameOver_HumansDefault";
		if (gamemode == MODE_ARENA)
			labelText = "#VGUI_GameOver_HumansArena";
		break;
	}
	}

	if (bTimeRanOut && (gamemode != MODE_DEATHMATCH) && (gamemode != MODE_ELIMINATION))
		labelText = "#VGUI_GameOver_NoTime";

	if (gamemode == MODE_DEATHMATCH)
		labelText = "#VGUI_GameOver_Deathmatch";

	m_pLabelWinner->SetText(labelText);

	int w, h;
	m_pLabelWinner->GetSize(w, h);
	GetAnimationController()->RunAnimationCommand(m_pLabelWinner, "ypos", (float)m_iLabelWinnerPositionY, 0.0f, 1.0f, AnimationController::INTERPOLATOR_LINEAR);
	GetAnimationController()->RunAnimationCommand(m_pLabelWinner, "alpha", 255.0f, 0.0f, 1.5f, AnimationController::INTERPOLATOR_LINEAR);
	GetAnimationController()->RunAnimationCommand(m_pCharacterStatsPreview, "alpha", 255.0f, 1.0f, 1.5f, AnimationController::INTERPOLATOR_ACCEL);

	m_pCharacterStatsPreview->ShowStatsForPlayer(GetLocalPlayerIndex());
}

void CEndScoreMenu::ForceClose(void)
{
	BaseClass::ForceClose();

	SetupLayout(true);
}

void CEndScoreMenu::SetData(KeyValues *data)
{
	SetupLayout(false, data->GetInt("winner"), data->GetBool("timeRanOut"));
}

void CEndScoreMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_iLabelWinnerPositionY = scheme()->GetProportionalScaledValue(atoi(pScheme->GetResourceString("ScoreMenu.WinnerPositionY")));

	m_pLabelWinner->SetFgColor(pScheme->GetColor("ScoreMenuWinnerTextColor", Color(255, 255, 255, 255)));
	m_pLabelWinner->SetFont(pScheme->GetFont("ScoreMenuBig"));
}

void CEndScoreMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

Panel *CEndScoreMenu::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CEndScoreMenu::Reset()
{
}
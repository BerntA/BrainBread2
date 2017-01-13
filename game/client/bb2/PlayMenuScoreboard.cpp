//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays Global Scores - Stat Tracking for all players. Global Scores...
//
//========================================================================================//

#include "cbase.h"
#include <stdio.h>
#include "filesystem.h"
#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "PlayMenuScoreboard.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include "vgui_controls/AnimationController.h"
#include <vgui_controls/SectionedListPanel.h>
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "inputsystem/iinputsystem.h"
#include "utlvector.h"
#include "KeyValues.h"
#include "filesystem.h"
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

PlayMenuScoreboard::PlayMenuScoreboard(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	for (int i = 0; i < ARRAYSIZE(m_pScoreItem); i++)
		m_pScoreItem[i] = NULL;

	for (int i = 0; i < ARRAYSIZE(m_pBtnArrowBox); i++)
	{
		m_pImgArrowBox[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgNavigation"));
		m_pImgArrowBox[i]->SetShouldScaleImage(true);
		m_pImgArrowBox[i]->SetZPos(40);

		m_pBtnArrowBox[i] = vgui::SETUP_PANEL(new vgui::Button(this, "btnNavigation", ""));
		m_pBtnArrowBox[i]->SetPaintBorderEnabled(false);
		m_pBtnArrowBox[i]->SetPaintEnabled(false);
		m_pBtnArrowBox[i]->SetReleasedSound("ui/button_click.wav");
		m_pBtnArrowBox[i]->SetArmedSound("ui/button_over.wav");
		m_pBtnArrowBox[i]->SetZPos(50);
		m_pBtnArrowBox[i]->AddActionSignalTarget(this);
	}

	for (int i = 0; i < ARRAYSIZE(m_pGridDetail); i++)
	{
		m_pGridDetail[i] = vgui::SETUP_PANEL(new vgui::Label(this, "DetailLabel", ""));
		m_pGridDetail[i]->SetZPos(40);
		m_pGridDetail[i]->SetContentAlignment(Label::Alignment::a_center);
	}

	m_pGridDetail[0]->SetText("#GameUI_Leaderboard_Name");
	m_pGridDetail[1]->SetText("#GameUI_Leaderboard_Level");
	m_pGridDetail[2]->SetText("#GameUI_Leaderboard_Kills");
	m_pGridDetail[3]->SetText("#GameUI_Leaderboard_Deaths");

	m_pBtnArrowBox[0]->SetCommand("Left");
	m_pBtnArrowBox[1]->SetCommand("Right");

	m_iCurrPage = 0;

	InvalidateLayout();

	PerformLayout();
}

PlayMenuScoreboard::~PlayMenuScoreboard()
{
	for (int i = 0; i < ARRAYSIZE(m_pScoreItem); i++)
	{
		if (m_pScoreItem[i] != NULL)
		{
			delete m_pScoreItem[i];
			m_pScoreItem[i] = NULL;
		}
	}
}

void PlayMenuScoreboard::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		int x, y;
		vgui::input()->GetCursorPos(x, y);

		if (m_pBtnArrowBox[0]->IsWithin(x, y))
			m_pImgArrowBox[0]->SetImage("mainmenu/arrow_left_over");
		else
			m_pImgArrowBox[0]->SetImage("mainmenu/arrow_left");

		if (m_pBtnArrowBox[1]->IsWithin(x, y))
			m_pImgArrowBox[1]->SetImage("mainmenu/arrow_right_over");
		else
			m_pImgArrowBox[1]->SetImage("mainmenu/arrow_right");

		m_pBtnArrowBox[0]->SetVisible(m_iCurrPage > 0);
		m_pBtnArrowBox[1]->SetVisible(m_iCurrPage < m_iPageNum);

		m_pImgArrowBox[0]->SetVisible(m_iCurrPage > 0);
		m_pImgArrowBox[1]->SetVisible(m_iCurrPage < m_iPageNum);
	}
}

void PlayMenuScoreboard::SetupLayout(void)
{
	BaseClass::SetupLayout();

	if (!IsVisible())
	{
		m_iCurrPage = 0;
		RefreshScores();
	}

	int w, h;
	GetSize(w, h);

	int width = (w / 2);

	for (int i = 0; i < ARRAYSIZE(m_pBtnArrowBox); i++)
	{
		m_pBtnArrowBox[i]->SetSize(scheme()->GetProportionalScaledValue(30), scheme()->GetProportionalScaledValue(20));
		m_pImgArrowBox[i]->SetSize(scheme()->GetProportionalScaledValue(30), scheme()->GetProportionalScaledValue(20));
	}

	m_pBtnArrowBox[0]->SetPos(width - (width / 2) - scheme()->GetProportionalScaledValue(31), scheme()->GetProportionalScaledValue(2));
	m_pBtnArrowBox[1]->SetPos(width + (width / 2) + scheme()->GetProportionalScaledValue(5), scheme()->GetProportionalScaledValue(2));

	m_pImgArrowBox[0]->SetPos(width - (width / 2) - scheme()->GetProportionalScaledValue(31), scheme()->GetProportionalScaledValue(2));
	m_pImgArrowBox[1]->SetPos(width + (width / 2) + scheme()->GetProportionalScaledValue(5), scheme()->GetProportionalScaledValue(2));

	for (int i = 0; i < ARRAYSIZE(m_pGridDetail); i++)
		m_pGridDetail[i]->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(20));

	m_pGridDetail[0]->SetPos((width / 2) - scheme()->GetProportionalScaledValue(4), scheme()->GetProportionalScaledValue(4));
	m_pGridDetail[1]->SetPos((width / 2) + scheme()->GetProportionalScaledValue(220), scheme()->GetProportionalScaledValue(4));
	m_pGridDetail[2]->SetPos((width / 2) + scheme()->GetProportionalScaledValue(287), scheme()->GetProportionalScaledValue(4));
	m_pGridDetail[3]->SetPos((width / 2) + scheme()->GetProportionalScaledValue(356), scheme()->GetProportionalScaledValue(4));

	m_pGridDetail[4]->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(20));
	m_pGridDetail[4]->SetPos((w / 2) - (scheme()->GetProportionalScaledValue(100) / 2), h - scheme()->GetProportionalScaledValue(20));
}

void PlayMenuScoreboard::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < ARRAYSIZE(m_pGridDetail); i++)
	{
		m_pGridDetail[i]->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
		m_pGridDetail[i]->SetFont(pScheme->GetFont("OptionTextMedium"));
	}
}

void PlayMenuScoreboard::RefreshScores(void)
{
	MoveToFront();
	RequestFocus();

	// 'Delete' in other words make everything invisible then re-add.
	for (int i = 0; i < ARRAYSIZE(m_pScoreItem); i++)
	{
		if (m_pScoreItem[i] != NULL)
		{
			delete m_pScoreItem[i];
			m_pScoreItem[i] = NULL;
		}
	}

	GameBaseClient->RefreshScoreboard((m_iCurrPage * _ARRAYSIZE(m_pScoreItem)));
}

void PlayMenuScoreboard::RefreshCallback(int iItems)
{
	m_iPageNum = 0;
	int iNeededItems = 5;
	for (int i = 0; i <= iItems; i++)
	{
		if (i > iNeededItems)
		{
			m_iPageNum++;
			iNeededItems += 5;
		}
	}

	if (m_iPageNum > 0)
		m_pGridDetail[4]->SetText(VarArgs("%i | %i", (m_iCurrPage + 1), (m_iPageNum + 1)));
	else
		m_pGridDetail[4]->SetText("");
}

void PlayMenuScoreboard::AddScoreItem(const char *pszNickName, const char *pszSteamID, int32 plLevel, int32 plKills, int32 plDeaths, int iIndex)
{
	if ((iIndex < 0) || (iIndex >= _ARRAYSIZE(m_pScoreItem)))
		return;

	if (m_pScoreItem[iIndex] != NULL)
	{
		delete m_pScoreItem[iIndex];
		m_pScoreItem[iIndex] = NULL;
	}

	int w, h;
	GetSize(w, h);
	float wide = (((float)w) * 0.60f);
	int width = (int)wide;

	m_pScoreItem[iIndex] = new vgui::LeaderboardItem(this, "ScoreItem", pszNickName, pszSteamID, plLevel, plKills, plDeaths);
	m_pScoreItem[iIndex]->SetSize(width, scheme()->GetProportionalScaledValue(34));
	m_pScoreItem[iIndex]->SetPos((w / 2) - (width / 2), scheme()->GetProportionalScaledValue(27) + (iIndex * scheme()->GetProportionalScaledValue(36)));
	m_pScoreItem[iIndex]->MoveToFront();
	m_pScoreItem[iIndex]->SetZPos(200);
}

void PlayMenuScoreboard::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Left"))
	{
		if (m_iCurrPage > 0)
		{
			m_iCurrPage--;
			RefreshScores();
		}
	}
	else if (!Q_stricmp(pcCommand, "Right"))
	{
		if (m_iCurrPage < m_iPageNum)
		{
			m_iCurrPage++;
			RefreshScores();
		}
	}

	BaseClass::OnCommand(pcCommand);
}

void PlayMenuScoreboard::OnKeyCodeTyped(KeyCode code)
{
	if (code == KEY_F5)
		RefreshScores();
	else
		BaseClass::OnKeyCodeTyped(code);
}
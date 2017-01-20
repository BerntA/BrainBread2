//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map Vote Menu (game over voting)
//
//========================================================================================//

#include "cbase.h"
#include <stdio.h>
#include <cdll_client_int.h>
#include "EndMapVoteMenu.h"
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
#include "GameBase_Client.h"
#include "vgui/MouseCode.h"
#include "cdll_util.h"
#include "IGameUIFuncs.h"
#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CEndMapVoteMenu::CEndMapVoteMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_ENDVOTE)
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

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);

	m_pLabelVoteAnnounce = new Label(this, "LabelVote", "");
	m_pLabelVoteAnnounce->SetZPos(20);
	m_pLabelVoteAnnounce->SetContentAlignment(Label::a_center);

	m_pBackgroundImg = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "BG"));
	m_pBackgroundImg->SetImage("scoreboard/mainbg");
	m_pBackgroundImg->SetZPos(-1);
	m_pBackgroundImg->SetShouldScaleImage(true);

	bool bButtonOnlyVoteItems[6] =
	{
		false,
		false,
		false,
		false,
		true,
		true,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pVoteOption); i++)
	{
		m_pVoteOption[i] = vgui::SETUP_PANEL(new vgui::MapVoteItem(this, "MapVoteItem", i, "", bButtonOnlyVoteItems[i]));
		m_pVoteOption[i]->SetZPos(30);
	}

	InvalidateLayout();
	PerformLayout();
}

CEndMapVoteMenu::~CEndMapVoteMenu()
{
}

void CEndMapVoteMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelVoteAnnounce->SetAlpha(0);

	int w, h;
	GetSize(w, h);

	m_pLabelVoteAnnounce->SetPos(0, (h / 2) - scheme()->GetProportionalScaledValue(90));
	m_pLabelVoteAnnounce->SetSize(w, scheme()->GetProportionalScaledValue(30));

	m_pVoteOption[0]->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(120));
	m_pVoteOption[0]->SetPos((w / 2) - (2 * scheme()->GetProportionalScaledValue(160)), (h / 2) - (scheme()->GetProportionalScaledValue(120) / 2));

	m_pVoteOption[1]->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(120));
	m_pVoteOption[1]->SetPos((w / 2) - scheme()->GetProportionalScaledValue(160), (h / 2) - (scheme()->GetProportionalScaledValue(120) / 2));

	m_pVoteOption[2]->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(120));
	m_pVoteOption[2]->SetPos((w / 2) + scheme()->GetProportionalScaledValue(10), (h / 2) - (scheme()->GetProportionalScaledValue(120) / 2));

	m_pVoteOption[3]->SetSize(scheme()->GetProportionalScaledValue(150), scheme()->GetProportionalScaledValue(120));
	m_pVoteOption[3]->SetPos((w / 2) + scheme()->GetProportionalScaledValue(170), (h / 2) - (scheme()->GetProportionalScaledValue(120) / 2));

	m_pVoteOption[4]->SetPos((w / 2) - (2 * scheme()->GetProportionalScaledValue(160)), (h / 2) + scheme()->GetProportionalScaledValue(75));
	m_pVoteOption[4]->SetSize(scheme()->GetProportionalScaledValue(128), scheme()->GetProportionalScaledValue(16));

	m_pVoteOption[5]->SetPos((w / 2) - (2 * scheme()->GetProportionalScaledValue(160)), (h / 2) + scheme()->GetProportionalScaledValue(95));
	m_pVoteOption[5]->SetSize(scheme()->GetProportionalScaledValue(128), scheme()->GetProportionalScaledValue(16));

	for (int i = 0; i < _ARRAYSIZE(m_pVoteOption); i++)
	{
		m_pVoteOption[i]->SetSelection(false);
		m_pVoteOption[i]->SetAlpha(0);
	}

	m_pBackgroundImg->SetAlpha(0);
}

void CEndMapVoteMenu::OnThink()
{
	BaseClass::OnThink();

	SetSize(ScreenWidth(), ScreenHeight());
	SetPos(0, 0);

	m_pBackgroundImg->SetPos(0, 0);
	m_pBackgroundImg->SetSize(ScreenWidth(), ScreenHeight());

	float timeLeft = MAX((m_flVoteTimeEnd - gpGlobals->curtime), 0.0f);

	wchar_t wszTime[16], wszText[80];
	g_pVGuiLocalize->ConvertANSIToUnicode(VarArgs("%i", ((int)timeLeft)), wszTime, sizeof(wszTime));
	g_pVGuiLocalize->ConstructString(wszText, sizeof(wszText), g_pVGuiLocalize->Find("#VGUI_GameOver_VoteMap"), 1, wszTime);
	m_pLabelVoteAnnounce->SetText(wszText);
}

void CEndMapVoteMenu::Update()
{
}

void CEndMapVoteMenu::ShowPanel(bool bShow)
{
	if (IsVisible() && bShow)
		return;

	GameBaseClient->CloseConsole();

	PerformLayout();
	SetMouseInputEnabled(bShow);
	SetKeyBoardInputEnabled(bShow);

	SetVisible(bShow);
	m_pViewPort->ShowBackGround(bShow);
	gViewPortInterface->ShowBackGround(bShow);
}

void CEndMapVoteMenu::Paint(void)
{
	BaseClass::Paint();
}

void CEndMapVoteMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelVoteAnnounce->SetFgColor(pScheme->GetColor("ScoreMenuWinnerTextColor", Color(255, 255, 255, 255)));
	m_pLabelVoteAnnounce->SetFont(pScheme->GetFont("ScoreMenuBig"));
}

void CEndMapVoteMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

Panel *CEndMapVoteMenu::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CEndMapVoteMenu::Reset()
{
}

void CEndMapVoteMenu::SetData(KeyValues *data)
{
	m_flVoteTimeEnd = data->GetFloat("timeleft");
	int iMapChoices = data->GetInt("mapChoices");

	// Setup the maps:
	m_pVoteOption[0]->SetMapLink(data->GetString("map1"));
	m_pVoteOption[1]->SetMapLink(data->GetString("map2"));
	m_pVoteOption[2]->SetMapLink(data->GetString("map3"));
	m_pVoteOption[3]->SetMapLink(data->GetString("map4"));

	for (int i = 0; i < 4; i++)
	{
		bool bShouldHide = ((i + 1) > iMapChoices);
		m_pVoteOption[i]->SetVisible(!bShouldHide);
		m_pVoteOption[i]->SetEnabled(!bShouldHide);
	}

	// Don't do anything fancy if we just wanted to reload stuff:
	bool bRefresh = data->GetBool("refresh");
	if (bRefresh && IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pVoteOption); i++)
			m_pVoteOption[i]->SetSelection(false);

		return;
	}

	m_pLabelVoteAnnounce->SetAlpha(0);
	GetAnimationController()->RunAnimationCommand(m_pLabelVoteAnnounce, "alpha", 255.0f, 0.0f, 1.5f, AnimationController::INTERPOLATOR_LINEAR);
	GetAnimationController()->RunAnimationCommand(m_pBackgroundImg, "alpha", 255.0f, 0.0f, 1.0f, AnimationController::INTERPOLATOR_LINEAR);

	for (int i = 0; i < _ARRAYSIZE(m_pVoteOption); i++)
	{
		m_pVoteOption[i]->SetMouseInputEnabled(true);
		m_pVoteOption[i]->SetKeyBoardInputEnabled(true);

		m_pVoteOption[i]->PerformLayout();
		m_pVoteOption[i]->SetAlpha(0);
		GetAnimationController()->RunAnimationCommand(m_pVoteOption[i], "alpha", 255.0f, 0.0f, 1.0f, AnimationController::INTERPOLATOR_LINEAR);
	}
}

void CEndMapVoteMenu::OnPlayerVote(KeyValues *data)
{
	int choice = data->GetInt("choice");

	for (int i = 0; i < _ARRAYSIZE(m_pVoteOption); i++)
	{
		m_pVoteOption[i]->SetSelection(false);
		if (m_pVoteOption[i]->GetVoteType() == choice)
			m_pVoteOption[i]->SetSelection(true);
	}

	engine->ClientCmd(VarArgs("player_vote_endmap_choice %i\n", (choice + 1)));
}
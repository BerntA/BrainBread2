//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Main Menu overlay screen : draws out main background image and renders film grain above it.
//
//========================================================================================//

#include "cbase.h"
#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include <vgui/IVGui.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "MenuOverlay.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "KeyValues.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define BOTTOM_DIVIDER_HEIGHT scheme()->GetProportionalScaledValue(105)

CMenuOverlay::CMenuOverlay(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	SetZPos(-1);

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImgBackground"));
	m_pImgCustomOverlay = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImgCustom"));

	m_pImgBackground->SetShouldScaleImage(true);
	m_pImgBackground->SetImage("mainmenu/mainbackground");

	m_pImgCustomOverlay->SetShouldScaleImage(true);
	m_pImgCustomOverlay->SetImage("mainmenu/filmgrain");

	m_pDividerBottom = vgui::SETUP_PANEL(new vgui::Divider(this, "DivBottom"));

	InvalidateLayout();
	PerformLayout();
}

CMenuOverlay::~CMenuOverlay()
{
}

void CMenuOverlay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_iAlpha = atoi(pScheme->GetResourceString("MainMenuOverlayAlpha"));

	m_pDividerBottom->SetPaintBorderEnabled(false);
	m_pDividerBottom->SetBorder(NULL);
	m_pDividerBottom->SetFgColor(Color(0, 0, 0, 255));
	m_pDividerBottom->SetBgColor(Color(0, 0, 0, 255));
}

void CMenuOverlay::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

void CMenuOverlay::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CMenuOverlay::OnUpdate()
{
	vgui::surface()->MovePopupToBack(GetVPanel());
	SetZPos(-1);
	SetPos(0, 0);
	SetSize(ScreenWidth(), ScreenHeight());

	int w, h;
	GetSize(w, h);

	//m_pImgBackground->SetVisible(!InGame());
	//m_pImgCustomOverlay->SetVisible(!InGame());

	m_pDividerBottom->SetVisible(GameBaseClient->IsMainMenuVisibleWhileInGame());
	if (GameBaseClient->IsMainMenuVisibleWhileInGame())
	{
		if ((GetAlpha() <= 0) && (m_iActiveCommand == VGUI_OVERLAY_INGAME_CLOSE))
			GameBaseClient->RunMainMenuCommand(VGUI_OVERLAY_INGAME_FINISH_CLOSE);

		m_pImgBackground->SetSize(ScreenWidth(), h - BOTTOM_DIVIDER_HEIGHT);
		m_pImgCustomOverlay->SetSize(ScreenWidth(), h - BOTTOM_DIVIDER_HEIGHT);

		m_pDividerBottom->SetPos(0, h - BOTTOM_DIVIDER_HEIGHT);
		m_pDividerBottom->SetSize(w, BOTTOM_DIVIDER_HEIGHT);
	}
	else
	{
		m_pImgBackground->SetSize(ScreenWidth(), ScreenHeight());
		m_pImgCustomOverlay->SetSize(ScreenWidth(), ScreenHeight());
	}
}

void CMenuOverlay::OnSetupLayout(int value)
{
	m_iActiveCommand = value;

	if (value == VGUI_OVERLAY_MAINMENU || value == VGUI_OVERLAY_INGAME_FINISH)
	{
		m_pImgBackground->SetImage("mainmenu/mainbackground");
		m_pImgBackground->SetAlpha(255);
		m_pImgCustomOverlay->SetAlpha(m_iAlpha);
		m_pDividerBottom->SetAlpha(0);
		SetAlpha(255);
	}
	else if (value == VGUI_OVERLAY_INGAME_OPEN)
	{
		m_pDividerBottom->SetFgColor(Color(0, 0, 0, 255));
		m_pDividerBottom->SetBgColor(Color(0, 0, 0, 255));

		engine->ClientCmd_Unrestricted("gameui_preventescape\n");
		m_pImgBackground->SetImage("scoreboard/mainbg");
		m_pImgBackground->SetAlpha(0);
		m_pImgCustomOverlay->SetAlpha(0);
		m_pDividerBottom->SetAlpha(0);
		SetAlpha(0);
		GetAnimationController()->RunAnimationCommand(this, "alpha", 256.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pImgBackground, "alpha", 256.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pImgCustomOverlay, "alpha", (float)m_iAlpha, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pDividerBottom, "alpha", 256.0f, 0.0f, 0.125f, AnimationController::INTERPOLATOR_LINEAR);
	}
	else if (value == VGUI_OVERLAY_INGAME_CLOSE)
	{
		GetAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pImgBackground, "alpha", 0.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pImgCustomOverlay, "alpha", 0.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pDividerBottom, "alpha", 0.0f, 0.0f, 0.125f, AnimationController::INTERPOLATOR_LINEAR);
	}
}

bool CMenuOverlay::InGame()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer)
		return true;
	else
		return false;
}
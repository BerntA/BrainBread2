//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Class for Frame panels, reimplements the Fade feature + setting escape to main menu fixes to prevent just that when you're in the actual panel.
//
//========================================================================================//

#include "cbase.h"
#include "vgui_base_frame.h"
#include "iclientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define BASE_FRAME_UPD_FREQ 0.5f

void CVGUIBaseFrame::OnShowPanel(bool bShow)
{
	if (m_bFadeOut && !bShow)
		return;

	m_pBackground->SetVisible(true);
	m_pBackground->SetSize(ScreenWidth(), ScreenHeight());

	if (bShow)
	{
		if (m_bFadeOut)
			m_bFadeOut = false;

		if (!m_bDisableInput)
			engine->ClientCmd_Unrestricted("gameui_preventescapetoshow\n");
		m_flUpdateTime = 0.0f;

		if (m_bOpenBuildMode)
			ActivateBuildMode();
	}
	else
	{
		m_bFadeOut = true;
		GetAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, m_flFadeFrequency, AnimationController::INTERPOLATOR_LINEAR);
		GetAnimationController()->RunAnimationCommand(m_pBackground, "alpha", 0.0f, 0.0f, m_flFadeFrequency, AnimationController::INTERPOLATOR_LINEAR);
		return;
	}

	MoveToFront();
	if (IsKeyBoardInputEnabled())
		RequestFocus();

	SetVisible(bShow);
	SetEnabled(bShow);

	if (!m_bDisableInput)
	{
		SetKeyBoardInputEnabled(bShow);
		SetMouseInputEnabled(bShow);
	}

	SetAlpha(0);
	m_pBackground->SetAlpha(0);
	GetAnimationController()->RunAnimationCommand(this, "alpha", 255.0f, 0.0f, m_flFadeFrequency, AnimationController::INTERPOLATOR_LINEAR);
	GetAnimationController()->RunAnimationCommand(m_pBackground, "alpha", 255.0f, 0.0f, m_flFadeFrequency, AnimationController::INTERPOLATOR_LINEAR);
}

void CVGUIBaseFrame::OnThink()
{
	BaseClass::OnThink();

	if (m_bFadeOut)
	{
		if (GetAlpha() < 1)
		{
			m_bFadeOut = false;
			ForceClose();
			return;
		}
	}

	// Update Us:
	if (engine->Time() > m_flUpdateTime)
	{
		m_flUpdateTime = engine->Time() + BASE_FRAME_UPD_FREQ;
		m_pBackground->SetSize(ScreenWidth(), ScreenHeight());
		UpdateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVGUIBaseFrame::CVGUIBaseFrame(vgui::VPANEL parent, const char *panelName, bool bOpenBuildMode, float flFadeTime, bool bDisableInput) : BaseClass(NULL, panelName)
{
	m_bDisableInput = bDisableInput;
	m_bOpenBuildMode = bOpenBuildMode; // for testing.
	SetParent(parent);

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	SetProportional(true);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetZPos(0);

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, ""));
	m_pBackground->SetImage("scoreboard/mainbg");
	m_pBackground->SetZPos(-1);
	m_pBackground->SetShouldScaleImage(true);
	m_pBackground->SetAlpha(0);

	SetScheme("BaseScheme");

	PerformLayout();
	InvalidateLayout();

	m_flFadeFrequency = flFadeTime;
	m_bFadeOut = false;
	SetVisible(false);
	SetAlpha(0);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CVGUIBaseFrame::~CVGUIBaseFrame()
{
}

void CVGUIBaseFrame::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (RecordKeyPresses() && (code == KEY_ESCAPE))
		OnShowPanel(false);
	else
		BaseClass::OnKeyCodeTyped(code);
}

void CVGUIBaseFrame::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

void CVGUIBaseFrame::ForceClose(void)
{
	m_bFadeOut = false;

	if (IsVisible() && !m_bDisableInput)
		engine->ClientCmd_Unrestricted("gameui_allowescapetoshow\n");

	SetVisible(false);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
}
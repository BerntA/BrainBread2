//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Class for Panels that need a proper fade feature, instead of reinplementing this in every class that inherits Panel and needs a Fade func.
// Notice: Fading is implemented in Frame, however we've overriden some of those features in the new frame base class.
//
//========================================================================================//

#include "cbase.h"
#include "vgui_base_panel.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

void CVGUIBasePanel::OnShowPanel(bool bShow)
{
	SetupLayout();

	if (m_bFadeOut && !bShow)
		return;

	if (bShow)
	{
		if (m_bFadeOut)
			m_bFadeOut = false;
	}
	else
	{
		m_bFadeOut = true;
		GetAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, m_flFadeDuration, AnimationController::INTERPOLATOR_LINEAR);
		return;
	}

	MoveToFront();
	if (IsKeyBoardInputEnabled())
		RequestFocus();

	SetVisible(bShow);
	SetKeyBoardInputEnabled(bShow);
	SetMouseInputEnabled(bShow);

	SetAlpha(0);
	GetAnimationController()->RunAnimationCommand(this, "alpha", 255.0f, 0.0f, m_flFadeDuration, AnimationController::INTERPOLATOR_LINEAR);
	m_bIsActive = true;
}

void CVGUIBasePanel::OnThink()
{
	BaseClass::OnThink();

	if (m_bFadeOut)
	{
		if (GetAlpha() < 1)
		{
			m_bFadeOut = false;
			ForceClose();
		}
	}
}

CVGUIBasePanel::CVGUIBasePanel(vgui::Panel *parent, char const *panelName, float flFadeDuration) : vgui::Panel(parent, panelName)
{
	m_flFadeDuration = flFadeDuration;
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	InvalidateLayout();
	PerformLayout();

	SetVisible(false);
	SetAlpha(0);
	m_bFadeOut = false;
	m_bIsActive = false;
}

CVGUIBasePanel::~CVGUIBasePanel()
{
}

void CVGUIBasePanel::ForceClose(void)
{
	m_bFadeOut = false;
	m_bIsActive = false;
	SetVisible(false);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
	FullyClosed();
}

void CVGUIBasePanel::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

void CVGUIBasePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	//SetAlpha(0);
	SetupLayout();
}

// If you override this function and you don't call the base class then you'll not update the panel's children on closing/opening.
void CVGUIBasePanel::SetupLayout(void)
{
	for (int i = 0; i < GetChildCount(); ++i)
	{
		Panel *pChild = GetChild(i);
		if (pChild)
			UpdateAllChildren(pChild);
	}
}

void CVGUIBasePanel::UpdateAllChildren(Panel *pChild)
{
	if (pChild)
	{
		pChild->PerformLayout();
		for (int i = 0; i < pChild->GetChildCount(); ++i)
		{
			Panel *pNextChild = pChild->GetChild(i);
			if (pNextChild)
				UpdateAllChildren(pNextChild);
		}
	}
}
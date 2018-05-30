//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Circular Animated Button - Animates a circle overlay (images) when you hover over the control.
//
//========================================================================================//

#include "cbase.h"
#include "CircularButton.h"
#include <vgui/IInput.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Add us to build mode...
DECLARE_BUILD_FACTORY(CircularButton);

#define ANIM_FRAMES 14
#define ANIM_TIME 0.25f

CircularButton::CircularButton(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "Button", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/selection.wav");
	m_pButton->SetZPos(140);
	m_pButton->AddActionSignalTarget(this);

	m_pOverlay = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Overlay"));
	m_pOverlay->SetShouldScaleImage(true);
	m_pOverlay->SetZPos(130);

	InvalidateLayout();

	PerformLayout();
}

CircularButton::~CircularButton()
{
}

void CircularButton::SetCMD(const char *szCommand)
{
	m_pButton->SetCommand(szCommand);
}

void CircularButton::OnUpdate()
{
	// Get mouse coords
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (m_pButton->IsWithin(x, y))
	{
		if (!m_bMouseOver)
		{
			m_bMouseOver = true;
			GetAnimationController()->RunAnimationCommand(this, "PictureFrame", 1.0f, 0.0f, ANIM_TIME, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else
	{
		if (m_bMouseOver)
		{
			m_bMouseOver = false;
			GetAnimationController()->RunAnimationCommand(this, "PictureFrame", 0.0f, 0.0f, ANIM_TIME, AnimationController::INTERPOLATOR_LINEAR);
		}
	}

	m_pOverlay->SetImage(VarArgs("shared/circle_frame_%i", (ANIM_FRAMES * m_flFrameFrac)));
}

void CircularButton::PerformLayout()
{
	BaseClass::PerformLayout();

	m_flFrameFrac = 0;
	m_bMouseOver = false;
	m_pOverlay->SetImage("shared/circle_frame_0");

	int w, h;
	GetSize(w, h);

	m_pButton->SetSize(w, h);
	m_pOverlay->SetSize(w, h);
}

void CircularButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void CircularButton::OnCommand(const char* pcCommand)
{
	// Forward to parent.
	vgui::Panel *vParent = GetParent();
	if (vParent)
		vParent->OnCommand(pcCommand);
}
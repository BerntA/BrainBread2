//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Animated Menu Button - Fades in an overlay behind the text.
//
//========================================================================================//

#include "cbase.h"
#include "AnimatedMenuButton.h"
#include <vgui/IInput.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define FADE_TIME 0.5f

AnimatedMenuButton::AnimatedMenuButton(vgui::Panel *parent, char const *panelName, const char *text, int iCommand, bool bSendToParent) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "BaseText", ""));
	m_pLabelTitle->SetZPos(30);
	m_pLabelTitle->SetContentAlignment(Label::a_center);
	m_pLabelTitle->SetText(text);

	m_pRolloverImage = vgui::SETUP_PANEL(new ImageProgressBar(this, "OverlayImg", "vgui/mainmenu/menubutton_over", "vgui/mainmenu/menubutton"));
	m_pRolloverImage->SetProgressDirection(ProgressBar::PROGRESS_EAST);
	m_pRolloverImage->SetZPos(20);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "BaseButton", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/button_over.wav");
	m_pButton->SetZPos(40);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Activate");

	m_bSendToParent = bSendToParent;

	InvalidateLayout();

	m_iMenuCommand = iCommand;

	PerformLayout();
}

AnimatedMenuButton::~AnimatedMenuButton()
{
}

void AnimatedMenuButton::OnUpdate()
{
	// Get mouse coords
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	m_pRolloverImage->SetProgress(m_flFadeTime);

	if (m_pButton->IsWithin(x, y))
	{
		if (!bGotActivated)
		{
			bGotActivated = true;
			GetAnimationController()->RunAnimationCommand(this, "FadeTime", 1.0f, 0.0f, FADE_TIME, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else
	{
		if (bGotActivated)
		{
			bGotActivated = false;
			GetAnimationController()->RunAnimationCommand(this, "FadeTime", 0.0f, 0.0f, FADE_TIME, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
}

void AnimatedMenuButton::PerformLayout()
{
	BaseClass::PerformLayout();
	bGotActivated = false;
	m_pRolloverImage->SetProgress(0);
	m_flFadeTime = 0.0f;

	int w, h;
	GetSize(w, h);

	m_pButton->SetSize(w, h);
	m_pButton->SetPos(0, 0);

	m_pRolloverImage->SetSize(w, h);
	m_pRolloverImage->SetPos(0, 0);

	m_pLabelTitle->SetSize(w, h);
	m_pLabelTitle->SetPos(0, 0);
}

void AnimatedMenuButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelTitle->SetFont(pScheme->GetFont("MainMenuTextBig"));
	m_pLabelTitle->SetFgColor(pScheme->GetColor("AnimatedMenuButtonTextColor", Color(255, 255, 255, 255)));
}

void AnimatedMenuButton::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Activate"))
	{
		if (m_bSendToParent)
		{
			if (GetParent())
				GetParent()->OnCommand("Activate");
		}
		else
			GameBaseClient->RunCommand(m_iMenuCommand);
	}

	BaseClass::OnCommand(pcCommand);
}
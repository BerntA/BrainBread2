//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A button which fades in an image within its bounds when hovering and stays that way until it is deselected if selected.
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
#include "OverlayButton.h"
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

OverlayButton::OverlayButton(vgui::Panel *parent, char const *panelName, const char *text, const char *command, const char *font) : vgui::Panel(parent, panelName)
{
	Q_strncpy(pchFont, font, 64);

	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "BaseText", ""));
	m_pLabelTitle->SetZPos(30);
	m_pLabelTitle->SetContentAlignment(Label::a_center);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "BaseButton", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/button_over.wav");
	m_pButton->SetZPos(40);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand(command);

	m_pOverlay = new vgui::ImagePanel(this, "Divider");
	m_pOverlay->SetZPos(20);
	m_pOverlay->SetShouldScaleImage(true);
	m_pOverlay->SetFgColor(Color(255, 255, 255, 255));
	m_pOverlay->SetBgColor(Color(255, 255, 255, 255));
	m_pOverlay->SetVisible(true);
	m_pOverlay->SetImage("server/white");

	InvalidateLayout();

	m_pLabelTitle->SetContentAlignment(Label::a_center);
	m_pLabelTitle->SetText(text);

	PerformLayout();
}

OverlayButton::~OverlayButton()
{
}

void OverlayButton::Reset(void)
{
	PerformLayout();
}

void OverlayButton::OnUpdate()
{
	// Get mouse coords
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (m_pButton->IsWithin(x, y) || IsSelected())
	{
		if (!bGotActivated)
		{
			bGotActivated = true;
			GetAnimationController()->RunAnimationCommand(m_pOverlay, "alpha", 201.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_SIMPLESPLINE);
		}
	}
	else
	{
		if (bGotActivated)
		{
			bGotActivated = false;
			GetAnimationController()->RunAnimationCommand(m_pOverlay, "alpha", 0.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_SIMPLESPLINE);
		}
	}

	if (IsSelected() || bGotActivated)
		m_pLabelTitle->SetFgColor(Color(0, 0, 0, 255));
	else
		m_pLabelTitle->SetFgColor(Color(255, 255, 255, 255));
}

void OverlayButton::SetSelection(bool bValue)
{
	m_bIsSelected = bValue;

	if (bValue)
		m_pOverlay->SetAlpha(200);
}

void OverlayButton::PerformLayout()
{
	BaseClass::PerformLayout();

	bGotActivated = false;
	m_bIsSelected = false;
	m_pOverlay->SetAlpha(0);

	int w, h;
	GetSize(w, h);

	m_pLabelTitle->SetContentAlignment(Label::a_center);
	m_pLabelTitle->SetSize(w, h);
	m_pLabelTitle->SetPos(0, 0);

	m_pButton->SetSize(w, h);
	m_pButton->SetPos(0, 0);

	m_pOverlay->SetSize(w, h);
	m_pOverlay->SetPos(0, 0);
}

void OverlayButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelTitle->SetFont(pScheme->GetFont(pchFont));

	if (IsSelected() || bGotActivated)
		m_pLabelTitle->SetFgColor(Color(0, 0, 0, 255));
	else
		m_pLabelTitle->SetFgColor(Color(255, 255, 255, 255));
}

void OverlayButton::OnCommand(const char* pcCommand)
{
	Panel *pParent = GetParent();
	if (!pParent)
		return;

	if (IsEnabled())
	{
		if (!m_bToggleMode)
		{
			m_bIsSelected = true;
			pParent->OnCommand(pcCommand);
		}
		else
			pParent->OnCommand(pcCommand);
	}

	BaseClass::OnCommand(pcCommand);
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A menu button which renders an icon (fades in) related to what it will do and smoothly interpolates a divider along the text.
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
#include "InlineMenuButton.h"
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

InlineMenuButton::InlineMenuButton(vgui::Panel *parent, char const *panelName, int iCommand, const char *text, const char *fontName, int iconSize, bool bSendToParent) : vgui::Panel(parent, panelName)
{
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
	m_pButton->SetCommand("Activate");

	m_pOverlay = new vgui::Divider(this, "Divider");
	m_pOverlay->SetZPos(35);
	m_pOverlay->SetBorder(NULL);
	m_pOverlay->SetPaintBorderEnabled(false);

	m_pIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImgIconAnim"));
	m_pIcon->SetShouldScaleImage(true);
	m_pIcon->SetZPos(50);
	m_pIcon->SetImage("mainmenu/icons/audio");

	InvalidateLayout();

	m_pLabelTitle->SetContentAlignment(Label::a_center);
	m_pLabelTitle->SetText(text);
	m_bSendToParent = bSendToParent;
	m_iMenuCommand = iCommand;
	m_iIconSize = scheme()->GetProportionalScaledValue(iconSize);
	if (m_iIconSize <= 0)
		m_pIcon->SetVisible(false);

	Q_strncpy(szFont, fontName, 32);

	PerformLayout();
}

InlineMenuButton::~InlineMenuButton()
{
}

void InlineMenuButton::OnUpdate()
{
	// Get mouse coords
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (m_pButton->IsWithin(x, y))
	{
		if (!bGotActivated)
		{
			bGotActivated = true;

			GetAnimationController()->RunAnimationCommand(m_pOverlay, "wide", (float)m_iLabelLength, 0.0f, 0.2f, AnimationController::INTERPOLATOR_SIMPLESPLINE);
			GetAnimationController()->RunAnimationCommand(m_pOverlay, "alpha", 200.0f, 0.0f, 0.1f, AnimationController::INTERPOLATOR_SIMPLESPLINE);
			GetAnimationController()->RunAnimationCommand(m_pIcon, "alpha", 256.0f, 0.0f, 0.2f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else
	{
		if (bGotActivated)
		{
			bGotActivated = false;

			GetAnimationController()->RunAnimationCommand(m_pOverlay, "wide", 0.0f, 0.0f, 0.2f, AnimationController::INTERPOLATOR_SIMPLESPLINE);
			GetAnimationController()->RunAnimationCommand(m_pOverlay, "alpha", 0.0f, 0.0f, 0.1f, AnimationController::INTERPOLATOR_SIMPLESPLINE);
			GetAnimationController()->RunAnimationCommand(m_pIcon, "alpha", 0.0f, 0.0f, 0.2f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
}

void InlineMenuButton::PerformLayout()
{
	BaseClass::PerformLayout();

	bGotActivated = false;
	m_pIcon->SetAlpha(0);
	m_pIcon->SetSize(m_iIconSize, m_iIconSize);
	m_pLabelTitle->SetContentAlignment(Label::a_center);

	wchar_t szText[128];
	m_pLabelTitle->GetText(szText, 128);
	vgui::HFont font = m_pLabelTitle->GetFont();
	m_iLabelLength = UTIL_ComputeStringWidth(font, szText);
	int strHeight = surface()->GetFontTall(font);
	int difference = ((GetWide() / 2) - (m_iLabelLength / 2));

	m_pOverlay->SetSize(0, scheme()->GetProportionalScaledValue(2));
	m_pOverlay->SetPos(difference, (GetTall() / 2) + (strHeight / 2) - scheme()->GetProportionalScaledValue(2));

	if (m_iIconSize > 0)
		m_pIcon->SetPos(difference - (strHeight / 2) - (m_iIconSize / 2), (GetTall() / 2) - (m_iIconSize / 2));

	m_pLabelTitle->SetSize(GetWide(), GetTall());
	m_pLabelTitle->SetPos(0, 0);

	m_pButton->SetSize(GetWide(), GetTall());
	m_pButton->SetPos(0, 0);
}

void InlineMenuButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pOverlay->SetFgColor(pScheme->GetColor("OptionButtonDividerFgCol", Color(255, 0, 0, 255)));
	m_pOverlay->SetBgColor(pScheme->GetColor("OptionButtonDividerBgCol", Color(255, 0, 0, 75)));

	m_pLabelTitle->SetFgColor(pScheme->GetColor("OptionButtonTextColor", Color(255, 255, 255, 245)));
	m_pLabelTitle->SetFont(pScheme->GetFont(szFont));
}

void InlineMenuButton::OnCommand(const char* pcCommand)
{
	if (m_bSendToParent)
	{
		if (GetParent())
			GetParent()->OnCommand(pcCommand);
	}
	else
	{
		if (!Q_stricmp(pcCommand, "Activate"))
			GameBaseClient->RunCommand(m_iMenuCommand);
	}

	BaseClass::OnCommand(pcCommand);
}
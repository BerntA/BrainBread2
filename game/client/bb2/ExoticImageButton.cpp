//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Image Button with basic hovering features.
//
//========================================================================================//

#include "cbase.h"
#include "ExoticImageButton.h"
#include <vgui/IInput.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ExoticImageButton::ExoticImageButton(vgui::Panel *parent, char const *panelName, const char *image, const char *hoverImage, const char *command) : vgui::Panel(parent, panelName)
{
	Q_strncpy(pchImageDefault, image, 80);
	Q_strncpy(pchImageHover, hoverImage, 80);

	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetZPos(20);
	m_pBackground->SetShouldScaleImage(true);
	m_pBackground->SetImage(image);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "Button", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/button_over.wav");
	m_pButton->SetZPos(40);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand(command);

	InvalidateLayout();
	PerformLayout();
}

ExoticImageButton::~ExoticImageButton()
{
}

void ExoticImageButton::OnThink()
{
	BaseClass::OnThink();

	// Get mouse coords
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (m_pButton->IsWithin(x, y))
	{
		if (!m_bRollover)
		{
			m_bRollover = true;
			m_pBackground->SetImage(pchImageHover);
		}
	}
	else if (m_bRollover)
	{
		m_bRollover = false;
		m_pBackground->SetImage(pchImageDefault);
	}
}

void ExoticImageButton::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize(w, h);

	m_pButton->SetPos(0, 0);
	m_pButton->SetSize(w, h);

	m_pBackground->SetPos(0, 0);
	m_pBackground->SetSize(w, h);

	m_bRollover = false;
}

void ExoticImageButton::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void ExoticImageButton::OnCommand(const char* pcCommand)
{
	if (!IsVisible() || !IsEnabled())
		return;

	Panel *pParent = GetParent();
	if (pParent)
		pParent->OnCommand(pcCommand);
}
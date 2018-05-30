//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Advertisement Panel - Links to our bb2 related pages.
//
//========================================================================================//

#include "cbase.h"
#include "AdvertisementPanel.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <steam/steam_api.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

AdvertisementPanel::AdvertisementPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szAdvImgs[] =
	{
		"forums",
		"steam_community",
		"steam_group",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pImageAdvert); i++)
	{
		m_pImageAdvert[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "AdvertImg"));
		m_pImageAdvert[i]->SetShouldScaleImage(true);
		m_pImageAdvert[i]->SetImage(szAdvImgs[i]);
		m_pImageAdvert[i]->SetZPos(110);

		m_pButtonAdvert[i] = vgui::SETUP_PANEL(new vgui::Button(this, VarArgs("AdvertBtn%i", i), ""));
		m_pButtonAdvert[i]->SetPaintBorderEnabled(false);
		m_pButtonAdvert[i]->SetPaintEnabled(false);
		m_pButtonAdvert[i]->SetReleasedSound("ui/button_click.wav");
		m_pButtonAdvert[i]->SetArmedSound("ui/button_over.wav");
		m_pButtonAdvert[i]->SetZPos(125);
		m_pButtonAdvert[i]->AddActionSignalTarget(this);
		m_pButtonAdvert[i]->SetCommand(VarArgs("OpenLink%i", i));
	}

	InvalidateLayout();
	PerformLayout();
}

AdvertisementPanel::~AdvertisementPanel()
{
}

const char *AdvertisementPanel::GetAdvertisementURL(int iIndex)
{
	switch (iIndex)
	{
	case 0:
		return "http://brainbread2.eu/"; // BB2 Site
	case 1:
		return "http://steamcommunity.com/app/346330"; // Steam Community
	case 2:
		return "http://steamcommunity.com/games/brainbread2/"; // Steam Group
	default:
		return "http://brainbread2.eu/"; // BB2 Site (default)
	}
}

void AdvertisementPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < _ARRAYSIZE(m_pButtonAdvert); i++)
	{
		m_pImageAdvert[i]->SetSize(scheme()->GetProportionalScaledValue(64), scheme()->GetProportionalScaledValue(32));
		m_pButtonAdvert[i]->SetSize(scheme()->GetProportionalScaledValue(64), scheme()->GetProportionalScaledValue(32));

		m_pImageAdvert[i]->SetPos((i * scheme()->GetProportionalScaledValue(64)), 0);
		m_pButtonAdvert[i]->SetPos((i * scheme()->GetProportionalScaledValue(64)), 0);
	}
}

void AdvertisementPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void AdvertisementPanel::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);

	for (int i = 0; i < _ARRAYSIZE(m_pButtonAdvert); i++)
		m_pButtonAdvert[i]->SetEnabled(state);
}

void AdvertisementPanel::OnCommand(const char* pcCommand)
{
	for (int i = 0; i < _ARRAYSIZE(m_pButtonAdvert); i++)
	{
		if (!Q_stricmp(pcCommand, VarArgs("OpenLink%i", i)))
		{
			if (steamapicontext && steamapicontext->SteamFriends())
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(GetAdvertisementURL(i));
		}
	}

	BaseClass::OnCommand(pcCommand);
}
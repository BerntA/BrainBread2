//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Global Scoreboard Item
//
//========================================================================================//

#include "cbase.h"
#include "LeaderboardItem.h"
#include <vgui/IInput.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "hl2mp_gamerules.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

LeaderboardItem::LeaderboardItem(vgui::Panel *parent, char const *panelName, const char *pszPlayerName, const char *pszSteamID, int32 plLevel, int32 plKills, int32 plDeaths, int iCommand) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgBackground"));
	m_pImgBackground->SetShouldScaleImage(true);
	m_pImgBackground->SetZPos(110);

	m_pSteamAvatar = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgAvatar"));
	m_pSteamAvatar->SetShouldScaleImage(true);
	m_pSteamAvatar->SetZPos(120);

	Q_strncpy(szSteamID, pszSteamID, 80);

	unsigned long long llSteamID = (unsigned long long)Q_atoui64(szSteamID);
	CSteamID steamIDForPlayer(llSteamID);
	m_pAvatarIMG = new CAvatarImage();
	m_pAvatarIMG->SetAvatarSteamID(steamIDForPlayer);
	m_pAvatarIMG->SetAvatarSize(32, 32);
	m_pAvatarIMG->SetSize(32, 32);
	m_pSteamAvatar->SetImage(m_pAvatarIMG);

	m_pUserName = vgui::SETUP_PANEL(new vgui::Label(this, "lbName", "DefaultLargeBold"));
	m_pUserName->SetZPos(150);
	m_pUserName->SetText(pszPlayerName);

	m_pKills = vgui::SETUP_PANEL(new vgui::Label(this, "lbKills", ""));
	m_pKills->SetZPos(150);
	m_pKills->SetContentAlignment(vgui::Label::Alignment::a_center);
	m_pKills->SetText(VarArgs("%i", plKills));

	m_pDeaths = vgui::SETUP_PANEL(new vgui::Label(this, "lbDeaths", ""));
	m_pDeaths->SetZPos(150);
	m_pDeaths->SetContentAlignment(vgui::Label::Alignment::a_center);
	m_pDeaths->SetText(VarArgs("%i", plDeaths));

	m_pLevel = vgui::SETUP_PANEL(new vgui::Label(this, "lbLevel", ""));
	m_pLevel->SetZPos(150);
	m_pLevel->SetContentAlignment(vgui::Label::Alignment::a_center);
	m_pLevel->SetText(VarArgs("%i", plLevel));
	m_pLevel->SetVisible(iCommand == COMMAND_SHOW_SCOREBOARD_PVE);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "btnSteamProfile", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetZPos(200);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Redirect");
	m_pButton->SetEnabled(true);

	InvalidateLayout();
	PerformLayout();
}

LeaderboardItem::~LeaderboardItem()
{
	if (m_pAvatarIMG)
	{
		delete m_pAvatarIMG;
		m_pAvatarIMG = NULL;
	}
}

void LeaderboardItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	int w, h;
	w = wide;
	h = tall;

	m_pUserName->SetSize(w / 2, h);
	m_pUserName->SetPos(h + scheme()->GetProportionalScaledValue(6), 0);

	m_pLevel->SetSize(h, h);
	m_pLevel->SetPos(w - (h * 7), 0);

	m_pKills->SetSize(h, h);
	m_pKills->SetPos(w - (h * 5), 0);

	m_pDeaths->SetSize(h, h);
	m_pDeaths->SetPos(w - (h * 3), 0);

	m_pImgBackground->SetSize(w, h);
	m_pButton->SetSize(w, h);
	m_pSteamAvatar->SetSize(h - scheme()->GetProportionalScaledValue(6), h - scheme()->GetProportionalScaledValue(6));

	m_pSteamAvatar->SetPos(scheme()->GetProportionalScaledValue(3), scheme()->GetProportionalScaledValue(3));
	m_pImgBackground->SetPos(0, 0);
	m_pButton->SetPos(0, 0);

	m_pButton->MoveToFront();
}

void LeaderboardItem::OnThink()
{
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (m_pButton->IsWithin(x, y))
		m_pImgBackground->SetImage("scoreboard/bg_hover");
	else
		m_pImgBackground->SetImage("scoreboard/defaultoverlay");

	if (m_pAvatarIMG && (m_flSteamImgThink > 0.0f) && (m_flSteamImgThink < engine->Time()))
	{
		if (m_pAvatarIMG->IsValid())
			m_flSteamImgThink = 0.0f;
		else
			m_flSteamImgThink = engine->Time() + 0.25f;

		m_pSteamAvatar->SetImage(m_pAvatarIMG);
	}

	BaseClass::OnThink();
}

void LeaderboardItem::PerformLayout()
{
	BaseClass::PerformLayout();

	m_flSteamImgThink = engine->Time() + 0.5f;
}

void LeaderboardItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pUserName->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
	m_pLevel->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
	m_pKills->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
	m_pDeaths->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));

	m_pUserName->SetFont(pScheme->GetFont("OptionTextMedium"));
	m_pKills->SetFont(pScheme->GetFont("OptionTextSmall"));
	m_pDeaths->SetFont(pScheme->GetFont("OptionTextSmall"));
	m_pLevel->SetFont(pScheme->GetFont("OptionTextSmall"));
}

void LeaderboardItem::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Redirect"))
	{
		if (steamapicontext && steamapicontext->SteamFriends())
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(VarArgs("http://steamcommunity.com/profiles/%s", szSteamID));
	}

	BaseClass::OnCommand(pcCommand);
}
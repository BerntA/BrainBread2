//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Scoreboard Item - Adds to the scoreboard. Contains player score, stats, etc...
//
//========================================================================================//

#include "cbase.h"
#include "ScoreboardItem.h"
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Client.h"
#include "hl2mp_gamerules.h"
#include "c_playerresource.h"
#include "c_world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ScoreboardItem::ScoreboardItem(vgui::Panel *parent, char const *panelName, KeyValues *pkvPlayerData) : vgui::Panel(parent, panelName)
{
	m_iTotalScore = 0;
	m_bRollover = false;
	m_bIsBot = pkvPlayerData->GetBool("fakeplr");
	m_pAvatarIMG = NULL;
	m_iSectionTeam = pkvPlayerData->GetInt("sectionTeam", TEAM_SPECTATOR);
	m_iPlayerIndex = pkvPlayerData->GetInt("playerIndex");
	Q_strncpy(szSteamID, pkvPlayerData->GetString("steamid"), 80);

	m_bIsLocalPlayer = ((m_iPlayerIndex == GetLocalPlayerIndex()) || m_bIsBot);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgBackground"));
	m_pImgBackground->SetShouldScaleImage(true);
	m_pImgBackground->SetZPos(110);
	m_pImgBackground->SetImage("scoreboard/spectatoroverlay");

	m_pImgMuteStatus = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgMuteStatus"));
	m_pImgMuteStatus->SetShouldScaleImage(true);
	m_pImgMuteStatus->SetZPos(120);
	m_pImgMuteStatus->SetImage("scoreboard/icon_unmuted");
	m_pImgMuteStatus->SetEnabled(!m_bIsLocalPlayer);
	m_pImgMuteStatus->SetVisible(!m_bIsLocalPlayer);

	m_pSteamAvatar = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgAvatar"));
	m_pSteamAvatar->SetShouldScaleImage(true);
	m_pSteamAvatar->SetZPos(120);

	m_pSkullIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgSkull"));
	m_pSkullIcon->SetShouldScaleImage(true);
	m_pSkullIcon->SetZPos(130);
	m_pSkullIcon->SetImage("hud/notices/suicide");
	m_pSkullIcon->SetVisible(false);

	player_info_t pi;
	if (engine->GetPlayerInfo(m_iPlayerIndex, &pi))
	{
		if (!pi.fakeplayer && pi.friendsID && steamapicontext && steamapicontext->SteamUtils())
		{
			CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
			m_pAvatarIMG = new CAvatarImage();
			m_pAvatarIMG->SetAvatarSteamID(steamIDForPlayer, k_EAvatarSize32x32);
			m_pAvatarIMG->SetAvatarSize(32, 32);
			m_pAvatarIMG->SetSize(32, 32);
			m_pSteamAvatar->SetImage(m_pAvatarIMG);
		}
		else
			m_pSteamAvatar->SetImage("steam_default_avatar");
	}

	bool bIsAdmin = g_PR ? g_PR->IsAdmin(m_iPlayerIndex) : false;
	const char *szNameTAG = pkvPlayerData->GetString("namePrefix");

	char pchNameTag[64];
	if (bIsAdmin && szNameTAG && szNameTAG[0])
		Q_snprintf(pchNameTag, 64, "%s|ADMIN", szNameTAG);
	else if (bIsAdmin)
		Q_snprintf(pchNameTag, 64, "ADMIN");
	else
		Q_strncpy(pchNameTag, szNameTAG, 64);

	const char *szNameStrings[] =
	{
		(pchNameTag && pchNameTag[0]) ? "[" : "",
		pchNameTag,
		(pchNameTag && pchNameTag[0]) ? "]" : "",
		pkvPlayerData->GetString("name"),
	};

	Color colorNameStrings[] =
	{
		Color(255, 255, 255, 255),
		Color(0, 255, 0, 255),
		Color(255, 255, 255, 255),
		Color(255, 255, 255, 255),
	};

	m_pUserName = vgui::SETUP_PANEL(new vgui::MultiLabel(this, "lbName", "OptionTextSmall"));
	m_pUserName->SetZPos(140);
	m_pUserName->SetTextColorSegmented(szNameStrings, colorNameStrings, 3);

	m_pPing = vgui::SETUP_PANEL(new vgui::Label(this, "lbPing", ""));
	m_pPing->SetZPos(150);

	m_pKills = vgui::SETUP_PANEL(new vgui::Label(this, "lbKills", ""));
	m_pKills->SetZPos(150);

	m_pDeaths = vgui::SETUP_PANEL(new vgui::Label(this, "lbDeaths", ""));
	m_pDeaths->SetZPos(150);

	m_pLevel = vgui::SETUP_PANEL(new vgui::Label(this, "lbLevel", ""));
	m_pLevel->SetZPos(150);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "btnSteamProfile", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetZPos(200);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Redirect");
	m_pButton->SetEnabled(true);

	m_pMuteButton = vgui::SETUP_PANEL(new vgui::Button(this, "btnMuteToggle", ""));
	m_pMuteButton->SetPaintBorderEnabled(false);
	m_pMuteButton->SetPaintEnabled(false);
	m_pMuteButton->SetReleasedSound("ui/button_click.wav");
	m_pMuteButton->SetZPos(220);
	m_pMuteButton->AddActionSignalTarget(this);
	m_pMuteButton->SetCommand("Mute");

	m_pMuteButton->SetEnabled(!m_bIsLocalPlayer);
	m_pMuteButton->SetVisible(!m_bIsLocalPlayer);

	InvalidateLayout();

	PerformLayout();

	UpdateItem(pkvPlayerData);
}

ScoreboardItem::~ScoreboardItem()
{
	if (m_pAvatarIMG)
	{
		delete m_pAvatarIMG;
		m_pAvatarIMG = NULL;
	}
}

void ScoreboardItem::UpdateItem(KeyValues *pkvPlayerData)
{
	m_iTotalScore = pkvPlayerData->GetInt("kills");
	m_pLevel->SetText(pkvPlayerData->GetString("level"));
	m_pKills->SetText(pkvPlayerData->GetString("kills"));
	m_pDeaths->SetText(pkvPlayerData->GetString("deaths"));
	m_pPing->SetText(pkvPlayerData->GetString("ping"));

	const char *szPing = pkvPlayerData->GetString("ping");
	m_pPing->SetFgColor(GetPingColorState((m_bIsBot ? -1 : atoi(szPing))));

	int plIndex = GetPlayerIndex();
	if (plIndex && !m_bIsLocalPlayer)
		m_pImgMuteStatus->SetImage(GameBaseClient->IsPlayerGameVoiceMuted(plIndex) ? "scoreboard/icon_muted" : "scoreboard/icon_unmuted");

	if (g_PR)
	{
		m_pSkullIcon->SetVisible((!g_PR->IsAlive(m_iPlayerIndex) || (g_PR->GetTeam(m_iPlayerIndex) <= TEAM_SPECTATOR)));

		// Spectators in this mode are treated as escapees.
		if ((g_PR->GetTeam(m_iPlayerIndex) == TEAM_SPECTATOR) && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE) && GetClientWorldEntity() && !GetClientWorldEntity()->m_bIsStoryMap)
			m_pSkullIcon->SetImage("hud/objectiveicons/escapeicon");
		else
			m_pSkullIcon->SetImage("hud/notices/suicide");
	}

	if (HL2MPRules())
		m_pLevel->SetVisible(HL2MPRules()->CanUseSkills());

	if (m_iSectionTeam == TEAM_SPECTATOR)
	{
		m_pSkullIcon->SetVisible(false);
		m_pLevel->SetVisible(false);
		m_pKills->SetVisible(false);
		m_pDeaths->SetVisible(false);
		m_pPing->SetVisible(false);
	}
}

Color ScoreboardItem::GetPingColorState(int latency)
{
	if (latency < 100)
		return Color(0, 255, 0, 255);
	else if (latency >= 100 && latency < 200)
		return Color(255, 255, 0, 255);
	else
		return Color(255, 0, 0, 255);
}

void ScoreboardItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	int w, h, x, y;
	w = wide;
	h = tall;

	float percent = ((float)w) * 0.48f;

	m_pImgBackground->SetPos(0, 0);
	m_pImgBackground->SetSize(w, h);

	m_pButton->SetPos(0, 0);
	m_pButton->SetSize(w, h);

	m_pUserName->SetSize(w - scheme()->GetProportionalScaledValue(2), h);
	m_pUserName->SetPos(scheme()->GetProportionalScaledValue(23), 0);
	m_pUserName->GetPos(x, y);

	m_pLevel->SetPos(percent + x, 0);
	percent = (((float)w) * 0.12f);
	m_pLevel->SetSize(percent, h);
	m_pLevel->GetPos(x, y);

	m_pKills->SetPos(percent + x, 0);
	m_pKills->SetSize(percent, h);
	m_pKills->GetPos(x, y);

	m_pDeaths->SetPos(percent + x, 0);
	m_pDeaths->SetSize(percent, h);
	m_pDeaths->GetPos(x, y);

	m_pPing->SetPos(percent + x, 0);
	m_pPing->SetSize(percent, h);
	m_pPing->GetPos(x, y);

	m_pSteamAvatar->SetSize(h - scheme()->GetProportionalScaledValue(4), h - scheme()->GetProportionalScaledValue(4));
	m_pSteamAvatar->SetPos(scheme()->GetProportionalScaledValue(3), scheme()->GetProportionalScaledValue(2));

	m_pImgMuteStatus->SetSize(h - scheme()->GetProportionalScaledValue(12), h - scheme()->GetProportionalScaledValue(12));
	m_pImgMuteStatus->SetPos(w - h + scheme()->GetProportionalScaledValue(6), scheme()->GetProportionalScaledValue(6));

	m_pMuteButton->SetSize(h, h);
	m_pMuteButton->SetPos(w - h, 0);

	m_pButton->MoveToFront();

	int wx, wy, wz, wh;
	m_pSteamAvatar->GetBounds(wx, wy, wz, wh);

	m_pSkullIcon->SetPos(wx, wy + (wh / 4));
	m_pSkullIcon->SetSize(wz, (wh / 2));
}

void ScoreboardItem::OnThink()
{
	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (m_pButton->IsWithin(x, y))
	{
		if (!m_bRollover)
		{
			m_bRollover = true;
			GetAnimationController()->RunAnimationCommand(m_pImgBackground, "alpha", 256.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else
	{
		if (m_bRollover)
		{
			m_bRollover = false;
			GetAnimationController()->RunAnimationCommand(m_pImgBackground, "alpha", 0.0f, 0.0f, 0.2f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}

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

void ScoreboardItem::PerformLayout()
{
	BaseClass::PerformLayout();

	m_flSteamImgThink = engine->Time() + 0.5f;
	m_pImgBackground->SetAlpha(0);
}

void ScoreboardItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pKills->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
	m_pDeaths->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
	m_pLevel->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));

	m_pKills->SetFont(pScheme->GetFont("Default"));
	m_pDeaths->SetFont(pScheme->GetFont("Default"));
	m_pPing->SetFont(pScheme->GetFont("Default"));
	m_pLevel->SetFont(pScheme->GetFont("Default"));
}

void ScoreboardItem::OnCommand(const char* pcCommand)
{
	// Redirect to community link / steam profile. From Id 32 to Id 64
	if (!Q_stricmp(pcCommand, "Redirect"))
	{
		if (!m_bIsBot && steamapicontext && steamapicontext->SteamFriends())
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage(VarArgs("http://steamcommunity.com/profiles/%s", szSteamID));
	}
	else if (!Q_stricmp(pcCommand, "Mute"))
	{
		if (!m_bIsLocalPlayer)
		{
			int plIndex = GetPlayerIndex();
			if (GameBaseClient->IsPlayerGameVoiceMuted(plIndex))
				GameBaseClient->MutePlayerGameVoice(plIndex, false);
			else
				GameBaseClient->MutePlayerGameVoice(plIndex, true);
		}
	}

	BaseClass::OnCommand(pcCommand);
}
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Scoreboard Team Section Handler
//
//========================================================================================//

#include "cbase.h"
#include "ScoreBoardSectionPanel.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Client.h"
#include "GameBase_Shared.h"
#include "c_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define BANNER_HEIGHT scheme()->GetProportionalScaledValue(15)
#define UPDATE_FREQUENCY 0.3f

typedef vgui::ScoreboardItem* ScoreboardObject;
static int SortPlayerScorePredicate(const ScoreboardObject *data1, const ScoreboardObject *data2)
{
	int score1 = (*data1)->GetPlayerScore();
	int score2 = (*data2)->GetPlayerScore();

	if (score1 == score2)
		return 0;

	if (score1 < score2)
		return 1;

	return -1;
}

CScoreBoardSectionPanel::CScoreBoardSectionPanel(vgui::Panel *parent, char const *panelName, int targetTeam) : vgui::Panel(parent, panelName)
{
	m_pItems.Purge();
	m_flLastUpdate = 0.0f;
	m_iTeamLink = targetTeam;

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);
	SetPaintBackgroundEnabled(true);

	SetScheme("BaseScheme");

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetShouldScaleImage(true);
	m_pBackground->SetZPos(5);
	m_pBackground->SetImage("scoreboard/background");

	m_pBanner = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Banner"));
	m_pBanner->SetShouldScaleImage(true);
	m_pBanner->SetZPos(10);

	switch (m_iTeamLink)
	{
	case TEAM_HUMANS:
		m_pBanner->SetImage("scoreboard/human_banner");
		break;
	case TEAM_DECEASED:
		m_pBanner->SetImage("scoreboard/zombie_banner");
		break;
	default:
		m_pBanner->SetImage("scoreboard/spectator_banner");
		break;
	}

	const char *pchSectionNames[] =
	{
		"#GameUI_Leaderboard_Name",
		"#GameUI_Leaderboard_Level",
		"#GameUI_Leaderboard_Kills",
		"#GameUI_Leaderboard_Deaths",
		"#GameUI_Leaderboard_Ping",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pSectionInfo); i++)
	{
		m_pSectionInfo[i] = vgui::SETUP_PANEL(new vgui::Label(this, "Info", ""));
		m_pSectionInfo[i]->SetZPos(15);
		m_pSectionInfo[i]->SetText(pchSectionNames[i]);
		//m_pSectionInfo[i]->SetContentAlignment(Label::Alignment::a_center);
	}

	InvalidateLayout();
	PerformLayout();
}

CScoreBoardSectionPanel::~CScoreBoardSectionPanel()
{
	Cleanup();
}

void CScoreBoardSectionPanel::OnThink()
{
	BaseClass::OnThink();

	if (m_flLastUpdate < gpGlobals->curtime)
	{
		m_flLastUpdate = gpGlobals->curtime + UPDATE_FREQUENCY;
		UpdateScoreInfo();
	}
}

void CScoreBoardSectionPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h, x, y;
	GetSize(w, h);

	m_pBackground->SetSize(w, h);
	m_pBackground->SetPos(0, 0);

	m_pBanner->SetSize(w, BANNER_HEIGHT);
	m_pBanner->SetPos(0, 0);

	float percent = ((float)w) * 0.47f;
	int offset = scheme()->GetProportionalScaledValue(24);

	m_pSectionInfo[0]->SetPos(offset, 0);
	m_pSectionInfo[0]->SetSize(percent, BANNER_HEIGHT);
	m_pSectionInfo[0]->GetPos(x, y);

	m_pSectionInfo[1]->SetPos(percent + x, 0);
	percent = (((float)w) * 0.12f);
	m_pSectionInfo[1]->SetSize(percent, BANNER_HEIGHT);
	m_pSectionInfo[1]->GetPos(x, y);

	m_pSectionInfo[2]->SetPos(percent + x, 0);
	m_pSectionInfo[2]->SetSize(percent, BANNER_HEIGHT);
	m_pSectionInfo[2]->GetPos(x, y);

	m_pSectionInfo[3]->SetPos(percent + x, 0);
	m_pSectionInfo[3]->SetSize(percent, BANNER_HEIGHT);
	m_pSectionInfo[3]->GetPos(x, y);

	m_pSectionInfo[4]->SetPos(percent + x, 0);
	m_pSectionInfo[4]->SetSize(percent, BANNER_HEIGHT);

	if (HL2MPRules())
		m_pSectionInfo[1]->SetVisible(HL2MPRules()->CanUseSkills());

	if (m_iTeamLink == TEAM_SPECTATOR)
	{
		for (int i = 1; i < _ARRAYSIZE(m_pSectionInfo); i++)
			m_pSectionInfo[i]->SetVisible(false);

		m_pSectionInfo[0]->SetSize(w, BANNER_HEIGHT);
		m_pSectionInfo[0]->SetPos(0, 0);
		m_pSectionInfo[0]->SetText("#GameUI_Scoreboard_Waiting");
		m_pSectionInfo[0]->SetContentAlignment(Label::Alignment::a_center);
	}

	m_flLastUpdate = 0.0f;
}

void CScoreBoardSectionPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pSectionInfo); i++)
	{
		m_pSectionInfo[i]->SetFgColor(pScheme->GetColor("ScoreboardItemTextColor", Color(255, 255, 255, 255)));
		m_pSectionInfo[i]->SetFont(pScheme->GetFont("OptionTextSmall"));
	}
}

void CScoreBoardSectionPanel::Cleanup()
{
	for (int i = (m_pItems.Count() - 1); i >= 0; i--)
	{
		delete m_pItems[i];
		m_pItems.Remove(i);
	}

	m_pItems.Purge();
	m_flLastUpdate = 0.0f;
}

void CScoreBoardSectionPanel::UpdateScoreInfo()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer || !g_PR)
		return;

	int w, h;
	GetSize(w, h);

	// Remove incompatible items:
	for (int i = gpGlobals->maxClients; i >= 1; i--)
	{
		int itemID = GetIndexFromScoreItemList(i);
		int selectedTeam = g_PR->GetSelectedTeam(i);
		int currentTeam = g_PR->GetTeam(i);

		if ((!g_PR->IsConnected(i) || g_PR->IsHLTV(i) || ((m_iTeamLink == TEAM_SPECTATOR) && (selectedTeam != 0)) || ((selectedTeam != m_iTeamLink) && (currentTeam != m_iTeamLink)))
			&& (itemID != -1))
		{
			delete m_pItems[itemID];
			m_pItems.Remove(itemID);
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!g_PR->IsConnected(i) || g_PR->IsHLTV(i))
			continue;

		int selectedTeam = g_PR->GetSelectedTeam(i);
		int currentTeam = g_PR->GetTeam(i);

		if (m_iTeamLink == TEAM_SPECTATOR)
		{
			if (selectedTeam != 0)
				continue;
		}
		else
		{
			if (selectedTeam != m_iTeamLink && currentTeam != m_iTeamLink)
				continue;
		}

		int itemID = GetIndexFromScoreItemList(i);

		KeyValues *playerData = new KeyValues("data");
		GetScoreInfo(i, playerData);

		if (itemID == -1)
		{
			vgui::ScoreboardItem *scoreboardItem = vgui::SETUP_PANEL(new vgui::ScoreboardItem(this, "ScoreItem", playerData));
			scoreboardItem->SetZPos(25);
			m_pItems.AddToTail(scoreboardItem);
		}
		else
			m_pItems[itemID]->UpdateItem(playerData);

		playerData->deleteThis();
	}

	CUtlVector<vgui::ScoreboardItem*> tempList;
	for (int i = 0; i < m_pItems.Count(); i++)
		tempList.AddToTail(m_pItems[i]);

	// Sort high -> low
	if (tempList.Count() > 1)
		tempList.Sort(SortPlayerScorePredicate);

	for (int i = 0; i < tempList.Count(); i++)
	{
		tempList[i]->SetSize(w, scheme()->GetProportionalScaledValue(20));
		tempList[i]->SetPos(0, scheme()->GetProportionalScaledValue(17 + (i * 20)));
	}

	tempList.RemoveAll();
}

void CScoreBoardSectionPanel::GetScoreInfo(int playerIndex, KeyValues *kv)
{
	player_info_t pi;
	engine->GetPlayerInfo(playerIndex, &pi);

	kv->SetInt("sectionTeam", m_iTeamLink);
	kv->SetInt("playerIndex", playerIndex);
	kv->SetString("name", g_PR->GetPlayerName(playerIndex));
	kv->SetString("steamid", pi.guid);

	if (g_PR->IsFakePlayer(playerIndex))
	{
		kv->SetString("ping", "BOT");
		kv->SetBool("fakeplr", true);
	}
	else
	{
		kv->SetString("ping", VarArgs("%i", g_PR->GetPing(playerIndex)));
		kv->SetBool("fakeplr", false);
	}

	kv->SetInt("level", g_PR->GetLevel(playerIndex));
	kv->SetInt("kills", g_PR->GetTotalScore(playerIndex));
	kv->SetInt("deaths", g_PR->GetTotalDeaths(playerIndex));

	if (g_PR->IsGroupIDFlagActive(playerIndex, GROUPID_IS_DEVELOPER))
		kv->SetString("namePrefix", "Developer");
	else if (g_PR->IsGroupIDFlagActive(playerIndex, GROUPID_IS_DONATOR))
		kv->SetString("namePrefix", "Donator");
	else if (g_PR->IsGroupIDFlagActive(playerIndex, GROUPID_IS_TESTER))
		kv->SetString("namePrefix", "Tester");
	else
		kv->SetString("namePrefix", "");
}

int CScoreBoardSectionPanel::GetIndexFromScoreItemList(int playerIndex)
{
	for (int i = 0; i < m_pItems.Count(); i++)
	{
		if (m_pItems[i]->GetPlayerIndex() == playerIndex)
			return i;
	}

	return -1;
}
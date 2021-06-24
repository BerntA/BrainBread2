//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Show top 4 players GUI! Finds the 4 top scorers and theirs steam avatars.
//
//========================================================================================//

#include "cbase.h"
#include "TopPlayersPanel.h"
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "c_playerresource.h"
#include "KeyValues.h"
#include "c_hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define MAX_SHOW_SCORE_FOR_PLR 4

struct PlayerScoreItem
{
	PlayerScoreItem(int index, int score)
	{
		this->index = index;
		this->score = score;
	}

	int index;
	int score;

	static int __cdecl SortPlayerScorePredicate(const PlayerScoreItem *data1, const PlayerScoreItem *data2)
	{
		int score1 = data1->score;
		int score2 = data2->score;

		if (score1 == score2)
			return 0;

		if (score1 < score2)
			return 1;

		return -1;
	}
};

TopPlayersPanel::TopPlayersPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/1st", "scoreboard/2nd", };

	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
	{
		m_pImageAvatars[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "AvatarIMG"));
		m_pImageAvatars[i]->SetShouldScaleImage(true);
		m_pImageAvatars[i]->SetZPos(20);

		m_pImageStars[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "StarIMG"));
		m_pImageStars[i]->SetShouldScaleImage(true);
		m_pImageStars[i]->SetZPos(30);
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

		m_pLabelNames[i] = vgui::SETUP_PANEL(new vgui::Label(this, "NameLabel", ""));
		m_pLabelNames[i]->SetContentAlignment(Label::a_center);
		m_pLabelNames[i]->SetZPos(40);
		m_pLabelNames[i]->SetText("");

		m_pSteamAvatars[i] = NULL;
	}

	for (int i = 0; i < _ARRAYSIZE(m_pLabelInfo); i++)
	{
		m_pLabelInfo[i] = vgui::SETUP_PANEL(new vgui::Label(this, "InfoLabel", ""));
		m_pLabelInfo[i]->SetContentAlignment(Label::a_center);
		m_pLabelInfo[i]->SetZPos(40);
		m_pLabelInfo[i]->SetText("");
	}

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	InvalidateLayout();

	PerformLayout();
}

TopPlayersPanel::~TopPlayersPanel()
{
	Reset();
}

void TopPlayersPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void TopPlayersPanel::FindAndSetAvatarForPlayer(int playerIndex, int panelIndex)
{
	if ((IsPlayerIndex(playerIndex) == false) || (g_PR == NULL) ||
		(panelIndex < 0) || (panelIndex >= _ARRAYSIZE(m_pSteamAvatars)))
		return;

	m_pLabelNames[panelIndex]->SetText(g_PR->GetPlayerName(playerIndex));

	player_info_t pi;
	if (engine->GetPlayerInfo(playerIndex, &pi))
	{
		if (!pi.fakeplayer && pi.friendsID && steamapicontext && steamapicontext->SteamUtils())
		{
			CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
			m_pSteamAvatars[panelIndex] = new CAvatarImage();
			m_pSteamAvatars[panelIndex]->SetAvatarSteamID(steamIDForPlayer);
			m_pSteamAvatars[panelIndex]->SetAvatarSize(32, 32);
			m_pSteamAvatars[panelIndex]->SetSize(32, 32);
			m_pImageAvatars[panelIndex]->SetImage(m_pSteamAvatars[panelIndex]);
		}
		else
			m_pImageAvatars[panelIndex]->SetImage("steam_default_avatar");
	}
}

/*static*/ void TopPlayersPanel::FindPlayersWithHighestScore(CUtlVector<PlayerScoreItem> &list, int wantedTeam /*= TEAM_ANY*/, bool roundScores /*= false*/)
{
	if (g_PR == NULL)
		return;

	list.Purge();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (g_PR->IsConnected(i) == false)
			continue;

		if (wantedTeam != TEAM_ANY)
		{
			if (g_PR->GetSelectedTeam(i) != wantedTeam)
				continue;
		}

		int score = (roundScores ? g_PR->GetRoundScore(i) : g_PR->GetTotalScore(i));
		if (score <= 0)
			continue;

		list.AddToTail(PlayerScoreItem(i, score));
	}

	if (list.Count() <= 1)
		return;

	list.Sort(PlayerScoreItem::SortPlayerScorePredicate);
}

void TopPlayersPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
	{
		m_pLabelNames[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pLabelNames[i]->SetFont(pScheme->GetFont("AchievementTitleFontSmaller"));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pLabelInfo); i++)
	{
		m_pLabelInfo[i]->SetFgColor(Color(255, 255, 255, 255));
		m_pLabelInfo[i]->SetFont(pScheme->GetFont("AchievementTitleFont"));
	}
}

void TopPlayersPanel::OnThink()
{
	BaseClass::OnThink();

	int w, h;
	GetSize(w, h);

	bool bShowSingleLineOverall = !m_pImageAvatars[1]->IsVisible();
	bool bShowSingleLineRound = !m_pImageAvatars[3]->IsVisible();

	for (int i = 0; i < _ARRAYSIZE(m_pLabelInfo); i++)
	{
		m_pLabelInfo[i]->SetPos(0, (i * (h / 2)));
		m_pLabelInfo[i]->SetSize(w, scheme()->GetProportionalScaledValue(18));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
	{
		m_pImageAvatars[i]->SetSize(scheme()->GetProportionalScaledValue(64), scheme()->GetProportionalScaledValue(64));
		m_pImageStars[i]->SetSize(scheme()->GetProportionalScaledValue(24), scheme()->GetProportionalScaledValue(24));
		m_pLabelNames[i]->SetSize(scheme()->GetProportionalScaledValue(128), scheme()->GetProportionalScaledValue(18));
	}

	m_pLabelNames[0]->SetPos(bShowSingleLineOverall ? (w / 2) - scheme()->GetProportionalScaledValue(64) : 0, scheme()->GetProportionalScaledValue(104));
	m_pImageAvatars[0]->SetPos(bShowSingleLineOverall ? (w / 2) - scheme()->GetProportionalScaledValue(32) : scheme()->GetProportionalScaledValue(32), scheme()->GetProportionalScaledValue(30));
	m_pImageStars[0]->SetPos(bShowSingleLineOverall ? (w / 2) - scheme()->GetProportionalScaledValue(44) : scheme()->GetProportionalScaledValue(20), scheme()->GetProportionalScaledValue(82));

	m_pLabelNames[1]->SetPos(w - scheme()->GetProportionalScaledValue(128), scheme()->GetProportionalScaledValue(104));
	m_pImageAvatars[1]->SetPos(w - scheme()->GetProportionalScaledValue(96), scheme()->GetProportionalScaledValue(30));
	m_pImageStars[1]->SetPos((w - scheme()->GetProportionalScaledValue(108)), scheme()->GetProportionalScaledValue(82));

	int iPage2 = (h / 2) + scheme()->GetProportionalScaledValue(10);

	m_pLabelNames[2]->SetPos(bShowSingleLineRound ? (w / 2) - scheme()->GetProportionalScaledValue(64) : 0, iPage2 + scheme()->GetProportionalScaledValue(104));
	m_pImageAvatars[2]->SetPos(bShowSingleLineRound ? (w / 2) - scheme()->GetProportionalScaledValue(32) : scheme()->GetProportionalScaledValue(32), iPage2 + scheme()->GetProportionalScaledValue(30));
	m_pImageStars[2]->SetPos(bShowSingleLineRound ? (w / 2) - scheme()->GetProportionalScaledValue(44) : scheme()->GetProportionalScaledValue(20), iPage2 + scheme()->GetProportionalScaledValue(82));

	m_pLabelNames[3]->SetPos(w - scheme()->GetProportionalScaledValue(128), iPage2 + scheme()->GetProportionalScaledValue(104));
	m_pImageAvatars[3]->SetPos(w - scheme()->GetProportionalScaledValue(96), iPage2 + scheme()->GetProportionalScaledValue(30));
	m_pImageStars[3]->SetPos((w - scheme()->GetProportionalScaledValue(108)), iPage2 + scheme()->GetProportionalScaledValue(82));
}

void TopPlayersPanel::Reset()
{
	for (int i = 0; i < _ARRAYSIZE(m_pSteamAvatars); i++)
	{
		if (m_pSteamAvatars[i] != NULL)
		{
			delete m_pSteamAvatars[i];
			m_pSteamAvatars[i] = NULL;
		}
	}

	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageAvatars[i]->SetImage("transparency");
}

bool TopPlayersPanel::FindTopPlayers(void)
{
	Reset();

	if (!g_PR || !HL2MPRules())
		return false;

	CUtlVector<PlayerScoreItem> pScoreList1, pScoreList2;

	bool bRes = false;
	switch (HL2MPRules()->GetCurrentGamemode())
	{
	case MODE_OBJECTIVE:
		bRes = FindTopPlayersObjective(pScoreList1, pScoreList2);
		break;
	case MODE_ELIMINATION:
		bRes = FindTopPlayersElimination(pScoreList1, pScoreList2);
		break;
	case MODE_ARENA:
		bRes = FindTopPlayersArena(pScoreList1, pScoreList2);
		break;
	case MODE_DEATHMATCH:
		bRes = FindTopPlayersDeathmatch(pScoreList1, pScoreList2);
		break;
	}

	pScoreList1.Purge();
	pScoreList2.Purge();
	return bRes;
}

bool TopPlayersPanel::FindTopPlayersElimination(CUtlVector<PlayerScoreItem> &list1, CUtlVector<PlayerScoreItem> &list2)
{
	FindPlayersWithHighestScore(list1, TEAM_HUMANS, true);
	int sizeHumans = list1.Count();

	FindPlayersWithHighestScore(list2, TEAM_DECEASED, true);
	int sizeZombies = list2.Count();

	if ((sizeHumans <= 0) && (sizeZombies <= 0))
		return false;

	for (int i = 0; i < 2; i++)
	{
		m_pImageAvatars[i]->SetVisible(sizeHumans >= (1 + i));
		m_pImageStars[i]->SetVisible(sizeHumans >= (1 + i));
		m_pLabelNames[i]->SetVisible(sizeHumans >= (1 + i));
	}

	for (int i = 2; i < MAX_SHOW_SCORE_FOR_PLR; i++)
	{
		m_pImageAvatars[i]->SetVisible(sizeZombies >= (1 + (i - 2)));
		m_pImageStars[i]->SetVisible(sizeZombies >= (1 + (i - 2)));
		m_pLabelNames[i]->SetVisible(sizeZombies >= (1 + (i - 2)));
	}

	m_pLabelInfo[0]->SetVisible(sizeHumans > 0);
	m_pLabelInfo[1]->SetVisible(sizeZombies > 0);

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Humans");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Zombies");

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/1st", "scoreboard/2nd", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	int idx = 0;
	for (int i = 0; i < 2; i++)
	{
		if (i >= sizeHumans)
			break;

		FindAndSetAvatarForPlayer(list1[i].index, idx);
		idx++;
	}
	idx = 2;
	for (int i = 0; i < 2; i++)
	{
		if (i >= sizeZombies)
			break;

		FindAndSetAvatarForPlayer(list2[i].index, idx);
		idx++;
	}

	return true;
}

bool TopPlayersPanel::FindTopPlayersObjective(CUtlVector<PlayerScoreItem> &list1, CUtlVector<PlayerScoreItem> &list2)
{
	FindPlayersWithHighestScore(list1);
	int sizeOverall = list1.Count();

	// If we can't find any score above 0 that means we have no 'scores' to show!
	if (sizeOverall <= 0)
		return false;

	// Hide controls if the amount of scores are not:
	for (int i = 0; i < MAX_SHOW_SCORE_FOR_PLR; i++)
	{
		m_pImageAvatars[i]->SetVisible(sizeOverall >= (1 + i));
		m_pImageStars[i]->SetVisible(sizeOverall >= (1 + i));
		m_pLabelNames[i]->SetVisible(sizeOverall >= (1 + i));
	}

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/3rd", "scoreboard/4th", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	m_pLabelInfo[0]->SetVisible(true);
	m_pLabelInfo[1]->SetVisible(false);

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	for (int i = 0; i < MAX_SHOW_SCORE_FOR_PLR; i++)
	{
		if (i >= sizeOverall)
			break;

		FindAndSetAvatarForPlayer(list1[i].index, i);
	}

	return true;
}

bool TopPlayersPanel::FindTopPlayersArena(CUtlVector<PlayerScoreItem> &listHighOverall, CUtlVector<PlayerScoreItem> &listHighRound)
{
	FindPlayersWithHighestScore(listHighOverall);
	int sizeOverall = listHighOverall.Count();

	FindPlayersWithHighestScore(listHighRound, TEAM_ANY, true);
	int sizeRound = listHighRound.Count();

	// If we can't find any score above 0 that means we have no 'scores' to show!
	if (sizeOverall <= 0)
		return false;

	for (int i = 0; i < 2; i++)
	{
		m_pImageAvatars[i]->SetVisible(sizeOverall >= (1 + i));
		m_pImageStars[i]->SetVisible(sizeOverall >= (1 + i));
		m_pLabelNames[i]->SetVisible(sizeOverall >= (1 + i));
	}

	for (int i = 2; i < MAX_SHOW_SCORE_FOR_PLR; i++)
	{
		m_pImageAvatars[i]->SetVisible(sizeRound >= (1 + (i - 2)));
		m_pImageStars[i]->SetVisible(sizeRound >= (1 + (i - 2)));
		m_pLabelNames[i]->SetVisible(sizeRound >= (1 + (i - 2)));
	}

	m_pLabelInfo[0]->SetVisible(sizeOverall > 0);
	m_pLabelInfo[1]->SetVisible(sizeRound > 0);

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/1st", "scoreboard/2nd", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	int idx = 0;
	for (int i = 0; i < 2; i++)
	{
		if (i >= sizeOverall)
			break;

		FindAndSetAvatarForPlayer(listHighOverall[i].index, idx);
		idx++;
	}
	idx = 2;
	for (int i = 0; i < 2; i++)
	{
		if (i >= sizeRound)
			break;

		FindAndSetAvatarForPlayer(listHighRound[i].index, idx);
		idx++;
	}

	return true;
}

bool TopPlayersPanel::FindTopPlayersDeathmatch(CUtlVector<PlayerScoreItem> &list1, CUtlVector<PlayerScoreItem> &list2)
{
	FindPlayersWithHighestScore(list1);
	int sizeOverall = list1.Count();

	// If we can't find any score above 0 that means we have no 'scores' to show!
	if (sizeOverall <= 0)
		return false;

	// Hide controls if the amount of scores are not:
	for (int i = 0; i < MAX_SHOW_SCORE_FOR_PLR; i++)
	{
		m_pImageAvatars[i]->SetVisible(sizeOverall >= (1 + i));
		m_pImageStars[i]->SetVisible(sizeOverall >= (1 + i));
		m_pLabelNames[i]->SetVisible(sizeOverall >= (1 + i));
	}

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/3rd", "scoreboard/4th", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	m_pLabelInfo[0]->SetVisible(true);
	m_pLabelInfo[1]->SetVisible(false);

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	for (int i = 0; i < MAX_SHOW_SCORE_FOR_PLR; i++)
	{
		if (i >= sizeOverall)
			break;

		FindAndSetAvatarForPlayer(list1[i].index, i);
	}

	return true;
}
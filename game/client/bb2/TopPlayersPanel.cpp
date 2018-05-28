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

void TopPlayersPanel::FindAndSetAvatarForPlayer(int index, vgui::ImagePanel *image, int avatarIndex)
{
	if ((IsPlayerIndex(index) == false) || 
		(avatarIndex < 0) || (avatarIndex >= _ARRAYSIZE(m_pSteamAvatars)) ||
		(image == NULL))
		return;

	player_info_t pi;
	if (engine->GetPlayerInfo(index, &pi))
	{
		if (!pi.fakeplayer && pi.friendsID)
		{
			CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
			m_pSteamAvatars[avatarIndex] = new CAvatarImage();
			m_pSteamAvatars[avatarIndex]->SetAvatarSteamID(steamIDForPlayer);
			m_pSteamAvatars[avatarIndex]->SetAvatarSize(32, 32);
			m_pSteamAvatars[avatarIndex]->SetSize(32, 32);
			image->SetImage(m_pSteamAvatars[avatarIndex]);
		}
		else
			image->SetImage("steam_default_avatar");
	}
}

int TopPlayersPanel::FindPlayerIndexWithHighestScore(int *excluded, int size, int wantedTeam)
{
	bool bShouldExclude = (size > 0);

	int index = -1;
	int score = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int team = g_PR->GetSelectedTeam(i);
		if (team != wantedTeam)
			continue;

		if (bShouldExclude)
		{
			for (int x = 0; x < size; x++)
			{
				if (i == excluded[x])
					
			}
		}

		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > score)
		{
			score = playerScore;
			index = i;
		}
	}

	return index;
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

	switch (HL2MPRules()->GetCurrentGamemode())
	{
	case MODE_OBJECTIVE:
		return FindTopPlayersObjective();
	case MODE_ELIMINATION:
		return FindTopPlayersElimination();
	case MODE_ARENA:
		return FindTopPlayersArena();
	case MODE_DEATHMATCH:
		return FindTopPlayersDeathmatch();
	}

	return false;
}

bool TopPlayersPanel::FindTopPlayersElimination(void)
{
	bool bFound = false;

	int m_iPlayerIndex[4];
	int iScore = 0;

	// Find highest for the human team.
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int team = g_PR->GetSelectedTeam(i);
		if (team != TEAM_HUMANS)
			continue;

		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[0] = i;
		}
	}

	// Check for highest human score? none? Hide it then.
	m_pImageAvatars[0]->SetVisible(iScore > 0);
	m_pImageStars[0]->SetVisible(iScore > 0);
	m_pLabelNames[0]->SetVisible(iScore > 0);

	iScore = 0;

	// Find second highest score on the human team.
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int team = g_PR->GetSelectedTeam(i);
		if (team != TEAM_HUMANS)
			continue;

		if (i == m_iPlayerIndex[0])
			continue;

		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[1] = i;
		}
	}

	// No secondary? Hide it then.
	m_pImageAvatars[1]->SetVisible(iScore > 0);
	m_pImageStars[1]->SetVisible(iScore > 0);
	m_pLabelNames[1]->SetVisible(iScore > 0);

	iScore = 0;

	// Find highest score on the zombie team:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int team = g_PR->GetSelectedTeam(i);
		if (team != TEAM_DECEASED)
			continue;

		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[2] = i;
		}
	}

	// If there's no best round scorer, hide.
	m_pImageAvatars[2]->SetVisible(iScore > 0);
	m_pImageStars[2]->SetVisible(iScore > 0);
	m_pLabelNames[2]->SetVisible(iScore > 0);

	iScore = 0;

	// Find second highest score for the zombie team.
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int team = g_PR->GetSelectedTeam(i);
		if (team != TEAM_DECEASED)
			continue;

		if (i == m_iPlayerIndex[2])
			continue;

		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[3] = i;
		}
	}

	// If there's no best second round scorer, hide.
	m_pImageAvatars[3]->SetVisible(iScore > 0);
	m_pImageStars[3]->SetVisible(iScore > 0);
	m_pLabelNames[3]->SetVisible(iScore > 0);

	m_pLabelInfo[0]->SetVisible((m_pImageAvatars[0]->IsVisible() || m_pImageAvatars[1]->IsVisible()));
	m_pLabelInfo[1]->SetVisible((m_pImageAvatars[2]->IsVisible() || m_pImageAvatars[3]->IsVisible()));

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Humans");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Zombies");

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/1st", "scoreboard/2nd", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
	{
		if (m_pImageAvatars[i]->IsVisible())
			bFound = true;

		m_pImageStars[i]->SetImage(szStarIMGs[i]);
	}

	// Set the player labels:
	for (int i = 0; i < _ARRAYSIZE(m_iPlayerIndex); i++)
	{
		if (m_iPlayerIndex[i] <= 0)
			continue;

		m_pLabelNames[i]->SetText(g_PR->GetPlayerName(m_iPlayerIndex[i]));

		// Get the image avatars for the scorers:
		for (int player = 1; player <= gpGlobals->maxClients; player++)
		{
			C_HL2MP_Player *pCast = ToHL2MPPlayer(UTIL_PlayerByIndex(player));
			if (!pCast)
				continue;

			if (player != m_iPlayerIndex[i])
				continue;

			player_info_t pi;
			if (engine->GetPlayerInfo(player, &pi))
			{
				if (!pi.fakeplayer)
				{
					if (pi.friendsID)
					{
						CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
						m_pSteamAvatars[i] = new CAvatarImage();
						m_pSteamAvatars[i]->SetAvatarSteamID(steamIDForPlayer);
						m_pSteamAvatars[i]->SetAvatarSize(32, 32);
						m_pSteamAvatars[i]->SetSize(32, 32);
						m_pImageAvatars[i]->SetImage(m_pSteamAvatars[i]);
					}
				}
				else
					m_pImageAvatars[i]->SetImage("steam_default_avatar");
			}
		}
	}

	return bFound;
}

bool TopPlayersPanel::FindTopPlayersObjective(void)
{
	int m_iPlayerIndex[4];
	int iScore = 0;

	// Find highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[0] = i;
		}
	}

	// If we can't find any score above 0 that means we have no 'scores' to show!
	if (iScore <= 0)
		return false;

	m_pImageAvatars[0]->SetVisible(iScore > 0);
	m_pImageStars[0]->SetVisible(iScore > 0);
	m_pLabelNames[0]->SetVisible(iScore > 0);

	iScore = 0;

	// Find second highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (i == m_iPlayerIndex[0])
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[1] = i;
		}
	}

	// No secondary? Hide it then.
	m_pImageAvatars[1]->SetVisible(iScore > 0);
	m_pImageStars[1]->SetVisible(iScore > 0);
	m_pLabelNames[1]->SetVisible(iScore > 0);

	iScore = 0;

	// Find third highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if ((i == m_iPlayerIndex[0]) || (i == m_iPlayerIndex[1]))
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[2] = i;
		}
	}

	// No third? Hide it then.
	m_pImageAvatars[2]->SetVisible(iScore > 0);
	m_pImageStars[2]->SetVisible(iScore > 0);
	m_pLabelNames[2]->SetVisible(iScore > 0);

	iScore = 0;

	// Find fourth highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if ((i == m_iPlayerIndex[0]) || (i == m_iPlayerIndex[1]) || (i == m_iPlayerIndex[2]))
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[3] = i;
		}
	}

	// No fourth? Hide it then.
	m_pImageAvatars[3]->SetVisible(iScore > 0);
	m_pImageStars[3]->SetVisible(iScore > 0);
	m_pLabelNames[3]->SetVisible(iScore > 0);

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/3rd", "scoreboard/4th", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	m_pLabelInfo[0]->SetVisible((m_pImageAvatars[0]->IsVisible() || m_pImageAvatars[1]->IsVisible()));
	m_pLabelInfo[1]->SetVisible(false);

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	// Set the player labels:
	for (int i = 0; i < _ARRAYSIZE(m_iPlayerIndex); i++)
	{
		if (m_iPlayerIndex[i] <= 0)
			continue;

		m_pLabelNames[i]->SetText(g_PR->GetPlayerName(m_iPlayerIndex[i]));

		// Get the image avatars for the scorers:
		for (int player = 1; player <= gpGlobals->maxClients; player++)
		{
			C_HL2MP_Player *pCast = ToHL2MPPlayer(UTIL_PlayerByIndex(player));
			if (!pCast)
				continue;

			if (player != m_iPlayerIndex[i])
				continue;

			player_info_t pi;
			if (engine->GetPlayerInfo(player, &pi))
			{
				if (!pi.fakeplayer)
				{
					if (pi.friendsID)
					{
						CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
						m_pSteamAvatars[i] = new CAvatarImage();
						m_pSteamAvatars[i]->SetAvatarSteamID(steamIDForPlayer);
						m_pSteamAvatars[i]->SetAvatarSize(32, 32);
						m_pSteamAvatars[i]->SetSize(32, 32);
						m_pImageAvatars[i]->SetImage(m_pSteamAvatars[i]);
					}
				}
				else
					m_pImageAvatars[i]->SetImage("steam_default_avatar");
			}
		}
	}

	return true;
}

bool TopPlayersPanel::FindTopPlayersArena(void)
{
	int m_iPlayerIndex[4];
	int iScore = 0;

	// Find highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[0] = i;
		}
	}

	// If we can't find any score above 0 that means we have no 'scores' to show!
	if (iScore <= 0)
		return false;

	m_pImageAvatars[0]->SetVisible(iScore > 0);
	m_pImageStars[0]->SetVisible(iScore > 0);
	m_pLabelNames[0]->SetVisible(iScore > 0);

	iScore = 0;

	// Find second highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (i == m_iPlayerIndex[0])
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[1] = i;
		}
	}

	// No secondary? Hide it then.
	m_pImageAvatars[1]->SetVisible(iScore > 0);
	m_pImageStars[1]->SetVisible(iScore > 0);
	m_pLabelNames[1]->SetVisible(iScore > 0);

	iScore = 0;

	// Find highest score for this round:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[2] = i;
		}
	}

	// If there's no best round scorer, hide.
	m_pImageAvatars[2]->SetVisible(iScore > 0);
	m_pImageStars[2]->SetVisible(iScore > 0);
	m_pLabelNames[2]->SetVisible(iScore > 0);

	iScore = 0;

	// Find second highest score for this round:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (i == m_iPlayerIndex[2])
			continue;

		int playerScore = g_PR->GetRoundScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[3] = i;
		}
	}

	// If there's no best second round scorer, hide.
	m_pImageAvatars[3]->SetVisible(iScore > 0);
	m_pImageStars[3]->SetVisible(iScore > 0);
	m_pLabelNames[3]->SetVisible(iScore > 0);

	m_pLabelInfo[0]->SetVisible((m_pImageAvatars[0]->IsVisible() || m_pImageAvatars[1]->IsVisible()));
	m_pLabelInfo[1]->SetVisible((m_pImageAvatars[2]->IsVisible() || m_pImageAvatars[3]->IsVisible()));

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/1st", "scoreboard/2nd", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	// Set the player labels:
	for (int i = 0; i < _ARRAYSIZE(m_iPlayerIndex); i++)
	{
		if (m_iPlayerIndex[i] <= 0)
			continue;

		m_pLabelNames[i]->SetText(g_PR->GetPlayerName(m_iPlayerIndex[i]));

		// Get the image avatars for the scorers:
		for (int player = 1; player <= gpGlobals->maxClients; player++)
		{
			C_HL2MP_Player *pCast = ToHL2MPPlayer(UTIL_PlayerByIndex(player));
			if (!pCast)
				continue;

			if (player != m_iPlayerIndex[i])
				continue;

			player_info_t pi;
			if (engine->GetPlayerInfo(player, &pi))
			{
				if (!pi.fakeplayer)
				{
					if (pi.friendsID)
					{
						CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
						m_pSteamAvatars[i] = new CAvatarImage();
						m_pSteamAvatars[i]->SetAvatarSteamID(steamIDForPlayer);
						m_pSteamAvatars[i]->SetAvatarSize(32, 32);
						m_pSteamAvatars[i]->SetSize(32, 32);
						m_pImageAvatars[i]->SetImage(m_pSteamAvatars[i]);
					}
				}
				else
					m_pImageAvatars[i]->SetImage("steam_default_avatar");
			}
		}
	}

	return true;
}

bool TopPlayersPanel::FindTopPlayersDeathmatch(void)
{
	int m_iPlayerIndex[4];
	int iScore = 0;

	// Find highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[0] = i;
		}
	}

	// If we can't find any score above 0 that means we have no 'scores' to show!
	if (iScore <= 0)
		return false;

	m_pImageAvatars[0]->SetVisible(iScore > 0);
	m_pImageStars[0]->SetVisible(iScore > 0);
	m_pLabelNames[0]->SetVisible(iScore > 0);

	iScore = 0;

	// Find second highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (i == m_iPlayerIndex[0])
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[1] = i;
		}
	}

	// No secondary? Hide it then.
	m_pImageAvatars[1]->SetVisible(iScore > 0);
	m_pImageStars[1]->SetVisible(iScore > 0);
	m_pLabelNames[1]->SetVisible(iScore > 0);

	iScore = 0;

	// Find third highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if ((i == m_iPlayerIndex[0]) || (i == m_iPlayerIndex[1]))
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[2] = i;
		}
	}

	// No third? Hide it then.
	m_pImageAvatars[2]->SetVisible(iScore > 0);
	m_pImageStars[2]->SetVisible(iScore > 0);
	m_pLabelNames[2]->SetVisible(iScore > 0);

	iScore = 0;

	// Find fourth highest score overall:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if ((i == m_iPlayerIndex[0]) || (i == m_iPlayerIndex[1]) || (i == m_iPlayerIndex[2]))
			continue;

		int playerScore = g_PR->GetTotalScore(i);
		if (playerScore > iScore)
		{
			iScore = playerScore;
			m_iPlayerIndex[3] = i;
		}
	}

	// No fourth? Hide it then.
	m_pImageAvatars[3]->SetVisible(iScore > 0);
	m_pImageStars[3]->SetVisible(iScore > 0);
	m_pLabelNames[3]->SetVisible(iScore > 0);

	const char *szStarIMGs[] = { "scoreboard/1st", "scoreboard/2nd", "scoreboard/3rd", "scoreboard/4th", };
	for (int i = 0; i < _ARRAYSIZE(m_pImageAvatars); i++)
		m_pImageStars[i]->SetImage(szStarIMGs[i]);

	m_pLabelInfo[0]->SetVisible((m_pImageAvatars[0]->IsVisible() || m_pImageAvatars[1]->IsVisible()));
	m_pLabelInfo[1]->SetVisible(false);

	m_pLabelInfo[0]->SetText("#VGUI_TopPlayersPanel_Overall");
	m_pLabelInfo[1]->SetText("#VGUI_TopPlayersPanel_Round");

	// Set the player labels:
	for (int i = 0; i < _ARRAYSIZE(m_iPlayerIndex); i++)
	{
		if (m_iPlayerIndex[i] <= 0)
			continue;

		m_pLabelNames[i]->SetText(g_PR->GetPlayerName(m_iPlayerIndex[i]));

		// Get the image avatars for the scorers:
		for (int player = 1; player <= gpGlobals->maxClients; player++)
		{
			C_HL2MP_Player *pCast = ToHL2MPPlayer(UTIL_PlayerByIndex(player));
			if (!pCast)
				continue;

			if (player != m_iPlayerIndex[i])
				continue;

			player_info_t pi;
			if (engine->GetPlayerInfo(player, &pi))
			{
				if (!pi.fakeplayer)
				{
					if (pi.friendsID)
					{
						CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
						m_pSteamAvatars[i] = new CAvatarImage();
						m_pSteamAvatars[i]->SetAvatarSteamID(steamIDForPlayer);
						m_pSteamAvatars[i]->SetAvatarSize(32, 32);
						m_pSteamAvatars[i]->SetSize(32, 32);
						m_pImageAvatars[i]->SetImage(m_pSteamAvatars[i]);
					}
				}
				else
					m_pImageAvatars[i]->SetImage("steam_default_avatar");
			}
		}
	}

	return true;
}
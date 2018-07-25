//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map vote item, displays some map or option @ the end game menu.
//
//========================================================================================//

#include "cbase.h"
#include "MapVoteItem.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Shared.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MapVoteItem::MapVoteItem(vgui::Panel *parent, char const *panelName, int type, const char *mapName, bool bButtonOnly) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "BaseText", ""));
	m_pLabelTitle->SetZPos(45);

	m_pVoteInfo = vgui::SETUP_PANEL(new vgui::Label(this, "VoteText", ""));
	m_pVoteInfo->SetZPos(50);
	m_pVoteInfo->SetContentAlignment(Label::a_center);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "BaseButton", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/button_over.wav");
	m_pButton->SetZPos(55);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Activate");

	m_pButton->SetMouseInputEnabled(true);
	m_pButton->SetKeyBoardInputEnabled(true);

	m_pBackground = vgui::SETUP_PANEL(new vgui::Divider(this, "Divider1"));
	m_pBackground->SetZPos(35);
	m_pBackground->SetBorder(NULL);
	m_pBackground->SetPaintBorderEnabled(false);

	m_pVoteBG = vgui::SETUP_PANEL(new vgui::Divider(this, "Divider2"));
	m_pVoteBG->SetZPos(45);
	m_pVoteBG->SetBorder(NULL);
	m_pVoteBG->SetPaintBorderEnabled(false);

	m_pMapImage = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImageForeground"));
	m_pMapImage->SetShouldScaleImage(true);
	m_pMapImage->SetZPos(40);

	InvalidateLayout();

	m_flUpdateTime = 0.0f;
	m_iVoteType = type;
	m_bIsButtonOnly = bButtonOnly;
	m_bSelected = false;
	SetMapLink(mapName);
}

MapVoteItem::~MapVoteItem()
{
}

void MapVoteItem::OnThink()
{
	BaseClass::OnThink();

	if (m_flUpdateTime < engine->Time())
	{
		m_flUpdateTime = engine->Time() + 0.4f;

		if (HL2MPRules())
		{
			int voteCount = HL2MPRules()->m_iEndMapVotesForType.Get(m_iVoteType);
			m_pVoteInfo->SetText(VarArgs("%i", voteCount));
		}
	}

	Color fgColor = Color(255, 255, 255, GetAlpha());
	Color currentColor = fgColor;
	if (m_bSelected)
		currentColor = Color(15, 250, 15, GetAlpha());

	m_pBackground->SetFgColor(currentColor);
	m_pBackground->SetBgColor(currentColor);

	if (m_bIsButtonOnly && m_bSelected)
		m_pLabelTitle->SetFgColor(currentColor);
	else
		m_pLabelTitle->SetFgColor(fgColor);

	m_pVoteInfo->SetFgColor(fgColor);
}

void MapVoteItem::SetMapLink(const char *map)
{
	Q_strncpy(pchMapLink, map, MAX_MAP_NAME);

	m_pLabelTitle->SetText("");
	m_pMapImage->SetImage("steam_default_avatar");
	m_pMapImage->SetVisible(!m_bIsButtonOnly);

	if (GameBaseShared()->GetSharedMapData() && !m_bIsButtonOnly)
	{
		int iMapIndex = GameBaseShared()->GetSharedMapData()->GetMapIndex(pchMapLink);
		if (iMapIndex != -1)
		{
			gameMapItem_t *mapItem = &GameBaseShared()->GetSharedMapData()->pszGameMaps[iMapIndex];
			m_pLabelTitle->SetText(VarArgs("%s - %s", mapItem->pszMapTitle, GetGamemodeNameForPrefix(pchMapLink)));
			m_pMapImage->SetImage(VarArgs("loading/%s_0", pchMapLink));
		}
		else
			m_pLabelTitle->SetText(VarArgs("%s - %s", pchMapLink, GetGamemodeNameForPrefix(pchMapLink)));
	}

	PerformLayout();
}

void MapVoteItem::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize(w, h);

	int voteBGSize = scheme()->GetProportionalScaledValue(12);
	int imageOffset = scheme()->GetProportionalScaledValue(4);

	m_pButton->SetPos(0, 0);
	m_pButton->SetSize(w, h);

	m_pVoteBG->SetPos(0, h - voteBGSize);
	m_pVoteBG->SetSize(voteBGSize, voteBGSize);

	m_pMapImage->SetPos((imageOffset / 2), (imageOffset / 2));
	m_pMapImage->SetSize(w - imageOffset, h - imageOffset - voteBGSize);

	m_pBackground->SetPos(0, 0);
	m_pBackground->SetSize(w, h - voteBGSize);

	m_pLabelTitle->SetSize(w - voteBGSize - scheme()->GetProportionalScaledValue(4), voteBGSize);
	m_pLabelTitle->SetPos(voteBGSize + scheme()->GetProportionalScaledValue(4), h - voteBGSize);

	m_pVoteInfo->SetPos(0, h - voteBGSize);
	m_pVoteInfo->SetSize(voteBGSize, voteBGSize);

	m_pMapImage->SetVisible(!m_bIsButtonOnly);
	m_pBackground->SetVisible(!m_bIsButtonOnly);

	if (m_bIsButtonOnly)
	{
		m_pVoteBG->SetPos(0, 0);
		m_pVoteBG->SetSize(h, h);

		m_pVoteInfo->SetPos(0, 0);
		m_pVoteInfo->SetSize(h, h);

		m_pLabelTitle->SetSize(w - h - scheme()->GetProportionalScaledValue(4), h);
		m_pLabelTitle->SetPos(h + scheme()->GetProportionalScaledValue(4), 0);

		int type = m_iVoteType + 1;
		if (type == ENDMAP_VOTE_RETRY)
			m_pLabelTitle->SetText("#VGUI_GameOver_RetryMap");
		else if (type == ENDMAP_VOTE_REFRESH)
			m_pLabelTitle->SetText("#VGUI_GameOver_RefreshVote");
	}
}

void MapVoteItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pVoteBG->SetFgColor(Color(25, 25, 25, 125));
	m_pVoteBG->SetBgColor(Color(25, 25, 25, 125));

	if (m_bIsButtonOnly)
	{
		m_pLabelTitle->SetFont(pScheme->GetFont("MainMenuTextSmall"));
		m_pVoteInfo->SetFont(pScheme->GetFont("EndVoteMenuFont"));
	}
	else
	{
		m_pLabelTitle->SetFont(pScheme->GetFont("EndVoteMenuFont"));
		m_pVoteInfo->SetFont(pScheme->GetFont("ConsoleText"));
	}
}

void MapVoteItem::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);

	if (!m_bSelected)
		PostActionSignal(new KeyValues("OnPlayerVote", "choice", m_iVoteType));
}
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Vote Menu - Kick, Ban & Map voting.
//
//========================================================================================//

#include "cbase.h"
#include "vote_menu.h"
#include "c_hl2mp_player.h" 
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include "IGameUIFuncs.h"
#include "c_playerresource.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

enum VoteOptions_t
{
	VOTE_OPTION_NONE = 0,
	VOTE_OPTION_KICK,
	VOTE_OPTION_BAN,
	VOTE_OPTION_MAP,
};

#define VOTE_BUTTON_INDEX (_ARRAYSIZE(m_pButton) - 2)
#define RETURN_BUTTON_INDEX (_ARRAYSIZE(m_pButton) - 1)

CVotePanel::CVotePanel(vgui::VPANEL parent) : BaseClass(NULL, "VotePanel", false, 0.15f)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(true);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetVisible(false);

	m_pTitle = vgui::SETUP_PANEL(new vgui::Label(this, "TextTitle", ""));
	m_pTitle->SetZPos(65);
	m_pTitle->SetContentAlignment(Label::Alignment::a_center);

	const char *pchOptions[] =
	{
		"#VOTE_MENU_OPTION_KICK",
		"#VOTE_MENU_OPTION_BAN",
		"#VOTE_MENU_OPTION_MAP",
		"#VOTE_MENU_OPTION_VOTE",
		"#VOTE_MENU_OPTION_BACK",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButton[i] = vgui::SETUP_PANEL(new vgui::Button(this, "Button", pchOptions[i], this, VarArgs("Action%i", (i + 1))));
		m_pButton[i]->SetZPos(50);
		m_pButton[i]->SetContentAlignment(Label::Alignment::a_center);
	}

	m_pItemList = vgui::SETUP_PANEL(new vgui::SectionedListPanel(this, "ItemList"));
	m_pItemList->SetZPos(60);
	m_pItemList->AddActionSignalTarget(this);
	m_pItemList->SetBorder(NULL);
	m_pItemList->SetPaintBorderEnabled(false);
	m_pItemList->SetDrawHeaders(false);
	m_pItemList->AddSection(0, "");
	m_pItemList->SetSectionAlwaysVisible(0);
	m_pItemList->AddColumnToSection(0, "name", "", 0, scheme()->GetProportionalScaledValue(160));

	SetScheme("BaseScheme");

	PerformLayout();
	InvalidateLayout();
}

CVotePanel::~CVotePanel()
{
}

void CVotePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	w = ScreenWidth();
	h = ScreenHeight();

	int width = scheme()->GetProportionalScaledValue(160);
	int height = scheme()->GetProportionalScaledValue(220);

	SetPos((w / 2) - (width / 2), (h / 2) - (height / 2));
	SetSize(width, height);

	m_pItemList->RemoveAll();
	m_pTitle->SetText("#VOTE_MENU_INFO");
	m_pTitle->SetSize(width, scheme()->GetProportionalScaledValue(15));
	m_pTitle->SetPos(0, scheme()->GetProportionalScaledValue(2));

	m_pButton[RETURN_BUTTON_INDEX]->SetVisible(false);
	m_pButton[RETURN_BUTTON_INDEX]->SetEnabled(false);

	m_pButton[VOTE_BUTTON_INDEX]->SetVisible(false);
	m_pButton[VOTE_BUTTON_INDEX]->SetEnabled(false);

	for (int i = 0; i < (_ARRAYSIZE(m_pButton) - 2); i++)
	{
		m_pButton[i]->SetVisible(true);
		m_pButton[i]->SetEnabled(true);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButton[i]->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(14));
		m_pButton[i]->SetPos((width / 2) - (scheme()->GetProportionalScaledValue(60) / 2), scheme()->GetProportionalScaledValue(22) + (scheme()->GetProportionalScaledValue(16) * i));
	}

	m_pItemList->SetVisible(false);
	m_pItemList->SetEnabled(false);

	m_pItemList->SetPos(0, scheme()->GetProportionalScaledValue(22));
	m_pItemList->SetSize(width, scheme()->GetProportionalScaledValue(150));

	m_pButton[VOTE_BUTTON_INDEX]->SetPos((width / 2) - (scheme()->GetProportionalScaledValue(60) / 2), height - (scheme()->GetProportionalScaledValue(18) * 2));
	m_pButton[RETURN_BUTTON_INDEX]->SetPos((width / 2) - (scheme()->GetProportionalScaledValue(60) / 2), height - scheme()->GetProportionalScaledValue(18));

	m_iSelectedOption = VOTE_OPTION_NONE;
}

void CVotePanel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	PerformLayout();
}

void CVotePanel::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	if (GetBackground())
		GetBackground()->SetPaintBackgroundType(2);

	PerformLayout();
}

void CVotePanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	m_pItemList->SetBorder(NULL);

	for (int i = 0; i <= m_pItemList->GetHighestItemID(); i++)
	{
		if (m_pItemList->GetSelectedItem() != i)
			m_pItemList->SetItemFgColor(i, pScheme->GetColor("SeverListItemSelectedColor", Color(255, 255, 255, 255)));
		else
			m_pItemList->SetItemFgColor(i, pScheme->GetColor("SeverListItemColor", Color(0, 0, 0, 255)));

		m_pItemList->SetItemFont(i, pScheme->GetFont("ServerBrowser"));
	}

	m_pItemList->SetSectionFgColor(0, Color(255, 255, 255, 255));
	m_pItemList->SetSectionDividerColor(0, Color(0, 0, 0, 0));
	m_pItemList->SetBgColor(Color(0, 0, 0, 0));
	m_pItemList->SetFontSection(0, pScheme->GetFont("ServerBrowser"));
	m_pItemList->SetFgColor(Color(0, 0, 0, 0));
}

void CVotePanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ((code == KEY_ESCAPE) || (gameuifuncs->GetButtonCodeForBind("vote_menu") == code))
	{
		if (m_iSelectedOption != VOTE_OPTION_NONE)
		{
			PerformLayout();
			return;
		}

		OnShowPanel(false);
		return;
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CVotePanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);

	if (!g_PR)
		return;

	bool bWantsToPassVote = false;
	if (!Q_stricmp(pcCommand, "Action1"))
		m_iSelectedOption = VOTE_OPTION_KICK;
	else if (!Q_stricmp(pcCommand, "Action2"))
		m_iSelectedOption = VOTE_OPTION_BAN;
	else if (!Q_stricmp(pcCommand, "Action3"))
		m_iSelectedOption = VOTE_OPTION_MAP;
	else if (!Q_stricmp(pcCommand, "Action4"))
		bWantsToPassVote = true;
	else if (!Q_stricmp(pcCommand, "Action5"))
		PerformLayout();

	if (m_iSelectedOption != VOTE_OPTION_NONE)
	{
		for (int i = 0; i < (_ARRAYSIZE(m_pButton) - 2); i++)
		{
			m_pButton[i]->SetVisible(false);
			m_pButton[i]->SetEnabled(false);
		}

		m_pButton[RETURN_BUTTON_INDEX]->SetVisible(true);
		m_pButton[RETURN_BUTTON_INDEX]->SetEnabled(true);

		m_pButton[VOTE_BUTTON_INDEX]->SetVisible(true);
		m_pButton[VOTE_BUTTON_INDEX]->SetEnabled(true);

		m_pItemList->SetVisible(true);
		m_pItemList->SetEnabled(true);

		char pchCommand[80];
		KeyValues *itemData = NULL;

		if (bWantsToPassVote)
		{
			int itemID = m_pItemList->GetSelectedItem();
			if (itemID == -1)
				return;

			itemData = m_pItemList->GetItemData(itemID);
			if (!itemData)
				return;
		}

		if ((m_iSelectedOption == VOTE_OPTION_KICK) || (m_iSelectedOption == VOTE_OPTION_BAN))
		{
			if (itemData)
				Q_snprintf(pchCommand, 80, "player_vote_kickban %i %i\n", itemData->GetInt("userid"), ((m_iSelectedOption == VOTE_OPTION_KICK) ? 0 : 1));
			else
			{
				for (int i = 1; i <= MAX_PLAYERS; i++)
				{
					if (!g_PR->IsConnected(i))
						continue;

					if (i == GetLocalPlayerIndex())
						continue;

					player_info_t info;
					if (!engine->GetPlayerInfo(i, &info))
						continue;

					KeyValues *itemData = new KeyValues("data");
					itemData->SetInt("userid", info.userID);
					itemData->SetString("name", g_PR->GetPlayerName(i));
					m_pItemList->AddItem(0, itemData);
					itemData->deleteThis();
				}
			}
		}
		else if (m_iSelectedOption == VOTE_OPTION_MAP)
		{
			m_pTitle->SetText("#VOTE_MENU_OPTION_MAP");

			if (itemData)
				Q_snprintf(pchCommand, 80, "player_vote_map %s\n", itemData->GetString("name"));
			else
			{
				m_VoteSetupMapCycle.RemoveAll();

				INetworkStringTable *pStringTable = g_pStringTableServerMapCycle;
				if (pStringTable)
				{
					int index = pStringTable->FindStringIndex("ServerMapCycle");
					if (index != ::INVALID_STRING_INDEX)
					{
						int nLength = 0;
						const char *pszMapCycle = (const char *)pStringTable->GetStringUserData(index, &nLength);
						if (pszMapCycle && pszMapCycle[0])
						{
							if (pszMapCycle && nLength)
								V_SplitString(pszMapCycle, "\n", m_VoteSetupMapCycle);
						}
					}
				}

				for (int i = 0; i < m_VoteSetupMapCycle.Count(); i++)
				{
					KeyValues *itemData = new KeyValues("data");
					itemData->SetString("name", m_VoteSetupMapCycle[i]);
					m_pItemList->AddItem(0, itemData);
					itemData->deleteThis();
				}
			}
		}

		switch (m_iSelectedOption)
		{
		case VOTE_OPTION_KICK:
			m_pTitle->SetText("#VOTE_MENU_OPTION_KICK");
			break;

		case VOTE_OPTION_BAN:
			m_pTitle->SetText("#VOTE_MENU_OPTION_BAN");
			break;
		}

		if (bWantsToPassVote)
		{
			engine->ClientCmd_Unrestricted(pchCommand);
			OnShowPanel(false);
		}
	}
}
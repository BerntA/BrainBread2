//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: The base vgui for humans and zombie, this panel will contain the quest, char overview and inventory. However we might add the skill tree here as well.
//
//========================================================================================//

#include "cbase.h"
#include "base_panel.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include "ienginevgui.h"
#include "c_baseplayer.h" 
#include "hud_numericdisplay.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"
#include "InventoryItem.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Shared.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/RichText.h>
#include "IGameUIFuncs.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

void CBaseGamePanel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	LoadControlSettings("resource/ui/basegamepanel.res");
}

CBaseGamePanel::CBaseGamePanel(vgui::VPANEL parent) : BaseClass(NULL, "BaseGamePanel", false, 0.2f)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	SetProportional(true);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetVisible(false);

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetImage("base/background");
	m_pBackground->SetZPos(5);
	m_pBackground->SetShouldScaleImage(true);

	m_pTitleDivider = vgui::SETUP_PANEL(new vgui::Divider(this, "TitleDivider"));
	m_pTitleDivider->SetZPos(10);

	m_pNavigationDivider = vgui::SETUP_PANEL(new vgui::Divider(this, "NavigationDivider"));
	m_pNavigationDivider->SetZPos(10);

	m_pPageTitle = vgui::SETUP_PANEL(new vgui::Label(this, "PageTitle", ""));
	m_pPageTitle->SetZPos(20);
	m_pPageTitle->SetContentAlignment(Label::a_center);

	m_pLabelWarning = vgui::SETUP_PANEL(new vgui::Label(this, "PageTitle", ""));
	m_pLabelWarning->SetZPos(100);
	m_pLabelWarning->SetContentAlignment(Label::a_center);
	m_pLabelWarning->SetVisible(false);

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButton[i] = vgui::SETUP_PANEL(new vgui::Button(this, VarArgs("Button%i", (i + 1)), ""));
		m_pButton[i]->SetPaintBorderEnabled(false);
		m_pButton[i]->SetPaintEnabled(false);
		m_pButton[i]->SetReleasedSound("ui/button_click.wav");
		m_pButton[i]->SetArmedSound("ui/button_over.wav");
		m_pButton[i]->SetZPos(80);
		m_pButton[i]->SetCommand(VarArgs("Page%i", (i + 1)));

		m_pImage[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, VarArgs("Image%i", (i + 1))));
		m_pImage[i]->SetZPos(30);
		m_pImage[i]->SetShouldScaleImage(true);
		m_pImage[i]->SetImage("base/button");

		m_pButtonText[i] = vgui::SETUP_PANEL(new vgui::Label(this, VarArgs("Text%i", (i + 1)), ""));
		m_pButtonText[i]->SetZPos(35);
		m_pButtonText[i]->SetContentAlignment(Label::a_center);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pInventoryItem); i++)
	{
		m_pInventoryItem[i] = vgui::SETUP_PANEL(new vgui::InventoryItem(this, "InventoryItem", 1.5f));
		m_pInventoryItem[i]->AddActionSignalTarget(this);
		m_pInventoryItem[i]->SetZPos(80);
		m_pInventoryItem[i]->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(100));
	}

	m_pQuestDetailList = vgui::SETUP_PANEL(new vgui::QuestDetailPanel(this, "QuestDetailList"));
	m_pQuestDetailList->AddActionSignalTarget(this);
	m_pQuestDetailList->SetZPos(85);
	m_pQuestDetailList->SetVisible(false);
	m_pQuestDetailList->MoveToFront();

	SetScheme("BaseScheme");

	LoadControlSettings("resource/ui/basegamepanel.res");

	PerformLayout();

	InvalidateLayout();

	UpdateLayout();
}

CBaseGamePanel::~CBaseGamePanel()
{
}

void CBaseGamePanel::UpdateLayout(void)
{
	SetSize(ScreenWidth(), ScreenHeight());
	SetPos(0, 0);

	int zx, zy;
	vgui::input()->GetCursorPos(zx, zy);

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		int w, h;
		w = scheme()->GetProportionalScaledValue(80);
		h = scheme()->GetProportionalScaledValue(12);
		m_pButton[i]->SetSize(w, h);
		m_pButton[i]->SetKeyBoardInputEnabled(true);
		m_pButton[i]->SetMouseInputEnabled(true);
		m_pImage[i]->SetSize(w, h);
		m_pButtonText[i]->SetSize(w, h);

		m_pButton[i]->MoveToFront();

		if (m_iCurrentPage != i)
		{
			if (m_pButton[i]->IsWithin(zx, zy))
				m_pImage[i]->SetImage("base/button_active");
			else
				m_pImage[i]->SetImage("base/button");
		}
		else
			m_pImage[i]->SetImage("base/button_active");
	}

	int x, y;
	int w, h, w2, h2;

	m_pBackground->GetSize(w, h);

	x = (ScreenWidth() / 2) - (w / 2);
	y = (ScreenHeight() / 2) - (h / 2);

	m_pBackground->SetPos(x, y);

	m_pTitleDivider->GetSize(w, h);
	m_pTitleDivider->SetPos(x + scheme()->GetProportionalScaledValue(4), y);
	m_pPageTitle->SetPos(x + scheme()->GetProportionalScaledValue(4), y);

	m_pButton[0]->GetSize(w2, h2);

	m_pNavigationDivider->SetPos(x + w2 + scheme()->GetProportionalScaledValue(57), y + h);

	m_pButton[0]->GetSize(w, h);

	// We set the right positions for the nav btns.
	int x2, y2;

	x2 = x - scheme()->GetProportionalScaledValue(5);
	y2 = y + scheme()->GetProportionalScaledValue(5);

	m_pImage[0]->SetPos(x2 + (w / 2), y2 + h + scheme()->GetProportionalScaledValue(5));
	m_pButtonText[0]->SetPos(x2 + (w / 2), y2 + h + scheme()->GetProportionalScaledValue(5));
	m_pButton[0]->SetPos(x2 + (w / 2), y2 + h + scheme()->GetProportionalScaledValue(5));
	m_pButtonText[0]->SetText("#VGUI_BasePanel_InventoryHeaderSmall");

	m_pImage[1]->SetPos(x2 + (w / 2), y2 + (h * 2) + scheme()->GetProportionalScaledValue(8));
	m_pButtonText[1]->SetPos(x2 + (w / 2), y2 + (h * 2) + scheme()->GetProportionalScaledValue(8));
	m_pButton[1]->SetPos(x2 + (w / 2), y2 + (h * 2) + scheme()->GetProportionalScaledValue(8));
	m_pButtonText[1]->SetText("#VGUI_BasePanel_QuestHeaderSmall");

	m_pNavigationDivider->GetPos(x, y);
	m_pNavigationDivider->GetSize(w, h);

	int x_Clamp = 0;
	int y_Clamp = 0;
	for (int i = 0; i < _ARRAYSIZE(m_pInventoryItem); i++)
	{
		if (x_Clamp >= 4)
			x_Clamp = 0;

		if (i < 4)
			y_Clamp = 0;
		else if (i >= 4 && i < 8)
			y_Clamp = 1;
		else
			y_Clamp = 2;

		int xz, yz;

		xz = x + scheme()->GetProportionalScaledValue(30) + w;
		xz += x_Clamp * scheme()->GetProportionalScaledValue(110);

		x_Clamp++;

		yz = y + scheme()->GetProportionalScaledValue(30);
		yz += y_Clamp * scheme()->GetProportionalScaledValue(110);
		m_pInventoryItem[i]->SetMouseInputEnabled(true);
		m_pInventoryItem[i]->SetKeyBoardInputEnabled(true);
		m_pInventoryItem[i]->SetPos(xz, yz);

		if (m_iCurrentPage != 0)
			m_pInventoryItem[i]->SetVisible(false);
	}

	int w3, w4, h3, h4;
	m_pPageTitle->GetSize(w2, h2);
	m_pPageTitle->GetPos(x2, y2);
	m_pNavigationDivider->GetPos(x, y);
	m_pBackground->GetSize(w, h);
	m_pBackground->GetPos(w4, h4);
	m_pNavigationDivider->GetSize(w3, h3);

	m_pQuestDetailList->SetSize((w - (x - w4) - (w3 * 2)), (h3));
	m_pQuestDetailList->SetPos((x + w3), (y2 + h2));

	if (m_iCurrentPage != 1)
		m_pQuestDetailList->SetVisible(false);

	m_pQuestDetailList->GetSize(w, h);
	m_pQuestDetailList->GetPos(x, y);
	m_pLabelWarning->SetPos(x, y + (h / 2) - scheme()->GetProportionalScaledValue(15));
	m_pLabelWarning->SetSize(w, scheme()->GetProportionalScaledValue(30));
}

void CBaseGamePanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pTitleDivider->SetFgColor(pScheme->GetColor("BasePanelTitleDividerColor", Color(47, 47, 47, 255)));
	m_pNavigationDivider->SetFgColor(pScheme->GetColor("BasePanelNavDividerColor", Color(25, 25, 25, 255)));

	m_pTitleDivider->SetBgColor(pScheme->GetColor("BasePanelTitleDividerColor", Color(47, 47, 47, 255)));
	m_pTitleDivider->SetBorder(NULL);

	m_pNavigationDivider->SetBgColor(pScheme->GetColor("BasePanelNavDividerColor", Color(25, 25, 25, 255)));
	m_pNavigationDivider->SetBorder(NULL);

	m_pPageTitle->SetFgColor(pScheme->GetColor("BasePanelTitleTextColor", Color(255, 255, 255, 255)));
	m_pPageTitle->SetFont(pScheme->GetFont("DefaultLargeBold"));

	m_pLabelWarning->SetFgColor(pScheme->GetColor("BasePanelWarningTextColor", Color(255, 255, 255, 255)));
	m_pLabelWarning->SetFont(pScheme->GetFont("DefaultExtraLargeBold"));

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButtonText[i]->SetFgColor(pScheme->GetColor("BasePanelTextColor", Color(255, 255, 255, 255)));
		m_pButtonText[i]->SetFont(pScheme->GetFont("DefaultBold"));
	}
}

void CBaseGamePanel::OnCommand(const char *pcCommand)
{
	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		if (!Q_stricmp(pcCommand, VarArgs("Page%i", (i + 1))))
		{
			m_iCurrentPage = i;
			UpdateLayout();
			SetPage();
			break;
		}
	}

	BaseClass::OnCommand(pcCommand);
}

void CBaseGamePanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ((code == KEY_ESCAPE) || (gameuifuncs->GetButtonCodeForBind("player_tree") == code))
	{
		OnShowPanel(false);
		return;
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CBaseGamePanel::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	// Set the default start page:
	m_iCurrentPage = 0;
	UpdateLayout();
	SetPage();
}

void CBaseGamePanel::SetPage()
{
	m_pLabelWarning->SetVisible(false);
	m_pLabelWarning->MoveToFront();

	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return;

	if (m_iCurrentPage == 0)
	{
		m_pPageTitle->SetText("#VGUI_BasePanel_InventoryHeader");

		if (GameBaseShared()->GetGameInventory().Count() <= 0)
		{
			m_pLabelWarning->SetText("#VGUI_BasePanel_InventoryEmpty");
			m_pLabelWarning->SetVisible(true);
		}

		for (int i = 0; i < _ARRAYSIZE(m_pInventoryItem); i++)
			m_pInventoryItem[i]->SetVisible(false);

		for (int i = 0; i < GameBaseShared()->GetGameInventory().Count(); i++)
		{
			if (i >= _ARRAYSIZE(m_pInventoryItem))
				continue;

			uint iItemID = GameBaseShared()->GetGameInventory()[i].m_iItemID;
			bool bMapItem = GameBaseShared()->GetGameInventory()[i].bIsMapItem;
			KeyValues *pkvData = GameBaseShared()->GetInventoryItemDetails(iItemID, bMapItem);
			if (pkvData)
			{
				KeyValues *pkvItem = GameBaseShared()->GetInventoryItemByID(pkvData, iItemID);
				if (pkvItem)
				{
					m_pInventoryItem[i]->SetItemDetails(pkvItem, bMapItem);
					m_pInventoryItem[i]->SetVisible(true);
				}

				pkvData->deleteThis();
			}
		}
	}
	else if (m_iCurrentPage == 1)
	{
		m_pPageTitle->SetText("#VGUI_BasePanel_QuestHeader");
		if ((pClient->GetTeamNumber() != TEAM_HUMANS) || (HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) || (!GameBaseShared()->GetSharedQuestData()->IsAnyQuestActive()))
		{
			m_pLabelWarning->SetText("#VGUI_BasePanel_NoQuests");
			m_pLabelWarning->SetVisible(true);
			m_pQuestDetailList->SetVisible(false);
		}
		else
		{
			m_pQuestDetailList->SetVisible(true);
			m_pQuestDetailList->SetupLayout();
		}
	}
}

void CBaseGamePanel::OnDropInvItem(KeyValues *data)
{
	char pszDropCommand[80];
	Q_snprintf(pszDropCommand, 80, "bb_inventory_item_drop %u %i\n", (uint)data->GetInt("item"), data->GetInt("mapitem"));
	engine->ClientCmd(pszDropCommand);
	engine->ClientCmd("player_tree 1\n");
}

void CBaseGamePanel::OnSelectQuest(int index)
{
	m_pQuestDetailList->ChooseQuestItem(index);
}
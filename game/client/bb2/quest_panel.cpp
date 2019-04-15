//=========       Copyright © Reperio Studios 2015-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the extra objectives, also known as quests. Completing quests give more XP than completing base objectives.
//
//=============================================================================================//

#include "cbase.h"
#include "quest_panel.h"
#include "hl2mp_gamerules.h"
#include "c_hl2mp_player.h"
#include "GameBase_Shared.h"
#include "IGameUIFuncs.h"

extern IGameUIFuncs* gameuifuncs; // for key binding details

using namespace vgui;

CQuestPanel* g_pQuestPanel = NULL;

CQuestPanel::CQuestPanel(vgui::VPANEL parent) : BaseClass(NULL, "QuestPanel", true, 0.2f, true)
{
	g_pQuestPanel = this;
	m_iSelectedQuestIdx = 0;

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

	m_pPageDivider = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Divider"));
	m_pPageDivider->SetShouldScaleImage(true);
	m_pPageDivider->SetZPos(5);
	m_pPageDivider->SetImage("base/quest/multi_divider");

	m_pPageTitle = vgui::SETUP_PANEL(new vgui::Label(this, "PageTitle", ""));
	m_pPageTitle->SetZPos(20);
	m_pPageTitle->SetContentAlignment(Label::a_center);

	m_pLabelWarning = vgui::SETUP_PANEL(new vgui::Label(this, "PageWarning", ""));
	m_pLabelWarning->SetZPos(100);
	m_pLabelWarning->SetContentAlignment(Label::a_center);
	m_pLabelWarning->SetVisible(false);

	m_pQuestDetailList = vgui::SETUP_PANEL(new vgui::QuestDetailPanel(this, "QuestDetailList"));
	m_pQuestDetailList->AddActionSignalTarget(this);
	m_pQuestDetailList->SetZPos(85);
	m_pQuestDetailList->SetVisible(false);
	m_pQuestDetailList->MoveToFront();

	m_pQuestLists = vgui::SETUP_PANEL(new vgui::QuestList(this, "QuestList"));
	m_pQuestLists->SetZPos(30);
	m_pQuestLists->AddActionSignalTarget(this);

	SetScheme("BaseScheme");
	LoadControlSettings("resource/ui/questpanel.res");

	PerformLayout();
	InvalidateLayout();
	UpdateLayout();

	GetBackground()->SetImage("briefing/briefing_bg");
}

CQuestPanel::~CQuestPanel()
{
	g_pQuestPanel = NULL;
}

void CQuestPanel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	LoadControlSettings("resource/ui/questpanel.res");
}

void CQuestPanel::UpdateLayout(void)
{
	MoveToCenterOfScreen();

	int w, h, x, y;
	GetSize(w, h);
	
	GetBackground()->SetSize(w, h);
	GetBackground()->SetPos(0, 0);

	m_pPageDivider->SetSize(scheme()->GetProportionalScaledValue(181), scheme()->GetProportionalScaledValue(3));
	m_pPageDivider->SetPos(scheme()->GetProportionalScaledValue(26), scheme()->GetProportionalScaledValue(99));

	m_pPageTitle->SetSize(scheme()->GetProportionalScaledValue(181), scheme()->GetProportionalScaledValue(12));
	m_pPageTitle->SetPos(scheme()->GetProportionalScaledValue(26), scheme()->GetProportionalScaledValue(88));

	m_pLabelWarning->SetSize(scheme()->GetProportionalScaledValue(181), scheme()->GetProportionalScaledValue(258));
	m_pLabelWarning->SetPos(scheme()->GetProportionalScaledValue(26), scheme()->GetProportionalScaledValue(88));

	m_pQuestLists->SetSize(scheme()->GetProportionalScaledValue(181), scheme()->GetProportionalScaledValue(245));
	m_pQuestLists->SetPos(scheme()->GetProportionalScaledValue(26), scheme()->GetProportionalScaledValue(101));

	m_pQuestDetailList->SetSize(scheme()->GetProportionalScaledValue(179), scheme()->GetProportionalScaledValue(245));
	m_pQuestDetailList->SetPos(scheme()->GetProportionalScaledValue(228), scheme()->GetProportionalScaledValue(88));
}

void CQuestPanel::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pPageTitle->SetFgColor(pScheme->GetColor("BasePanelTitleTextColor", Color(255, 255, 255, 255)));
	m_pPageTitle->SetFont(pScheme->GetFont("DefaultLargeBold"));

	m_pLabelWarning->SetFgColor(pScheme->GetColor("BasePanelWarningTextColor", Color(255, 255, 255, 255)));
	m_pLabelWarning->SetFont(pScheme->GetFont("DefaultExtraLargeBold"));
}

void CQuestPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ((code == KEY_ESCAPE) || (gameuifuncs->GetButtonCodeForBind("player_tree") == code))
	{
		OnShowPanel(false);
		return;
	}

	BaseClass::OnKeyCodeTyped(code);
}

void CQuestPanel::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

void CQuestPanel::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	UpdateLayout();

	m_pLabelWarning->SetVisible(false);
	m_pLabelWarning->MoveToFront();

	C_HL2MP_Player* pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return;

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

		m_pQuestLists->CreateList();
		OnSelectQuest(m_pQuestLists->GetFirstItem());
	}
}

void CQuestPanel::OnDropInvItem(KeyValues* data)
{
	char pszDropCommand[80];
	Q_snprintf(pszDropCommand, 80, "bb_inventory_item_drop %u %i\n", (uint)data->GetInt("item"), data->GetInt("mapitem"));
	engine->ClientCmd(pszDropCommand);
	engine->ClientCmd("player_tree 1\n");
}

void CQuestPanel::OnSelectQuest(int index)
{	
	if (m_pQuestDetailList->SelectID(index))
	{
		m_iSelectedQuestIdx = index;
		m_pQuestLists->SetActiveIndex(index);
	}		
}

void CQuestPanel::UpdateSelectedQuest(void)
{
	if (IsVisible() == false)
		return;

	m_pQuestLists->CreateList();
	OnSelectQuest(m_iSelectedQuestIdx);
}
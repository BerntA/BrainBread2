//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Human Skill Tree Panel
//
//========================================================================================//

#include "cbase.h"
#include "skill_tree.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/IInput.h>
#include <vgui_controls/ImagePanel.h>
#include "c_hl2mp_player.h"
#include "GameBase_Client.h"
#include "filesystem.h"
#include "IGameUIFuncs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Console Helpers:
extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

#define SKILL_SPECIAL_EXTRA_SIZE 28

CSkillTree::CSkillTree(IViewPort *pViewPort) : Frame(NULL, PANEL_SKILL)
{
	for (int i = 0; i < _ARRAYSIZE(m_pSkillIcon); i++)
		m_pSkillIcon[i] = NULL;

	m_pViewPort = pViewPort;

	SetZPos(10);

	// initialize dialog
	SetTitle("", false);

	// load the new scheme early!!
	SetScheme("BaseScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetCloseButtonVisible(false);
	SetProportional(true);

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetImage("skills/humans/background");
	m_pBackground->SetZPos(20);
	m_pBackground->SetShouldScaleImage(true);

	KeyValues *pkvSkillData = new KeyValues("SkillData");
	if (pkvSkillData->LoadFromFile(filesystem, "scripts/skills/skill_base.txt", "MOD"))
	{
		KeyValues *pkvHumanSkills = pkvSkillData->FindKey("Humans");
		if (pkvHumanSkills)
		{
			int index = 0;
			for (KeyValues *sub = pkvHumanSkills->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				if (index >= _ARRAYSIZE(m_pSkillIcon))
					break;

				m_pSkillIcon[index] = vgui::SETUP_PANEL(new vgui::SkillTreeIcon(this, VarArgs("Icon%i", (index + 1)), sub->GetString("Name"), sub->GetString("Desc"), sub->GetString("Command"), sub->GetString("Texture")));
				m_pSkillIcon[index]->SetZPos(35);
				m_pSkillIcon[index]->AddActionSignalTarget(this);

				index++;
			}
		}
	}
	pkvSkillData->deleteThis();

	m_pToolTip = vgui::SETUP_PANEL(new vgui::Label(this, "toolTip", ""));
	m_pToolTip->SetContentAlignment(Label::a_north);
	m_pToolTip->SetZPos(70);

	for (int i = 0; i < _ARRAYSIZE(m_pSpecialSkill); i++)
	{
		m_pSpecialSkill[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "SpecialBG"));
		m_pSpecialSkill[i]->SetZPos(20);
		m_pSpecialSkill[i]->SetShouldScaleImage(true);
		m_pSpecialSkill[i]->SetImage("skills/shared/prestige_bg");
	}

	m_pLabelTalents = vgui::SETUP_PANEL(new vgui::Label(this, "txtTalents", ""));
	m_pLabelTalents->SetZPos(55);

	LoadControlSettings("resource/ui/skill_tree.res");

	InvalidateLayout();
	PerformLayout();
}

CSkillTree::~CSkillTree()
{
}

void CSkillTree::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < _ARRAYSIZE(m_pSkillIcon); i++)
	{
		if (m_pSkillIcon[i])
			m_pSkillIcon[i]->PerformLayout();
	}
}

void CSkillTree::OnThink()
{
	C_HL2MP_Player *pPlayer = (C_HL2MP_Player *)C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return;

	wchar_t wszArg1[10], wszUnicodeString[128];
	V_swprintf_safe(wszArg1, L"%i", pPlayer->m_BB2Local.m_iSkill_Talents);
	g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#VGUI_SkillPoints"), 1, wszArg1);
	m_pLabelTalents->SetText(wszUnicodeString);

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	bool bFound = false;
	for (int i = 0; i < _ARRAYSIZE(m_pSkillIcon); i++)
	{
		if (m_pSkillIcon[i] == NULL)
			continue;

		m_pSkillIcon[i]->SetProgressValue(((float)pPlayer->GetSkillValue(i)));

		if (m_pSkillIcon[i]->IsWithin(x, y))
		{
			bFound = true;

			wchar_t wszSkillInfo[512];
			g_pVGuiLocalize->ConstructString(wszSkillInfo, sizeof(wszSkillInfo),
				g_pVGuiLocalize->Find("#VGUI_SkillBase"), 2,
				g_pVGuiLocalize->Find(m_pSkillIcon[i]->GetSkillName()),
				g_pVGuiLocalize->Find(m_pSkillIcon[i]->GetSkillDescription())
				);

			m_pToolTip->SetText(wszSkillInfo);
		}
	}

	m_pToolTip->SetVisible(bFound);

	int w, h, wz, hz;
	GetSize(w, h);
	m_pToolTip->SetSize(w - scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(90));
	m_pToolTip->GetSize(wz, hz);
	m_pToolTip->SetPos((w / 2) - (wz / 2), h - scheme()->GetProportionalScaledValue(24));
	m_pToolTip->SetContentAlignment(Label::Alignment::a_north);

	m_pLabelTalents->SetPos(scheme()->GetProportionalScaledValue(10), h - scheme()->GetProportionalScaledValue(46));

	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos(((ww - GetWide()) / 2), ((wt - GetTall()) / 2));

	m_pSkillIcon[0]->GetSize(wz, hz);

	for (int i = 0; i < _ARRAYSIZE(m_pSpecialSkill); i++)
		m_pSpecialSkill[i]->SetSize(wz + scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE), hz + scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE));

	m_pSkillIcon[9]->GetPos(x, y);
	m_pSpecialSkill[0]->SetPos(x - (scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE) / 2), y - (scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE) / 2));

	m_pSkillIcon[19]->GetPos(x, y);
	m_pSpecialSkill[1]->SetPos(x - (scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE) / 2), y - (scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE) / 2));

	m_pSkillIcon[29]->GetPos(x, y);
	m_pSpecialSkill[2]->SetPos(x - (scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE) / 2), y - (scheme()->GetProportionalScaledValue(SKILL_SPECIAL_EXTRA_SIZE) / 2));
}

void CSkillTree::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);
}

void CSkillTree::ShowPanel(bool bShow)
{
	if (bShow && GameBaseClient->IsViewPortPanelVisible(PANEL_SCOREBOARD))
		return;

	PerformLayout();
	SetMouseInputEnabled(bShow);
	SetKeyBoardInputEnabled(bShow);

	if (bShow)
	{
		Activate();
		engine->ClientCmd_Unrestricted("gameui_preventescapetoshow\n");
	}
	else
		engine->ClientCmd_Unrestricted("gameui_allowescapetoshow\n");

	SetVisible(bShow);
	m_pViewPort->ShowBackGround(bShow);
	gViewPortInterface->ShowBackGround(bShow);
}

void CSkillTree::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelTalents->SetFgColor(pScheme->GetColor("SkillTreeTextColor", Color(255, 255, 255, 255)));
	m_pLabelTalents->SetFont(pScheme->GetFont("SkillOtherText"));

	m_pToolTip->SetFgColor(pScheme->GetColor("SkillTreeTextColor", Color(255, 255, 255, 255)));
	m_pToolTip->SetFont(pScheme->GetFont("SkillToolTipText"));
}

void CSkillTree::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

Panel *CSkillTree::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CSkillTree::Reset()
{
}

void CSkillTree::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ((code == KEY_ESCAPE) || (gameuifuncs->GetButtonCodeForBind("skill_tree") == code))
	{
		ShowPanel(false);
		gViewPortInterface->ShowBackGround(false);
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}
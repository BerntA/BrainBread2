//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie Skill Tree Panel
//
//========================================================================================//

#include "cbase.h"
#include <stdio.h>
#include <cdll_client_int.h>
#include "zombie_menu.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <filesystem.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include "vgui_controls/AnimationController.h"
#include <vgui/IInput.h>
#include "vgui_controls/ImagePanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include "c_hl2mp_player.h"
#include "vgui/MouseCode.h"
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "IGameUIFuncs.h"
#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Console Helpers:
extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

void CZombieMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < _ARRAYSIZE(m_pSkillIcon); i++)
	{
		if (m_pSkillIcon[i])
			m_pSkillIcon[i]->PerformLayout();
	}
}

void CZombieMenu::OnThink()
{
	C_HL2MP_Player *pPlayer = (C_HL2MP_Player *)C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return;

	MoveToCenterOfScreen();

	wchar_t wszArg1[10], wszUnicodeString[128];
	V_swprintf_safe(wszArg1, L"%i", pPlayer->m_BB2Local.m_iZombieCredits);
	g_pVGuiLocalize->ConstructString(wszUnicodeString, sizeof(wszUnicodeString), g_pVGuiLocalize->Find("#VGUI_SkillPoints"), 1, wszArg1);
	m_pCreditsInfo->SetText(wszUnicodeString);

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	bool bFound = false;
	for (int i = 0; i < _ARRAYSIZE(m_pSkillIcon); i++)
	{
		if (m_pSkillIcon[i] == NULL)
			continue;

		m_pSkillIcon[i]->SetProgressValue((float)pPlayer->GetSkillValue(i + PLAYER_SKILL_ZOMBIE_HEALTH));

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
	m_pToolTip->SetSize(w - scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(70));
	m_pToolTip->GetSize(wz, hz);
	m_pToolTip->SetPos((w / 2) - (wz / 2), h - scheme()->GetProportionalScaledValue(24));
	m_pToolTip->SetContentAlignment(Label::Alignment::a_north);

	m_pCreditsInfo->SetPos(scheme()->GetProportionalScaledValue(30), h - scheme()->GetProportionalScaledValue(46));
}

CZombieMenu::CZombieMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_ZOMBIE)
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

	// Init Default 
	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pImgBackground->SetImage("skills/zombies/background");
	m_pImgBackground->SetZPos(20);
	m_pImgBackground->SetShouldScaleImage(true);

	m_pCreditsInfo = vgui::SETUP_PANEL(new vgui::Label(this, "infoCredits", ""));
	m_pCreditsInfo->SetZPos(100);

	m_pToolTip = vgui::SETUP_PANEL(new vgui::Label(this, "toolTip", ""));
	m_pToolTip->SetContentAlignment(Label::a_north);
	m_pToolTip->SetZPos(70);

	KeyValues *pkvSkillData = new KeyValues("SkillData");
	if (pkvSkillData->LoadFromFile(filesystem, "scripts/skills/skill_base.txt", "MOD"))
	{
		KeyValues *pkvZombieSkills = pkvSkillData->FindKey("Zombies");
		if (pkvZombieSkills)
		{
			int index = 0;
			for (KeyValues *sub = pkvZombieSkills->GetFirstSubKey(); sub; sub = sub->GetNextKey())
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

	PerformLayout();

	LoadControlSettings("resource/ui/zombiemenu.res");

	InvalidateLayout();
}

CZombieMenu::~CZombieMenu()
{
}

void CZombieMenu::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);
}

void CZombieMenu::ShowPanel(bool bShow)
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

void CZombieMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pCreditsInfo->SetFgColor(pScheme->GetColor("SkillTreeTextColor", Color(255, 255, 255, 255)));
	m_pCreditsInfo->SetFont(pScheme->GetFont("SkillOtherText"));

	m_pToolTip->SetFgColor(pScheme->GetColor("SkillTreeTextColor", Color(255, 255, 255, 255)));
	m_pToolTip->SetFont(pScheme->GetFont("SkillToolTipText"));
}

void CZombieMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

Panel *CZombieMenu::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CZombieMenu::Reset()
{
}

void CZombieMenu::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ((code == KEY_ESCAPE) || (gameuifuncs->GetButtonCodeForBind("zombie_tree") == code))
	{
		PerformLayout();
		ShowPanel(false);
		gViewPortInterface->ShowBackGround(false);
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}
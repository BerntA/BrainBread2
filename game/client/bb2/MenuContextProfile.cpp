//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Profile Menu - Includes the achievement panel and perhaps some model selection editor in the future???
//
//========================================================================================//

#include "cbase.h"
#include "MenuContextProfile.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MenuContextProfile::MenuContextProfile(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.25f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szOptions[] =
	{
		"#GameUI_AchievementMenu",
		"#GameUI_CharacterMenu",
	};

	int m_iCmds[] =
	{
		COMMAND_ACHIEVEMENT_PANEL,
		COMMAND_CHARACTER_PANEL,
	};

	const char *szIcoImg[] =
	{
		"mainmenu/icons/list",
		"server/profile",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
	{
		m_pMenuButton[i] = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "ProfileButtons", m_iCmds[i], szOptions[i], "BB2_PANEL_BIG"));
		m_pMenuButton[i]->SetZPos(200);
		m_pMenuButton[i]->AddActionSignalTarget(this);
		m_pMenuButton[i]->SetIconImage(szIcoImg[i]);
	}

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImgBG"));
	m_pImgBackground->SetShouldScaleImage(true);
	m_pImgBackground->SetImage("mainmenu/selectionlist_background");
	m_pImgBackground->SetZPos(150);

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "LabelTitle", ""));
	m_pLabelTitle->SetZPos(220);
	m_pLabelTitle->SetText("#GameUI_ProfileMenu");

	InvalidateLayout();

	PerformLayout();
}

MenuContextProfile::~MenuContextProfile()
{
}

void MenuContextProfile::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
			m_pMenuButton[i]->OnUpdate();
	}
}

void MenuContextProfile::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelTitle->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
	m_pLabelTitle->SetFont(pScheme->GetFont("BB2_Scoreboard"));
}

void MenuContextProfile::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h;
	GetSize(w, h);

	m_pImgBackground->SetSize(w, h);
	m_pImgBackground->SetPos(0, 0);

	m_pLabelTitle->SetSize(w, scheme()->GetProportionalScaledValue(24));
	m_pLabelTitle->SetPos(0, scheme()->GetProportionalScaledValue(2));
	m_pLabelTitle->SetContentAlignment(Label::a_center);

	for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
	{
		m_pMenuButton[i]->SetSize(w, scheme()->GetProportionalScaledValue(24));
		m_pMenuButton[i]->SetPos(0, scheme()->GetProportionalScaledValue(30) + scheme()->GetProportionalScaledValue(30 * i));
	}
}
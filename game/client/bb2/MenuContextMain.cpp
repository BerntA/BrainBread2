//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Main Menu Context Preview
//
//========================================================================================//

#include "cbase.h"
#include "MenuContextMain.h"
#include <vgui/IInput.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MenuContextMain::MenuContextMain(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.25f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szOptions[] =
	{
		"#GameUI_PlayMenu",
		"#GameUI_ProfileMenu",
		"#GameUI_CreditsMenu",
		"#GameUI_OptionMenu",
		"#GameUI_QuitMenu",
	};

	int m_iCmds[] =
	{
		COMMAND_PLAY,
		COMMAND_PROFILE,
		COMMAND_CREDITS,
		COMMAND_OPTIONS,
		COMMAND_QUITCONFIRM,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
	{
		m_pMenuButton[i] = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "MenuButton", m_iCmds[i], szOptions[i], "MainMenuTextHuge", 0));
		m_pMenuButton[i]->SetSize(scheme()->GetProportionalScaledValue(125), scheme()->GetProportionalScaledValue(60));
		m_pMenuButton[i]->SetPos((scheme()->GetProportionalScaledValue(125) * i), 0);
		m_pMenuButton[i]->SetZPos(200);
		m_pMenuButton[i]->SetVisible(true);
		m_pMenuButton[i]->AddActionSignalTarget(this);
	}

	InvalidateLayout();

	PerformLayout();
}

MenuContextMain::~MenuContextMain()
{
}

void MenuContextMain::SetupLayout(void)
{
	BaseClass::SetupLayout();

	for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
	{
		m_pMenuButton[i]->SetSize(scheme()->GetProportionalScaledValue(115), scheme()->GetProportionalScaledValue(60));
		m_pMenuButton[i]->SetPos((scheme()->GetProportionalScaledValue(125) * i), 0);
	}
}

void MenuContextMain::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
			m_pMenuButton[i]->OnUpdate();

		if (bInGame)
			m_pMenuButton[4]->SetText("#GameUI_DisconnectMenu");
		else
			m_pMenuButton[4]->SetText("#GameUI_QuitMenu");
	}
}

void MenuContextMain::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}
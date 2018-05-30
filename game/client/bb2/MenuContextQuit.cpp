//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Exit/Disconnect Menu - If you're in-game it disconnects you when confirming to exit.
//
//========================================================================================//

#include "cbase.h"
#include "MenuContextQuit.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MenuContextQuit::MenuContextQuit(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szOptions[] =
	{
		"#GameUI_Yes",
		"#GameUI_No",
	};

	int m_iCmds[] =
	{
		COMMAND_QUIT,
		COMMAND_RETURN,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
	{
		m_pMenuButton[i] = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "MenuButton", m_iCmds[i], szOptions[i], "OptionTextMedium", 0));
		m_pMenuButton[i]->SetZPos(200);
		m_pMenuButton[i]->AddActionSignalTarget(this);
	}

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImgBG"));
	m_pImgBackground->SetShouldScaleImage(true);
	m_pImgBackground->SetSize(ScreenWidth(), 512);
	m_pImgBackground->SetImage("mainmenu/backgroundother");
	m_pImgBackground->SetZPos(150);
	m_pImgBackground->AddActionSignalTarget(this);

	m_pLabelMessage = vgui::SETUP_PANEL(new vgui::Label(this, "Message", ""));
	m_pLabelMessage->SetZPos(210);
	m_pLabelMessage->SetContentAlignment(vgui::Label::a_center);
	m_pLabelMessage->SetText("");
	m_pLabelMessage->SetContentAlignment(vgui::Label::a_center);
	m_pLabelMessage->AddActionSignalTarget(this);

	InvalidateLayout();

	PerformLayout();
}

MenuContextQuit::~MenuContextQuit()
{
}

void MenuContextQuit::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
			m_pMenuButton[i]->OnUpdate();

		if (bInGame)
			m_pMenuButton[0]->SetCommandValue(COMMAND_DISCONNECT);
		else
			m_pMenuButton[0]->SetCommandValue(COMMAND_QUIT);
	}
}

void MenuContextQuit::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelMessage->SetFgColor(pScheme->GetColor("QuitMenuTextColor", Color(255, 255, 255, 245)));
	m_pLabelMessage->SetFont(pScheme->GetFont("BB2_Scoreboard"));
}

void MenuContextQuit::SetupLayout(void)
{
	BaseClass::SetupLayout();

	if (!IsVisible())
	{
		const char *randomMessage[] =
		{
			"#GameUI_QuitOrLeave_Message1",
			"#GameUI_QuitOrLeave_Message2",
			"#GameUI_QuitOrLeave_Message3",
			"#GameUI_QuitOrLeave_Message4",
			"#GameUI_QuitOrLeave_Message5",
			"#GameUI_QuitOrLeave_Message6",
			"#GameUI_QuitOrLeave_Message7",
			"#GameUI_QuitOrLeave_Message8",
		};

		const char *message = randomMessage[random->RandomInt(0, (_ARRAYSIZE(randomMessage) - 1))];
		if (!strcmp(message, "#GameUI_QuitOrLeave_Message5"))
		{
			const char *playerName = "unnamed";
			if (steamapicontext && steamapicontext->SteamFriends())
				playerName = steamapicontext->SteamFriends()->GetPersonaName();

			wchar_t messageWithArgs[256];
			wchar_t arg1[32];
			g_pVGuiLocalize->ConvertANSIToUnicode(playerName, arg1, sizeof(arg1));
			g_pVGuiLocalize->ConstructString(messageWithArgs, sizeof(messageWithArgs), g_pVGuiLocalize->Find(message), 1, arg1);
			m_pLabelMessage->SetText(messageWithArgs);
		}
		else
			m_pLabelMessage->SetText(message);
	}

	int w, h, wz, hz;
	GetSize(wz, hz);
	m_pImgBackground->SetSize(wz, hz);

	for (int i = 0; i < _ARRAYSIZE(m_pMenuButton); i++)
		m_pMenuButton[i]->SetSize(scheme()->GetProportionalScaledValue(32), scheme()->GetProportionalScaledValue(32));

	m_pMenuButton[0]->SetPos(((ScreenWidth() / 2) - scheme()->GetProportionalScaledValue(36)), (hz / 2) + scheme()->GetProportionalScaledValue(4));
	m_pMenuButton[1]->SetPos(((ScreenWidth() / 2) + scheme()->GetProportionalScaledValue(4)), (hz / 2) + scheme()->GetProportionalScaledValue(4));

	m_pLabelMessage->SetSize(wz, scheme()->GetProportionalScaledValue(20));
	m_pLabelMessage->GetSize(w, h);
	m_pLabelMessage->SetPos(0, (hz / 2) - h - scheme()->GetProportionalScaledValue(4));
}
//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Keypad VGUI Screen.
//
//========================================================================================//

#include "cbase.h"
#include "KeyPadScreen.h"
#include "vgui/IVGui.h"
#include "vgui/IScheme.h"

using namespace vgui;

KeyPadScreen::KeyPadScreen(vgui::Panel *pParent, const char *pMetaClassName) : CVGuiScreenPanel(pParent, pMetaClassName)
{
	SetScheme("BaseScheme");

	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
	{
		const char *name = VarArgs("Button%i", (i + 1));

		m_pButtonBG[i] = vgui::SETUP_PANEL(new ImagePanel(this, "ButtonImg"));
		m_pButtonBG[i]->SetZPos(5);
		m_pButtonBG[i]->SetShouldScaleImage(true);
		m_pButtonBG[i]->SetImage("keypad/button");

		m_pButtons[i] = vgui::SETUP_PANEL(new Button(this, name, "", this, name));
		m_pButtons[i]->SetPaintBorderEnabled(true);
		m_pButtons[i]->SetPaintEnabled(true);
		m_pButtons[i]->SetZPos(10);
	}
}

KeyPadScreen::~KeyPadScreen()
{
	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
	{
		m_pButtonBG[i]->MarkForDeletion();
		m_pButtons[i]->MarkForDeletion();
	}
}

bool KeyPadScreen::Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData)
{
	if (!CVGuiScreenPanel::Init(pKeyValues, pInitData))
		return false;

	vgui::ivgui()->AddTickSignal(GetVPanel(), 1000);
	return true;
}

void KeyPadScreen::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	Msg("CMD: %s\n", command);

	if (!Q_stricmp(command, "Button1"))
	{
	}
	else if (!Q_stricmp(command, "Button2"))
	{
	}
	else if (!Q_stricmp(command, "Button3"))
	{
	}
	else if (!Q_stricmp(command, "Button4"))
	{
	}
	else if (!Q_stricmp(command, "Button5"))
	{
	}
	else if (!Q_stricmp(command, "Button6"))
	{
	}
	else if (!Q_stricmp(command, "Button7"))
	{
	}
	else if (!Q_stricmp(command, "Button8"))
	{
	}
	else if (!Q_stricmp(command, "Button9"))
	{
	}
	else if (!Q_stricmp(command, "Button10")) // Reset
	{
	}
	else if (!Q_stricmp(command, "Button11")) // 0
	{
	}
	else if (!Q_stricmp(command, "Button12")) // Try
	{
	}
}

void KeyPadScreen::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
	{
		m_pButtons[i]->SetFont(pScheme->GetFont("Default"));
		m_pButtons[i]->SetFgColor(Color(0, 0, 0, 255));
	}
}

void KeyPadScreen::OnTick()
{
	CVGuiScreenPanel::OnTick();
}

void KeyPadScreen::PerformLayout()
{
	BaseClass::PerformLayout();

	const char *textInfo[] =
	{
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"X",
		"0",
		"V",
	};

	int initialX = 0, initialY = 80;
	int buttonSize = 20;

	int indexFixed = 0;
	for (int i = 0; i < 4; i++)
	{
		if (indexFixed >= 12)
			break;

		initialX = 240 - buttonSize - (buttonSize / 2);
		for (int x = 0; x < 3; x++)
		{
			m_pButtons[indexFixed]->SetSize(buttonSize, buttonSize);
			m_pButtons[indexFixed]->SetPos(initialX, initialY);
			m_pButtons[indexFixed]->SetText(textInfo[indexFixed]);

			m_pButtonBG[indexFixed]->SetSize(buttonSize, buttonSize);
			m_pButtonBG[indexFixed]->SetPos(initialX, initialY);

			initialX += buttonSize;
			indexFixed++;
		}

		initialY += buttonSize;
	}
}

DECLARE_VGUI_SCREEN_FACTORY(KeyPadScreen, "KeyPadScreen");
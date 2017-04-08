//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Keypad VGUI Screen.
//
//========================================================================================//

#include "cbase.h"
#include "KeyPadScreen.h"
#include "vgui/IVGui.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"

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
		m_pButtons[i]->SetPaintBorderEnabled(false);
		m_pButtons[i]->SetPaintBackgroundEnabled(false);
		m_pButtons[i]->SetPaintEnabled(true);
		m_pButtons[i]->SetZPos(10);
	}

	m_pText = vgui::SETUP_PANEL(new vgui::Label(this, "TextInfo", ""));
	m_pText->SetContentAlignment(Label::Alignment::a_center);
	m_pText->SetZPos(15);

	Q_strncpy(szTempCode, "", sizeof(szTempCode));
}

KeyPadScreen::~KeyPadScreen()
{
	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
	{
		m_pButtonBG[i]->MarkForDeletion();
		m_pButtons[i]->MarkForDeletion();
	}

	m_pText->MarkForDeletion();
}

bool KeyPadScreen::Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData)
{
	if (!CVGuiScreenPanel::Init(pKeyValues, pInitData))
		return false;

	return true;
}

void KeyPadScreen::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	vgui::surface()->PlaySound("buttons/button24.wav");

	int length = strlen(szTempCode);
	if (length < 16)
	{
		if (!Q_stricmp(command, "Button1"))
			szTempCode[length] += '1';
		else if (!Q_stricmp(command, "Button2"))
			szTempCode[length] += '2';
		else if (!Q_stricmp(command, "Button3"))
			szTempCode[length] += '3';
		else if (!Q_stricmp(command, "Button4"))
			szTempCode[length] += '4';
		else if (!Q_stricmp(command, "Button5"))
			szTempCode[length] += '5';
		else if (!Q_stricmp(command, "Button6"))
			szTempCode[length] += '6';
		else if (!Q_stricmp(command, "Button7"))
			szTempCode[length] += '7';
		else if (!Q_stricmp(command, "Button8"))
			szTempCode[length] += '8';
		else if (!Q_stricmp(command, "Button9"))
			szTempCode[length] += '9';
		else if (!Q_stricmp(command, "Button11")) // 0
			szTempCode[length] += '0';
	}

	if (!Q_stricmp(command, "Button10")) // Reset
		Q_strncpy(szTempCode, "", sizeof(szTempCode));
	else if (!Q_stricmp(command, "Button12")) // Try
	{
		C_BaseEntity *pEnt = GetEntity();
		if (!pEnt || (length <= 0))
			return;

		engine->ClientCmd(VarArgs("bb2_keypad_unlock %i %s\n", pEnt->entindex(), szTempCode));
		Q_strncpy(szTempCode, "", sizeof(szTempCode));
	}

	m_pText->SetText(szTempCode);
}

void KeyPadScreen::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pButtons); i++)
		m_pButtons[i]->SetFont(pScheme->GetFont("Default"));

	m_pText->SetFont(pScheme->GetFont("SkillOtherText"));
}

void KeyPadScreen::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize(w, h);

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

	int initialY = 21;
	int buttonSize = 20;

	int indexFixed = 0;
	for (int i = 0; i < 4; i++)
	{
		if (indexFixed >= 12)
			break;

		int initialX = 0;
		for (int x = 0; x < 3; x++)
		{
			m_pButtons[indexFixed]->SetSize(buttonSize, buttonSize);
			m_pButtons[indexFixed]->SetPos(initialX, initialY);
			m_pButtons[indexFixed]->SetText(textInfo[indexFixed]);
			m_pButtons[indexFixed]->SetDefaultColor(Color(0, 0, 0, 255), Color(0, 0, 0, 0));
			m_pButtons[indexFixed]->SetDepressedColor(Color(0, 0, 0, 255), Color(0, 0, 0, 0));

			m_pButtonBG[indexFixed]->SetSize(buttonSize, buttonSize);
			m_pButtonBG[indexFixed]->SetPos(initialX, initialY);

			initialX += buttonSize;
			indexFixed++;
		}

		initialY += buttonSize;
	}

	m_pText->SetFgColor(Color(0, 0, 0, 255));
	m_pText->SetBgColor(Color(30, 27, 25, 200));

	m_pText->SetSize(w, buttonSize);
	m_pText->SetPos(0, 0);
}

DECLARE_VGUI_SCREEN_FACTORY(KeyPadScreen, "KeyPadScreen");
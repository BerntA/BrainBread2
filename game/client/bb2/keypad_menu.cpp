//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: VGUI KeyPad Screen
//
//========================================================================================//

#include "cbase.h"
#include <stdio.h>
#include <cdll_client_int.h>
#include "keypad_menu.h"
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
#include "cdll_util.h"
#include <game/client/iviewport.h>
#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CKeyPadMenu::CKeyPadMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_KEYPAD)
{
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

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pImgBackground->SetImage("keypad/keypadbg");
	m_pImgBackground->SetZPos(20);
	m_pImgBackground->SetShouldScaleImage(true);

	for (int i = 0; i < _ARRAYSIZE(m_pButtonKey); i++)
	{
		m_pButtonKey[i] = vgui::SETUP_PANEL(new vgui::Button(this, VarArgs("btnKey%i", i), ""));
		m_pButtonKey[i]->SetPaintBorderEnabled(false);
		m_pButtonKey[i]->SetPaintEnabled(false);
		m_pButtonKey[i]->SetZPos(100);
		m_pButtonKey[i]->SetReleasedSound("buttons/button24.wav");
		m_pButtonKey[i]->SetCommand(VarArgs("Code%i", i));
	}

	LoadControlSettings("resource/ui/KeyPadMenu.res");

	InvalidateLayout();
	PerformLayout();
}

CKeyPadMenu::~CKeyPadMenu()
{
}

Panel *CKeyPadMenu::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CKeyPadMenu::Reset()
{
}

void CKeyPadMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	Q_strncpy(szTempCode, "", sizeof(szTempCode));
	m_pImgBackground->SetImage("keypad/keypadbg");
}

void CKeyPadMenu::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	for (int i = 0; i <= 9; i++)
	{
		if (!Q_stricmp(command, VarArgs("Code%i", i)))
		{
			m_pImgBackground->SetImage("keypad/keypadbg");

			int length = strlen(szTempCode);
			if (length > 3)
				continue;

			unsigned short num = 48 + i; // ASCII 0-9.
			szTempCode[length] += ((char)num);
		}
	}

	if (strlen(szTempCode) >= 4)
	{
		engine->ClientCmd(VarArgs("bb2_keypad_unlock %i %s\n", iEntityIndex, szTempCode));
		m_pImgBackground->SetImage("keypad/keypadfail");
		Q_strncpy(szTempCode, "", sizeof(szTempCode));
	}
}

void CKeyPadMenu::ShowPanel(bool bShow)
{
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

void CKeyPadMenu::SetData(KeyValues *data)
{
	iEntityIndex = data->GetInt("entity");
	MoveToCenterOfScreen();
}

void CKeyPadMenu::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == KEY_ESCAPE)
	{
		PerformLayout();
		ShowPanel(false);
		gViewPortInterface->ShowBackGround(false);
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}

void CKeyPadMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void CKeyPadMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}
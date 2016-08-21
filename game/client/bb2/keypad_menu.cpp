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

void CKeyPadMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pImgBackground->SetImage("keypad/keypadbg");
}

bool CKeyPadMenu::IsKeyPadCodeCorrect(const char *szCode)
{
	if (!strcmp(szCode, szKeyPadCode))
		return true;

	return false;
}

void CKeyPadMenu::OnThink()
{
	if (strlen(szTempCode) >= 4)
	{
		if (IsKeyPadCodeCorrect(szTempCode))
		{
			vgui::surface()->PlaySound("buttons/button14.wav");
			engine->ClientCmd_Unrestricted(VarArgs("bb2_keypad_unlock_output %i\n", iEntityIndex));
			m_pImgBackground->SetImage("keypad/keypadcorrect");
		}
		else
		{
			vgui::surface()->PlaySound("buttons/button10.wav");
			m_pImgBackground->SetImage("keypad/keypadfail");
		}

		Q_strncpy(szTempCode, "", sizeof(szTempCode));
	}

	MoveToCenterOfScreen();
}

void CKeyPadMenu::UpdateKeyPadCode(const char *szCode, int iEntIndex)
{
	Q_strncpy(szKeyPadCode, szCode, 16);
	Q_strncpy(szTempCode, "", sizeof(szTempCode));
	iEntityIndex = iEntIndex;
}

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

	// Init Default 
	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));

	for (int i = 0; i < _ARRAYSIZE(m_pButtonKey); i++)
	{
		m_pButtonKey[i] = vgui::SETUP_PANEL(new vgui::Button(this, VarArgs("btnKey%i", i), ""));
		m_pButtonKey[i]->SetPaintBorderEnabled(false);
		m_pButtonKey[i]->SetPaintEnabled(false);
		m_pButtonKey[i]->SetZPos(100);
		m_pButtonKey[i]->SetReleasedSound("ui/buttonclick.wav");
		m_pButtonKey[i]->SetCommand(VarArgs("Code%i", i));
	}

	m_pImgBackground->SetImage("keypad/keypadbg");
	m_pImgBackground->SetZPos(20);
	m_pImgBackground->SetShouldScaleImage(true);

	PerformLayout();

	LoadControlSettings("resource/ui/KeyPadMenu.res");

	szTempCode[0] = 0;

	InvalidateLayout();
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

void CKeyPadMenu::OnCommand(const char *command)
{
	for (int i = 0; i <= 9; i++)
	{
		if (!Q_stricmp(command, VarArgs("Code%i", i)))
		{
			m_pImgBackground->SetImage("keypad/keypadbg");

			int length = strlen(szTempCode);
			if (length > 3)
				continue;

			const char *szValue = VarArgs("%i", i);
			szTempCode[length] += szValue[0];
		}
	}

	BaseClass::OnCommand(command);
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
	int index = data->GetInt("entity");
	const char *code = data->GetString("code");
	if ((index <= 0) || (strlen(code) <= 0))
		return;

	UpdateKeyPadCode(code, index);
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
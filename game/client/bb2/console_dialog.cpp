//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Console Dialog - We also redirect certain console commands here.
//
//========================================================================================//

#include "cbase.h"
#include "console_dialog.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui/KeyCode.h"
#include "ienginevgui.h"
#include "modes.h"
#include "materialsystem/materialsystem_config.h"
#include "GameBase_Shared.h"
#include "GameBase_Client.h"
#include "vgui_controls/AnimationController.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CGameConsoleDialog::CGameConsoleDialog() : BaseClass(NULL, "GameConsole", false)
{
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/BaseConsole.res", "BaseConsole"));
	SetVisible(false);
	//
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	//
	SetTitle("BrainBread Console", true);
	bShouldClose = false;

	SetSize(ScreenWidth(), 512);
	SetPos(0, -512);

	PerformLayout();

	InvalidateLayout();

	m_flPosY = 0.0f;
	m_flConsoleMoveSpeed = 1.0f;
}

void CGameConsoleDialog::OnCommandSubmitted(const char *pCommand)
{
	const char *szNewCMD = pCommand;
	if (!Q_strnicmp(pCommand, "map ", 4))
	{
		szNewCMD += 4;
		GameBaseClient->RunMap(szNewCMD);
		return;
	}
	else if (!Q_strnicmp(pCommand, "disconnect", 10) || !Q_strnicmp(pCommand, "startupmenu", 11))
	{
		GameBaseClient->RunCommand(COMMAND_DISCONNECT);
		return;
	}
	else if (!Q_strnicmp(pCommand, "connect ", 8))
	{
		if (pCommand && strlen(pCommand) > 8)
		{
			const char *szServerIP = pCommand;
			szServerIP += 8;
			GameBaseShared()->GetSteamServerManager()->DirectConnect(szServerIP);
			return;
		}
	}

	engine->ClientCmd_Unrestricted(pCommand);
}

void CGameConsoleDialog::OnThink()
{
	float flPosition = -GetTall() + (GetTall() * m_flPosY);
	SetPos(0, (int)flPosition);

	if (bShouldClose && (m_flPosY <= 0))
	{
		bShouldClose = false;
		engine->ClientCmd_Unrestricted("gameui_allowescapetoshow\n");
		SetVisible(false);
	}

	BaseClass::OnThink();
}

void CGameConsoleDialog::ToggleConsole(bool bVisible, bool bForceOff)
{
	// Force Console to Close = QUICK CLOSE...
	if (bForceOff)
	{
		SetSize(ScreenWidth(), 512);
		bShouldClose = false;
		m_flPosY = 0.0f;
		SetPos(0, -512);
		engine->ClientCmd_Unrestricted("gameui_allowescapetoshow\n");
		SetVisible(false);
		return;
	}

	SetKeyBoardInputEnabled(bVisible);
	SetMouseInputEnabled(bVisible);
	SetSize(ScreenWidth(), 512);

	InvalidateLayout(false, true);
	vgui::surface()->PlaySound("common/console.wav");

	if (bVisible)
	{
		SetVisible(true);
		Activate();
		RequestFocus();
		bShouldClose = false;
		engine->ClientCmd_Unrestricted("gameui_preventescapetoshow\n");
		GetAnimationController()->RunAnimationCommand(this, "ConsolePosition", 1.0f, 0.0f, m_flConsoleMoveSpeed, AnimationController::INTERPOLATOR_LINEAR);
	}
	else
	{
		bShouldClose = true;
		GetAnimationController()->RunAnimationCommand(this, "ConsolePosition", 0.0f, 0.0f, m_flConsoleMoveSpeed, AnimationController::INTERPOLATOR_LINEAR);
	}
}

void CGameConsoleDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(NULL);
	SetPaintBorderEnabled(false);

	m_flConsoleMoveSpeed = atof(pScheme->GetResourceString("Console.MoveSpeed"));
	if (!m_flConsoleMoveSpeed)
		m_flConsoleMoveSpeed = 1.0f;
}

void CGameConsoleDialog::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == KEY_ESCAPE)
		ToggleConsole(false);
	else
		BaseClass::OnKeyCodeTyped(code);
}

void CGameConsoleDialog::PaintBackground()
{
	SetPaintBackgroundType(0);
	SetPaintBackgroundEnabled(true);
	BaseClass::PaintBackground();
}
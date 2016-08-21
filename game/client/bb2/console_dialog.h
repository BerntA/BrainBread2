//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Console Dialog - We also redirect certain console commands here.
//
//========================================================================================//

#ifndef GAMECONSOLEDIALOG_H
#define GAMECONSOLEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "vgui_controls/consoledialog.h"
#include <Color.h>
#include "vgui_controls/Frame.h"


//-----------------------------------------------------------------------------
// Purpose: Game/dev console dialog
//-----------------------------------------------------------------------------
class CGameConsoleDialog : public vgui::CConsoleDialog
{
	DECLARE_CLASS_SIMPLE(CGameConsoleDialog, vgui::CConsoleDialog);

public:
	CGameConsoleDialog();

	void ToggleConsole(bool bVisible, bool bForceOff = false);
	void OnThink();

private:
	MESSAGE_FUNC_CHARPTR(OnCommandSubmitted, "CommandSubmitted", command);
	CPanelAnimationVar(float, m_flPosY, "ConsolePosition", "0.0f");
	float m_flConsoleMoveSpeed;
	bool bShouldClose;

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void PaintBackground();
};


#endif // GAMECONSOLEDIALOG_H

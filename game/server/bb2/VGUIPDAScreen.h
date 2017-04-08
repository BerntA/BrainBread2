//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: PDA VGUI Screen Server Ent. Displays KeyPad codes.
//
//========================================================================================//

#ifndef VGUI_PDA_SCREEN_H
#define VGUI_PDA_SCREEN_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "vguiscreen.h"
#include "BaseKeyPadEntity.h"

class CVGuiPDAScreen : public CVGuiScreen
{
public:
	DECLARE_CLASS(CVGuiPDAScreen, CVGuiScreen);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CVGuiPDAScreen();

	void Spawn(void);
	void UpdateState(void);

private:
	string_t szKeyPadEntity;
	CNetworkString(m_szKeyPadCode, MAX_KEYCODE_SIZE);
};

#endif // VGUI_PDA_SCREEN_H
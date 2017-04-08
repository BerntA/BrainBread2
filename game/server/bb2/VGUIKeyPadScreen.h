//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: KeyPad VGUI Screen Server Ent.
//
//========================================================================================//

#ifndef VGUI_KEYPAD_SCREEN_H
#define VGUI_KEYPAD_SCREEN_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "vguiscreen.h"
#include "BaseKeyPadEntity.h"

class CVGuiKeyPadScreen : public CVGuiScreen, public CBaseKeyPadEntity
{
public:
	DECLARE_CLASS(CVGuiKeyPadScreen, CVGuiScreen);
	DECLARE_DATADESC();

	CVGuiKeyPadScreen();

	void Spawn(void);
	void UnlockSuccess(CHL2MP_Player *pUnlocker);
	void UnlockFail(CHL2MP_Player *pUnlocker);

private:
	COutputEvent m_OnKeyPadSuccess;
	COutputEvent m_OnKeyPadFail;
};

#endif // VGUI_KEYPAD_SCREEN_H
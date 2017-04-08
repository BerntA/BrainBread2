//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: PDA VGUI Screen Client Ent. Displays KeyPad codes.
//
//========================================================================================//

#ifndef C_VGUIPDA_SCREEN_H
#define C_VGUIPDA_SCREEN_H

#ifdef _WIN32
#pragma once
#endif

#include "c_vguiscreen.h"

class C_VGuiPDAScreen : public C_VGuiScreen
{
public:
	DECLARE_CLASS(C_VGuiPDAScreen, C_VGuiScreen);
	DECLARE_CLIENTCLASS();

	const char *GetKeyPadCode(void) const { return m_szKeyPadCode; }

private:
	char m_szKeyPadCode[16];
};

#endif // C_VGUIPDA_SCREEN_H
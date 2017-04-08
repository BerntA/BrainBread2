//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: KeyPad VGUI Screen Server Ent.
//
//========================================================================================//

#include "cbase.h"
#include "VGUIKeyPadScreen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CVGuiKeyPadScreen)
DEFINE_KEYFIELD(szKeyPadCode, FIELD_STRING, "KeyPadCode"),
DEFINE_KEYFIELD(m_iMaxChars, FIELD_INTEGER, "MaxChars"),
DEFINE_OUTPUT(m_OnKeyPadSuccess, "OnKeyPadSuccess"),
DEFINE_OUTPUT(m_OnKeyPadFail, "OnKeyPadFail"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(vgui_screen_keypad, CVGuiKeyPadScreen);
PRECACHE_REGISTER(vgui_screen_keypad);

CVGuiKeyPadScreen::CVGuiKeyPadScreen() : CVGuiScreen(), CBaseKeyPadEntity()
{
}

void CVGuiKeyPadScreen::Spawn(void)
{
	BaseClass::Spawn();

	if (!IsKeyCodeValid(this))
		SetRandomCode(m_iMaxChars);
}

void CVGuiKeyPadScreen::UnlockSuccess(CHL2MP_Player *pUnlocker)
{
	CBaseKeyPadEntity::UnlockSuccess(pUnlocker);

	m_OnKeyPadSuccess.FireOutput(pUnlocker, this);
}

void CVGuiKeyPadScreen::UnlockFail(CHL2MP_Player *pUnlocker)
{
	CBaseKeyPadEntity::UnlockFail(pUnlocker);

	m_OnKeyPadFail.FireOutput(pUnlocker, this);
}
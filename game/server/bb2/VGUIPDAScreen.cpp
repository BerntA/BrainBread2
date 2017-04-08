//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: PDA VGUI Screen Server Ent. Displays KeyPad codes.
//
//========================================================================================//

#include "cbase.h"
#include "VGUIPDAScreen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CVGuiPDAScreen)
DEFINE_THINKFUNC(UpdateState),
DEFINE_KEYFIELD(szKeyPadEntity, FIELD_STRING, "KeyPadEntity"),
DEFINE_ARRAY(m_szKeyPadCode, FIELD_CHARACTER, MAX_KEYCODE_SIZE),
END_DATADESC()

LINK_ENTITY_TO_CLASS(vgui_screen_pda, CVGuiPDAScreen);
PRECACHE_REGISTER(vgui_screen_pda);

IMPLEMENT_SERVERCLASS_ST(CVGuiPDAScreen, DT_VGuiPDAScreen)
SendPropString(SENDINFO(m_szKeyPadCode)),
END_SEND_TABLE();

CVGuiPDAScreen::CVGuiPDAScreen() : CVGuiScreen()
{
	szKeyPadEntity = NULL_STRING;
	Q_strncpy(m_szKeyPadCode.GetForModify(), "", MAX_KEYCODE_SIZE);
}

void CVGuiPDAScreen::Spawn(void)
{
	BaseClass::Spawn();

	SetThink(&CVGuiPDAScreen::UpdateState);
	SetNextThink(gpGlobals->curtime + 0.5f);
}

void CVGuiPDAScreen::UpdateState(void)
{
	SetThink(NULL);

	const char *code = CBaseKeyPadEntity::GetCodeFromKeyPadEnt(STRING(szKeyPadEntity));
	if (code == NULL)
	{
		Warning("No keypad entity '%s' was found for use with the PDA '%s'!\nRemoving!\n", STRING(szKeyPadEntity), STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	Q_strncpy(m_szKeyPadCode.GetForModify(), code, MAX_KEYCODE_SIZE);

	SetTransparency(true);
}
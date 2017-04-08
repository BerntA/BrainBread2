//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: KeyPad Base Definitions.
//
//========================================================================================//

#ifndef BASE_KEY_PAD_ENT_H
#define BASE_KEY_PAD_ENT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "hl2mp_player.h"

#define MAX_KEYCODE_SIZE 16

class CBaseKeyPadEntity
{
public:
	CBaseKeyPadEntity(int maxChars = 4);
	virtual ~CBaseKeyPadEntity();

	virtual void UnlockSuccess(CHL2MP_Player *pUnlocker);
	virtual void UnlockFail(CHL2MP_Player *pUnlocker);
	virtual void SetKeyPadCode(const char *code);
	virtual void SetRandomCode(int size);
	virtual const char *GetKeyPadCode();
	virtual bool IsKeyCodeValid(CBaseEntity *pChecker);
	virtual bool CanCheckCode(void) { return m_bCanCheckCode; }

	static const char *GetCodeFromKeyPadEnt(const char *name);
	static bool UseKeyPadCode(CHL2MP_Player *pUnlocker, CBaseEntity *pEnt, const char *code);

private:
	bool m_bCanCheckCode;

protected:
	string_t szKeyPadCode;
	unsigned int m_iMaxChars;
};

#endif // BASE_KEY_PAD_ENT_H
//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: KeyPad Base Definitions.
//
//========================================================================================//

#include "cbase.h"
#include "BaseKeyPadEntity.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MIN_ACTIVATION_RANGE 140.0f // hammer units.
#define MAX_CODE_CHARS 8
#define MIN_CODE_CHARS 4

CBaseKeyPadEntity::CBaseKeyPadEntity(int maxChars)
{
	szKeyPadCode = NULL_STRING;
	m_bCanCheckCode = true;
	m_iMaxChars = clamp(maxChars, MIN_CODE_CHARS, MAX_CODE_CHARS);
}

CBaseKeyPadEntity::~CBaseKeyPadEntity()
{
}

void CBaseKeyPadEntity::UnlockSuccess(CHL2MP_Player *pUnlocker)
{
	CSingleUserRecipientFilter filter(pUnlocker);
	pUnlocker->EmitSound(filter, pUnlocker->entindex(), "KeyPad.Unlock");
	m_bCanCheckCode = false;
}

void CBaseKeyPadEntity::UnlockFail(CHL2MP_Player *pUnlocker)
{
	CSingleUserRecipientFilter filter(pUnlocker);
	pUnlocker->EmitSound(filter, pUnlocker->entindex(), "KeyPad.Fail");
}

void CBaseKeyPadEntity::SetKeyPadCode(const char *code)
{
	szKeyPadCode = AllocPooledString(code);
}

void CBaseKeyPadEntity::SetRandomCode(int size)
{
	int newSize = clamp(size, MIN_CODE_CHARS, MAX_CODE_CHARS);
	char pchCode[MAX_KEYCODE_SIZE];
	pchCode[0] = 0;

	for (int i = 0; i < newSize; i++)
	{
		unsigned short num = random->RandomInt(48, 57);
		pchCode[strlen(pchCode)] += ((char)num);
	}

	szKeyPadCode = AllocPooledString(pchCode);
}

const char *CBaseKeyPadEntity::GetKeyPadCode()
{
	return STRING(szKeyPadCode);
}

bool CBaseKeyPadEntity::IsKeyCodeValid(CBaseEntity *pChecker)
{
	char szCodeCheck[MAX_KEYCODE_SIZE];
	Q_strncpy(szCodeCheck, STRING(szKeyPadCode), MAX_KEYCODE_SIZE);

	unsigned int stringLength = strlen(szCodeCheck);
	if (stringLength <= 0)
	{
		Warning("No code set for KeyPad ent '%s'\n", STRING(pChecker->GetEntityName()));
		return false;
	}

	if (stringLength != m_iMaxChars)
	{
		Warning("The code for the KeyPad ent '%s' must be %i digits!\n", STRING(pChecker->GetEntityName()), m_iMaxChars);
		return false;
	}

	for (unsigned int i = 0; i < stringLength; i++)
	{
		if (!(szCodeCheck[i] >= '0' && szCodeCheck[i] <= '9'))
		{
			Warning("Cannot have alphabetical characters in the code for the KeyPad ent '%s'!\n", STRING(pChecker->GetEntityName()));
			return false;
		}
	}

	return true;
}

const char *CBaseKeyPadEntity::GetCodeFromKeyPadEnt(const char *name)
{
	CBaseEntity *pEnt = gEntList.FindEntityByName(NULL, name);
	if (pEnt)
	{
		CBaseKeyPadEntity *pKeyPadBase = dynamic_cast<CBaseKeyPadEntity*> (pEnt);
		if (pKeyPadBase)
			return pKeyPadBase->GetKeyPadCode();
	}

	return NULL;
}

bool CBaseKeyPadEntity::UseKeyPadCode(CHL2MP_Player *pUnlocker, CBaseEntity *pEnt, const char *code)
{
	if (pEnt && pUnlocker)
	{
		if (pUnlocker->GetTeamNumber() != TEAM_HUMANS)
			return false;

		if (pUnlocker->GetAbsOrigin().DistTo(pEnt->GetAbsOrigin()) > MIN_ACTIVATION_RANGE) // If the activator is too far away then we have to ignore.
			return false;

		CBaseKeyPadEntity *pKeyPadBase = dynamic_cast<CBaseKeyPadEntity*> (pEnt);
		if (pKeyPadBase)
		{
			if (!pKeyPadBase->CanCheckCode())
				return false;

			if (!strcmp(pKeyPadBase->GetKeyPadCode(), code))
			{
				pKeyPadBase->UnlockSuccess(pUnlocker);
				return true;
			}

			pKeyPadBase->UnlockFail(pUnlocker);
		}
	}

	return false;
}
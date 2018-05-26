//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Display a tool tip for all the players in-game.
//
//========================================================================================//

#include "cbase.h"
#include "game_tip.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CGameTip)
DEFINE_KEYFIELD(szTip, FIELD_STRING, "Tip"),
DEFINE_KEYFIELD(szKeyBind, FIELD_STRING, "KeyBind"),
DEFINE_KEYFIELD(m_flRadiusEmit, FIELD_FLOAT, "Radius"),
DEFINE_KEYFIELD(m_flTipDuration, FIELD_FLOAT, "Duration"),
DEFINE_KEYFIELD(m_iTipType, FIELD_INTEGER, "Type"),
DEFINE_OUTPUT(m_OnShowTip, "OnShow"),
DEFINE_INPUTFUNC(FIELD_VOID, "ShowTip", InputShowTip),
END_DATADESC()

LINK_ENTITY_TO_CLASS(game_tip, CGameTip);

CGameTip::CGameTip()
{
	szTip = NULL_STRING;
	szKeyBind = NULL_STRING;
	m_flRadiusEmit = 0.0f;
	m_iTipType = 0;
	m_flTipDuration = 7.0f;
}

void CGameTip::Spawn()
{
	BaseClass::Spawn();

	if (szTip == NULL_STRING)
	{
		Warning("game_tip '%s' has no tip!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	if ((m_iTipType == 0) && szKeyBind == NULL_STRING)
	{
		Warning("game_tip '%s' has no key bind!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}
}

void CGameTip::InputShowTip(inputdata_t &data)
{
	const char *keybind = (szKeyBind == NULL_STRING) ? "" : STRING(szKeyBind);
	if (m_flRadiusEmit > 0.0f)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
			if (!pPlayer)
				continue;

			if (pPlayer->IsBot() || (pPlayer->GetAbsOrigin().DistTo(this->GetAbsOrigin()) > m_flRadiusEmit))
				continue;

			GameBaseServer()->SendToolTip(STRING(szTip), keybind, m_flTipDuration, m_iTipType, i);
		}

		return;
	}

	GameBaseServer()->SendToolTip(STRING(szTip), keybind, m_flTipDuration, m_iTipType);
}
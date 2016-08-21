//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Gives an achievement for everyone or the ones who touched the trigger.
//
//========================================================================================//

#include "cbase.h"
#include "trigger_achievement.h"
#include "triggers.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Gives an achievement for everyone or the ones who touched the trigger.
//-----------------------------------------------------------------------------

BEGIN_DATADESC(CTriggerAchievement)

DEFINE_KEYFIELD(szAchievementLink, FIELD_STRING, "Achievement"),
DEFINE_KEYFIELD(m_bGiveToAll, FIELD_BOOLEAN, "TransmitToAll"),
DEFINE_KEYFIELD(m_bRemoveOnTrigger, FIELD_BOOLEAN, "RemoveOnTrigger"),
DEFINE_KEYFIELD(m_iFilter, FIELD_INTEGER, "Filter"),

END_DATADESC()

LINK_ENTITY_TO_CLASS(trigger_achievement, CTriggerAchievement);

CTriggerAchievement::CTriggerAchievement(void)
{
	m_bDisabled = false;
	m_bGiveToAll = false;
	m_bRemoveOnTrigger = false;
	szAchievementLink = NULL_STRING;
}

void CTriggerAchievement::Spawn()
{
	BaseClass::Spawn();

	if (szAchievementLink == NULL_STRING)
	{
		Warning("Trigger Achievement '%s' with invalid achievement link!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
	}
}

void CTriggerAchievement::Touch(CBaseEntity *pOther)
{
	if (m_bDisabled)
		return;

	CHL2MP_Player *pClient = ToHL2MPPlayer(pOther);
	if (!pClient)
		return;

	if (pClient->IsBot())
		return;

	if (m_iFilter > 0)
	{
		if (pClient->GetTeamNumber() != m_iFilter)
			return;
	}

	// Give the achievement to everyone or the first player who touched us:
	GameBaseServer()->SendAchievement(szAchievementLink.ToCStr(), (m_bGiveToAll ? 0 : pClient->entindex()));

	BaseClass::Touch(pOther);

	// Now we're done.
	if (m_bRemoveOnTrigger)
		UTIL_Remove(this);
}
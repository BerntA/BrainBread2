//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: When a certain amount of players enter this volume the server will changelevel to the desired map. (used in story mode)
//
//========================================================================================//

#include "cbase.h"
#include "triggers.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"
#include "hl2mp_gamerules.h"
#include "team.h"

#define CHANGELEVEL_THINK_FREQ 1.0f

class CTriggerChangelevel : public CBaseTrigger
{
public:
	DECLARE_CLASS(CTriggerChangelevel, CBaseTrigger);
	DECLARE_DATADESC();

	CTriggerChangelevel();

	void Spawn();
	void StartTouch(CBaseEntity *pOther);
	void EndTouch(CBaseEntity *pOther);
	void OnTouch(CBaseEntity *pOther);
	void OnThink(void);
	bool IsEnoughPlayersInVolume(void);

private:

	bool m_bChangeLevel;
	float m_flPercentRequired;
	string_t pchNextLevel;
};

LINK_ENTITY_TO_CLASS(trigger_changelevel, CTriggerChangelevel);

BEGIN_DATADESC(CTriggerChangelevel)
DEFINE_KEYFIELD(pchNextLevel, FIELD_STRING, "NextMap"),
DEFINE_KEYFIELD(m_flPercentRequired, FIELD_FLOAT, "PercentRequired"),
DEFINE_THINKFUNC(OnThink),
END_DATADESC()

CTriggerChangelevel::CTriggerChangelevel()
{
	m_flPercentRequired = 60.0f;
	pchNextLevel = NULL_STRING;
	m_bChangeLevel = false;
}

void CTriggerChangelevel::Spawn()
{
	BaseClass::Spawn();

	if (pchNextLevel == NULL_STRING)
	{
		Warning("trigger_changelevel '%s' has no NextMap!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	// Does the next map exist on the server?
	char pszFilePath[80];
	Q_snprintf(pszFilePath, 80, "maps/%s.bsp", STRING(pchNextLevel));
	if (!filesystem->FileExists(pszFilePath, "MOD"))
	{
		Warning("trigger_changelevel '%s' has an invalid NextMap!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	InitTrigger();

	SetTouch(&CTriggerChangelevel::OnTouch);
	SetThink(&CTriggerChangelevel::OnThink);
	SetNextThink(gpGlobals->curtime + CHANGELEVEL_THINK_FREQ);
}

void CTriggerChangelevel::OnThink(void)
{
	if (!m_bDisabled && !m_bChangeLevel)
	{
		if (IsEnoughPlayersInVolume())
		{
			if (GameBaseShared()->GetPlayerLoadoutHandler())
			{
				GameBaseShared()->GetPlayerLoadoutHandler()->SetMapTransit(true);
				GameBaseShared()->GetPlayerLoadoutHandler()->SaveLoadoutData();
			}

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				if (!pPlayer || pPlayer->IsBot())
					continue;

				color32 black = { 0, 0, 0, 0 };
				UTIL_ScreenFade(this, black, 0.5f, 5.0f, FFADE_OUT | FFADE_PURGE | FFADE_STAYOUT);
			}

			m_bChangeLevel = true;
			GameBaseServer()->DoMapChange(STRING(pchNextLevel));
			SetThink(NULL);
			return;
		}
	}

	SetNextThink(gpGlobals->curtime + CHANGELEVEL_THINK_FREQ);
}

bool CTriggerChangelevel::IsEnoughPlayersInVolume(void)
{
	int players = 0;
	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBaseEntity *pToucher = m_hTouchingEntities[i].Get();
		if (!pToucher)
			continue;

		if (pToucher->IsPlayer() && (pToucher->GetTeamNumber() == TEAM_HUMANS))
			players++;
	}

	float teamSize = 0, teamSizeInVolume = (float)players;
	CTeam *pTeam = GetGlobalTeam(TEAM_HUMANS);
	if (pTeam)
		teamSize = (float)pTeam->GetNumPlayers();

	float flPercentInVolume = ((teamSizeInVolume / teamSize) * 100.0f);

	return (flPercentInVolume >= m_flPercentRequired);
}

void CTriggerChangelevel::StartTouch(CBaseEntity *pOther)
{
	if (!pOther || m_bDisabled)
		return;

	BaseClass::StartTouch(pOther);

	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if (pPlayer && pPlayer->IsAlive() && !IsEnoughPlayersInVolume())
	{
		float flRequiredPlayers = 0;
		CTeam *pTeam = GetGlobalTeam(TEAM_HUMANS);
		if (pTeam)
			flRequiredPlayers = (((float)pTeam->GetNumPlayers()) * (m_flPercentRequired / 100.0f));

		if (flRequiredPlayers > 0)
		{
			const char *subMsg = "player";
			if (flRequiredPlayers > 1)
				subMsg = "players";

			char pchArg1[16];
			Q_snprintf(pchArg1, 16, "%i", (int)flRequiredPlayers);
			GameBaseServer()->SendToolTip("#TOOLTIP_CHANGELEVEL_FAIL", "", 3.0f, GAME_TIP_WARNING, pPlayer->entindex(), pchArg1, subMsg);
		}
	}
}

void CTriggerChangelevel::EndTouch(CBaseEntity *pOther)
{
	if (!pOther || m_bDisabled)
		return;

	BaseClass::EndTouch(pOther);
}

void CTriggerChangelevel::OnTouch(CBaseEntity *pOther)
{
}
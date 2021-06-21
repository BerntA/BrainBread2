//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective Logic - Classic styled objective system.
//
//========================================================================================//

#include "cbase.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "logic_objective.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_iObjectiveIndex = 1;

BEGIN_DATADESC(CLogicObjective)
// Shared
DEFINE_KEYFIELD(szObjective, FIELD_STRING, "Objective"),
DEFINE_KEYFIELD(m_flTimeToCompleteObjective, FIELD_FLOAT, "DefaultTime"),
DEFINE_KEYFIELD(m_bShouldFailOnTimerEnd, FIELD_BOOLEAN, "ShouldFail"),
DEFINE_KEYFIELD(m_iScaleType, FIELD_INTEGER, "ScaleType"), // Non-Scaling, Scaling, Non-Time Scaling/Fixed...
DEFINE_KEYFIELD(m_iTeamLink, FIELD_INTEGER, "TeamLink"),
// Objective Icon - Overview Map View
DEFINE_KEYFIELD(szObjectiveIconTexture, FIELD_STRING, "IconTexture"),
DEFINE_KEYFIELD(szObjectiveIconLocation, FIELD_STRING, "IconLocation"),
// Entity Counting
DEFINE_KEYFIELD(szGoalEntity, FIELD_STRING, "TargetEntity"),
DEFINE_KEYFIELD(m_iKillsNeeded, FIELD_INTEGER, "KillsNeeded"),
// Scaling
DEFINE_KEYFIELD(m_flKillScaleFactor, FIELD_FLOAT, "KillScaleFactor"),
DEFINE_KEYFIELD(m_flTimeScaleFactor, FIELD_FLOAT, "TimeScaleFactor"),

DEFINE_KEYFIELD(m_flMaxKillsNeeded, FIELD_FLOAT, "MaxKillsNeeded"),
DEFINE_KEYFIELD(m_flMinKillsNeeded, FIELD_FLOAT, "MinKillsNeeded"),

DEFINE_KEYFIELD(m_flMaxTime, FIELD_FLOAT, "MaxTime"),
DEFINE_KEYFIELD(m_flMinTime, FIELD_FLOAT, "MinTime"),
// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "Start", InputStart),
DEFINE_INPUTFUNC(FIELD_VOID, "End", InputEnd),
DEFINE_INPUTFUNC(FIELD_VOID, "Fail", InputFail),
DEFINE_INPUTFUNC(FIELD_VOID, "Progress", InputProgress),
// Outputs
DEFINE_OUTPUT(m_OnStart, "OnStart"),
DEFINE_OUTPUT(m_OnEnd, "OnEnd"),
DEFINE_OUTPUT(m_OnFail, "OnFail"),
DEFINE_OUTPUT(m_OnTimeOver, "OnTimeOver"),

DEFINE_THINKFUNC(ObjectiveEndTimeDelay),
END_DATADESC()

LINK_ENTITY_TO_CLASS(logic_objective, CLogicObjective);

CLogicObjective::CLogicObjective()
{
	szObjective = NULL_STRING;
	szObjectiveIconTexture = NULL_STRING;
	szObjectiveIconLocation = NULL_STRING;
	szGoalEntity = NULL_STRING;
	m_iKillsNeeded = 0;
	m_iKillsLeft = 0;
	m_flTimeToCompleteObjective = 0.0f;
	m_flTimeStarted = 0.0f;
	m_iStatusOverall = STATUS_WAITING;
	pszObjectiveIcon = NULL;
	m_bShouldEntityCount = false;
	m_bShouldPinToMap = false;
	m_bShouldFailOnTimerEnd = false;
	m_iScaleType = OBJ_SCALING_NONE;
	m_flKillScaleFactor = 0.0f;
	m_flTimeScaleFactor = 0.0f;
	m_iTeamLink = TEAM_HUMANS;

	ListenForGameEvent("round_started");
	ListenForGameEvent("entity_killed");
	ListenForGameEvent("player_connection");

	m_iCurrIndex = g_iObjectiveIndex++;
}

CLogicObjective::~CLogicObjective()
{
	// Remove our obj. icon!
	CBaseEntity *pObjectiveIcon = pszObjectiveIcon.Get();
	if (pObjectiveIcon)
		UTIL_Remove(pObjectiveIcon);

	pszObjectiveIcon = NULL;
}

void CLogicObjective::Spawn()
{
	BaseClass::Spawn();

	if ((HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA))
	{
		Warning("logic_objective '%s' may only be used in the objective or arena gamemode!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	if (szObjective == NULL_STRING)
	{
		Warning("logic_objective '%s' has invalid parameters/empty parameters!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	m_bShouldEntityCount = false;
	if (Q_strstr(STRING(szObjective), "%s1"))
		m_bShouldEntityCount = true;

	if (m_bShouldEntityCount)
	{
		if (szGoalEntity == NULL_STRING || (m_iKillsNeeded <= 0))
		{
			Warning("logic_objective '%s' is missing goal entities to kill or kills to make for entity counting!\nRemoving!\n", STRING(GetEntityName()));
			UTIL_Remove(this);
			return;
		}
	}

	// Enable obj. icon + map point in overview in classic.
	if ((szObjectiveIconLocation != NULL_STRING) && (szObjectiveIconTexture != NULL_STRING))
		m_bShouldPinToMap = true;

	if (m_flTimeToCompleteObjective < 0)
		m_flTimeToCompleteObjective = 0.0f;

	// Scaling Properties : We need to remember the original values!
	m_flOriginalTime = m_flTimeToCompleteObjective;
	m_iOriginalKillCount = m_iKillsNeeded;
}

// We're done now! This is only called when we use a timer saying end in this time:
void CLogicObjective::ObjectiveEndTimeDelay(void)
{
	SendObjectiveParameters(STATUS_SUCCESS, false, m_bShouldFailOnTimerEnd);
	m_OnTimeOver.FireOutput(this, this);
	SetThink(NULL);
}

void CLogicObjective::SendObjectiveParameters(int iStatus, bool bEntCountUpdate, bool bFail)
{
	if ((m_iStatusOverall == STATUS_SUCCESS) || (iStatus == STATUS_FAILED) || ((m_iStatusOverall == STATUS_WAITING) && (iStatus == STATUS_SUCCESS)))
		return;

	if ((iStatus != m_iStatusOverall) && !bEntCountUpdate)
	{
		// Initialize:
		if (m_iStatusOverall == STATUS_WAITING)
		{
			// Stop all other active objective logics:
			CLogicObjective *pLogic = (CLogicObjective*)gEntList.FindEntityByClassname(NULL, this->GetClassname());
			while (pLogic)
			{
				if (pLogic->IsActive() && (pLogic != this) && (pLogic->GetTeamLink() == this->GetTeamLink()))
				{
					Warning("Ended objective %s because we're starting objective logic %s\nAlways remember to properly end and start objectives!\n", STRING(pLogic->GetEntityName()), STRING(this->GetEntityName()));
					pLogic->SendObjectiveParameters(STATUS_SUCCESS);
				}

				pLogic = (CLogicObjective*)gEntList.FindEntityByClassname(pLogic, this->GetClassname());
			}

			if (m_bShouldPinToMap && !pszObjectiveIcon)
			{
				CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, STRING(szObjectiveIconLocation));
				if (pEntity)
				{
					CBaseEntity *pObjIconEnt = CreateEntityByName("objective_icon");
					if (pObjIconEnt)
					{
						CObjectiveIcon *pIconCast = dynamic_cast<CObjectiveIcon*> (pObjIconEnt);
						if (pIconCast)
						{
							char szPath[MAX_WEAPON_STRING];
							Q_snprintf(szPath, MAX_WEAPON_STRING, "vgui/hud/objectiveicons/%s", STRING(szObjectiveIconTexture));

							pIconCast->SetTeamLink(GetTeamLink());
							pIconCast->SetAbsOrigin(pEntity->GetAbsOrigin());
							pIconCast->SetAbsAngles(pEntity->GetAbsAngles());
							pIconCast->SetObjectiveIconTexture(szPath, true);
							pIconCast->Spawn();
						}

						pszObjectiveIcon = pObjIconEnt;
					}
				}
			}

			// The time in minutes we must spend before this objective will automatically end.
			if (m_flTimeToCompleteObjective > 0)
			{
				m_flTimeStarted = gpGlobals->curtime;
				m_flTimeToCompleteObjective = gpGlobals->curtime + m_flTimeToCompleteObjective;
				SetThink(&CLogicObjective::ObjectiveEndTimeDelay);
				SetNextThink(m_flTimeToCompleteObjective);
			}

			m_iKillsLeft = m_iKillsNeeded;
		}

		m_iStatusOverall = iStatus;

		bool bFullUpdate = DoObjectiveScaling();

		// Send Event to our clients to fetch the objective details:
		IGameEvent *event = gameeventmanager->CreateEvent("objective_run");
		if (event)
		{
			event->SetInt("index", m_iCurrIndex);
			event->SetInt("team", m_iTeamLink);
			event->SetInt("status", m_iStatusOverall);
			event->SetString("objective", STRING(szObjective));
			event->SetString("icon_texture", STRING(szObjectiveIconTexture));
			event->SetInt("kills_left", m_iKillsLeft);
			event->SetInt("icon_location", (pszObjectiveIcon.Get() ? pszObjectiveIcon.Get()->entindex() : 0));
			event->SetFloat("time", m_flTimeToCompleteObjective);
			event->SetBool("update", bFullUpdate);
			gameeventmanager->FireEvent(event);
		}

		if (iStatus == STATUS_ONGOING)
			m_OnStart.FireOutput(this, this);
		else if (iStatus == STATUS_SUCCESS)
		{
			CBaseEntity *pObjIcon = pszObjectiveIcon.Get();
			if (pObjIcon)
				UTIL_Remove(pObjIcon);

			pszObjectiveIcon = NULL;

			if (!bFail)
			{
				// Give the reward!
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
					if (!pClient || pClient->IsBot() || (pClient->GetTeamNumber() != GetTeamLink()))
						continue;

					float xpToGet = ((float)pClient->m_BB2Local.m_iSkill_XPLeft) * MAX_OBJECTIVE_EXPERIENCE; // Give X % of XP needed as a reward.
					xpToGet = round(xpToGet);

					pClient->CanLevelUp(xpToGet, NULL);
					if (GetTeamLink() == TEAM_DECEASED)
						pClient->m_BB2Local.m_iZombieCredits += 5;
				}

				m_OnEnd.FireOutput(this, this);
			}
			else
				m_OnFail.FireOutput(this, this);

			SetThink(NULL);
		}
	}
	else if (bEntCountUpdate)
	{
		IGameEvent *event = gameeventmanager->CreateEvent("objective_update");
		if (event)
		{
			event->SetInt("index", m_iCurrIndex);
			event->SetInt("kills_left", m_iKillsLeft);
			gameeventmanager->FireEvent(event);
		}
	}
}

void CLogicObjective::InputStart(inputdata_t &data)
{
	SendObjectiveParameters(STATUS_ONGOING);
}

void CLogicObjective::InputEnd(inputdata_t &data)
{
	SendObjectiveParameters(STATUS_SUCCESS);
}

void CLogicObjective::InputFail(inputdata_t &data)
{
	SendObjectiveParameters(STATUS_SUCCESS, false, true);
}

void CLogicObjective::InputProgress(inputdata_t &data)
{
	if (!IsActive() || !m_bShouldEntityCount)
		return;

	ProgressEntityCounting();
}

bool CLogicObjective::DoObjectiveScaling(void)
{
	if (m_iScaleType <= OBJ_SCALING_NONE || (m_iStatusOverall == STATUS_SUCCESS))
		return false;

	float flKillScaleAmount = 0.0f, flTimeScaleAmount = 0.0f;
	float flNumPlayers = (float)GameBaseShared()->GetNumActivePlayers();

	flKillScaleAmount = (flNumPlayers * m_flKillScaleFactor);
	flTimeScaleAmount = (flNumPlayers * m_flTimeScaleFactor);

	// Do we want kill scaling?
	if (m_iOriginalKillCount > 0 && (m_iScaleType <= OBJ_SCALING_FIXED))
	{
		float newKillsRequired = (flKillScaleAmount * ((float)m_iOriginalKillCount / 100.0f)) + (float)m_iOriginalKillCount;
		newKillsRequired = Clamp(newKillsRequired, m_flMinKillsNeeded, m_flMaxKillsNeeded);
		float killsDone = ((float)m_iKillsNeeded - (float)m_iKillsLeft);

		m_iKillsNeeded = (int)newKillsRequired;
		m_iKillsLeft = ((int)newKillsRequired - (int)killsDone);

		if (m_iKillsLeft <= 0)
		{
			m_iKillsLeft = 0;
			SendObjectiveParameters(STATUS_SUCCESS);
		}
	}

	// Add Time Scaling?
	if ((m_iScaleType == OBJ_SCALING_ALL || m_iScaleType == OBJ_SCALING_TIMEONLY) && (m_flOriginalTime > 0))
	{
		SetThink(NULL);

		float timeElapsed = (gpGlobals->curtime - m_flTimeStarted);
		float newTime = (flTimeScaleAmount * (m_flOriginalTime / 100.0f)) + m_flOriginalTime;
		newTime = Clamp(newTime, m_flMinTime, m_flMaxTime);
		newTime -= timeElapsed;

		if (newTime <= 0)
		{
			m_flTimeToCompleteObjective = 0.0f;
			SendObjectiveParameters(STATUS_SUCCESS, false, m_bShouldFailOnTimerEnd);
			m_OnTimeOver.FireOutput(this, this);
		}
		else
		{
			m_flTimeToCompleteObjective = gpGlobals->curtime + newTime;
			SetThink(&CLogicObjective::ObjectiveEndTimeDelay);
			SetNextThink(m_flTimeToCompleteObjective);
		}
	}

	return true;
}

void CLogicObjective::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (!strcmp(type, "round_started"))
	{
		if (HasSpawnFlags(SF_STARTACTIVE))
			SendObjectiveParameters(STATUS_ONGOING);
	}
	else if (!strcmp(type, "player_connection"))
	{
		if (!IsActive())
			return;

		// Try to scale:
		bool bShouldUpdateEveryone = DoObjectiveScaling();
		bool bDisconnected = event->GetBool("state");

		// A new connection: If we're active then send the obj. params to everyone! 
		// We also upd. everyone if we're using scaling...
		if (!bDisconnected || bShouldUpdateEveryone)
		{
			// Send Event to our clients to fetch the objective details:
			IGameEvent *event = gameeventmanager->CreateEvent("objective_run");
			if (event)
			{
				event->SetInt("index", m_iCurrIndex);
				event->SetInt("team", m_iTeamLink);
				event->SetInt("status", m_iStatusOverall);
				event->SetString("objective", STRING(szObjective));
				event->SetString("icon_texture", STRING(szObjectiveIconTexture));
				event->SetInt("kills_left", m_iKillsLeft);
				event->SetInt("icon_location", (pszObjectiveIcon.Get() ? pszObjectiveIcon.Get()->entindex() : 0));
				event->SetFloat("time", m_flTimeToCompleteObjective);
				event->SetBool("update", bShouldUpdateEveryone);
				gameeventmanager->FireEvent(event);
			}
		}
	}
	else if (!strcmp(type, "entity_killed"))
	{
		if (!IsActive() || !m_bShouldEntityCount)
			return;

		const char *goalEnt = STRING(szGoalEntity);
		if (!strcmp(goalEnt, "custom")) // Must be progressed manually @ map.
			return;

		CBaseEntity *pVictim = UTIL_EntityByIndex(event->GetInt("entindex_killed", 0));
		CBaseEntity *pAttacker = UTIL_EntityByIndex(event->GetInt("entindex_attacker", 0));

		if (!pAttacker || !pVictim)
			return;

		bool bVictimIsABoss = (pVictim->IsNPC() && pVictim->MyNPCPointer() && (pVictim->MyNPCPointer()->IsBoss() || pVictim->IsHumanBoss() || pVictim->IsZombieBoss()));
		if (!bVictimIsABoss && pAttacker->IsNPC() && !bb2_allow_npc_to_score.GetBool())
			return;

		if (!pAttacker->IsHuman(true) && (GetTeamLink() == TEAM_HUMANS))
			return;

		if (!pAttacker->IsZombie(true) && (GetTeamLink() == TEAM_DECEASED))
			return;

		if (FClassnameIs(pVictim, goalEnt) || ((pVictim->Classify() == CLASS_ZOMBIE) && !strcmp(goalEnt, "zombies")))
			ProgressEntityCounting();
	}
}

void CLogicObjective::ProgressEntityCounting(void)
{
	m_iKillsLeft--;
	if (m_iKillsLeft < 0)
		m_iKillsLeft = 0;

	SendObjectiveParameters(m_iStatusOverall, true);
	if (m_iKillsLeft <= 0)
		SendObjectiveParameters(STATUS_SUCCESS);
}
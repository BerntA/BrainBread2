//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest system server entity.
//
//========================================================================================//

#include "cbase.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "logic_quest.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum QuestInputProgressType
{
	QUEST_INPUT_PROGRESS_NORMAL = 0,
	QUEST_INPUT_PROGRESS_COUNTING,
};

BEGIN_DATADESC(CLogicQuest)

DEFINE_KEYFIELD(szQuestID, FIELD_STRING, "QuestName"),

DEFINE_OUTPUT(m_OnStart, "OnStart"),
DEFINE_OUTPUT(m_OnQuestFailed, "OnFail"),
DEFINE_OUTPUT(m_OnQuestCompleted, "OnCompleted"),

// Objective Completed / Progressed outputs
DEFINE_OUTPUT(m_OnQuestProgressObjective[0], "OnProgressObjective1"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[1], "OnProgressObjective2"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[2], "OnProgressObjective3"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[3], "OnProgressObjective4"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[4], "OnProgressObjective5"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[5], "OnProgressObjective6"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[6], "OnProgressObjective7"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[7], "OnProgressObjective8"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[8], "OnProgressObjective9"),
DEFINE_OUTPUT(m_OnQuestProgressObjective[9], "OnProgressObjective10"),

DEFINE_INPUTFUNC(FIELD_VOID, "Start", InputStartQuest),
DEFINE_INPUTFUNC(FIELD_COLOR32, "Progress", InputProgressQuest),
DEFINE_INPUTFUNC(FIELD_VOID, "Fail", InputFailQuest),

DEFINE_FIELD(m_iQuestStatusOverall, FIELD_INTEGER),
DEFINE_FIELD(m_iProgressValue, FIELD_INTEGER),

END_DATADESC()

LINK_ENTITY_TO_CLASS(logic_quest, CLogicQuest);

CLogicQuest::CLogicQuest()
{
	m_pQuestData = NULL;
	szQuestID = NULL_STRING;

	m_iProgressValue = 0;
	m_iQuestStatusOverall = STATUS_WAITING;

	ListenForGameEvent("round_started");
	ListenForGameEvent("entity_killed");
	ListenForGameEvent("player_connection");
}

CLogicQuest::~CLogicQuest()
{
	m_pQuestData = NULL;

	// Remove our obj. icons!
	for (int i = (pszObjectiveIcons.Count() - 1); i >= 0; i--)
	{
		CBaseEntity *pObjIcon = pszObjectiveIcons[i].Get();
		if (pObjIcon)
			UTIL_Remove(pObjIcon);
	}

	pszObjectiveIcons.Purge();
}

void CLogicQuest::Spawn()
{
	BaseClass::Spawn();

	if (HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE)
	{
		Warning("logic_quest '%s' may only be used in the objective gamemode!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	if (szQuestID == NULL_STRING)
	{
		Warning("logic_quest '%s' with no quest name link!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	m_pQuestData = GameBaseShared()->GetSharedQuestData()->GetQuestDataForIndex(STRING(szQuestID));
	if (!m_pQuestData)
	{
		Warning("logic_quest '%s' cannot find the desired quest name link!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	// If pin to map is on we verify that the map has the target entities. (example: info_target entities)
	for (int i = 0; i < m_pQuestData->m_iObjectivesCount; i++)
	{
		const char *szLocation = m_pQuestData->pObjectives[i].szLocationTarget;
		if (szLocation && szLocation[0])
		{
			CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, szLocation);
			if (!pEntity)
			{
				Warning("logic_quest '%s' has one or more info_target entities in the location marking which is not valid/doesn't exist!\nEntity will be removed!\n", STRING(GetEntityName()));
				UTIL_Remove(this);
				break;
			}
		}
	}
}

void CLogicQuest::SendQuestParameters(int iObjectiveToProgress, bool bProgress, bool bFail, bool bEntityCountUpdate)
{
	if (m_iQuestStatusOverall >= STATUS_SUCCESS)
		return;

	if (!bProgress && !bFail && !bEntityCountUpdate && (iObjectiveToProgress < 0))
	{
		if (m_iQuestStatusOverall == STATUS_WAITING)
		{
			m_OnStart.FireOutput(this, this);
			m_iQuestStatusOverall = STATUS_ONGOING;

			// Add our obj. icons.
			for (int i = 0; i < GetLinkedQuestData()->m_iObjectivesCount; i++)
			{
				const char *szLocation = GetLinkedQuestData()->pObjectives[i].szLocationTarget;
				if (szLocation && szLocation[0])
				{
					CBaseEntity *pEntity = gEntList.FindEntityByName(NULL, szLocation);
					if (pEntity)
					{
						CObjectiveIcon *pObjIcon = (CObjectiveIcon*)CreateEntityByName("objective_icon");
						if (pObjIcon)
						{
							const char *szTexture = GetLinkedQuestData()->pObjectives[i].szLocationTexture;

							pObjIcon->SetRelatedQuestObjectiveID(i);
							pObjIcon->SetAbsOrigin(pEntity->GetAbsOrigin());
							pObjIcon->SetAbsAngles(pEntity->GetAbsAngles());
							pObjIcon->SetObjectiveIconTexture(szTexture, true);
							pObjIcon->Spawn();

							if (GetLinkedQuestData()->bShowInOrder && (i != m_iProgressValue))
								pObjIcon->HideIcon(true);

							EHANDLE objEntity = pObjIcon;
							pszObjectiveIcons.AddToTail(objEntity);
						}
					}
				}
			}

			// We run our event, send it to every client.
			IGameEvent *event = gameeventmanager->CreateEvent("quest_start");
			if (event)
			{
				event->SetString("name", STRING(szQuestID));
				gameeventmanager->FireEvent(event);
			}
		}
	}

	if (m_iQuestStatusOverall == STATUS_WAITING)
		return;

	if (bFail && (iObjectiveToProgress < 0))
	{
		m_iQuestStatusOverall = STATUS_FAILED;
		m_OnQuestFailed.FireOutput(this, this);

		// Tell our clients about this sorcery:
		IGameEvent *event = gameeventmanager->CreateEvent("quest_end");
		if (event)
		{
			event->SetString("name", STRING(szQuestID));
			event->SetInt("status", m_iQuestStatusOverall);
			gameeventmanager->FireEvent(event);
		}

		return;
	}

	if ((iObjectiveToProgress >= GetLinkedQuestData()->m_iObjectivesCount) || (iObjectiveToProgress < 0) ||
		GetLinkedQuestData()->pObjectives[iObjectiveToProgress].m_bObjectiveCompleted)
		return;

	if (!bFail && bProgress)
	{
		// Make sure we don't go out of 'bounds' when progressing an objective with the quest in order enabled:
		if (GetLinkedQuestData()->bShowInOrder && (iObjectiveToProgress > m_iProgressValue))
		{
			Warning("logic_quest '%s' can't progress to objective %i because the progress is at %i!\nWhen you're using a 'specified/DoInOrder' quest you can only progress the objective from 0 to 1 then 1 to 2, etc...\n", STRING(GetEntityName()), iObjectiveToProgress, m_iProgressValue);
			return;
		}

		IGameEvent *event = gameeventmanager->CreateEvent("quest_progress");
		if (event)
		{
			event->SetString("name", STRING(szQuestID));
			event->SetInt("id", iObjectiveToProgress);
			event->SetInt("kills_current", GetLinkedQuestData()->pObjectives[iObjectiveToProgress].m_iCurrentKills);
			event->SetBool("entity_count", bEntityCountUpdate);
			gameeventmanager->FireEvent(event);
		}

		if (bEntityCountUpdate)
			return;

		m_OnQuestProgressObjective[iObjectiveToProgress].FireOutput(this, this);

		if (!GetLinkedQuestData()->bShowInOrder && GetLinkedQuestData()->pObjectives[iObjectiveToProgress].szLocationTarget && GetLinkedQuestData()->pObjectives[iObjectiveToProgress].szLocationTarget[0])
		{
			for (int i = 0; i < pszObjectiveIcons.Count(); i++)
			{
				CObjectiveIcon *pObjIcon = dynamic_cast<CObjectiveIcon*> (pszObjectiveIcons[i].Get());
				if (pObjIcon && (pObjIcon->GetRelatedQuestObjectiveID() == iObjectiveToProgress))
				{
					pObjIcon->HideIcon(true);
					break;
				}
			}
		}

		// Give the reward!
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
			if (!pClient || pClient->IsBot() || (pClient->GetTeamNumber() != TEAM_HUMANS))
				continue;

			float xpToGet = ((float)pClient->m_BB2Local.m_iSkill_XPLeft) * MAX_QUEST_EXPERIENCE; // Give X % of XP needed as a reward.
			xpToGet = round(xpToGet);

			pClient->CanLevelUp(((int)xpToGet), NULL);
		}

		GetLinkedQuestData()->pObjectives[iObjectiveToProgress].m_bObjectiveCompleted = true;
		m_iProgressValue++;

		if (GetLinkedQuestData()->bShowInOrder && GetLinkedQuestData()->pObjectives[iObjectiveToProgress].szLocationTarget && GetLinkedQuestData()->pObjectives[iObjectiveToProgress].szLocationTarget[0])
		{
			for (int i = 0; i < pszObjectiveIcons.Count(); i++)
			{
				CObjectiveIcon *pObjIcon = dynamic_cast<CObjectiveIcon*> (pszObjectiveIcons[i].Get());
				if (pObjIcon)
					pObjIcon->HideIcon(!(pObjIcon->GetRelatedQuestObjectiveID() == m_iProgressValue));
			}
		}

		if (GetLinkedQuestData()->m_iObjectivesCount <= m_iProgressValue)
		{
			m_OnQuestCompleted.FireOutput(this, this);
			m_iQuestStatusOverall = STATUS_SUCCESS;

			IGameEvent *event = gameeventmanager->CreateEvent("quest_end");
			if (event)
			{
				event->SetString("name", STRING(szQuestID));
				event->SetInt("status", m_iQuestStatusOverall);
				gameeventmanager->FireEvent(event);
			}
		}
	}
}

void CLogicQuest::InputStartQuest(inputdata_t &data)
{
	SendQuestParameters(-1);
}

void CLogicQuest::InputProgressQuest(inputdata_t &data)
{
	// Index:
	// r - Objective ID
	// g - Type
	// b - N/A
	// a - N/A

	color32 prog = data.value.Color32();
	prog.r = clamp(prog.r, 0, (MAX_QUEST_OBJECTIVES - 1));

	if (prog.g <= QUEST_INPUT_PROGRESS_NORMAL)
		SendQuestParameters(prog.r, true);
	else if (CheckCanProgressCountingObjective(prog.r))
		ProgressCountingForObjective(prog.r);
}

void CLogicQuest::InputFailQuest(inputdata_t &data)
{
	SendQuestParameters(-1, false, true, false);
}

bool CLogicQuest::ShouldShowEntityCounting(void)
{
	if (m_iQuestStatusOverall <= STATUS_WAITING)
		return false;

	for (int i = 0; i < GetLinkedQuestData()->m_iObjectivesCount; i++)
	{
		if (CheckCanProgressCountingObjective(i))
			return true;
	}

	return false;
}

bool CLogicQuest::CheckCanProgressCountingObjective(int id)
{
	const char *szSubString = GetLinkedQuestData()->pObjectives[id].szTargetEntityToKill;
	if (szSubString && szSubString[0])
	{
		if (GetLinkedQuestData()->bShowInOrder)
		{
			if (m_iProgressValue != id)
				return false;
		}

		if (!GetLinkedQuestData()->pObjectives[id].m_bObjectiveCompleted)
			return true;
	}

	return false;
}

void CLogicQuest::ProgressCountingForObjective(int id)
{
	GetLinkedQuestData()->pObjectives[id].m_iCurrentKills++;
	SendQuestParameters(id, true, false, true);

	if (GetLinkedQuestData()->pObjectives[id].m_iCurrentKills >= GetLinkedQuestData()->pObjectives[id].m_iKillsNeeded)
		SendQuestParameters(id, true);
}

CQuestItem *CLogicQuest::GetLinkedQuestData(void)
{
	Assert(m_pQuestData != NULL);
	return m_pQuestData;
}

void CLogicQuest::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	bool bIsActive = (m_iQuestStatusOverall > STATUS_WAITING);

	if (!strcmp(type, "round_started"))
	{
		if (HasSpawnFlags(SF_STARTACTIVE))
			SendQuestParameters(-1);
	}

	if (bIsActive)
	{
		if (!strcmp(type, "player_connection"))
		{
			bool bDisconnected = event->GetBool("state");

			// A new connection: Update the quest list, send it again!
			if (!bDisconnected)
			{
				// We run our event, send it to every client. (only the ones who don't got the quest with this quest's id will add it to their list!)
				IGameEvent *event = gameeventmanager->CreateEvent("quest_send");
				if (event)
				{
					event->SetString("name", STRING(szQuestID));
					event->SetInt("status", m_iQuestStatusOverall);

					event->SetInt("objective1kills", GetLinkedQuestData()->pObjectives[0].m_iCurrentKills);
					event->SetBool("objective1completed", GetLinkedQuestData()->pObjectives[0].m_bObjectiveCompleted);

					event->SetInt("objective2kills", GetLinkedQuestData()->pObjectives[1].m_iCurrentKills);
					event->SetBool("objective2completed", GetLinkedQuestData()->pObjectives[1].m_bObjectiveCompleted);

					event->SetInt("objective3kills", GetLinkedQuestData()->pObjectives[2].m_iCurrentKills);
					event->SetBool("objective3completed", GetLinkedQuestData()->pObjectives[2].m_bObjectiveCompleted);

					event->SetInt("objective4kills", GetLinkedQuestData()->pObjectives[3].m_iCurrentKills);
					event->SetBool("objective4completed", GetLinkedQuestData()->pObjectives[3].m_bObjectiveCompleted);

					event->SetInt("objective5kills", GetLinkedQuestData()->pObjectives[4].m_iCurrentKills);
					event->SetBool("objective5completed", GetLinkedQuestData()->pObjectives[4].m_bObjectiveCompleted);

					event->SetInt("objective6kills", GetLinkedQuestData()->pObjectives[5].m_iCurrentKills);
					event->SetBool("objective6completed", GetLinkedQuestData()->pObjectives[5].m_bObjectiveCompleted);

					event->SetInt("objective7kills", GetLinkedQuestData()->pObjectives[6].m_iCurrentKills);
					event->SetBool("objective7completed", GetLinkedQuestData()->pObjectives[6].m_bObjectiveCompleted);

					event->SetInt("objective8kills", GetLinkedQuestData()->pObjectives[7].m_iCurrentKills);
					event->SetBool("objective8completed", GetLinkedQuestData()->pObjectives[7].m_bObjectiveCompleted);

					event->SetInt("objective9kills", GetLinkedQuestData()->pObjectives[8].m_iCurrentKills);
					event->SetBool("objective9completed", GetLinkedQuestData()->pObjectives[8].m_bObjectiveCompleted);

					event->SetInt("objective10kills", GetLinkedQuestData()->pObjectives[9].m_iCurrentKills);
					event->SetBool("objective10completed", GetLinkedQuestData()->pObjectives[9].m_bObjectiveCompleted);

					gameeventmanager->FireEvent(event);
				}
			}
		}
		else if (!strcmp(type, "entity_killed"))
		{
			CBaseEntity *pVictim = UTIL_EntityByIndex(event->GetInt("entindex_killed", 0));
			CBaseEntity *pAttacker = UTIL_EntityByIndex(event->GetInt("entindex_attacker", 0));

			if (!pAttacker || !pVictim || !pAttacker->IsHuman(true))
				return;

			bool bVictimIsABoss = (pVictim->IsNPC() && pVictim->MyNPCPointer() && (pVictim->MyNPCPointer()->IsBoss() || pVictim->IsHumanBoss() || pVictim->IsZombieBoss()));
			if (!bVictimIsABoss && pAttacker->IsNPC() && !bb2_allow_npc_to_score.GetBool())
				return;

			if ((m_iQuestStatusOverall >= STATUS_SUCCESS) || !ShouldShowEntityCounting())
				return;

			for (int i = 0; i < GetLinkedQuestData()->m_iObjectivesCount; i++)
			{
				const char *szSubString = GetLinkedQuestData()->pObjectives[i].szTargetEntityToKill;
				if (szSubString && szSubString[0] && !GetLinkedQuestData()->pObjectives[i].m_bObjectiveCompleted && FClassnameIs(pVictim, szSubString))
					ProgressCountingForObjective(i);
			}
		}
	}
}
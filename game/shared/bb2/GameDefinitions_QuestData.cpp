//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Quest Data Handler!
//
//========================================================================================//

#include "cbase.h"
#include "GameDefinitions_QuestData.h"
#include "GameBase_Shared.h"

CGameDefinitionsQuestData::CGameDefinitionsQuestData()
{
	ListenForGameEvent("round_start");

#ifdef CLIENT_DLL
	ListenForGameEvent("quest_start");
	ListenForGameEvent("quest_progress");
	ListenForGameEvent("quest_end");
	ListenForGameEvent("quest_send");
#endif
}

CGameDefinitionsQuestData::~CGameDefinitionsQuestData()
{
	CleanupQuestData();
}

void CGameDefinitionsQuestData::ParseQuestData(KeyValues *pkvData)
{
	if (pkvData)
	{
		int questItemIndex = 0;
		for (KeyValues *questData = pkvData->GetFirstSubKey(); questData; questData = questData->GetNextKey())
		{
			CQuestItem *questItem = new CQuestItem();

			questItem->m_iQuestIndex = questItemIndex;
			questItem->bIsActive = false;
			Q_strncpy(questItem->szQuestName, questData->GetName(), MAX_MAP_NAME_SAVE);
			Q_strncpy(questItem->szTitle, questData->GetString("Title"), MAX_MAP_NAME_SAVE);
			Q_strncpy(questItem->szHeader, questData->GetString("Header"), MAX_WEAPON_STRING);
			Q_strncpy(questItem->szDescription, questData->GetString("Description"), 256);
			Q_strncpy(questItem->szImage, questData->GetString("Image"), MAX_WEAPON_STRING);

			questItem->bIsSideQuest = (questData->GetInt("SideQuest") >= 1);
			questItem->bShowInOrder = (questData->GetInt("DoInOrder") >= 1);

			// Init default vars for all the objs.
			for (int objs = 0; objs < MAX_QUEST_OBJECTIVES; objs++)
			{
				questItem->pObjectives[objs].m_bObjectiveCompleted = false;
				questItem->pObjectives[objs].m_iCurrentKills = 0;
				questItem->pObjectives[objs].m_iKillsNeeded = 0;

				Q_strncpy(questItem->pObjectives[objs].szObjective, "", 128);
				Q_strncpy(questItem->pObjectives[objs].szLocationTarget, "", 64);
				Q_strncpy(questItem->pObjectives[objs].szLocationTexture, "", MAX_WEAPON_STRING);
				Q_strncpy(questItem->pObjectives[objs].szTargetEntityToKill, "", 64);
			}

			int iObjectiveCount = 0;
			KeyValues *pkvObjectives = questData->FindKey("Objectives");
			if (pkvObjectives)
			{
				for (KeyValues *questObjectiveData = pkvObjectives->GetFirstSubKey(); questObjectiveData; questObjectiveData = questObjectiveData->GetNextKey())
				{
					if ((iObjectiveCount >= 0) && (iObjectiveCount < MAX_QUEST_OBJECTIVES))
					{
						questItem->pObjectives[iObjectiveCount].m_bObjectiveCompleted = false;
						questItem->pObjectives[iObjectiveCount].m_iCurrentKills = 0;
						questItem->pObjectives[iObjectiveCount].m_iKillsNeeded = questObjectiveData->GetInt("KillsNeeded");

						Q_strncpy(questItem->pObjectives[iObjectiveCount].szObjective, questObjectiveData->GetName(), 128);
						Q_strncpy(questItem->pObjectives[iObjectiveCount].szLocationTarget, questObjectiveData->GetString("LocationTarget"), 64);
						Q_strncpy(questItem->pObjectives[iObjectiveCount].szLocationTexture, questObjectiveData->GetString("LocationTexture"), MAX_WEAPON_STRING);
						Q_strncpy(questItem->pObjectives[iObjectiveCount].szTargetEntityToKill, questObjectiveData->GetString("GoalEntity"), 64);

						iObjectiveCount++;
					}
				}
			}

			questItem->m_iQuestStatus = STATUS_WAITING;
			questItem->m_iObjectivesCount = iObjectiveCount;
			m_pQuestData.AddToTail(questItem);

			questItemIndex++;
		}
	}
}

void CGameDefinitionsQuestData::CleanupQuestData(void)
{
	for (int i = (m_pQuestData.Count() - 1); i >= 0; i--)
	{
		delete m_pQuestData[i];
	}

	m_pQuestData.Purge();
}

void CGameDefinitionsQuestData::ResetQuestStates(void)
{
	if (m_pQuestData.Count() <= 0)
		return;

	for (int i = 0; i < m_pQuestData.Count(); i++)
	{
		m_pQuestData[i]->bIsActive = false;
		m_pQuestData[i]->m_iQuestStatus = STATUS_WAITING;

		for (int obj = 0; obj < MAX_QUEST_OBJECTIVES; obj++)
		{
			m_pQuestData[i]->pObjectives[obj].m_bObjectiveCompleted = false;
			m_pQuestData[i]->pObjectives[obj].m_iCurrentKills = 0;
		}
	}
}

bool CGameDefinitionsQuestData::IsAnyQuestActive(void)
{
	for (int i = 0; i < m_pQuestData.Count(); i++)
	{
		if (m_pQuestData[i]->bIsActive)
			return true;
	}

	return false;
}

CQuestItem *CGameDefinitionsQuestData::GetQuestDataForIndex(const char *ID)
{
	if (ID)
	{
		for (int i = 0; i < m_pQuestData.Count(); i++)
		{
			if (!strcmp(ID, m_pQuestData[i]->szQuestName))
				return m_pQuestData[i];
		}
	}

	return NULL;
}

CQuestItem *CGameDefinitionsQuestData::GetQuestDataForIndex(int index)
{
	for (int i = 0; i < m_pQuestData.Count(); i++)
	{
		if (index == i)
			return m_pQuestData[i];
	}

	return NULL;
}

void CGameDefinitionsQuestData::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (!strcmp(type, "round_start"))
		ResetQuestStates();

#ifdef CLIENT_DLL
	if (!strcmp(type, "quest_start"))
	{
		const char *questName = event->GetString("name");
		CQuestItem *questData = GetQuestDataForIndex(questName);
		if (!questData)
			return;

		questData->bIsActive = true;
	}
	else if (!strcmp(type, "quest_progress"))
	{
		const char *questName = event->GetString("name");
		int id = event->GetInt("id");
		int iKillsCurrent = event->GetInt("kills_current");
		bool bUpdateCounter = event->GetBool("entity_count");

		CQuestItem *questData = GetQuestDataForIndex(questName);
		if (!questData)
			return;

		if (questData->m_iQuestStatus <= STATUS_ONGOING)
		{
			if (bUpdateCounter)
			{
				questData->pObjectives[id].m_iCurrentKills = iKillsCurrent;
			}
			else
			{
				questData->pObjectives[id].m_bObjectiveCompleted = true;

				// Refresh GUI here... ?
			}
		}
	}
	else if (!strcmp(type, "quest_end"))
	{
		const char *questName = event->GetString("name");
		int status = event->GetInt("status");

		CQuestItem *questData = GetQuestDataForIndex(questName);
		if (!questData)
			return;

		if (questData->m_iQuestStatus <= STATUS_ONGOING)
			questData->m_iQuestStatus = status;
	}
	else if (!strcmp(type, "quest_send"))
	{
		const char *questName = event->GetString("name");
		int status = event->GetInt("status");

		int killsForObjective1 = event->GetInt("objective1kills");
		bool bCompletedObjective1 = event->GetBool("objective1completed");

		int killsForObjective2 = event->GetInt("objective2kills");
		bool bCompletedObjective2 = event->GetBool("objective2completed");

		int killsForObjective3 = event->GetInt("objective3kills");
		bool bCompletedObjective3 = event->GetBool("objective3completed");

		int killsForObjective4 = event->GetInt("objective4kills");
		bool bCompletedObjective4 = event->GetBool("objective4completed");

		int killsForObjective5 = event->GetInt("objective5kills");
		bool bCompletedObjective5 = event->GetBool("objective5completed");

		int killsForObjective6 = event->GetInt("objective6kills");
		bool bCompletedObjective6 = event->GetBool("objective6completed");

		int killsForObjective7 = event->GetInt("objective7kills");
		bool bCompletedObjective7 = event->GetBool("objective7completed");

		int killsForObjective8 = event->GetInt("objective8kills");
		bool bCompletedObjective8 = event->GetBool("objective8completed");

		int killsForObjective9 = event->GetInt("objective9kills");
		bool bCompletedObjective9 = event->GetBool("objective9completed");

		int killsForObjective10 = event->GetInt("objective10kills");
		bool bCompletedObjective10 = event->GetBool("objective10completed");

		CQuestItem *questData = GetQuestDataForIndex(questName);
		if (!questData)
			return;

		if (questData->bIsActive)
			return;

		questData->bIsActive = true;
		questData->m_iQuestStatus = status;

		questData->pObjectives[0].m_iCurrentKills = killsForObjective1;
		questData->pObjectives[0].m_bObjectiveCompleted = bCompletedObjective1;

		questData->pObjectives[1].m_iCurrentKills = killsForObjective2;
		questData->pObjectives[1].m_bObjectiveCompleted = bCompletedObjective2;

		questData->pObjectives[2].m_iCurrentKills = killsForObjective3;
		questData->pObjectives[2].m_bObjectiveCompleted = bCompletedObjective3;

		questData->pObjectives[3].m_iCurrentKills = killsForObjective4;
		questData->pObjectives[3].m_bObjectiveCompleted = bCompletedObjective4;

		questData->pObjectives[4].m_iCurrentKills = killsForObjective5;
		questData->pObjectives[4].m_bObjectiveCompleted = bCompletedObjective5;

		questData->pObjectives[5].m_iCurrentKills = killsForObjective6;
		questData->pObjectives[5].m_bObjectiveCompleted = bCompletedObjective6;

		questData->pObjectives[6].m_iCurrentKills = killsForObjective7;
		questData->pObjectives[6].m_bObjectiveCompleted = bCompletedObjective7;

		questData->pObjectives[7].m_iCurrentKills = killsForObjective8;
		questData->pObjectives[7].m_bObjectiveCompleted = bCompletedObjective8;

		questData->pObjectives[8].m_iCurrentKills = killsForObjective9;
		questData->pObjectives[8].m_bObjectiveCompleted = bCompletedObjective9;

		questData->pObjectives[9].m_iCurrentKills = killsForObjective10;
		questData->pObjectives[9].m_bObjectiveCompleted = bCompletedObjective10;
	}
#endif
}
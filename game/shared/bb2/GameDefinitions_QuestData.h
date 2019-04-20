//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Quest Data Handler!
//
//========================================================================================//

#ifndef GAME_BASE_QUEST_DATA_SHARED_H
#define GAME_BASE_QUEST_DATA_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "filesystem.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
#define CGameDefinitionsQuestData C_GameDefinitionsQuestData
#endif

#define MAX_QUEST_OBJECTIVES 10
#define MAX_QUEST_EXPERIENCE 0.015f

enum QuestStatus
{
	STATUS_WAITING = 0,
	STATUS_ONGOING,
	STATUS_SUCCESS,
	STATUS_FAILED,
};

struct QuestObjectiveItem_t
{
	char szObjective[128];
	char szLocationTarget[64];
	char szLocationTexture[MAX_WEAPON_STRING];
	char szTargetEntityToKill[64];

	int iKillsNeeded;
	int iCurrentKills;

	bool bObjectiveCompleted;
};

class CQuestItem;
class CQuestItem
{
public:
	char szQuestName[MAX_MAP_NAME_SAVE];
	char szTitle[MAX_MAP_NAME_SAVE];
	char szDescription[256];

	int iQuestIndex;
	int iObjectivesCount;
	int iQuestStatus;

	bool bIsActive;
	bool bShowInOrder;

	QuestObjectiveItem_t pObjectives[MAX_QUEST_OBJECTIVES];
};

class CGameDefinitionsQuestData : public CGameEventListener
{
public:
	CGameDefinitionsQuestData();
	virtual ~CGameDefinitionsQuestData();

	virtual void ParseQuestData(KeyValues *pkvData = NULL);
	virtual void CleanupQuestData(void);
	virtual void ResetQuestStates(void);
	virtual bool IsAnyQuestActive(void);

	virtual CQuestItem *GetQuestDataForIndex(const char *ID);
	virtual CQuestItem *GetQuestDataForIndex(int index);

	virtual CUtlVector<CQuestItem*> &GetQuestList() { return m_pQuestData; }

protected:

	virtual void FireGameEvent(IGameEvent *event);

	CUtlVector<CQuestItem*> m_pQuestData;
};

#endif // GAME_BASE_QUEST_DATA_SHARED_H
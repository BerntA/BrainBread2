//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest system server entity.
//
//========================================================================================//

#ifndef LOGIC_QUEST_H
#define LOGIC_QUEST_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "filesystem.h"
#include "GameEventListener.h"
#include "objective_icon.h"
#include "GameBase_Shared.h"

class CLogicQuest : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_CLASS(CLogicQuest, CLogicalEntity);
	DECLARE_DATADESC();

	//
	// Spawnflags
	//
	enum SpawnFlags
	{
		SF_STARTACTIVE = 0x01,		// Start active
	};

	CLogicQuest();
	virtual ~CLogicQuest();

	virtual void Spawn();

protected:
	CQuestItem *GetLinkedQuestData(void);
	CQuestItem *m_pQuestData;

private:

	void FireGameEvent(IGameEvent *event);
	void SendQuestParameters(int iObjectiveToProgress, bool bProgress = false, bool bFail = false, bool bEntityCountUpdate = false);

	string_t szQuestID;

	void InputStartQuest(inputdata_t &data);
	void InputProgressQuest(inputdata_t &data);
	void InputFailQuest(inputdata_t &data);

	COutputEvent m_OnStart;
	COutputEvent m_OnQuestFailed;
	COutputEvent m_OnQuestProgressObjective[MAX_QUEST_OBJECTIVES];
	COutputEvent m_OnQuestCompleted;

	int m_iProgressValue;
	bool ShouldShowEntityCounting(void);
	bool CheckCanProgressCountingObjective(int id);
	void ProgressCountingForObjective(int id);

	int m_iQuestStatusOverall;

	// A list of objective icons.
	CUtlVector<EHANDLE> pszObjectiveIcons;
};

#endif // LOGIC_QUEST_H
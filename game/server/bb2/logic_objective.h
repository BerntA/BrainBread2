//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective Logic - Classic styled objective system.
//
//========================================================================================//

#ifndef LOGIC_OBJECTIVE_H
#define LOGIC_OBJECTIVE_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "filesystem.h"
#include "GameEventListener.h"
#include "objective_icon.h"
#include "GameBase_Shared.h"

class CLogicObjective : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_CLASS(CLogicObjective, CLogicalEntity);
	DECLARE_DATADESC();

	//
	// Spawnflags
	//
	enum SpawnFlags
	{
		SF_STARTACTIVE = 0x01,		// Start active
	};

	CLogicObjective();
	virtual ~CLogicObjective();
	virtual void Spawn();

	bool IsActive(void) { return (m_iStatusOverall == STATUS_ONGOING); }
	int GetTeamLink(void) { return m_iTeamLink; }
	void SendObjectiveParameters(int iStatus, bool bEntCountUpdate = false, bool bFail = false);

private:

	bool DoObjectiveScaling(void);
	void ObjectiveEndTimeDelay(void);
	void FireGameEvent(IGameEvent *event);
	void ProgressEntityCounting(void);

	// Shared - Globals
	string_t szObjective;
	bool m_bShouldEntityCount;
	bool m_bShouldPinToMap;
	bool m_bShouldFailOnTimerEnd;
	int m_iStatusOverall;
	int m_iScaleType;
	int m_iTeamLink;

	// Timer
	float m_flTimeToCompleteObjective;
	float m_flTimeStarted;

	// Objective Icon - Overview Map View
	EHANDLE pszObjectiveIcon;
	string_t szObjectiveIconTexture;
	string_t szObjectiveIconLocation;

	// Entity Counting
	string_t szGoalEntity;
	int m_iKillsLeft;
	int m_iKillsNeeded;

	// Scaling
	float m_flKillScaleFactor;
	float m_flMaxKillsNeeded;
	float m_flMinKillsNeeded;

	float m_flTimeScaleFactor;
	float m_flMaxTime;
	float m_flMinTime;

	float m_flOriginalTime;
	int m_iOriginalKillCount;

	// Inputs
	void InputStart(inputdata_t &data);
	void InputEnd(inputdata_t &data);
	void InputFail(inputdata_t &data);
	void InputProgress(inputdata_t &data);

	// Outpus
	COutputEvent m_OnStart;
	COutputEvent m_OnEnd;
	COutputEvent m_OnFail;
	COutputEvent m_OnTimeOver;
};

#endif // LOGIC_OBJECTIVE_H
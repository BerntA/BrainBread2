//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: 2 - Record the death of a certain entity X amount of times until firing an output.
//
//========================================================================================//

#ifndef ENTITY_COUNTER_H
#define ENTITY_COUNTER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "GameEventListener.h"

class CEntityCounter : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_CLASS(CEntityCounter, CLogicalEntity);
	DECLARE_DATADESC();

	// Spawnflags
	enum EntityCounterFlags
	{
		SF_STARTACTIVE = 0x01,		// Start active
	};

	CEntityCounter(void);

	void Spawn();
	int m_iGoalKills;
	int m_iKillCounter;
	bool IsActive() { return m_isActive; }

private:

	bool m_isActive;

	COutputEvent m_OutputAllEntitiesDead;
	string_t m_szTargetEntity;

	void ObjectiveKillsThink();
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputGoalKills(inputdata_t &inputdata);
	void InputSetTargetEntity(inputdata_t &inputdata);

	void FireGameEvent(IGameEvent *event);
};

#endif // ENTITY_COUNTER_H
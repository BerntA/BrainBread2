//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: 2 - Record the death of a certain entity X amount of times until firing an output.
//
//========================================================================================//

#include "cbase.h"
#include "bb2_entity_counter.h"
#include "triggers.h"
#include "props.h"
#include "saverestore_utlvector.h"
#include "ai_basenpc.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "hl2mp_player_shared.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CEntityCounter)

DEFINE_KEYFIELD(m_iGoalKills, FIELD_INTEGER, "goal_kills"),
DEFINE_KEYFIELD(m_szTargetEntity, FIELD_STRING, "goal_classname"),
DEFINE_OUTPUT(m_OutputAllEntitiesDead, "OnReachedValue"),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetGoalKills", InputGoalKills),
DEFINE_INPUTFUNC(FIELD_STRING, "SetTargetEntity", InputSetTargetEntity),

END_DATADESC()

LINK_ENTITY_TO_CLASS(entity_counter, CEntityCounter);

CEntityCounter::CEntityCounter(void)
{
	m_iKillCounter = 0;
	m_iGoalKills = 0;
	m_szTargetEntity = NULL_STRING;

	ListenForGameEvent("entity_killed");
	ListenForGameEvent("round_started");
}

void CEntityCounter::Spawn()
{
	BaseClass::Spawn();
}

void CEntityCounter::ObjectiveKillsThink()
{
	if (m_iKillCounter >= m_iGoalKills)
	{
		m_isActive = false;
		m_OutputAllEntitiesDead.FireOutput(this, this);
		m_iKillCounter = 0;
	}
}

void CEntityCounter::FireGameEvent(IGameEvent *event)
{
	const char * type = event->GetName();

	if (!strcmp(type, "entity_killed"))
	{
		CBaseEntity *pVictim = UTIL_EntityByIndex(event->GetInt("entindex_killed", 0));
		CBaseEntity *pAttacker = UTIL_EntityByIndex(event->GetInt("entindex_attacker", 0));

		if (!pAttacker || !m_isActive || !pVictim)
			return;

		bool bVictimIsABoss = (pVictim->IsNPC() && pVictim->MyNPCPointer() && (pVictim->MyNPCPointer()->IsBoss() || pVictim->IsHumanBoss() || pVictim->IsZombieBoss()));
		if (!bVictimIsABoss && pAttacker->IsNPC() && !bb2_allow_npc_to_score.GetBool())
			return;

		if (!pAttacker->IsHuman(true))
			return;

		if (!strcmp(pVictim->GetClassname(), STRING(m_szTargetEntity)))
			m_iKillCounter++;

		ObjectiveKillsThink();
	}
	else if (!strcmp(type, "round_started"))
	{
		if (HasSpawnFlags(SF_STARTACTIVE))
			m_isActive = true;
	}
}

void CEntityCounter::InputEnable(inputdata_t &inputdata)
{
	if (!m_isActive)
		m_isActive = true;
}

void CEntityCounter::InputDisable(inputdata_t &inputdata)
{
	m_isActive = false;
	m_iKillCounter = 0;
}

void CEntityCounter::InputGoalKills(inputdata_t &inputdata)
{
	m_iGoalKills = inputdata.value.Int();
}

void CEntityCounter::InputSetTargetEntity(inputdata_t &inputdata)
{
	m_szTargetEntity = inputdata.value.StringID();
}
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Entity Monitor - Searches for certain entity types and fires an output when the entity no longer exist. (Fires not found if none existed in the first place)...
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Spawnflags
enum EntityMonitorFlags_t
{
	SF_EXCLUDE_MILITARY = 0x01,
	SF_EXCLUDE_BANDITS = 0x02,
	SF_EXCLUDE_ZOMBIES = 0x04,
	SF_EXCLUDE_PLAYERS = 0x08,
	SF_CHECK_PLAYERS_AND_NPC = 0x10,
};

class CLogicEntityMonitor : public CLogicalEntity
{
	DECLARE_CLASS(CLogicEntityMonitor, CLogicalEntity);
	DECLARE_DATADESC();

public:

	CLogicEntityMonitor();
	~CLogicEntityMonitor();

	void Spawn();
	void SetMonitorState(bool value);
	void MonitorEntThink(void);
	int GetTargetEntitiesInWorld(void);

	void MonitorEntity(inputdata_t &inputdata);
	void StopMonitoring(inputdata_t &inputdata);

protected:

	bool m_bIsMonitoring;
	string_t pszEntityClassname;
	COutputEvent m_OnNoneFound;
	COutputEvent m_OnEntityFound;
	COutputEvent m_OnEntityNonExistant;
};

BEGIN_DATADESC(CLogicEntityMonitor)
DEFINE_KEYFIELD(pszEntityClassname, FIELD_STRING, "TargetEntity"),
DEFINE_INPUTFUNC(FIELD_VOID, "StartMonitoring", MonitorEntity),
DEFINE_INPUTFUNC(FIELD_VOID, "StopMonitoring", StopMonitoring),
DEFINE_OUTPUT(m_OnNoneFound, "OnNotFound"),
DEFINE_OUTPUT(m_OnEntityFound, "OnFound"),
DEFINE_OUTPUT(m_OnEntityNonExistant, "OnFinished"),
DEFINE_FIELD(m_bIsMonitoring, FIELD_BOOLEAN),
DEFINE_THINKFUNC(MonitorEntThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(logic_entity_monitor, CLogicEntityMonitor);

CLogicEntityMonitor::CLogicEntityMonitor()
{
	m_bIsMonitoring = false;
	pszEntityClassname = NULL_STRING;
}

CLogicEntityMonitor::~CLogicEntityMonitor()
{
}

void CLogicEntityMonitor::Spawn()
{
	BaseClass::Spawn();

	if (pszEntityClassname == NULL_STRING)
	{
		Warning("logic_entity_monitor '%s' with no ent classname!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}
}

void CLogicEntityMonitor::MonitorEntity(inputdata_t &inputdata)
{
	SetMonitorState(true);
}

void CLogicEntityMonitor::StopMonitoring(inputdata_t &inputdata)
{
	SetMonitorState(false);
}

void CLogicEntityMonitor::SetMonitorState(bool value)
{
	if (m_bIsMonitoring == value)
		return;

	m_bIsMonitoring = value;

	if (m_bIsMonitoring)
	{
		if (GetTargetEntitiesInWorld() <= 0)
		{
			m_OnNoneFound.FireOutput(this, this);
			SetMonitorState(false);
			return;
		}

		m_OnEntityFound.FireOutput(this, this);
		SetThink(&CLogicEntityMonitor::MonitorEntThink);
		SetNextThink(gpGlobals->curtime + 1.0f);
		return;
	}

	SetThink(NULL);
}

void CLogicEntityMonitor::MonitorEntThink(void)
{
	if (GetTargetEntitiesInWorld() <= 0)
	{
		m_OnEntityNonExistant.FireOutput(this, this);
		SetMonitorState(false);
		return;
	}

	SetNextThink(gpGlobals->curtime + 1.0f);
}

int CLogicEntityMonitor::GetTargetEntitiesInWorld(void)
{
	int entCount = 0;
	CBaseEntity *pTargetEntity = gEntList.FindEntityByClassname(NULL, STRING(pszEntityClassname));
	while (pTargetEntity)
	{
		bool bIsProperEnt = true;

		if (HasSpawnFlags(SF_EXCLUDE_MILITARY) && ((pTargetEntity->Classify() == CLASS_COMBINE) || (pTargetEntity->Classify() == CLASS_MILITARY_VEHICLE)))
			bIsProperEnt = false;

		if (HasSpawnFlags(SF_EXCLUDE_BANDITS) && (pTargetEntity->Classify() == CLASS_MILITARY))
			bIsProperEnt = false;

		if (HasSpawnFlags(SF_EXCLUDE_ZOMBIES) && ((pTargetEntity->Classify() == CLASS_ZOMBIE) || (pTargetEntity->Classify() == CLASS_ZOMBIE_BOSS) || (pTargetEntity->Classify() == CLASS_PLAYER_ZOMB)))
			bIsProperEnt = false;

		if (HasSpawnFlags(SF_EXCLUDE_PLAYERS) && ((pTargetEntity->Classify() == CLASS_PLAYER) || (pTargetEntity->Classify() == CLASS_PLAYER_INFECTED) || (pTargetEntity->Classify() == CLASS_PLAYER_ZOMB)))
			bIsProperEnt = false;

		if (HasSpawnFlags(SF_CHECK_PLAYERS_AND_NPC) && !pTargetEntity->MyCombatCharacterPointer())
			bIsProperEnt = false;

		if (bIsProperEnt)
			entCount++;

		pTargetEntity = gEntList.FindEntityByClassname(pTargetEntity, STRING(pszEntityClassname));
	}

	return entCount;
}

class CLogicPlayerMonitor : public CLogicalEntity, public CGameEventListener
{
	DECLARE_CLASS(CLogicPlayerMonitor, CLogicalEntity);
	DECLARE_DATADESC();

public:

	CLogicPlayerMonitor();
	~CLogicPlayerMonitor();

	void SetEnabled(inputdata_t &inputdata);
	void SetPlayerCount(inputdata_t &inputdata);
	void CheckState(inputdata_t &inputdata);
	void CheckPlayers(void);

private:
	int m_iPlayerCount;
	bool m_bIsActive;

protected:

	void FireGameEvent(IGameEvent *event);

	COutputEvent m_OnHigh;
	COutputEvent m_OnLow;
};

BEGIN_DATADESC(CLogicPlayerMonitor)
DEFINE_KEYFIELD(m_bIsActive, FIELD_BOOLEAN, "StartActive"),
DEFINE_KEYFIELD(m_iPlayerCount, FIELD_INTEGER, "PlayerCount"),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetEnabled", SetEnabled),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetCount", SetPlayerCount),
DEFINE_INPUTFUNC(FIELD_VOID, "CheckState", CheckState),
DEFINE_OUTPUT(m_OnHigh, "OnHigh"),
DEFINE_OUTPUT(m_OnLow, "OnLow"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(logic_player_monitor, CLogicPlayerMonitor);

CLogicPlayerMonitor::CLogicPlayerMonitor()
{
	ListenForGameEvent("player_connection");
	m_iPlayerCount = 2;
	m_bIsActive = false;
}

CLogicPlayerMonitor::~CLogicPlayerMonitor()
{
}

void CLogicPlayerMonitor::SetEnabled(inputdata_t &inputdata)
{
	m_bIsActive = (inputdata.value.Int() >= 1);
}

void CLogicPlayerMonitor::SetPlayerCount(inputdata_t &inputdata)
{
	m_iPlayerCount = inputdata.value.Int();
}

void CLogicPlayerMonitor::CheckState(inputdata_t &inputdata)
{
	CheckPlayers();
}

void CLogicPlayerMonitor::CheckPlayers(void)
{
	if (!m_bIsActive)
		return;

	int count = 0;

	// Someone has connected or disconnected, check current state:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pClient = UTIL_PlayerByIndex(i);
		if (!pClient)
			continue;

		if (pClient->IsDisconnecting() || !pClient->IsConnected())
			continue;

		count++;
	}

	if (count >= m_iPlayerCount)
		m_OnHigh.FireOutput(this, this);
	else
		m_OnLow.FireOutput(this, this);
}

void CLogicPlayerMonitor::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (!strcmp(type, "player_connection"))
	{
		CheckPlayers();
	}
}
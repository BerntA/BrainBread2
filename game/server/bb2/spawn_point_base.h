//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Human & Zombie custom spawn point entities which allows the mapper to disable/enable the spawn point as well.
//
//========================================================================================//

#ifndef SPAWN_POINT_BASE_H
#define SPAWN_POINT_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"

class CBaseSpawnPoint : public CPointEntity
{
public:
	DECLARE_CLASS(CBaseSpawnPoint, CPointEntity);
	DECLARE_DATADESC();

	CBaseSpawnPoint(void);
	void Spawn();

	bool IsEnabled() { return m_bIsEnabled; }
	bool IsMaster() { return m_bIsMaster; }

private:

	void InputState(inputdata_t &inputdata);

	COutputEvent m_OnEnabled;
	COutputEvent m_OnDisabled;

	bool m_bIsEnabled;
	bool m_bIsMaster;
};

#endif // SPAWN_POINT_BASE_H
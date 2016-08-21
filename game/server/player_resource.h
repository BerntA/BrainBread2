//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYER_RESOURCE_H
#define PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class CPlayerResource : public CBaseEntity
{
	DECLARE_CLASS(CPlayerResource, CBaseEntity);
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn(void);
	virtual	int	 ObjectCaps(void) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }
	virtual void ResourceThink(void);
	virtual void UpdatePlayerData(void);
	virtual int  UpdateTransmitState(void);

protected:
	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	CNetworkArray(int, m_nGroupID, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iLevel, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iTotalScore, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iTotalDeaths, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iRoundScore, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iRoundDeaths, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iSelectedTeam, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iPing, MAX_PLAYERS + 1);
	CNetworkArray(int, m_bInfected, MAX_PLAYERS + 1);
	CNetworkArray(int, m_bConnected, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iTeam, MAX_PLAYERS + 1);
	CNetworkArray(int, m_bAlive, MAX_PLAYERS + 1);
	CNetworkArray(int, m_bAdmin, MAX_PLAYERS + 1);
	CNetworkArray(int, m_iHealth, MAX_PLAYERS + 1);
	CNetworkArray(Vector, m_vecPosition, MAX_PLAYERS + 1);

	int	m_nUpdateCounter;
};

extern CPlayerResource *g_pPlayerResource;

#endif // PLAYER_RESOURCE_H
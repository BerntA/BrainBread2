//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_PLAYERRESOURCE_H
#define C_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "const.h"
#include "c_baseentity.h"
#include <igameresources.h>

#define PLAYER_UNCONNECTED_NAME	"unconnected"
#define PLAYER_ERROR_NAME		"ERRORNAME"

class C_PlayerResource : public C_BaseEntity, public IGameResources
{
	DECLARE_CLASS(C_PlayerResource, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_PlayerResource();
	virtual			~C_PlayerResource();

public: // IGameResources intreface

	// Team data access 
	virtual int	GetTeamScore(int index);
	virtual const char *GetTeamName(int index);
	virtual const Color&GetTeamColor(int index);

	// Player data access
	virtual bool	IsConnected(int index);
	virtual bool	IsAlive(int index);
	virtual bool	IsAdmin(int index);
	virtual bool	IsFakePlayer(int index);
	virtual bool	IsLocalPlayer(int index);
	virtual bool	IsHLTV(int index);
	virtual bool	IsReplay(int index);
	virtual bool	IsInfected(int index);
	virtual bool    IsGroupIDFlagActive(int index, int flag);

	virtual const char *GetPlayerName(int index);
	virtual int		GetPing(int index);
	virtual int		GetGroupIDFlags(int index);
	virtual int		GetLevel(int index);
	virtual int		GetTotalScore(int index);
	virtual int		GetTotalDeaths(int index);
	virtual int		GetRoundScore(int index);
	virtual int		GetRoundDeaths(int index);
	virtual int		GetSelectedTeam(int index);
	virtual int		GetTeam(int index);
	virtual int		GetHealth(int index);
	virtual Vector  GetPosition(int index);

	virtual void ClientThink();
	virtual	void	OnDataChanged(DataUpdateType_t updateType);

protected:
	void UpdatePlayerName(int slot);

	// Data for each player that's propagated to all clients
	// Stored in individual arrays so they can be sent down via datatables
	string_t m_szName[MAX_PLAYERS + 1];
	int m_nGroupID[MAX_PLAYERS + 1];
	int m_iLevel[MAX_PLAYERS + 1];
	int m_iTotalScore[MAX_PLAYERS + 1];
	int m_iTotalDeaths[MAX_PLAYERS + 1];
	int m_iRoundScore[MAX_PLAYERS + 1];
	int m_iRoundDeaths[MAX_PLAYERS + 1];
	int m_iSelectedTeam[MAX_PLAYERS + 1];
	int m_iPing[MAX_PLAYERS + 1];
	int m_bInfected[MAX_PLAYERS + 1];
	bool m_bConnected[MAX_PLAYERS + 1];
	int m_iTeam[MAX_PLAYERS + 1];
	bool m_bAlive[MAX_PLAYERS + 1];
	bool m_bAdmin[MAX_PLAYERS + 1];
	int m_iHealth[MAX_PLAYERS + 1];
	Vector m_vecPosition[MAX_PLAYERS + 1];
	Color m_Colors[MAX_TEAMS];
	string_t m_szUnconnectedName;
};

extern C_PlayerResource *g_PR;

#endif // C_PLAYERRESOURCE_H
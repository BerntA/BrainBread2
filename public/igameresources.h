//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: IGameResources interface
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMERESOURCES_H
#define IGAMERESOURCES_H

class Color;
class Vector;

abstract_class IGameResources
{
public:
	virtual	~IGameResources() {};

	// Team data access 
	virtual const char		*GetTeamName(int index) = 0;
	virtual int				GetTeamScore(int index) = 0;
	virtual const Color&	GetTeamColor(int index) = 0;

	// Player data access
	virtual bool	IsConnected(int index) = 0;
	virtual bool	IsAlive(int index) = 0;
	virtual bool	IsFakePlayer(int index) = 0;
	virtual bool	IsLocalPlayer(int index) = 0;
	virtual bool	IsInfected(int index) = 0;
	virtual bool    IsGroupIDFlagActive(int index, int flag) = 0;

	virtual const char *GetPlayerName(int index) = 0;
	virtual int		GetGroupIDFlags(int index) = 0;
	virtual int		GetLevel(int index) = 0;
	virtual int		GetTotalScore(int index) = 0;
	virtual int		GetTotalDeaths(int index) = 0;
	virtual int		GetRoundScore(int index) = 0;
	virtual int		GetRoundDeaths(int index) = 0;
	virtual int		GetSelectedTeam(int index) = 0;
	virtual int		GetPing(int index) = 0;
	virtual int		GetTeam(int index) = 0;
	virtual int		GetHealth(int index) = 0;
};

extern IGameResources *GameResources(void); // singelton accessor

#endif
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Friendly Soldier class npc
//
//========================================================================================//

#ifndef NPC_MILITARY_H
#define NPC_MILITARY_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_combine.h"

int g_pMilitaryQuestion = 0;

class CNPCMilitary : public CNPC_BaseSoldier
{
	DECLARE_CLASS(CNPCMilitary, CNPC_BaseSoldier);

public: 
	void		Spawn( void );
	void		Precache( void );
	void		DeathSound( const CTakeDamageInfo &info );
	void		BuildScheduleTestBits( void );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		OnChangeActivity( Activity eNewActivity );
	void		OnListened();
	int			OnTakeDamage( const CTakeDamageInfo &info );
	bool		UsesNavMesh(void) { return true; }
	void		ClearAttackConditions( void );
	bool		AllowedToIgnite( void ) { return true; }
	const char *GetNPCName() { return "Military"; }

	int			GetIdleState(void) { return g_pMilitaryQuestion; }
	void		SetIdleState(int state) { g_pMilitaryQuestion = state; }

private:
	bool		m_fIsBlocking;
};

#endif // NPC_MILITARY_H
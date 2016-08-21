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

//=========================================================
//	>> CNPCMilitary
//=========================================================
class CNPCMilitary : public CNPC_Combine
{
	DECLARE_CLASS( CNPCMilitary, CNPC_Combine );

public: 
	void		Spawn( void );
	void		Precache( void );
	void		DeathSound( const CTakeDamageInfo &info );
	void		PrescheduleThink( void );
	void		BuildScheduleTestBits( void );
	int			SelectSchedule ( void );
	float		GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		OnChangeActivity( Activity eNewActivity );
	void		Event_Killed( const CTakeDamageInfo &info );
	void		OnListened();
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	void		ClearAttackConditions( void );

	bool		m_fIsBlocking;

	bool		IsLightDamage( const CTakeDamageInfo &info );
	bool		IsHeavyDamage( const CTakeDamageInfo &info );

	virtual	bool		AllowedToIgnite( void ) { return true; }

	const char *GetNPCName() { return "Military"; }

	int GetIdleState(void) { return g_pMilitaryQuestion; }
	void SetIdleState(int state) { g_pMilitaryQuestion = state; }
};

#endif // NPC_MILITARY_H
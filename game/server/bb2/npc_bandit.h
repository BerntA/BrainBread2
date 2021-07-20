//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Bandit - Evil and foul mercenaries!
//
//========================================================================================//

#ifndef NPC_BANDIT_H
#define NPC_BANDIT_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_base_soldier.h"

class CNPCBandit : public CNPC_BaseSoldier
{
	DECLARE_CLASS(CNPCBandit, CNPC_BaseSoldier);

public:

	void		Spawn(void);
	void		Precache(void);
	void		DeathSound(const CTakeDamageInfo &info);
	void		BuildScheduleTestBits(void);
	void		HandleAnimEvent(animevent_t *pEvent);
	void		OnChangeActivity(Activity eNewActivity);
	void		OnListened();
	int			OnTakeDamage(const CTakeDamageInfo &info);
	void		ClearAttackConditions(void);
	bool		AllowedToIgnite(void) { return true; }
	Class_T		Classify(void);
	virtual BB2_SoundTypes GetNPCType() { return TYPE_BANDIT; }
	virtual int GetNPCClassType() { return NPC_CLASS_BANDIT; }

	int GetIdleState(void) { return g_pBanditQuestion; }
	void SetIdleState(int state) { g_pBanditQuestion = state; }

private:
	bool		m_fIsBlocking;

	static int g_pBanditQuestion;
};

#endif // NPC_BANDIT_H
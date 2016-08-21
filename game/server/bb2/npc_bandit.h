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

#include "npc_combine.h"

int g_pBanditQuestion = 0;

//=========================================================
//	>> CNPCBandit
//=========================================================
class CNPCBandit : public CNPC_Combine
{
	DECLARE_CLASS(CNPCBandit, CNPC_Combine);

public:

	void		Spawn(void);
	void		Precache(void);
	void		DeathSound(const CTakeDamageInfo &info);
	void		PrescheduleThink(void);
	void		BuildScheduleTestBits(void);
	int			SelectSchedule(void);
	float		GetHitgroupDamageMultiplier(int iHitGroup, const CTakeDamageInfo &info);
	void		HandleAnimEvent(animevent_t *pEvent);
	void		OnChangeActivity(Activity eNewActivity);
	void		Event_Killed(const CTakeDamageInfo &info);
	void		OnListened();
	virtual int OnTakeDamage(const CTakeDamageInfo &info);

	void		ClearAttackConditions(void);

	bool		m_fIsBlocking;

	bool		IsLightDamage(const CTakeDamageInfo &info);
	bool		IsHeavyDamage(const CTakeDamageInfo &info);

	virtual	bool		AllowedToIgnite(void) { return true; }

	Class_T Classify(void);

	BB2_SoundTypes GetNPCType() { return TYPE_BANDIT; }

	const char *GetNPCName() { return "Bandit"; }

	int GetIdleState(void) { return g_pBanditQuestion; }
	void SetIdleState(int state) { g_pBanditQuestion = state; }
};
#endif // NPC_BANDIT_H
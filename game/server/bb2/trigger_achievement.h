//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Gives an achievement for everyone or the ones who touched the trigger.
//
//========================================================================================//

#ifndef TRIGGER_ACHIEVEMENT_H
#define TRIGGER_ACHIEVEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CTriggerAchievement : public CTriggerMultiple
{
public:
	DECLARE_CLASS(CTriggerAchievement, CTriggerMultiple);
	DECLARE_DATADESC();

	CTriggerAchievement(void);

	void Spawn(void);
	void Touch(CBaseEntity *pOther);

protected:
	bool m_bRemoveOnTrigger;
	bool m_bGiveToAll;
	int m_iFilter;
	string_t szAchievementLink;
};

#endif // TRIGGER_ACHIEVEMENT_H
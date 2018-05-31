//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Smart trigger, trigger once/multi with filtrating.
//
//========================================================================================//

#ifndef TRIGGER_SMART_H
#define TRIGGER_SMART_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CSmartTrigger : public CTriggerMultiple
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS(CSmartTrigger, CTriggerMultiple);

	CSmartTrigger(void);

	void Spawn(void);
	bool PassesTriggerFilters(CBaseEntity *pOther);
	void Touch(CBaseEntity *pOther);

protected:
	int m_iExtraFilter;
	bool m_bTouchOnlyOnce;
};

#endif // TRIGGER_SMART_H
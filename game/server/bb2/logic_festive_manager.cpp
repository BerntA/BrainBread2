//=========       Copyright © Reperio Studios 2024 @ Bernt Andreas Eide!       ============//
//
// Purpose: A simple festivity manager - spawn diff map entities on festive trigger.
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"

class CLogicFestiveManager : public CLogicalEntity
{
public:

	DECLARE_CLASS(CLogicFestiveManager, CLogicalEntity);
	DECLARE_DATADESC();

	CLogicFestiveManager();

	void Activate(void);

private:

	COutputEvent m_OnNoFestiveEvent;
	COutputEvent m_OnHalloweenEvent;
	COutputEvent m_OnChristmasEvent;
};

LINK_ENTITY_TO_CLASS(logic_festive_manager, CLogicFestiveManager);

BEGIN_DATADESC(CLogicFestiveManager)
DEFINE_OUTPUT(m_OnNoFestiveEvent, "OnNoFestiveEvent"),
DEFINE_OUTPUT(m_OnHalloweenEvent, "OnHalloweenEvent"),
DEFINE_OUTPUT(m_OnChristmasEvent, "OnChristmasEvent"),
END_DATADESC()

CLogicFestiveManager::CLogicFestiveManager()
{
}

void CLogicFestiveManager::Activate(void)
{
	BaseClass::Activate();

	const int festiveEvent = GameBaseShared()->GetFestiveEvent();

	switch (festiveEvent)
	{
	case FESTIVE_EVENT_NONE:
		m_OnNoFestiveEvent.FireOutput(this, this);
		break;

	case FESTIVE_EVENT_HALLOWEEN:
		m_OnHalloweenEvent.FireOutput(this, this);
		break;

	case FESTIVE_EVENT_CHRISTMAS:
		m_OnChristmasEvent.FireOutput(this, this);
		break;
	}
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Give an Achievement to everyone.
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "GameBase_Server.h"

class CLogicAchievement : public CLogicalEntity
{
public:
	DECLARE_CLASS(CLogicAchievement, CLogicalEntity);
	DECLARE_DATADESC();

	CLogicAchievement();
	void GiveAchievement(inputdata_t &data);
};

LINK_ENTITY_TO_CLASS(logic_achievement, CLogicAchievement);

BEGIN_DATADESC(CLogicAchievement)
DEFINE_INPUTFUNC(FIELD_STRING, "GiveAchievement", GiveAchievement),
END_DATADESC()

CLogicAchievement::CLogicAchievement()
{
}

void CLogicAchievement::GiveAchievement(inputdata_t &data)
{
	GameBaseServer()->SendAchievement(STRING(data.value.StringID()));
}
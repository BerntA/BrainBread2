//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: NPC Ladder Brushes - Used for small obstacles.
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "modelentities.h"

class CFuncNPCLadder : public CFuncBrush
{
public:
	DECLARE_CLASS(CFuncNPCLadder, CFuncBrush);
	int	GetObstruction(void) { return ENTITY_OBSTRUCTION_NPC_OBSTACLE; }
};

LINK_ENTITY_TO_CLASS(func_npc_obstacle, CFuncNPCLadder);
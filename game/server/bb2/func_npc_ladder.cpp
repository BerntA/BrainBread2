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
	DECLARE_DATADESC();
};

BEGIN_DATADESC(CFuncNPCLadder)
END_DATADESC()

LINK_ENTITY_TO_CLASS(func_npc_obstacle, CFuncBrush);
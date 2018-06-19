//=========       Copyright © Reperio Studios 2013-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie NPC BaseClass, used for excluding various unnecessary netvars.
//
//==============================================================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_BaseZombie : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_NPC_BaseZombie, C_AI_BaseNPC);
public:
	DECLARE_CLIENTCLASS();

	C_NPC_BaseZombie();
};

C_NPC_BaseZombie::C_NPC_BaseZombie()
{
}

IMPLEMENT_CLIENTCLASS_DT(C_NPC_BaseZombie, DT_AI_BaseZombie, CNPC_BaseZombie)
END_RECV_TABLE()

//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Bandit - Evil and foul mercenaries!
//
//========================================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_bandit.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "soundenvelope.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(npc_bandit, CNPCBandit);

int CNPCBandit::g_pBanditQuestion = 0;

//-----------------------------------------------------------------------------
// Purpose: Take Damage
//-----------------------------------------------------------------------------
int CNPCBandit::OnTakeDamage(const CTakeDamageInfo &info)
{
	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
	return BaseClass::OnTakeDamage(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCBandit::Spawn(void)
{
	Precache();

	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);

	BaseClass::Spawn();

	SetCollisionGroup(COLLISION_GROUP_NPC_MERCENARY);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPCBandit::Precache()
{
	UTIL_PrecacheOther("weapon_frag");
	BaseClass::Precache();
}

void CNPCBandit::DeathSound(const CTakeDamageInfo &info)
{
	// NOTE: The response system deals with this at the moment
	if (GetFlags() & FL_DISSOLVING)
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPCBandit::ClearAttackConditions()
{
	bool fCanRangeAttack2 = HasCondition(COND_CAN_RANGE_ATTACK2);

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if (fCanRangeAttack2)
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition(COND_CAN_RANGE_ATTACK2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPCBandit::BuildScheduleTestBits(void)
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if (m_flGroundSpeed == 0.0 && !IsCurSchedule(SCHED_FLINCH_PHYSICS))
	{
		SetCustomInterruptCondition(COND_HEAR_PHYSICS_DANGER);
	}

	BaseClass::BuildScheduleTestBits();
}

void CNPCBandit::OnListened()
{
	BaseClass::OnListened();

	if (HasCondition(COND_HEAR_DANGER) && HasCondition(COND_HEAR_PHYSICS_DANGER))
	{
		if (HasInterruptCondition(COND_HEAR_DANGER))
		{
			ClearCondition(COND_HEAR_PHYSICS_DANGER);
		}
	}
}

Class_T	CNPCBandit::Classify(void)
{
	return CLASS_MILITARY;
}

class CNPCBanditLeader : public CNPCBandit
{
public:
	DECLARE_CLASS(CNPCBanditLeader, CNPCBandit);
	int GetNPCClassType() { return NPC_CLASS_BANDIT_LEADER; }
};
LINK_ENTITY_TO_CLASS(npc_bandit_leader, CNPCBanditLeader);

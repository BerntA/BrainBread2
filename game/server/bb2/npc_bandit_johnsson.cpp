//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Johnsson - King of the junkyard.
//
//========================================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_combine.h"
#include "bitstring.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "hl2/hl2_player.h"
#include "game.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "Sprite.h"
#include "soundenvelope.h"
#include "hl2mp_gamerules.h"
#include "vehicle_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPCBanditJohnsson : public CNPC_Combine
{
	DECLARE_CLASS(CNPCBanditJohnsson, CNPC_Combine);

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
	int OnTakeDamage(const CTakeDamageInfo &info);

	void		ClearAttackConditions(void);

	bool		m_fIsBlocking;

	bool		IsLightDamage(const CTakeDamageInfo &info);
	bool		IsHeavyDamage(const CTakeDamageInfo &info);

	bool AllowedToIgnite(void) { return false; }
	bool IsBoss() { return true; }
	bool CanAlwaysSeePlayers() { return true; }
	bool GetGender() { return true; } // force male
	// bool UsesNavMesh(void) { return true; }
	int AllowEntityToBeGibbed(void) { return GIB_NO_GIBS; }

	Class_T Classify(void);

	BB2_SoundTypes GetNPCType() { return TYPE_CUSTOM; }

	const char *GetNPCName() { return "Johnsson"; }
};

LINK_ENTITY_TO_CLASS(npc_bandit_johnsson, CNPCBanditJohnsson);

#define AE_SOLDIER_BLOCK_PHYSICS 20 // trying to block an incoming physics object...

//-----------------------------------------------------------------------------
// Purpose: Take Damage
//-----------------------------------------------------------------------------
int CNPCBanditJohnsson::OnTakeDamage(const CTakeDamageInfo &info)
{
	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
	int tookDamage = BaseClass::OnTakeDamage(info);
	return tookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::Spawn(void)
{
	Precache();

	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);

	BaseClass::Spawn();

	SetCollisionGroup(COLLISION_GROUP_NPC_MERCENARY);

	m_iNumGrenades = 2000;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::Precache()
{
	UTIL_PrecacheOther("weapon_frag");
	BaseClass::Precache();
}

void CNPCBanditJohnsson::DeathSound(const CTakeDamageInfo &info)
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
void CNPCBanditJohnsson::ClearAttackConditions()
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

void CNPCBanditJohnsson::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::BuildScheduleTestBits(void)
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if (m_flGroundSpeed == 0.0 && !IsCurSchedule(SCHED_FLINCH_PHYSICS))
	{
		SetCustomInterruptCondition(COND_HEAR_PHYSICS_DANGER);
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPCBanditJohnsson::SelectSchedule(void)
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPCBanditJohnsson::GetHitgroupDamageMultiplier(int iHitGroup, const CTakeDamageInfo &info)
{
	return BaseClass::GetHitgroupDamageMultiplier(iHitGroup, info);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::HandleAnimEvent(animevent_t *pEvent)
{
	switch (pEvent->event)
	{
	case AE_SOLDIER_BLOCK_PHYSICS:
		m_fIsBlocking = true;
		break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

void CNPCBanditJohnsson::OnChangeActivity(Activity eNewActivity)
{
	// Any new sequence stops us blocking.
	m_fIsBlocking = false;

	BaseClass::OnChangeActivity(eNewActivity);
}

void CNPCBanditJohnsson::OnListened()
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPCBanditJohnsson::IsLightDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsLightDamage(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPCBanditJohnsson::IsHeavyDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsHeavyDamage(info);
}

Class_T	CNPCBanditJohnsson::Classify(void)
{
	return CLASS_MILITARY;
}
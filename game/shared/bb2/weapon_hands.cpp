//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Hands Melee
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "weapon_melee_chargeable.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponHands C_WeaponHands
#endif

class CWeaponHands : public CHL2MPMeleeChargeable
{
public:
	DECLARE_CLASS(CWeaponHands, CHL2MPMeleeChargeable);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponHands();
	CWeaponHands(const CWeaponHands &);

	int GetMeleeDamageType() { return DMG_CLUB; }
	int GetMeleeSkillFlags(void) { return 0; }

	void Drop(const Vector &vecVelocity);
	bool VisibleInWeaponSelection() { return false; }

	Activity GetCustomActivity(int bIsSecondary);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHands, DT_WeaponHands)

BEGIN_NETWORK_TABLE(CWeaponHands, DT_WeaponHands)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponHands)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_hands, CWeaponHands);
PRECACHE_WEAPON_REGISTER(weapon_hands);

acttable_t	CWeaponHands::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_MELEE, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_MELEE, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_MELEE, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_MELEE, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_MELEE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_MELEE, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_MELEE, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_MELEE, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};

IMPLEMENT_ACTTABLE(CWeaponHands);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponHands::CWeaponHands(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHands::Drop(const Vector &vecVelocity)
{
	ResetStates();

#ifndef CLIENT_DLL
	UTIL_Remove(this);
#endif
}

Activity CWeaponHands::GetCustomActivity(int bIsSecondary)
{
	if (bIsSecondary)
		return ACT_VM_CHARGE_ATTACK;

	return ACT_VM_PRIMARYATTACK;
}
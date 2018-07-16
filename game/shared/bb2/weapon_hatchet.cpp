//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Hatchet Melee 
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponHatchet C_WeaponHatchet
#endif

class CWeaponHatchet : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponHatchet, CBaseHL2MPBludgeonWeapon);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponHatchet();
	int GetUniqueWeaponID() { return WEAPON_ID_HATCHET; }

private:
	CWeaponHatchet(const CWeaponHatchet &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHatchet, DT_WeaponHatchet)

BEGIN_NETWORK_TABLE(CWeaponHatchet, DT_WeaponHatchet)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponHatchet)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_hatchet, CWeaponHatchet);
PRECACHE_WEAPON_REGISTER(weapon_hatchet);

acttable_t	CWeaponHatchet::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_MELEE_1HANDED, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_MELEE_1HANDED, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_MELEE_1HANDED, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_MELEE_1HANDED, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_MELEE_1HANDED, false },
	{ ACT_MP_RUN, ACT_HL2MP_RUN_MELEE_1HANDED, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_MELEE_1HANDED, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE_1HANDED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE_1HANDED, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_MELEE_1HANDED, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_MELEE_1HANDED, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_MELEE_1HANDED, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};

IMPLEMENT_ACTTABLE(CWeaponHatchet);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponHatchet::CWeaponHatchet(void)
{
}
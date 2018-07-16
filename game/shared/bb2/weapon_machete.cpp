//=========       Copyright � Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Machete Melee 
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponMachete C_WeaponMachete
#endif

class CWeaponMachete : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponMachete, CBaseHL2MPBludgeonWeapon);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponMachete();
	int GetUniqueWeaponID() { return WEAPON_ID_MACHETE; }

private:
	CWeaponMachete(const CWeaponMachete &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMachete, DT_WeaponMachete)

BEGIN_NETWORK_TABLE(CWeaponMachete, DT_WeaponMachete)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponMachete)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_machete, CWeaponMachete);
PRECACHE_WEAPON_REGISTER(weapon_machete);

acttable_t	CWeaponMachete::m_acttable[] =
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

IMPLEMENT_ACTTABLE(CWeaponMachete);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponMachete::CWeaponMachete(void)
{
}
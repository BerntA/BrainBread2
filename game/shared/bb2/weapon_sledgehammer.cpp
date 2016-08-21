//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Sledgehammer Melee 
//
//========================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
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
#include "hl2_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponSledgehammer C_WeaponSledgehammer
#endif

class CWeaponSledgehammer : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponSledgehammer, CBaseHL2MPBludgeonWeapon);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponSledgehammer();
	CWeaponSledgehammer(const CWeaponSledgehammer &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSledgehammer, DT_WeaponSledgehammer)

BEGIN_NETWORK_TABLE(CWeaponSledgehammer, DT_WeaponSledgehammer)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponSledgehammer)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_sledgehammer, CWeaponSledgehammer);
PRECACHE_WEAPON_REGISTER(weapon_sledgehammer);

acttable_t	CWeaponSledgehammer::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_MELEE_2HANDED, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_MELEE_2HANDED, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_MELEE_2HANDED, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_MELEE_2HANDED, false },
	{ ACT_MP_RUN, ACT_HL2MP_RUN_MELEE_2HANDED, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_MELEE_2HANDED, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE_2HANDED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE_2HANDED, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_MELEE_2HANDED, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_MELEE_2HANDED, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_MELEE_2HANDED, false },

	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};

IMPLEMENT_ACTTABLE(CWeaponSledgehammer);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponSledgehammer::CWeaponSledgehammer(void)
{
}
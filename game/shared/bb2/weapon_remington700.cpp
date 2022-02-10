//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Remington-700 Sniper Rifle. 2X Zoom.
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "weapon_base_sniper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponRemington700 C_WeaponRemington700
#endif

class CWeaponRemington700 : public CHL2MPSniperRifle
{
public:
	DECLARE_CLASS(CWeaponRemington700, CHL2MPSniperRifle);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponRemington700();

	int GetMaxZoomLevel(void) { return 2; }
	int GetUniqueWeaponID() { return WEAPON_ID_REMINGTON700; }

	float GetMinRestTime() { return 1.0f; }
	float GetMaxRestTime() { return 1.5f; }

private:
	CWeaponRemington700(const CWeaponRemington700 &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponRemington700, DT_WeaponRemington700)

BEGIN_NETWORK_TABLE(CWeaponRemington700, DT_WeaponRemington700)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponRemington700)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_remington700, CWeaponRemington700);
PRECACHE_WEAPON_REGISTER(weapon_remington700);

acttable_t CWeaponRemington700::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_AR2, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_AR2, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_AR2, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_RIFLE, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_RIFLE, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_AR2, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_AR2, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_AR2, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_AR2, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_AR2, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_AR2, false },

	// NPC Stuff.
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR2, true },
	{ ACT_RELOAD, ACT_RELOAD_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE, ACT_IDLE_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SMG1, true },		// FIXME: hook to AR2 unique
	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_RIFLE, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_RIFLE, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_AR2, false },
	{ ACT_COVER_LOW, ACT_COVER_SMG1_LOW, false },		// FIXME: hook to AR2 unique
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_AR2_LOW, false },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SMG1_LOW, true },		// FIXME: hook to AR2 unique
	{ ACT_RELOAD_LOW, ACT_RELOAD_SMG1_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_SMG1, true },
};

IMPLEMENT_ACTTABLE(CWeaponRemington700);

CWeaponRemington700::CWeaponRemington700()
{
}
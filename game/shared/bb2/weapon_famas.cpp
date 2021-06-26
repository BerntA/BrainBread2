//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Famas Assault Rifle
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponFamas C_WeaponFamas
#endif

class CWeaponFamas : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CWeaponFamas, CHL2MPMachineGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponFamas();

	int GetUniqueWeaponID() { return WEAPON_ID_FAMAS; }
	int GetAmmoMaxCarry(void) { return 200; }
	int     GetOverloadCapacity() { return 10; }
	bool    AllowBurst(void) { return true; }
	const char *GetAmmoEntityLink(void) { return "ammo_rifle"; }

	float GetMinRestTime() { return 0.1; }
	float GetMaxRestTime() { return 0.3; }

	int		GetMinBurst() { return 3; }
	int		GetMaxBurst() { return 6; }

private:
	CWeaponFamas(const CWeaponFamas &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponFamas, DT_WeaponFamas)

BEGIN_NETWORK_TABLE(CWeaponFamas, DT_WeaponFamas)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponFamas)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_famas, CWeaponFamas);
PRECACHE_WEAPON_REGISTER(weapon_famas);

acttable_t CWeaponFamas::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_FAMAS, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_FAMAS, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_AR2, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_FAMAS, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_FAMAS, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_FAMAS, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_FAMAS, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_FAMAS, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_FAMAS, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_FAMAS, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_FAMAS, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_FAMAS, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_FAMAS, false },

	//NPC Stuff.
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

IMPLEMENT_ACTTABLE(CWeaponFamas);

CWeaponFamas::CWeaponFamas()
{
}
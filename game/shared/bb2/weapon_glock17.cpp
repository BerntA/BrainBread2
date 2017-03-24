//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Glock-17 Handgun
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_base_pistol.h"

#ifdef CLIENT_DLL
#define CWeaponGlock17 C_WeaponGlock17
#endif

class CWeaponGlock17 : public CHL2MPBasePistol
{
public:
	DECLARE_CLASS(CWeaponGlock17, CHL2MPBasePistol);

	CWeaponGlock17(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	int GetOverloadCapacity() { return 6; }
	bool AllowBurst(void) { return true; }
	float GetBurstFireRate(void) { return GetWpnData().m_flBurstFireRate; }

private:
	CWeaponGlock17(const CWeaponGlock17 &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGlock17, DT_WeaponGlock17)

BEGIN_NETWORK_TABLE(CWeaponGlock17, DT_WeaponGlock17)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGlock17)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_glock17, CWeaponGlock17);
PRECACHE_WEAPON_REGISTER(weapon_glock17);

acttable_t CWeaponGlock17::m_acttable[] =
{
#ifdef BB2_AI
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_PISTOL, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_PISTOL, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_PISTOL, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_PISTOL, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_PISTOL, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_PISTOL, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_PISTOL, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_PISTOL, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_PISTOL, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_PISTOL, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_PISTOL, false },

	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, false },

	// HL2
	{ ACT_IDLE, ACT_IDLE_PISTOL, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_PISTOL, true },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true },
	{ ACT_RELOAD, ACT_RELOAD, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_PISTOL, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_PISTOL, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_PISTOL, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_PISTOL, false },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_PISTOL_LOW, false },
	{ ACT_COVER_LOW, ACT_COVER_PISTOL_LOW, false },
	{ ACT_RANGE_AIM_LOW, ACT_RANGE_AIM_PISTOL_LOW, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD_PISTOL, false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE(CWeaponGlock17);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGlock17::CWeaponGlock17(void)
{
}
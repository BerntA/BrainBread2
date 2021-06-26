//=========       Copyright � Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: HK-MP5 SMG.
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_sniper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponMP5 C_WeaponMP5
#endif

class CWeaponMP5 : public CHL2MPSniperRifle
{
public:
	DECLARE_CLASS(CWeaponMP5, CHL2MPSniperRifle);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponMP5();

	bool ShouldDrawCrosshair(void) { return true; }
	bool ShouldPlayZoomSounds() { return false; }
	bool ShouldHideViewmodelOnZoom() { return false; }
	int GetMaxZoomLevel(void) { return 1; }
	int GetOverloadCapacity() { return 10; }
	int GetUniqueWeaponID() { return WEAPON_ID_MP5; }
	int GetWeaponType(void) { return WEAPON_TYPE_SMG; }
	const char *GetAmmoEntityLink(void) { return "ammo_smg"; }
	void PrimaryAttack(void) { CWeaponHL2MPBase::PrimaryAttack(); }

	int		GetMinBurst() { return 3; }
	int		GetMaxBurst() { return 5; }

	bool Reload(void)
	{
		if (m_flNextPrimaryAttack > gpGlobals->curtime)
			return false;

		CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
		if (pClient)
		{
			int reloadAct = GetReloadActivity();
			if (DefaultReload(GetMaxClip(), reloadAct))
			{
				pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, reloadAct);
				WeaponSound(RELOAD);
				SetZoomLevel(0);
				return true;
			}
		}

		return false;
	}

	const char		*GetAmmoTypeName(void) { return "SMG"; }
	int				GetAmmoMaxCarry(void) { return 180; }

private:
	CWeaponMP5(const CWeaponMP5 &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMP5, DT_WeaponMP5)

BEGIN_NETWORK_TABLE(CWeaponMP5, DT_WeaponMP5)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponMP5)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_mp5, CWeaponMP5);
PRECACHE_WEAPON_REGISTER(weapon_mp5);

acttable_t CWeaponMP5::m_acttable[] =
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

IMPLEMENT_ACTTABLE(CWeaponMP5);

CWeaponMP5::CWeaponMP5()
{
}
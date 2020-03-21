//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: MP412 REX - Revolver
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponREX C_WeaponREX
#endif

class CWeaponREX : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponREX, CBaseHL2MPCombatWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponREX(void);

	void PrimaryAttack(void);
	int GetOverloadCapacity() { return 2; }
	int GetUniqueWeaponID() { return WEAPON_ID_REXMP412; }
	int GetWeaponType(void) { return WEAPON_TYPE_REVOLVER; }
	const char *GetAmmoEntityLink(void) { return "ammo_revolver"; }

private:

	CWeaponREX(const CWeaponREX &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponREX, DT_WeaponREX)

BEGIN_NETWORK_TABLE(CWeaponREX, DT_WeaponREX)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponREX)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_rex, CWeaponREX);
PRECACHE_WEAPON_REGISTER(weapon_rex);

acttable_t CWeaponREX::m_acttable[] =
{
#ifdef BB2_AI
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_REVOLVER, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_REVOLVER, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_REVOLVER, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_REVOLVER, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_REVOLVER, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_REVOLVER, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_REVOLVER, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_REVOLVER, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_REVOLVER, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_REVOLVER, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_REVOLVER, false },

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

IMPLEMENT_ACTTABLE(CWeaponREX);

CWeaponREX::CWeaponREX(void)
{
	m_bReloadsSingly = m_bFiresUnderwater = false;
}

void CWeaponREX::PrimaryAttack(void)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer || !(pPlayer->m_afButtonPressed & IN_ATTACK))
		return;

	WeaponSound(SINGLE);
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector();

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetLocalOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;
	info.m_vecSpread = pPlayer->GetAttackSpread(this);

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt(-1, 1);
	angles.y += random->RandomInt(-1, 1);
	angles.z = 0;

#ifndef CLIENT_DLL
	pPlayer->SnapEyeAngles(angles);
#endif

	pPlayer->ViewPunch(QAngle(-8, random->RandomFloat(-2, 2), 0));
	m_iShotsFired++;

	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_PRIMARYATTACK);
}
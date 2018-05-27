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
#define CWeaponREXAkimbo C_WeaponREXAkimbo
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
	bool Reload(void);

	int GetOverloadCapacity() { return 2; }
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
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

void CWeaponREX::PrimaryAttack(void)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	if (!(pPlayer->m_afButtonPressed & IN_ATTACK))
		return;

	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetAbsOrigin();
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

bool CWeaponREX::Reload(void)
{
	bool fRet;

	CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
	if (pClient)
	{
		int reloadAct = ACT_VM_RELOAD0 + pClient->GetSkillValue(PLAYER_SKILL_HUMAN_PISTOL_MASTER);
		if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
			reloadAct = ACT_VM_RELOAD10;

		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), reloadAct);
		if (fRet)
			pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, reloadAct);
	}
	else
		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);

	if (fRet)
		WeaponSound(RELOAD);

	return fRet;
}

class CWeaponREXAkimbo : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponREXAkimbo, CBaseHL2MPCombatWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponREXAkimbo(void);

	void PerformAttack(bool bPrimary);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void ItemPostFrame(void);

	bool Reload(void);

	int GetOverloadCapacity() { return 3; }
	int GetWeaponType(void) { return WEAPON_TYPE_REVOLVER; }
	const char *GetAmmoEntityLink(void) { return "ammo_revolver"; }
	bool IsAkimboWeapon(void) { return true; }

	const char *GetMuzzleflashAttachment(bool bPrimaryAttack)
	{
		if (bPrimaryAttack)
			return "left_muzzle";

		return "right_muzzle";
	}

private:

	CWeaponREXAkimbo(const CWeaponREXAkimbo &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponREXAkimbo, DT_WeaponREXAkimbo)

BEGIN_NETWORK_TABLE(CWeaponREXAkimbo, DT_WeaponREXAkimbo)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponREXAkimbo)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_akimbo_rex, CWeaponREXAkimbo);
PRECACHE_WEAPON_REGISTER(weapon_akimbo_rex);

acttable_t CWeaponREXAkimbo::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_AKIMBO, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_AKIMBO, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_REVOLVER, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_AKIMBO, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_AKIMBO, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_AKIMBO, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_AKIMBO, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_AKIMBO, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_AKIMBO, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_AKIMBO, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_AKIMBO, false },
};

IMPLEMENT_ACTTABLE(CWeaponREXAkimbo);

CWeaponREXAkimbo::CWeaponREXAkimbo(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

void CWeaponREXAkimbo::PerformAttack(bool bPrimary)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	if ((bPrimary && !(pPlayer->m_afButtonPressed & IN_ATTACK)) || (!bPrimary && !(pPlayer->m_afButtonPressed & IN_ATTACK2)))
		return;

	WeaponSound(SINGLE);

	int shootAct = (bPrimary ? ACT_VM_SHOOT_LEFT : ACT_VM_SHOOT_RIGHT);
	SendWeaponAnim(shootAct);

	if (bPrimary)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
		m_iClip1--;
	}
	else
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();
		m_iClip2--;
	}

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetAbsOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;
	info.m_vecSpread = pPlayer->GetAttackSpread(this);
	info.m_bPrimaryAttack = bPrimary;

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

	m_iShotsFired++;
	pPlayer->ViewPunch(QAngle(-5, random->RandomFloat(-1, 1), 0));

	pPlayer->DoAnimationEvent((bPrimary ? PLAYERANIMEVENT_ATTACK_PRIMARY : PLAYERANIMEVENT_ATTACK_SECONDARY), shootAct);
}

void CWeaponREXAkimbo::PrimaryAttack(void)
{
	PerformAttack(true);
}

void CWeaponREXAkimbo::SecondaryAttack(void)
{
	PerformAttack(false);
}

void CWeaponREXAkimbo::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();
}

bool CWeaponREXAkimbo::Reload(void)
{
	bool fRet;

	CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
	if (pClient)
	{
		int reloadAct = ACT_VM_RELOAD0 + pClient->GetSkillValue(PLAYER_SKILL_HUMAN_PISTOL_MASTER);
		if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
			reloadAct = ACT_VM_RELOAD10;

		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), reloadAct);
		if (fRet)
			pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, reloadAct);
	}
	else
		fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);

	if (fRet)
		WeaponSound(RELOAD);

	return fRet;
}
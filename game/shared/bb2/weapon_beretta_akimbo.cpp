//=========       Copyright © Reperio Studios 2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: Beretta M9 Akimbo Handguns
//
//========================================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponBerettaAkimbo C_WeaponBerettaAkimbo
#endif

class CWeaponBerettaAkimbo : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponBerettaAkimbo, CBaseHL2MPCombatWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponBerettaAkimbo(void);

	void PerformAttack(bool bPrimary);
	int GetUniqueWeaponID() { return WEAPON_ID_BERETTA_AKIMBO; }	

private:
	CWeaponBerettaAkimbo(const CWeaponBerettaAkimbo &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponBerettaAkimbo, DT_WeaponBerettaAkimbo)

BEGIN_NETWORK_TABLE(CWeaponBerettaAkimbo, DT_WeaponBerettaAkimbo)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponBerettaAkimbo)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_akimbo_beretta, CWeaponBerettaAkimbo);
PRECACHE_WEAPON_REGISTER(weapon_akimbo_beretta);

CWeaponBerettaAkimbo::CWeaponBerettaAkimbo(void)
{
	m_bReloadsSingly = m_bFiresUnderwater = false;
}

void CWeaponBerettaAkimbo::PerformAttack(bool bPrimary)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer || (bPrimary && !(pPlayer->m_afButtonPressed & IN_ATTACK)) || (!bPrimary && !(pPlayer->m_afButtonPressed & IN_ATTACK2)))
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
	info.m_vecFirstStartPos = pPlayer->GetLocalOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;
	info.m_vecSpread = pPlayer->GetAttackSpread(this);
	info.m_bPrimaryAttack = bPrimary;
	pPlayer->FireBullets(info);

	m_iShotsFired++;
	pPlayer->ViewPunch(GetViewKickAngle());
	pPlayer->DoAnimationEvent((bPrimary ? PLAYERANIMEVENT_ATTACK_PRIMARY : PLAYERANIMEVENT_ATTACK_SECONDARY), shootAct);
}
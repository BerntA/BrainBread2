//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Sawed-Off Double Barrel Shotgun
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_base_shotgun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponSawedOff C_WeaponSawedOff
#endif

enum WeaponSawedOffFlags
{
	SAWEDOFF_FIRED_LEFT = 0x01,
	SAWEDOFF_FIRED_RIGHT = 0x02,
};

class CWeaponSawedOff : public CHL2MPBaseShotgun
{
public:
	DECLARE_CLASS(CWeaponSawedOff, CHL2MPBaseShotgun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponSawedOff(void);

	int GetOverloadCapacity() { return 1; }
	int GetUniqueWeaponID() { return WEAPON_ID_SAWEDOFF; }

	bool Reload(void);
	void ItemPostFrame(void);
	void PerformAttack(bool bDouble = false);
	void PrimaryAttack(void) { }
	void SecondaryAttack() { }
	void AffectedByPlayerSkill(int skill);
	const char *GetMuzzleflashAttachment(bool bPrimaryAttack);

private:
	CWeaponSawedOff(const CWeaponSawedOff &);

	CNetworkVar(int, m_iFiringFlags);
	CNetworkVar(int, m_iFiringState);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSawedOff, DT_WeaponSawedOff)

BEGIN_NETWORK_TABLE(CWeaponSawedOff, DT_WeaponSawedOff)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iFiringFlags)),
RecvPropInt(RECVINFO(m_iFiringState)),
#else
SendPropInt(SENDINFO(m_iFiringFlags), 2, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iFiringState), 2, SPROP_UNSIGNED),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSawedOff)
DEFINE_PRED_FIELD(m_iFiringFlags, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_iFiringState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_sawedoff, CWeaponSawedOff);
PRECACHE_WEAPON_REGISTER(weapon_sawedoff);

acttable_t	CWeaponSawedOff::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_SHOTGUN, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_SHOTGUN, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_SHOTGUN, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_SHOTGUN, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_SAWEDOFF, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_SAWEDOFF, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, false },
#ifdef BB2_AI
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SAWEDOFF, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, false },

	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, false },

	// HL2
	{ ACT_IDLE, ACT_IDLE_SMG1, true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_RELOAD, ACT_RELOAD, false },
	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SHOTGUN, true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED, ACT_IDLE_SHOTGUN_RELAXED, false },//never aims
	{ ACT_IDLE_STIMULATED, ACT_IDLE_SHOTGUN_STIMULATED, false },
	{ ACT_IDLE_AGITATED, ACT_IDLE_SHOTGUN_AGITATED, false },//always aims

	{ ACT_WALK_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_STIMULATED, ACT_WALK_RIFLE_STIMULATED, false },
	{ ACT_WALK_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_STIMULATED, ACT_RUN_RIFLE_STIMULATED, false },
	{ ACT_RUN_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED, ACT_IDLE_SMG1_RELAXED, false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED, ACT_IDLE_AIM_RIFLE_STIMULATED, false },
	{ ACT_IDLE_AIM_AGITATED, ACT_IDLE_ANGRY_SMG1, false },//always aims

	{ ACT_WALK_AIM_RELAXED, ACT_WALK_RIFLE_RELAXED, false },//never aims
	{ ACT_WALK_AIM_STIMULATED, ACT_WALK_AIM_RIFLE_STIMULATED, false },
	{ ACT_WALK_AIM_AGITATED, ACT_WALK_AIM_RIFLE, false },//always aims

	{ ACT_RUN_AIM_RELAXED, ACT_RUN_RIFLE_RELAXED, false },//never aims
	{ ACT_RUN_AIM_STIMULATED, ACT_RUN_AIM_RIFLE_STIMULATED, false },
	{ ACT_RUN_AIM_AGITATED, ACT_RUN_AIM_RIFLE, false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM, ACT_WALK_AIM_SHOTGUN, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_SHOTGUN, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SHOTGUN_LOW, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD, false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE(CWeaponSawedOff);

CWeaponSawedOff::CWeaponSawedOff(void)
{
	m_iFiringFlags = m_iFiringState = 0;
}

void CWeaponSawedOff::AffectedByPlayerSkill(int skill)
{
	BaseClass::AffectedByPlayerSkill(skill);

	switch (skill)
	{
	case PLAYER_SKILL_HUMAN_GUNSLINGER:
	case PLAYER_SKILL_HUMAN_MAGAZINE_REFILL:
	{
		m_iFiringFlags = 0;
		break;
	}
	}
}

const char *CWeaponSawedOff::GetMuzzleflashAttachment(bool bPrimaryAttack)
{
	if (bPrimaryAttack)
	{
		if (m_iFiringState == SAWEDOFF_FIRED_LEFT)
			return "left_muzzle";
		else
			return "right_muzzle";
	}

	return "muzzle";
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
//-----------------------------------------------------------------------------
bool CWeaponSawedOff::Reload(void)
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return false;

	int iAmmo = pOwner->GetAmmoCount(m_iPrimaryAmmoType);
	if (iAmmo <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, iAmmo);
	if (j <= 0)
		return false;

	m_bInReload = true;

	int iReloadActivity = ACT_VM_RELOAD0;
	if (m_iClip1 <= 0 && iAmmo > 1)
		iReloadActivity = ACT_VM_RELOAD_EMPTY0;

	iReloadActivity += pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_SHOTGUN_MASTER);
	if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
	{
		if (m_iClip1 <= 0 && iAmmo > 1)
			iReloadActivity = ACT_VM_RELOAD_EMPTY10;
		else
			iReloadActivity = ACT_VM_RELOAD10;
	}

	SendWeaponAnim(iReloadActivity);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, iReloadActivity);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

void CWeaponSawedOff::PerformAttack(bool bDouble)
{
	Activity shootActivity = (m_iFiringState == SAWEDOFF_FIRED_LEFT) ? ACT_VM_SHOOT_LEFT : ACT_VM_SHOOT_RIGHT;
	if (bDouble)
		shootActivity = ACT_VM_SHOOT_BOTH;

	BaseClass::PrimaryAttack(shootActivity, (bDouble ? WPN_DOUBLE : SINGLE), !bDouble);
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponSawedOff::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	if (m_bInReload)
	{
		if (IsViewModelSequenceFinished() && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			int iAmmo = pOwner->GetAmmoCount(m_iPrimaryAmmoType);
			if (m_iClip1 <= 0 && iAmmo > 1)
				FillClip(2);
			else
				FillClip(1);

			m_bInReload = false;
			m_iFiringFlags = m_iFiringState = 0;
		}
	}

	WeaponIdle();

	if (m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		if (pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2))
		{
			if ((m_iClip1 <= 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType)))
			{
				DryFire();
				return;
			}
			else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
			{
				WeaponSound(EMPTY);
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
				return;
			}
		}

		if (pOwner->m_nButtons & IN_ATTACK)
		{
			if (m_iFiringFlags & SAWEDOFF_FIRED_LEFT)
			{
				DryFire();
				return;
			}
			else
				m_iFiringState = SAWEDOFF_FIRED_LEFT;
		}
		else if (pOwner->m_nButtons & IN_ATTACK2)
		{
			if (m_iFiringFlags & SAWEDOFF_FIRED_RIGHT)
			{
				DryFire();
				return;
			}
			else
				m_iFiringState = SAWEDOFF_FIRED_RIGHT;
		}

		int oldFlags = m_iFiringFlags;
		m_iFiringFlags |= m_iFiringState;

		if (oldFlags != m_iFiringFlags)
		{
			PerformAttack();
			return;
		}
	}

	if ((pOwner->m_nButtons & IN_RELOAD) && UsesClipsForAmmo1() && !m_bInReload)
	{
		if ((m_iClip1 < GetMaxClip1()) && (pOwner->GetAmmoCount(m_iPrimaryAmmoType) >= 1))
			Reload();
	}
}
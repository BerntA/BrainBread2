//=========       Copyright � Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Winchester 1894 Rifle
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_base_sniper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponWinchester1894 C_WeaponWinchester1894
#endif

class CWeaponWinchester1894 : public CHL2MPSniperRifle
{
public:
	DECLARE_CLASS(CWeaponWinchester1894, CHL2MPSniperRifle);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponWinchester1894(void);

	bool ShouldDrawCrosshair(void) { return true; }
	bool ShouldPlayZoomSounds() { return false; }
	bool ShouldHideViewmodelOnZoom() { return false; }
	int GetMaxZoomLevel(void) { return 1; }
	int GetOverloadCapacity() { return 3; }
	int GetWeaponType(void) { return WEAPON_TYPE_RIFLE; }
	int GetUniqueWeaponID() { return WEAPON_ID_WINCHESTER1894; }
	const char *GetAmmoEntityLink(void) { return "ammo_trapper"; }

	bool StartReload(void);
	bool Reload(void);
	void FillClip(void);
	void FinishReload(void);

	void StartHolsterSequence();
	bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void Drop(const Vector &vecVelocity);

	void ItemPostFrame(void);
	void PrimaryAttack(void);
	void DryFire(void);
	float GetFireRate(void) { return GetWpnData().m_flFireRate; }

	void AffectedByPlayerSkill(int skill);

	float GetMinRestTime() { return 0.85f; }
	float GetMaxRestTime() { return 1.25f; }

private:
	CWeaponWinchester1894(const CWeaponWinchester1894 &);

	CNetworkVar(bool, m_bInReload);
	CNetworkVar(bool, m_bShouldReloadEmpty);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponWinchester1894, DT_WeaponWinchester1894)

BEGIN_NETWORK_TABLE(CWeaponWinchester1894, DT_WeaponWinchester1894)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bInReload)),
RecvPropBool(RECVINFO(m_bShouldReloadEmpty)),
#else
SendPropBool(SENDINFO(m_bInReload)),
SendPropBool(SENDINFO(m_bShouldReloadEmpty)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponWinchester1894)
DEFINE_PRED_FIELD(m_bInReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bShouldReloadEmpty, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_winchester1894, CWeaponWinchester1894);
PRECACHE_WEAPON_REGISTER(weapon_winchester1894);

acttable_t	CWeaponWinchester1894::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_SHOTGUN, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_SHOTGUN, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_SHOTGUN, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_SHOTGUN, false },

	{ ACT_MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN_INSERT, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN_INSERT, false },

	{ ACT_MP_RELOAD_STAND_END, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, false },
	{ ACT_MP_RELOAD_CROUCH_END, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, false },

	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, false },

	// HL2
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_IDLE, ACT_IDLE_SMG1, true },	// FIXME: hook to shotgun unique
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_RELOAD, ACT_RELOAD, false },
	{ ACT_WALK, ACT_WALK_RIFLE, true },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_SHOTGUN, true },
	{ ACT_WALK_AIM, ACT_WALK_AIM_SHOTGUN, true },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH_RIFLE, true },
	{ ACT_WALK_CROUCH_AIM, ACT_WALK_CROUCH_AIM_RIFLE, true },
	{ ACT_RUN, ACT_RUN_RIFLE, true },
	{ ACT_RUN_AIM, ACT_RUN_AIM_SHOTGUN, true },
	{ ACT_RUN_CROUCH, ACT_RUN_CROUCH_RIFLE, true },
	{ ACT_RUN_CROUCH_AIM, ACT_RUN_CROUCH_AIM_RIFLE, true },
	{ ACT_GESTURE_RANGE_ATTACK1, ACT_GESTURE_RANGE_ATTACK_SHOTGUN, true },
	{ ACT_RANGE_ATTACK1_LOW, ACT_RANGE_ATTACK_SHOTGUN_LOW, true },
	{ ACT_RELOAD_LOW, ACT_RELOAD_SHOTGUN, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD, false },
};

IMPLEMENT_ACTTABLE(CWeaponWinchester1894);

void CWeaponWinchester1894::AffectedByPlayerSkill(int skill)
{
	switch (skill)
	{
	case PLAYER_SKILL_HUMAN_GUNSLINGER:
	case PLAYER_SKILL_HUMAN_MAGAZINE_REFILL:
	{
		m_bInReload = false;
		m_bShouldReloadEmpty = false;
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponWinchester1894::StartReload(void)
{
	if ((GetAmmoCount() <= 0) || (Clip() >= GetMaxClip()))
		return false;

	int j = MIN(1, GetAmmoCount());
	if (j <= 0)
		return false;

	SetZoomLevel(0);
	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponWinchester1894::Reload(void)
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Shotgun Reload called incorrectly!\n");
	}

	if ((GetAmmoCount() <= 0) || (Clip() >= GetMaxClip()))
		return false;

	int j = MIN(1, GetAmmoCount());
	if (j <= 0)
		return false;

	FillClip();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);

	int reloadAct = GetReloadActivity(false);
	SendWeaponAnim(reloadAct);

	CHL2MP_Player* pClient = ToHL2MPPlayer(GetOwner());
	if (pClient)
		pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, reloadAct);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponWinchester1894::FinishReload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return;

	m_bInReload = false;
	int reloadAct = ACT_SHOTGUN_RELOAD_FINISH;

	// Finish reload animation
	if (m_bShouldReloadEmpty && (m_iClip > 0))
		reloadAct = ACT_VM_RELOAD_EMPTY0;

	SendWeaponAnim(reloadAct);

	CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
	if (pClient)
		pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD_END, reloadAct);

	m_bShouldReloadEmpty = false;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponWinchester1894::FillClip(void)
{
	if ((GetAmmoCount() > 0) && (Clip() < GetMaxClip()))
	{
		m_iClip++;
		RemoveAmmo(1);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponWinchester1894::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponWinchester1894::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip--;

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_PRIMARYATTACK);

	Vector	vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecAiming = pPlayer->GetAutoaimVector();

	FireBulletsInfo_t info(GetWpnData().m_iPellets, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, GetAmmoTypeID());
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetLocalOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2);
#endif

	pPlayer->ViewPunch(GetViewKickAngle());
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponWinchester1894::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	// Implement base properties:
	if (!m_bInReload)
		BaseClass::GenericBB2Animations();

	if (m_bInReload)
	{
		// If the user wants to cancel the reload then do so.
		if (pOwner->m_nButtons & IN_ATTACK)
		{
			FinishReload();
			return;
		}

		if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			if (GetAmmoCount() <= 0)
			{
				FinishReload();
				return;
			}

			// If clip not full reload again
			if (Clip() < GetMaxClip())
			{
				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				return;
			}
		}
	}

	if (IsViewModelSequenceFinished() && (m_flNextBashAttack <= gpGlobals->curtime) && !(pOwner->m_nButtons & IN_BASH) && !(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_RELOAD))
	{
		if (!m_bInReload && (gpGlobals->curtime >= m_flNextPrimaryAttack))
			WeaponIdle();
	}

	if ((pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		if ((Clip() <= 0 && UsesClipsForAmmo()) || (!UsesClipsForAmmo() && !GetAmmoCount()))
			DryFire();
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if (pOwner->m_afButtonPressed & IN_ATTACK)
				m_flNextPrimaryAttack = gpGlobals->curtime;

			PrimaryAttack();
		}
	}

	if ((pOwner->m_afButtonPressed & IN_ATTACK2) && m_flNextPrimaryAttack <= gpGlobals->curtime)
		SecondaryAttack();

	if (CheckShouldReload(pOwner) && UsesClipsForAmmo() && !m_bInReload && HasAnyAmmo() && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bShouldReloadEmpty = (m_iClip <= 0);
		StartReload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponWinchester1894::CWeaponWinchester1894(void)
{
	m_bReloadsSingly = true;
	m_bShouldReloadEmpty = false;
	m_bInReload = false;
}

void CWeaponWinchester1894::StartHolsterSequence()
{
	if (m_bInReload)
		FinishReload();

	BaseClass::StartHolsterSequence();
}

bool CWeaponWinchester1894::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_bInReload = false;
	m_bShouldReloadEmpty = false;

	return BaseClass::Holster(pSwitchingTo);
}

void CWeaponWinchester1894::Drop(const Vector &vecVelocity)
{
	m_bInReload = false;
	m_bShouldReloadEmpty = false;

	BaseClass::Drop(vecVelocity);
}
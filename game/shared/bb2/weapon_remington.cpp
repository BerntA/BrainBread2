//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Remington 700 Shotgun
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_base_shotgun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponRemington C_WeaponRemington
#endif

class CWeaponRemington : public CHL2MPBaseShotgun
{
public:
	DECLARE_CLASS(CWeaponRemington, CHL2MPBaseShotgun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponRemington(void);

	int GetOverloadCapacity() { return 4; }
	int GetUniqueWeaponID() { return WEAPON_ID_REMINGTON870; }

	bool StartReload(void);
	bool Reload(void);
	void FinishReload(void);

	void StartHolsterSequence();

	void ItemPostFrame(void);
	void PrimaryAttack(void);

private:
	CWeaponRemington(const CWeaponRemington &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponRemington, DT_WeaponRemington)

BEGIN_NETWORK_TABLE(CWeaponRemington, DT_WeaponRemington)
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponRemington)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_remington, CWeaponRemington);
PRECACHE_WEAPON_REGISTER(weapon_remington);

acttable_t	CWeaponRemington::m_acttable[] =
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
#ifdef BB2_AI
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, false },
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
	{ ACT_RELOAD_LOW, ACT_RELOAD_SHOTGUN, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD, false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE(CWeaponRemington);

CWeaponRemington::CWeaponRemington(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
//-----------------------------------------------------------------------------
bool CWeaponRemington::StartReload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

	m_bInReload = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
//-----------------------------------------------------------------------------
bool CWeaponRemington::Reload(void)
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Shotgun Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	FillClip(1);
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);

	CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
	if (pClient)
	{
		int reloadAct = ACT_VM_RELOAD0 + pClient->GetSkillValue(PLAYER_SKILL_HUMAN_SHOTGUN_MASTER);
		if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
			reloadAct = ACT_VM_RELOAD10;

		SendWeaponAnim(reloadAct);
		pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, reloadAct);
	}
	else
		SendWeaponAnim(ACT_VM_RELOAD0);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
//-----------------------------------------------------------------------------
void CWeaponRemington::FinishReload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner == NULL)
		return;

	m_bInReload = false;
	int reloadAct = ACT_SHOTGUN_RELOAD_FINISH;

	// Finish reload animation
	if (m_bShouldReloadEmpty && (m_iClip1 > 0))
		reloadAct = ACT_VM_RELOAD_EMPTY0;

	SendWeaponAnim(reloadAct);

	CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
	if (pClient)
		pClient->DoAnimationEvent(PLAYERANIMEVENT_RELOAD_END, reloadAct);

	m_bShouldReloadEmpty = false;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponRemington::PrimaryAttack(void)
{
	BaseClass::PrimaryAttack(ACT_VM_PRIMARYATTACK, SINGLE);
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponRemington::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

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
			if ((pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0) || (m_iClip1 >= GetMaxClip1()))
			{
				FinishReload();
				return;
			}

			Reload();
			return;
		}
	}

	WeaponIdle();

	if ((pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
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
		else
		{
			// If the firing button was just pressed, reset the firing time
			if (pOwner->m_afButtonPressed & IN_ATTACK)
				m_flNextPrimaryAttack = gpGlobals->curtime;

			PrimaryAttack();
		}
	}

	if ((pOwner->m_nButtons & IN_RELOAD) && UsesClipsForAmmo1() && !m_bInReload && HasAnyAmmo() && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bShouldReloadEmpty = (m_iClip1 <= 0);
		StartReload();
	}
}

void CWeaponRemington::StartHolsterSequence()
{
	if (m_bInReload)
		FinishReload();

	BaseClass::StartHolsterSequence();
}
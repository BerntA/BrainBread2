//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Deagle Handgun
//
//========================================================================================//

#include "cbase.h"
#include "weapon_base_pistol.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponDeagle C_WeaponDeagle
#endif

#define POSEPARAM_1HAND_SEQUENCE "1hand" // Between 0-1! (1.0 = use only one hand to hold the gun)
#define HAND_SWITCH_TIME 0.525f // How many sec. should it take to switch hands.

enum DeagleWeaponState
{
	DEAGLE_STATE_TWO_HAND = 0,
	DEAGLE_STATE_ONE_HAND,

	DEAGLE_STATE_SWITCH_ONE_HAND,
	DEAGLE_STATE_SWITCH_TWO_HAND,
};

class CWeaponDeagle : public CHL2MPBasePistol
{
public:
	DECLARE_CLASS(CWeaponDeagle, CHL2MPBasePistol);

	CWeaponDeagle(void);

	int GetUniqueWeaponID() { return WEAPON_ID_DEAGLE; }
	int GetMinBurst() { return 1; }
	int GetMaxBurst() { return 1; }

	void ResetToTwoHands(void);
	void ItemPostFrame(void);
	void SecondaryAttack(void);

	void StartHolsterSequence();
	bool Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void Drop(const Vector &vecVelocity);
	bool Deploy(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

private:
	CWeaponDeagle(const CWeaponDeagle &);

	CNetworkVar(int, m_iSwitchState);
	CNetworkVar(float, m_flSwitchTime);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponDeagle, DT_WeaponDeagle)

BEGIN_NETWORK_TABLE(CWeaponDeagle, DT_WeaponDeagle)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iSwitchState)),
RecvPropTime(RECVINFO(m_flSwitchTime)),
#else
SendPropInt(SENDINFO(m_iSwitchState), 2, SPROP_UNSIGNED),
SendPropTime(SENDINFO(m_flSwitchTime)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponDeagle)
DEFINE_PRED_FIELD(m_iSwitchState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD_TOL(m_flSwitchTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_deagle, CWeaponDeagle);
PRECACHE_WEAPON_REGISTER(weapon_deagle);

acttable_t CWeaponDeagle::m_acttable[] =
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

IMPLEMENT_ACTTABLE(CWeaponDeagle);

CWeaponDeagle::CWeaponDeagle(void)
{
	m_iSwitchState = 0;
	m_flSwitchTime = 0.0f;
}

void CWeaponDeagle::ResetToTwoHands(void)
{
	m_iSwitchState = DEAGLE_STATE_TWO_HAND;
	m_flSwitchTime = 0.0f;

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	CBaseViewModel *pVM = pOwner->GetViewModel();
	if (!pVM)
		return;

	int iPoseParam = pVM->LookupPoseParameter(POSEPARAM_1HAND_SEQUENCE);
	if (iPoseParam == -1)
		return;

	pVM->SetPoseParameter(iPoseParam, 0.0f);
}

void CWeaponDeagle::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	CBaseViewModel *pVM = pOwner->GetViewModel();
	if (!pVM)
		return;

	if (pOwner->m_nButtons & IN_ATTACK2)
		SecondaryAttack();

	int state = m_iSwitchState.Get();
	if (state <= DEAGLE_STATE_ONE_HAND)
		return;

	int iParam = pVM->LookupPoseParameter(POSEPARAM_1HAND_SEQUENCE);
	if (iParam == -1)
	{
		m_iSwitchState.Set(DEAGLE_STATE_TWO_HAND);
		return;
	}

	float fraction = ((gpGlobals->curtime - m_flSwitchTime) / HAND_SWITCH_TIME);
	fraction = clamp(fraction, 0.0f, 1.0f);
	pVM->SetPoseParameter(iParam, (state == DEAGLE_STATE_SWITCH_ONE_HAND) ? fraction : (1.0f - fraction));

	if ((state == DEAGLE_STATE_SWITCH_ONE_HAND) && (fraction >= 1.0f))
		m_iSwitchState.Set(DEAGLE_STATE_ONE_HAND);

	if ((state == DEAGLE_STATE_SWITCH_TWO_HAND) && (fraction >= 1.0f))
		m_iSwitchState.Set(DEAGLE_STATE_TWO_HAND);
}

void CWeaponDeagle::SecondaryAttack(void)
{
	// m_bInReload || (m_flNextBashAttack > gpGlobals->curtime) || (m_flNextPrimaryAttack > gpGlobals->curtime) || 
	if (m_iSwitchState.Get() > DEAGLE_STATE_ONE_HAND)
		return;

	m_flSwitchTime = gpGlobals->curtime;

	if (m_iSwitchState.Get() <= DEAGLE_STATE_TWO_HAND)
		m_iSwitchState = DEAGLE_STATE_SWITCH_ONE_HAND;
	else
		m_iSwitchState = DEAGLE_STATE_SWITCH_TWO_HAND;
}

void CWeaponDeagle::StartHolsterSequence()
{
	ResetToTwoHands();
	BaseClass::StartHolsterSequence();
}

bool CWeaponDeagle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	ResetToTwoHands();
	return BaseClass::Holster(pSwitchingTo);
}

void CWeaponDeagle::Drop(const Vector &vecVelocity)
{
	ResetToTwoHands();
	BaseClass::Drop(vecVelocity);
}

bool CWeaponDeagle::Deploy(void)
{
	ResetToTwoHands();
	return BaseClass::Deploy();
}
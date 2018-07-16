//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie Hands Melee
//
//========================================================================================//

#include "cbase.h"
#include "weapon_melee_chargeable.h"

#if defined( CLIENT_DLL )
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponZombieHands C_WeaponZombieHands
#endif

class CWeaponZombieHands : public CHL2MPMeleeChargeable
{
public:
	DECLARE_CLASS(CWeaponZombieHands, CHL2MPMeleeChargeable);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

	CWeaponZombieHands();

	float GetDamageForActivity(Activity hitActivity);
	int GetMeleeSkillFlags(void) { return 0; }
	int GetMeleeDamageType() { return DMG_ZOMBIE; }
	int GetUniqueWeaponID() { return WEAPON_ID_ZOMBHANDS; }

	void PrimaryAttack(void);
	void Drop(const Vector &vecVelocity);

	bool VisibleInWeaponSelection() { return false; }

	Activity GetCustomActivity(int bIsSecondary);

private:
	CWeaponZombieHands(const CWeaponZombieHands &);
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZombieHands, DT_WeaponZombieHands )

	BEGIN_NETWORK_TABLE( CWeaponZombieHands, DT_WeaponZombieHands )
	END_NETWORK_TABLE()

	BEGIN_PREDICTION_DATA( CWeaponZombieHands )
	END_PREDICTION_DATA()

	LINK_ENTITY_TO_CLASS( weapon_zombhands, CWeaponZombieHands );
PRECACHE_WEAPON_REGISTER( weapon_zombhands );

acttable_t	CWeaponZombieHands::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH, false },

	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK, false },
	{ ACT_MP_RUN, ACT_HL2MP_RUN, false },
	{ ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_MELEE_ATTACK1,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_MELEE_ATTACK1,	false },

	{ ACT_MP_JUMP, ACT_HL2MP_JUMP, false },

	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK1, true }, 
	{ ACT_IDLE,				ACT_IDLE,	false }, 
	{ ACT_IDLE_ANGRY,		ACT_IDLE,	false }, 
};

IMPLEMENT_ACTTABLE(CWeaponZombieHands);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponZombieHands::CWeaponZombieHands( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponZombieHands::GetDamageForActivity( Activity hitActivity )
{
	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>(this);
	if (pWeapon)
	{
		float flDamage = (float)pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
		bool bSpecialAttack = (hitActivity == ACT_VM_CHARGE_ATTACK);
		if (bSpecialAttack)
			flDamage += GetChargeDamage();

		float flMultiplier = 0.0f;
		CHL2MP_Player *pClient = ToHL2MPPlayer(GetOwner());
		if (pClient)
		{
			flMultiplier = (flDamage / 100.0f) * (pClient->GetSkillValue(PLAYER_SKILL_ZOMBIE_DAMAGE, TEAM_DECEASED));

			float iZombieTeamBonusPercIncr = pClient->GetSkillValue(PLAYER_SKILL_ZOMBIE_MASS_INVASION, TEAM_DECEASED);
			if (pClient->m_BB2Local.m_iPerkTeamBonus && (iZombieTeamBonusPercIncr > 0.0f))
				flMultiplier += ((flMultiplier / 100.0f) * (iZombieTeamBonusPercIncr * (float)pClient->m_BB2Local.m_iPerkTeamBonus));

#ifndef CLIENT_DLL
			flMultiplier += pClient->GetTeamPerkValue(flMultiplier);
#endif
		}

		return (flDamage + flMultiplier);
	}

	return 20.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponZombieHands::Drop( const Vector &vecVelocity )
{
	ResetStates();

#ifndef CLIENT_DLL
	UTIL_Remove( this );
#endif
}

Activity CWeaponZombieHands::GetCustomActivity(int bIsSecondary)
{
	if (bIsSecondary)
		return ACT_VM_CHARGE_ATTACK;

	return ACT_VM_PRIMARYATTACK;
}

void CWeaponZombieHands::PrimaryAttack()
{
	if (m_iChargeState != 0)
		return;

	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	int swingAct = GetCustomActivity(false);
	WeaponSound(SINGLE);
	SendWeaponAnim(swingAct);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, swingAct);

	// Setup our next attack times GetFireRate() <- old
	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration() + GetFireRate();
}
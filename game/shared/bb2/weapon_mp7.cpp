//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: HK-MP7 SMG.
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "grenade_ar2.h"
#include "hl2mp_player.h"
#include "basegrenade_shared.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponMP7 C_WeaponMP7
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponMP7 : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CWeaponMP7, CHL2MPMachineGun);

	CWeaponMP7();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	int GetOverloadCapacity() { return 12; }
	int GetWeaponType(void) { return WEAPON_TYPE_SMG; }

	int		GetMinBurst() { return 1; }
	int		GetMaxBurst() { return 3; }

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	DECLARE_ACTTABLE();

#ifndef CLIENT_DLL
#ifdef BB2_AI
	int CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	void FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir);
	void Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary);
#endif //BB2_AI
#endif

private:
	CWeaponMP7(const CWeaponMP7 &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMP7, DT_WeaponMP7)

BEGIN_NETWORK_TABLE(CWeaponMP7, DT_WeaponMP7)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponMP7)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_mp7, CWeaponMP7);
PRECACHE_WEAPON_REGISTER(weapon_mp7);

acttable_t CWeaponMP7::m_acttable[] =
{
#ifdef BB2_AI
	{ ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE_PISTOL, false },
	{ ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH_PISTOL, false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_PISTOL, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_PISTOL, false },
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

IMPLEMENT_ACTTABLE(CWeaponMP7);

//=========================================================
CWeaponMP7::CWeaponMP7()
{
}

#ifdef BB2_AI
#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP7::FireNPCPrimaryAttack(CBaseCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir)
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(GetWpnData().m_iPellets, vecShootOrigin, vecShootDir, GetBulletSpread(),
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0);

	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP7::Operator_ForceNPCFire(CBaseCombatCharacter *pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP7::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_AR2:
	case EVENT_WEAPON_SMG1:
	{
		Vector vecShootOrigin, vecShootDir;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
		{
			vecShootOrigin = pOperator->Weapon_ShootPosition();
		}

		CAI_BaseNPC *npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

		FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
	break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif
#endif //BB2_AI

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponMP7::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0, 0.75 },
		{ 5.00, 0.75 },
		{ 10.0 / 3.0, 0.75 },
		{ 5.0 / 3.0, 0.75 },
		{ 1.00, 1.0 },
	};

	COMPILE_TIME_ASSERT(ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
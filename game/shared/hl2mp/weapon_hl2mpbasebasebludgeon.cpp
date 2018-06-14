//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Melee HL2MP base wep class.
//
//==============================================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasebasebludgeon.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "GameBase_Shared.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#include "ilagcompensationmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(BaseHL2MPBludgeonWeapon, DT_BaseHL2MPBludgeonWeapon)

BEGIN_NETWORK_TABLE(CBaseHL2MPBludgeonWeapon, DT_BaseHL2MPBludgeonWeapon)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CBaseHL2MPBludgeonWeapon)
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBaseHL2MPBludgeonWeapon::CBaseHL2MPBludgeonWeapon()
{
	m_bFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the weapon
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Spawn(void)
{
	m_fMinRange1 = 0;
	m_fMinRange2 = 0;
	m_fMaxRange1 = 64;
	m_fMaxRange2 = 64;
	
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache the weapon
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Precache(void)
{
	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;

	BaseClass::HandleWeaponSelectionTime();
	BaseClass::MeleeAttackUpdate();

	bool bCanAttack = ((m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime));

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		if (CanDoPrimaryAttack())
			PrimaryAttack();
	}
	else if ((pOwner->m_nButtons & IN_ATTACK2) && bCanAttack)
	{
		if (CanDoSecondaryAttack())
			SecondaryAttack();
	}
	else
	{
		WeaponIdle();
	}

	if ((m_flNextSecondaryAttack < gpGlobals->curtime) && (m_flMeleeCooldown > 0.0f))
		m_flMeleeCooldown = 0.0f;
}

float CBaseHL2MPBludgeonWeapon::GetFireRate(void)
{
	return GetWpnData().m_flFireRate;
}

float CBaseHL2MPBludgeonWeapon::GetRange(void)
{
	return ((float)GetWpnData().m_iRangeMax);
}

float CBaseHL2MPBludgeonWeapon::GetSpecialAttackDamage(void)
{
	return GetSpecialDamage();
}

float CBaseHL2MPBludgeonWeapon::GetDamageForActivity(Activity hitActivity)
{
	float flNewDamage = GetHL2MPWpnData().m_iPlayerDamage;
	
	CHL2MP_Player *pClient = ToHL2MPPlayer(this->GetOwner());
	if (pClient)
	{
		int iTeamBonus = (pClient->m_BB2Local.m_iPerkTeamBonus - 5);
		flNewDamage += ((flNewDamage / 100.0f) * (((float)pClient->GetSkillValue(PLAYER_SKILL_HUMAN_MELEE_MASTER)) * GetWpnData().m_flSkillDamageFactor));

		if (pClient->IsPerkFlagActive(PERK_HUMAN_BLOODRAGE))
			flNewDamage += ((flNewDamage / 100.0f) * (pClient->GetSkillValue(PLAYER_SKILL_HUMAN_BLOOD_RAGE, TEAM_HUMANS)));

		if (pClient->IsPerkFlagActive(PERK_POWERUP_CRITICAL))
		{
			const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(PERK_POWERUP_CRITICAL);
			if (data)
				flNewDamage += ((flNewDamage / 100.0f) * data->flExtraFactor);
		}

		if (iTeamBonus > 0)
			flNewDamage += ((flNewDamage / 100.0f) * (iTeamBonus * GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->iTeamBonusDamageIncrease));
	}

	bool bSpecialAttack = (hitActivity >= ACT_VM_SPECIALATTACK0 && hitActivity <= ACT_VM_SPECIALATTACK10);
	if (bSpecialAttack)
		flNewDamage += GetSpecialAttackDamage();

	return flNewDamage;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::PrimaryAttack()
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	int swingAct = GetCustomActivity(false);
	WeaponSound(SINGLE);
	SendWeaponAnim(swingAct);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, swingAct);

	// Setup our next attack times GetFireRate() <- old
	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::SecondaryAttack()
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	int swingAct = GetCustomActivity(true);
	WeaponSound(SINGLE);
	SendWeaponAnim(swingAct);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, swingAct);

	// Setup our next attack times GetFireRate() <- old
	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SpecialPunishTime() + GetViewModelSequenceDuration();
	m_flMeleeCooldown = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::ImpactEffect(trace_t &traceHit)
{
	// See if we hit water (we don't do the other impact effects in this case)
	if (ImpactWater(traceHit.startpos, traceHit.endpos))
		return;

	UTIL_ImpactTrace(&traceHit, GetMeleeDamageType());
}

Activity CBaseHL2MPBludgeonWeapon::GetCustomActivity(int bIsSecondary)
{
	int iActivity = ((bIsSecondary >= 1) ? ACT_VM_SPECIALATTACK0 : ACT_VM_PRIMARYATTACK0);

	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (pOwner && (pOwner->GetTeamNumber() == TEAM_HUMANS))
		iActivity += pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_MELEE_SPEED);

	if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
		iActivity = ((bIsSecondary >= 1) ? ACT_VM_SPECIALATTACK10 : ACT_VM_PRIMARYATTACK10);

	return (Activity)iActivity;
}

float CBaseHL2MPBludgeonWeapon::SpecialPunishTime()
{
	return GetWpnData().m_flSecondaryAttackCooldown;
}

void CBaseHL2MPBludgeonWeapon::AddViewKick(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer == NULL)
		return;

	QAngle punchAng;

	punchAng.x = SharedRandomFloat("crowbarpax", 1.0f, 2.0f);
	punchAng.y = SharedRandomFloat("crowbarpay", -2.0f, -1.0f);
	punchAng.z = 0.0f;

	pPlayer->ViewPunch(punchAng);
}

#ifndef CLIENT_DLL
int CBaseHL2MPBludgeonWeapon::WeaponMeleeAttack1Condition(float flDot, float flDist)
{
	CAI_BaseNPC *pNPC = GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity();

	// Project where the enemy will be in a little while
	float dt = 0.9;
	dt += SharedRandomFloat("crowbarmelee1", -0.3f, 0.2f);
	if (dt < 0.0f)
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA(pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos);

	Vector vecDelta;
	VectorSubtract(vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta);

	if (fabs(vecDelta.z) > (GetRange() * 1.15f))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D();
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize(vecDelta.AsVector2D());
	if ((flDist > GetRange()) && (flExtrapolatedDist > GetRange()))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D(vecDelta.AsVector2D(), vecForward.AsVector2D());
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::HandleAnimEventMeleeHit(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors(GetAbsAngles(), &vecDirection);

#ifdef BB2_AI
	CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if (pEnemy)
	{
		Vector vecDelta;
		VectorSubtract(pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta);
		VectorNormalize(vecDelta);

		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize(vecDelta2D);
		if (DotProduct2D(vecDelta2D, vecDirection.AsVector2D()) > 0.8f)
		{
			vecDirection = vecDelta;
		}
	}
#endif //BB2_AI

	Vector vecEnd;
	VectorMA(pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd);
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack(pOperator->Weapon_ShootPosition(), vecEnd,
		Vector(-16, -16, -16), Vector(36, 36, 36), GetDamageForActivity(GetActivity()), GetMeleeDamageType(), 0.75);

	// did I hit someone?
	if (pHurt)
	{
		// play sound
		WeaponSound(MELEE_HIT);

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine(pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit);
		ImpactEffect(traceHit);
	}
	else
	{
		WeaponSound(MELEE_MISS);
	}
}

//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CBaseHL2MPBludgeonWeapon::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_MELEE_HIT:
	case EVENT_WEAPON_MELEE_SWISH:
		HandleAnimEventMeleeHit(pEvent, pOperator);
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

int CBaseHL2MPBludgeonWeapon::GetMeleeSkillFlags(void)
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return 0;

	int iSkillFlags = 0;

	if (pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_BLEED) > 0)
	{
		int iPercChance = (int)pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_BLEED, TEAM_HUMANS);
		if (random->RandomInt(0, 100) <= iPercChance)
			iSkillFlags |= SKILL_FLAG_BLEED;
	}

	if (pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_CRIPPLING_BLOW) > 0)
	{
		int iPercChance = (int)pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_CRIPPLING_BLOW, TEAM_HUMANS);
		if (random->RandomInt(0, 100) <= iPercChance)
		{
			iSkillFlags |= SKILL_FLAG_CRIPPLING_BLOW;
		}
	}

	return iSkillFlags;
}
#endif
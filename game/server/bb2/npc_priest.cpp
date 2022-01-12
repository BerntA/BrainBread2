//=========       Copyright © Reperio Studios 2022 @ Bernt Andreas Eide!       ============//
//
// Purpose: Priest - Crazy preacher
//
//========================================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_base_soldier.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "soundenvelope.h"
#include "hl2mp_gamerules.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPCPriest : public CNPC_BaseSoldier
{
	DECLARE_CLASS(CNPCPriest, CNPC_BaseSoldier);

public:

	void		Spawn(void);
	void		Precache(void);
	void		DeathSound(const CTakeDamageInfo &info);
	void		BuildScheduleTestBits(void);
	int			SelectCombatSchedule();
	void		OnListened();
	int			OnTakeDamage(const CTakeDamageInfo &info);
	void		ClearAttackConditions(void);
	void		NotifyDeadFriend(CBaseEntity* pFriend);
	void		AnnounceEnemyKill(CBaseEntity *pEnemy);
	bool		IsLightDamage(const CTakeDamageInfo &info);
	bool		IsHeavyDamage(const CTakeDamageInfo &info);
	bool		AllowedToIgnite(void) { return false; }
	bool		IsBoss() { return true; }
	bool		CanAlwaysSeePlayers() { return true; }
	bool		GetGender() { return true; } // force male
	bool		ShouldAlwaysThink() { return true; }
	bool		CanThrowGrenade(const Vector &vecTarget);
	int			AllowEntityToBeGibbed(void) { return GIB_NO_GIBS; }
	Activity	NPC_TranslateActivity(Activity eNewActivity);
	void		FireBullets(const FireBulletsInfo_t &info);
	Class_T		Classify(void);
	BB2_SoundTypes GetNPCType() { return TYPE_CUSTOM; }
	int			GetNPCClassType() { return NPC_CLASS_PRIEST; }
};

LINK_ENTITY_TO_CLASS(npc_priest, CNPCPriest);

int CNPCPriest::OnTakeDamage(const CTakeDamageInfo &info)
{
	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
	return BaseClass::OnTakeDamage(info);
}

void CNPCPriest::Spawn(void)
{
	Precache();

	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);
	AddSpawnFlags(SF_NPC_LONG_RANGE);

	BaseClass::Spawn();

	m_iNumGrenades = 2000;
	SetCollisionGroup(COLLISION_GROUP_NPC_MERCENARY);
	CapabilitiesRemove(bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_MOVE_JUMP);

	string_t cWeaponEnt = FindPooledString("weapon_winchester1894");
	GiveWeapon((cWeaponEnt == NULL_STRING) ? AllocPooledString("weapon_winchester1894") : cWeaponEnt);
}

void CNPCPriest::Precache()
{
	UTIL_PrecacheOther("weapon_frag");
	BaseClass::Precache();
}

void CNPCPriest::DeathSound(const CTakeDamageInfo &info)
{
	// NOTE: The response system deals with this at the moment
	if (GetFlags() & FL_DISSOLVING)
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

void CNPCPriest::ClearAttackConditions()
{
	bool fCanRangeAttack2 = HasCondition(COND_CAN_RANGE_ATTACK2);

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if (fCanRangeAttack2)
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition(COND_CAN_RANGE_ATTACK2);
	}
}

void CNPCPriest::NotifyDeadFriend(CBaseEntity* pFriend)
{
	if (FInViewCone(pFriend) && FVisible(pFriend))
		HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());

	BaseClass::NotifyDeadFriend(pFriend);
}

void CNPCPriest::AnnounceEnemyKill(CBaseEntity *pEnemy)
{
	if (!pEnemy)
		return;

	if (pEnemy->IsPlayer())
		HL2MPRules()->EmitSoundToClient(this, "PlayerDown", GetNPCType(), GetGender());
	else
		BaseClass::AnnounceEnemyKill(pEnemy);
}

int CNPCPriest::SelectCombatSchedule()
{
	if (HasCondition(COND_SEE_ENEMY) && CanGrenadeEnemy())
		return SCHED_RANGE_ATTACK2;

	return BaseClass::SelectCombatSchedule();
}

void CNPCPriest::BuildScheduleTestBits(void)
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if (m_flGroundSpeed == 0.0 && !IsCurSchedule(SCHED_FLINCH_PHYSICS))
		SetCustomInterruptCondition(COND_HEAR_PHYSICS_DANGER);

	BaseClass::BuildScheduleTestBits();
}

void CNPCPriest::OnListened()
{
	BaseClass::OnListened();

	if (HasCondition(COND_HEAR_DANGER) && HasCondition(COND_HEAR_PHYSICS_DANGER) && HasInterruptCondition(COND_HEAR_DANGER))
		ClearCondition(COND_HEAR_PHYSICS_DANGER);
}

bool CNPCPriest::IsLightDamage(const CTakeDamageInfo &info)
{
	return false;
}

bool CNPCPriest::IsHeavyDamage(const CTakeDamageInfo &info)
{
	return false;
}

bool CNPCPriest::CanThrowGrenade(const Vector &vecTarget)
{
	// Not allowed to throw another grenade right now.
	if (gpGlobals->curtime < m_flNextGrenadeCheck)
		return false;

	// Too far away!
	float flDist = (vecTarget - GetAbsOrigin()).Length();
	if (flDist > 1024.0f)
	{
		m_flNextGrenadeCheck = gpGlobals->curtime + 0.5f;
		return false;
	}

	// If moving, don't check.
	if (m_flGroundSpeed != 0)
		return false;

	return CheckCanThrowGrenade(vecTarget);
}

Activity CNPCPriest::NPC_TranslateActivity(Activity activity)
{
	switch (activity)
	{
	case ACT_IDLE:
	case ACT_IDLE_ANGRY:
	case ACT_IDLE_ANGRY_SHOTGUN:
	case ACT_IDLE_SMG1:
		return ACT_MONK_GUN_IDLE;

	case ACT_RUN_AIM_SHOTGUN:
		return ACT_RUN_AIM_RIFLE;

	case ACT_WALK_AIM_SHOTGUN:
		return ACT_WALK_AIM_RIFLE;

	case ACT_RANGE_ATTACK2:
		return ACT_RANGE_ATTACK_THROW;

	default:
		return BaseClass::NPC_TranslateActivity(activity);
	}
}

void CNPCPriest::FireBullets(const FireBulletsInfo_t &info)
{
	FireBulletsInfo_t adjustedInfo = info;
	adjustedInfo.m_iShots = 5;
	adjustedInfo.m_vecSpread = VECTOR_CONE_6DEGREES;
	BaseClass::FireBullets(adjustedInfo);
}

Class_T	CNPCPriest::Classify(void)
{
	return CLASS_MILITARY;
}
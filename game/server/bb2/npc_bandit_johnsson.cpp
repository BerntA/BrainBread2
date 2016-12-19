//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Johnsson - King of the junkyard.
//
//========================================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_combine.h"
#include "bitstring.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "hl2/hl2_player.h"
#include "game.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "Sprite.h"
#include "soundenvelope.h"
#include "hl2mp_gamerules.h"
#include "vehicle_base.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPCBanditJohnsson : public CNPC_Combine
{
	DECLARE_DATADESC();
	DECLARE_CLASS(CNPCBanditJohnsson, CNPC_Combine);

public:

	void		Spawn(void);
	void		Precache(void);
	void		DeathSound(const CTakeDamageInfo &info);
	void		PrescheduleThink(void);
	void		BuildScheduleTestBits(void);
	int			SelectSchedule(void);
	float		GetHitgroupDamageMultiplier(int iHitGroup, const CTakeDamageInfo &info);
	void		HandleAnimEvent(animevent_t *pEvent);
	void		OnChangeActivity(Activity eNewActivity);
	void		Event_Killed(const CTakeDamageInfo &info);
	void		OnListened();
	int OnTakeDamage(const CTakeDamageInfo &info);

	void		ClearAttackConditions(void);

	void NotifyDeadFriend(CBaseEntity* pFriend);
	void AnnounceEnemyKill(CBaseEntity *pEnemy);

	bool		m_fIsBlocking;

	bool		IsLightDamage(const CTakeDamageInfo &info);
	bool		IsHeavyDamage(const CTakeDamageInfo &info);

	bool ShouldChargePlayer();

	bool AllowedToIgnite(void) { return false; }
	bool IsBoss() { return true; }
	bool CanAlwaysSeePlayers() { return true; }
	bool GetGender() { return true; } // force male
	bool UsesNavMesh(void) { return true; }
	int AllowEntityToBeGibbed(void) { return GIB_NO_GIBS; }

	Class_T Classify(void);
	BB2_SoundTypes GetNPCType() { return TYPE_CUSTOM; }

	const char *GetNPCName() { return "Johnsson"; }

private:

	float m_flHealthFractionToCheck;

	void InputEnteredHideout(inputdata_t &inputdata);
	void InputLeftHideout(inputdata_t &inputdata);

	COutputEvent m_OnLostQuarterOfHealth;
};

LINK_ENTITY_TO_CLASS(npc_bandit_johnsson, CNPCBanditJohnsson);

BEGIN_DATADESC(CNPCBanditJohnsson)
DEFINE_FIELD(m_flHealthFractionToCheck, FIELD_FLOAT),
DEFINE_INPUTFUNC(FIELD_VOID, "EnterHideout", InputEnteredHideout),
DEFINE_INPUTFUNC(FIELD_VOID, "LeaveHideout", InputLeftHideout),
DEFINE_OUTPUT(m_OnLostQuarterOfHealth, "OnLostQuarterHealth"),
END_DATADESC()

#define AE_SOLDIER_BLOCK_PHYSICS 20 // trying to block an incoming physics object...
#define TELEPORT_PARTICLE "bb2_healing_effect"

//-----------------------------------------------------------------------------
// Purpose: Take Damage
//-----------------------------------------------------------------------------
int CNPCBanditJohnsson::OnTakeDamage(const CTakeDamageInfo &info)
{
	HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
	int tookDamage = BaseClass::OnTakeDamage(info);
	return tookDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::Spawn(void)
{
	Precache();

	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);
	AddSpawnFlags(SF_NPC_LONG_RANGE);

	BaseClass::Spawn();

	SetCollisionGroup(COLLISION_GROUP_NPC_MERCENARY);

	m_iNumGrenades = 2000;

	// No kicking for Johnsson.
	CapabilitiesRemove(bits_CAP_INNATE_MELEE_ATTACK1);

	m_flHealthFractionToCheck = 75.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::Precache()
{
	UTIL_PrecacheOther("weapon_frag");
	PrecacheParticleSystem(TELEPORT_PARTICLE);
	BaseClass::Precache();
}

void CNPCBanditJohnsson::DeathSound(const CTakeDamageInfo &info)
{
	// NOTE: The response system deals with this at the moment
	if (GetFlags() & FL_DISSOLVING)
		return;

	HL2MPRules()->EmitSoundToClient(this, "Die", GetNPCType(), GetGender());
}

//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::ClearAttackConditions()
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

void CNPCBanditJohnsson::NotifyDeadFriend(CBaseEntity* pFriend)
{
	if (FInViewCone(pFriend) && FVisible(pFriend))
	{
		if (m_flHealthFractionToCheck <= 25)
			HL2MPRules()->EmitSoundToClient(this, "TauntRage", GetNPCType(), GetGender());
		else
			HL2MPRules()->EmitSoundToClient(this, "Taunt", GetNPCType(), GetGender());
	}

	BaseClass::NotifyDeadFriend(pFriend);
}

void CNPCBanditJohnsson::AnnounceEnemyKill(CBaseEntity *pEnemy)
{
	if (!pEnemy)
		return;

	if (pEnemy->IsPlayer())
	{
		if (m_flHealthFractionToCheck <= 25)
			HL2MPRules()->EmitSoundToClient(this, "PlayerDownRage", GetNPCType(), GetGender());
		else
			HL2MPRules()->EmitSoundToClient(this, "PlayerDown", GetNPCType(), GetGender());
	}
	else
		BaseClass::AnnounceEnemyKill(pEnemy);
}

void CNPCBanditJohnsson::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	float maxHealth = ((float)m_iTotalHP);
	float currHealth = ((float)GetHealth());
	if ((currHealth < (maxHealth * (m_flHealthFractionToCheck / 100.0f))) && (m_flHealthFractionToCheck > 0.0f))
	{
		HL2MPRules()->EmitSoundToClient(this, "Retreat", GetNPCType(), GetGender());

#ifdef BB2_AI
		IPredictionSystem::SuppressHostEvents(NULL);
#endif //BB2_AI

		DispatchParticleEffect(TELEPORT_PARTICLE, GetAbsOrigin(), GetAbsAngles(), this);
		m_flHealthFractionToCheck -= 25.0f;

		m_OnLostQuarterOfHealth.FireOutput(this, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::BuildScheduleTestBits(void)
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if (m_flGroundSpeed == 0.0 && !IsCurSchedule(SCHED_FLINCH_PHYSICS))
	{
		SetCustomInterruptCondition(COND_HEAR_PHYSICS_DANGER);
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPCBanditJohnsson::SelectSchedule(void)
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPCBanditJohnsson::GetHitgroupDamageMultiplier(int iHitGroup, const CTakeDamageInfo &info)
{
	return BaseClass::GetHitgroupDamageMultiplier(iHitGroup, info);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::HandleAnimEvent(animevent_t *pEvent)
{
	switch (pEvent->event)
	{
	case AE_SOLDIER_BLOCK_PHYSICS:
		m_fIsBlocking = true;
		break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

void CNPCBanditJohnsson::OnChangeActivity(Activity eNewActivity)
{
	// Any new sequence stops us blocking.
	m_fIsBlocking = false;

	BaseClass::OnChangeActivity(eNewActivity);
}

void CNPCBanditJohnsson::OnListened()
{
	BaseClass::OnListened();

	if (HasCondition(COND_HEAR_DANGER) && HasCondition(COND_HEAR_PHYSICS_DANGER))
	{
		if (HasInterruptCondition(COND_HEAR_DANGER))
		{
			ClearCondition(COND_HEAR_PHYSICS_DANGER);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPCBanditJohnsson::Event_Killed(const CTakeDamageInfo &info)
{
	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPCBanditJohnsson::IsLightDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsLightDamage(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPCBanditJohnsson::IsHeavyDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsHeavyDamage(info);
}

bool CNPCBanditJohnsson::ShouldChargePlayer()
{
	return GetEnemy() && GetEnemy()->IsPlayer();
}

Class_T	CNPCBanditJohnsson::Classify(void)
{
	return CLASS_MILITARY;
}

void CNPCBanditJohnsson::InputEnteredHideout(inputdata_t &inputdata)
{
	// Remove Skill Affections:
	if (m_pActiveSkillEffects.Count())
	{
		if (IsAffectedBySkillFlag(SKILL_FLAG_CRIPPLING_BLOW))
			SetFrozen();

		if (IsAffectedBySkillFlag(SKILL_FLAG_COLDSNAP) && IsMoving())
		{
			GetMotor()->MoveStop();
			GetMotor()->MoveStart();
		}

		m_nMaterialOverlayFlags = 0;
		m_pActiveSkillEffects.Purge();
	}

	if (m_flHealthFractionToCheck <= 25)
	{
		bool bGiveNewWeapon = true;
		CBaseCombatWeapon *pWeapon = GetActiveWeapon();
		if (pWeapon && FClassnameIs(pWeapon, "weapon_minigun"))
			bGiveNewWeapon = false;

		if (bGiveNewWeapon)
		{
			string_t cWeaponEnt = FindPooledString("weapon_minigun");
			if (cWeaponEnt == NULL_STRING)
				cWeaponEnt = AllocPooledString("weapon_minigun");

			GiveWeapon(cWeaponEnt);
		}

		HL2MPRules()->EmitSoundToClient(this, "EnterHideoutRage", GetNPCType(), GetGender());
		return;
	}

	HL2MPRules()->EmitSoundToClient(this, "EnterHideout", GetNPCType(), GetGender());
}

void CNPCBanditJohnsson::InputLeftHideout(inputdata_t &inputdata)
{
	if (m_flHealthFractionToCheck <= 25)
	{
		HL2MPRules()->EmitSoundToClient(this, "LeaveHideoutRage", GetNPCType(), GetGender());
		return;
	}

	HL2MPRules()->EmitSoundToClient(this, "LeaveHideout", GetNPCType(), GetGender());
}
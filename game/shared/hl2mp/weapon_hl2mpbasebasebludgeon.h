//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Melee HL2MP base wep class.
//
//==============================================================================================//

#ifndef BASEBLUDGEONWEAPON_H
#define BASEBLUDGEONWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#if defined( CLIENT_DLL )
#define CBaseHL2MPBludgeonWeapon C_BaseHL2MPBludgeonWeapon
#endif

class CBaseHL2MPBludgeonWeapon : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS(CBaseHL2MPBludgeonWeapon, CBaseHL2MPCombatWeapon);
public:
	CBaseHL2MPBludgeonWeapon();
	CBaseHL2MPBludgeonWeapon(const CBaseHL2MPBludgeonWeapon &);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL 
	virtual int CapabilitiesGet(void) { return bits_CAP_WEAPON_MELEE_ATTACK1; }
	virtual int WeaponMeleeAttack1Condition(float flDot, float flDist);

	// Animation event
	virtual void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
	virtual void HandleAnimEventMeleeHit(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	virtual int GetMeleeSkillFlags(void);
#endif 

	virtual	void	Spawn(void);
	virtual	void	Precache(void);

	//Attack functions
	virtual	void	PrimaryAttack(void);
	virtual	void	SecondaryAttack(void);

	virtual void	ItemPostFrame(void);

	//Functions to select animation sequences 
	virtual Activity	GetPrimaryAttackActivity(void)	{ return ACT_VM_HITCENTER; }
	virtual Activity	GetSecondaryAttackActivity(void)	{ return ACT_VM_HITCENTER2; }

	virtual	float	GetFireRate(void);
	virtual float   GetRange(void);
	virtual float   GetSpecialAttackDamage(void);
	virtual	float	GetDamageForActivity(Activity hitActivity);

	virtual void AddViewKick(void);

	// Allows a custom Primary Attack Activity, this can be randomized (logic).
	virtual Activity GetCustomActivity(int bIsSecondary);

protected:
	virtual	void	ImpactEffect(trace_t &trace);

	// Do we have what it takes to do this attack?
	virtual bool CanDoPrimaryAttack() { return true; }
	virtual bool CanDoSecondaryAttack() { return true; }
	// Time to wait on doing special attacks (adds to rate of fire).
	virtual float SpecialPunishTime();
};
#endif
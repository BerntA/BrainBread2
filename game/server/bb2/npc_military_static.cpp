//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Static Military - Friendly fella...
//
//========================================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_base_soldier_static.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "hl2/hl2_player.h"
#include "ammodef.h"
#include "ai_memory.h"
#include "soundenvelope.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPCMilitaryStatic : public CNPCBaseSoldierStatic
{
	DECLARE_CLASS(CNPCMilitaryStatic, CNPCBaseSoldierStatic);

public:
	void		Spawn(void);
	void		Precache(void);
	void		DeathSound(const CTakeDamageInfo &info);
	int         OnTakeDamage(const CTakeDamageInfo &info);
	void		ClearAttackConditions(void);
	bool		IsHeavyDamage(const CTakeDamageInfo &info);
	bool		AllowedToIgnite(void) { return true; }
	const char *GetNPCName() { return "Military"; }
};

LINK_ENTITY_TO_CLASS(npc_military_static, CNPCMilitaryStatic);

//-----------------------------------------------------------------------------
// Purpose: BB2 No dmg from plr or teamkilling for the military bois.
//-----------------------------------------------------------------------------
int CNPCMilitaryStatic::OnTakeDamage(const CTakeDamageInfo &info)
{
	CBaseEntity *pAttacker = info.GetAttacker();
	if (pAttacker != this)
	{
		if (pAttacker && pAttacker->IsHuman())
		{
			HL2MPRules()->EmitSoundToClient(this, "FriendlyFire", GetNPCType(), GetGender());
			return 0;
		}
		else
			HL2MPRules()->EmitSoundToClient(this, "Pain", GetNPCType(), GetGender());
	}
	return BaseClass::OnTakeDamage(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPCMilitaryStatic::Spawn(void)
{
	Precache();

	BaseClass::Spawn();

	SetCollisionGroup(COLLISION_GROUP_NPC_MILITARY);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPCMilitaryStatic::Precache()
{
	UTIL_PrecacheOther("weapon_frag");
	BaseClass::Precache();
}

void CNPCMilitaryStatic::DeathSound(const CTakeDamageInfo &info)
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
void CNPCMilitaryStatic::ClearAttackConditions()
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPCMilitaryStatic::IsHeavyDamage(const CTakeDamageInfo &info)
{
	if (info.GetDamageType() & DMG_BUCKSHOT)
		return true;

	return BaseClass::IsHeavyDamage(info);
}
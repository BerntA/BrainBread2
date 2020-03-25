//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Ammo Refill Box.
//
//========================================================================================//

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"
#include "ammodef.h"
#include "eventlist.h"
#include "npcevent.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ==================================================================
// Ammo crate which will supply infinite ammo of any ammo type.
// ==================================================================
class CItem_AmmoCrate : public CBaseAnimating
{
public:
	DECLARE_CLASS(CItem_AmmoCrate, CBaseAnimating);

	void	Spawn(void);
	void	Precache(void);
	bool	CreateVPhysics(void);

	virtual void HandleAnimEvent(animevent_t *pEvent);

	void	OnRestore(void);

	//FIXME: May not want to have this used in a radius
	int		ObjectCaps(void) { return (BaseClass::ObjectCaps() | (FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS)); };
	void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void	InputKill(inputdata_t &data);
	void	CrateThink(void);

	virtual int OnTakeDamage(const CTakeDamageInfo &info);

protected:

	float	m_flCloseTime;
	COutputEvent	m_OnUsed;
	CHandle< CBasePlayer > m_hActivator;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(item_ammo_crate, CItem_AmmoCrate);

BEGIN_DATADESC(CItem_AmmoCrate)
DEFINE_FIELD(m_flCloseTime, FIELD_FLOAT),
DEFINE_FIELD(m_hActivator, FIELD_EHANDLE),
DEFINE_OUTPUT(m_OnUsed, "OnUsed"),
DEFINE_INPUTFUNC(FIELD_VOID, "Kill", InputKill),
DEFINE_THINKFUNC(CrateThink),
END_DATADESC()

#define	AMMO_CRATE_CLOSE_DELAY	1.5f
#define AMMOCRATE_MODEL_DEFAULT "models/items/ammo_box.mdl"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::Spawn(void)
{
	Precache();

	BaseClass::Spawn();

	SetModel(AMMOCRATE_MODEL_DEFAULT);
	SetMoveType(MOVETYPE_NONE);
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	SetSolid(SOLID_BBOX);
	SetBlocksLOS(false);
	AddEFlags(EFL_NO_ROTORWASH_PUSH);
	AddSolidFlags(FSOLID_TRIGGER | GetSolidFlags());

	//SetSolid(SOLID_VPHYSICS);
	//CreateVPhysics();

	// Bloat the box for player pickup
	const model_t *pModel = modelinfo->GetModel(this->GetModelIndex());
	if (pModel)
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds(pModel, mins, maxs);
		this->SetCollisionBounds(mins, maxs);
	}

	CollisionProp()->UseTriggerBounds(true, 10.0f);

	ResetSequence(LookupSequence("Idle"));
	SetBodygroup(1, true);

	m_flCloseTime = gpGlobals->curtime;
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle(0);

	m_takedamage = DAMAGE_EVENTS_ONLY;

	color32 col32 = { 10, 200, 14, 240 };
	m_GlowColor = col32;

	SetGlowMode(GLOW_MODE_RADIUS);
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CItem_AmmoCrate::CreateVPhysics(void)
{
	return (VPhysicsInitStatic() != NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::Precache(void)
{
	PrecacheModel(AMMOCRATE_MODEL_DEFAULT);
	PrecacheScriptSound("AmmoCrate.Open");
	PrecacheScriptSound("AmmoCrate.Close");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::OnRestore(void)
{
	BaseClass::OnRestore();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = ToBasePlayer(pActivator);
	if (pPlayer == NULL)
		return;

	if (!pPlayer->IsHuman())
		return;

	int timeLeft = ((int)(pPlayer->m_flNextResupplyTime - gpGlobals->curtime));
	if (timeLeft > 0)
	{
		char pchArg1[16];
		Q_snprintf(pchArg1, 16, "%i", timeLeft);
		GameBaseServer()->SendToolTip("#TOOLTIP_AMMOSUPPLY_WAIT", "", 1.4f, GAME_TIP_WARNING, pPlayer->entindex(), pchArg1);
		return;
	}

	m_OnUsed.FireOutput(pActivator, this);

	int iSequence = LookupSequence("Open");

	// See if we're not opening already
	if (GetSequence() != iSequence)
	{
		m_hActivator = pPlayer;

		// Animate!
		ResetSequence(iSequence);

		// Make sound
		CPASAttenuationFilter sndFilter(this, "AmmoCrate.Open");
		EmitSound(sndFilter, entindex(), "AmmoCrate.Open");

		// Start thinking to make it return
		SetThink(&CItem_AmmoCrate::CrateThink);
		SetNextThink(gpGlobals->curtime + 0.1f);
	}

	// Don't close again for two seconds
	m_flCloseTime = gpGlobals->curtime + AMMO_CRATE_CLOSE_DELAY;
}

//-----------------------------------------------------------------------------
// Purpose: No damage!
//-----------------------------------------------------------------------------
int CItem_AmmoCrate::OnTakeDamage(const CTakeDamageInfo &info)
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific messages that occur when tagged
//			animation frames are played.
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->event == AE_AMMOCRATE_PICKUP_AMMO)
	{
		CBasePlayer *pActivator = m_hActivator.Get();
		if (pActivator)
		{
			bool bUsed = false;
			for (int i = 0; i < MAX_WEAPONS; i++)
			{
				CBaseCombatWeapon *pWeapon = pActivator->GetWeapon(i);
				if ((pWeapon == NULL) || (pWeapon->GetWeaponType() == WEAPON_TYPE_SPECIAL) || (pWeapon->GetAmmoTypeID() == -1) || !pWeapon->UsesClipsForAmmo())
					continue;

				if (pWeapon->GiveAmmo(pWeapon->GetAmmoMaxCarry()))
					bUsed = true;
			}

			if (bUsed)
			{
				SetBodygroup(1, false);
				if (!GameBaseServer()->IsTutorialModeEnabled())
					pActivator->m_flNextResupplyTime = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flAmmoResupplyTime;
			}

			pActivator = NULL;
		}

		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::CrateThink(void)
{
	StudioFrameAdvance();
	DispatchAnimEvents(this);

	SetNextThink(gpGlobals->curtime + 0.1f);

	// Start closing if we're not already
	if (GetSequence() != LookupSequence("Close"))
	{
		// Not ready to close?
		if (m_flCloseTime <= gpGlobals->curtime)
		{
			m_hActivator = NULL;

			ResetSequence(LookupSequence("Close"));
		}
	}
	else
	{
		// See if we're fully closed
		if (IsSequenceFinished())
		{
			// Stop thinking
			SetThink(NULL);
			CPASAttenuationFilter sndFilter(this, "AmmoCrate.Close");
			EmitSound(sndFilter, entindex(), "AmmoCrate.Close");

			// FIXME: We're resetting the sequence here
			// but setting Think to NULL will cause this to never have
			// StudioFrameAdvance called. What are the consequences of that?
			ResetSequence(LookupSequence("Idle"));
			SetBodygroup(1, true);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_AmmoCrate::InputKill(inputdata_t &data)
{
	UTIL_Remove(this);
}
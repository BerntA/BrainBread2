//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Player Class
//
//========================================================================================//

#include "cbase.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "prediction.h"
#define CRecipientFilter C_RecipientFilter
#else
#include "hl2mp_player.h"
#include "ilagcompensationmanager.h"
#include "ai_basenpc.h"
#endif

#include "GameBase_Shared.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_footsteps;

void PrecacheFootStepSounds(void)
{
	// Precache armor / clothing sounds.
	CBaseEntity::PrecacheScriptSound("Player.Foley_Clothing");
	CBaseEntity::PrecacheScriptSound("Player.Foley_Light_Armor");
	CBaseEntity::PrecacheScriptSound("Player.Foley_Medium_Armor");
	CBaseEntity::PrecacheScriptSound("Player.Foley_Heavy_Armor");
}

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CHL2MP_Player::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetBulletSpread();
	
	return VECTOR_CONE_5DEGREES;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : step - 
//			fvol - 
//			force - force sound to play
//-----------------------------------------------------------------------------
void CHL2MP_Player::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
	if ((gpGlobals->maxClients > 1 && !sv_footsteps.GetFloat()) || (psurface == NULL))
		return;

#if defined( CLIENT_DLL )
	// during prediction play footstep sounds only once
	if ( !prediction->IsFirstTimePredicted() )
		return;
#endif

	if ( GetFlags() & FL_DUCKING )
		return;

	m_Local.m_nStepside = !m_Local.m_nStepside;

	const char *soundFile = physprops->GetString(psurface->sounds.stepright);
	if (m_Local.m_nStepside)
		soundFile = physprops->GetString(psurface->sounds.stepleft);

	CSoundParameters params;
	if (GetParametersForSound(soundFile, params, NULL) == false)
		return;

	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

#ifndef CLIENT_DLL
	// im MP, server removed all players in origins PVS, these players 
	// generate the footsteps clientside
	if ( gpGlobals->maxClients > 1 )
		filter.RemoveRecipientsByPVS( vecOrigin );
#endif

	// Emit footstep sound:
	EmitSound_t ep;
	ep.m_nChannel = CHAN_BODY;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = fvol;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );

	// Emit foley clothing sound:
	const char *szFoleyScript = "Player.Foley_Clothing";

	switch (m_BB2Local.m_iActiveArmorType)
	{
	case TYPE_LIGHT:
		szFoleyScript = "Player.Foley_Light_Armor";
		break;
	case TYPE_MED:
		szFoleyScript = "Player.Foley_Medium_Armor";
		break;
	case TYPE_HEAVY:
		szFoleyScript = "Player.Foley_Heavy_Armor";
		break;
	}

	if (GetParametersForSound(szFoleyScript, params, NULL) == false)
		return;

	ep.m_nChannel = CHAN_AUTO;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = fvol;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound(filter, entindex(), ep);
}

float CHL2MP_Player::GetPlayerSpeed()
{
	return m_BB2Local.m_flPlayerSpeed;
}

float CHL2MP_Player::GetLeapLength()
{
	return m_BB2Local.m_flLeapLength;
}

float CHL2MP_Player::GetJumpHeight()
{
	return m_BB2Local.m_flJumpHeight;
}

void CHL2MP_Player::OnUpdateInfected(void)
{
	// When we're infected we want to play the infection anim from time to time:
	if (m_flLastInfectionTwitchTime <= gpGlobals->curtime)
	{
		m_flLastInfectionTwitchTime = gpGlobals->curtime + random->RandomFloat(5, 7);
		DoAnimationEvent(PLAYERANIMEVENT_INFECTED);
	}
}

void CHL2MP_Player::SharedPostThinkHL2MP(void)
{
	if (!HL2MPRules()->m_bRoundStarted && HL2MPRules()->ShouldHideHUDDuringRoundWait())
		return;

	if (!IsAlive() || (GetTeamNumber() != TEAM_HUMANS) || (m_BB2Local.m_flSlideKickCooldownEnd > gpGlobals->curtime))
		return;

	// Kick Attack Check:
	if (m_afButtonPressed & IN_ATTACK3)
	{
		ViewPunch(QAngle(2, 2, 0));

		float cooldown = gpGlobals->curtime + GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flKickCooldown;
		m_BB2Local.m_flSlideKickCooldownEnd = cooldown;
		m_BB2Local.m_flSlideKickCooldownStart = gpGlobals->curtime;

		DoAnimationEvent(PLAYERANIMEVENT_KICK);
		DoPlayerKick();
	}
}

bool CHL2MP_Player::IsSliding(void) const
{
#ifdef CLIENT_DLL
	if (!IsLocalPlayer())
		return m_bIsInSlide;
#endif

	return (m_bIsInSlide || m_BB2Local.m_bSliding || m_BB2Local.m_bStandToSlide);
}

void CHL2MP_Player::DoPlayerKick(void)
{
	float damage = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flKickDamage;
	float knockbackForce = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flKickKnockbackForce;
	float range = GameBaseShared()->GetSharedGameDetails()->GetPlayerMiscSkillData()->flKickRange;

	if (GetSkillValue(PLAYER_SKILL_HUMAN_POWER_KICK) > 0)
	{
		float skillPercent = GetSkillValue(PLAYER_SKILL_HUMAN_POWER_KICK, TEAM_HUMANS);
		damage += (damage / 100.0f) * skillPercent;
		knockbackForce += (knockbackForce / 100.0f) * skillPercent;
	}

	trace_t traceHit;
	CBaseEntity *pHitEnt = NULL;

	Vector swingStart = Weapon_ShootPosition();
	Vector forward;
	EyeVectors(&forward, NULL, NULL);

	VectorNormalize(forward);
	Vector swingEnd = swingStart + forward * range;

#ifndef CLIENT_DLL
	HL2MPRules()->EmitSoundToClient(this, "KickAttack", GetSoundType(), GetSoundsetGender());
	lagcompensation->TraceRealtime(this, swingStart, swingEnd, -Vector(3, 3, 3), Vector(3, 3, 3), &traceHit, (LAGCOMP_TRACE_REVERT_HULL | LAGCOMP_TRACE_BOX), range);
	forward = traceHit.endpos - traceHit.startpos;
	VectorNormalize(forward);
#else
	UTIL_TraceLine(swingStart, swingEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &traceHit);
#endif

	pHitEnt = traceHit.m_pEnt;
	if (traceHit.fraction != 1.0f)
	{
		if (pHitEnt != NULL)
		{
#ifndef CLIENT_DLL
			CTakeDamageInfo info(this, this, damage, DMG_CLUB);
			info.SetSkillFlags(0);
			info.SetForcedWeaponID(WEAPON_ID_KICK);
			info.SetDamageCustom(DMG_CLUB);
			CalculateMeleeDamageForce(&info, forward, traceHit.endpos);
			pHitEnt->DispatchTraceAttack(info, forward, &traceHit);
			ApplyMultiDamage();

			// Now hit all triggers along the ray that... 
			TraceAttackToTriggers(info, traceHit.startpos, traceHit.endpos, forward);

			// push the enemy away if you're bashing..
			CAI_BaseNPC *m_pNPC = pHitEnt->MyNPCPointer();
			if (m_pNPC && pHitEnt->IsNPC() && (pHitEnt->IsMercenary() || pHitEnt->IsZombie(true)))
			{
				if ((m_pNPC->GetNavType() != NAV_CLIMB) && !m_pNPC->IsBreakingDownObstacle())
				{
					Vector vecExtraVelocity = (forward * knockbackForce);
					vecExtraVelocity.z = 0; // Don't send them flying upwards...

					IPhysicsObject *pPhys = pHitEnt->VPhysicsGetObject();
					if (pPhys)
						vecExtraVelocity /= pPhys->GetMass(); // Reduce if this target is 'heavy'...

					pHitEnt->SetAbsVelocity(pHitEnt->GetAbsVelocity() + vecExtraVelocity);
				}
			}
#endif
		}

		UTIL_ImpactTrace(&traceHit, DMG_CLUB);
	}
}

const Vector CHL2MP_Player::GetPlayerMins(void) const
{
	if (!IsObserver() && IsSliding())
		return VEC_SLIDE_HULL_MIN_SCALED(this);

	return BaseClass::GetPlayerMins();
}

const Vector CHL2MP_Player::GetPlayerMaxs(void) const
{
	if (!IsObserver() && IsSliding())
		return VEC_SLIDE_HULL_MAX_SCALED(this);

	return BaseClass::GetPlayerMaxs();
}

float CHL2MP_Player::GetPlaybackRateForAnimEvent(PlayerAnimEvent_t event, int nData)
{
	if (nData > 0)
	{
		Activity thirdpersonAct = ACT_INVALID;
		switch (event)
		{
		case PLAYERANIMEVENT_ATTACK_PRIMARY:
			thirdpersonAct = ACT_MP_ATTACK_STAND_PRIMARYFIRE;
			break;

		case PLAYERANIMEVENT_ATTACK_SECONDARY:
			thirdpersonAct = ACT_MP_ATTACK_STAND_SECONDARYFIRE;
			break;

		case PLAYERANIMEVENT_RELOAD:
			thirdpersonAct = ACT_MP_RELOAD_STAND;
			break;

		case PLAYERANIMEVENT_RELOAD_END:
			thirdpersonAct = ACT_MP_RELOAD_STAND_END;
			break;

		case PLAYERANIMEVENT_BASH:
			thirdpersonAct = ACT_MP_BASH;
			break;
		}

		if (thirdpersonAct != ACT_INVALID)
			return GameBaseShared()->GetPlaybackSpeedThirdperson(this, nData, thirdpersonAct);
	}

	return 1.0f;
}
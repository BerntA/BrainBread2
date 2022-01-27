//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Local Data for Players.
//
//========================================================================================//

#ifndef BB2_PLAYERLOCALDATA_H
#define BB2_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "networkvar.h"
#include "skills_shareddefs.h"

class CBB2PlayerLocalData
{
public:
	DECLARE_CLASS_NOBASE(CBB2PlayerLocalData);
	DECLARE_EMBEDDED_NETWORKVAR();

	CBB2PlayerLocalData();

	// Human Skills:
	CNetworkVar(int, m_iSkill_XPCurrent);
	CNetworkVar(int, m_iSkill_XPLeft);
	CNetworkVar(int, m_iSkill_Talents);

	// Zombie Skills
	CNetworkVar(int, m_iZombieCredits);
	CNetworkVar(int, m_iZombKills); // Zombie Stored Kills Purpose: Since every zombie needs to be an individual we store a kill amount to respawn the zombie as a human after the death with the X score!
	CNetworkVar(int, m_iZombDeaths); // To make use of bb2_allow_mercy we store the death count, if it exceeds the mercy value we allow you to spawn as a human because the humans are too OP.
	CNetworkVar(float, m_flZombieRageThresholdDamage);

	// Shared Skills
	CNetworkArray(int, m_iPlayerSkills, MAX_SKILL_ARRAY);
	CNetworkVar(int, m_iPerkTeamBonus);
	CNetworkVar(bool, m_bCanActivatePerk);
	CNetworkVar(bool, m_bHasPlayerEscaped);
	CNetworkVar(bool, m_bCanRespawnAsHuman);

	// Misc
	CNetworkVar(int, m_iActiveArmorType);
	CNetworkVar(float, m_flInfectionTimer);
	CNetworkVar(float, m_flCarryWeight);
	CNetworkVar(float, m_flPlayerRespawnTime);
	CNetworkVar(float, m_flPerkTimer);
	CNetworkVar(bool, m_bEnableAutoReload);

	// Movement:
	CNetworkVar(float, m_flPlayerSpeed);
	CNetworkVar(float, m_flLeapLength);
	CNetworkVar(float, m_flJumpHeight);
	CNetworkVar(float, m_flSlideKickCooldownEnd);
	CNetworkVar(float, m_flSlideKickCooldownStart);
	CNetworkVar(float, m_flNewSurfaceFriction);
	CNetworkVar(bool, m_bPlayerJumped);

	// Slide:
	CNetworkVar(float, m_flSlideTime);
	CNetworkVar(bool, m_bStandToSlide);
	CNetworkVar(bool, m_bSliding);
};

EXTERN_SEND_TABLE(DT_BB2Local);

#endif // BB2_PLAYERLOCALDATA_H
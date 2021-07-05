//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Local Data for Players.
//
//========================================================================================//

#include "cbase.h"
#include "bb2_playerlocaldata.h"
#include "hl2mp_player.h"
#include "mathlib/mathlib.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SEND_TABLE_NOBASE(CBB2PlayerLocalData, DT_BB2Local)

SendPropInt(SENDINFO(m_iSkill_XPCurrent), 16, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iSkill_XPLeft), 16, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iSkill_Talents), 8, SPROP_UNSIGNED),

SendPropInt(SENDINFO(m_iZombieCredits), 12, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_flZombieRageThresholdDamage)),

SendPropArray3(SENDINFO_ARRAY3(m_iPlayerSkills), SendPropInt(SENDINFO_ARRAY(m_iPlayerSkills), 4, SPROP_UNSIGNED)),
SendPropInt(SENDINFO(m_iPerkTeamBonus), 6, SPROP_UNSIGNED),
SendPropBool(SENDINFO(m_bCanActivatePerk)),
SendPropBool(SENDINFO(m_bHasPlayerEscaped)),
SendPropBool(SENDINFO(m_bCanRespawnAsHuman)),

SendPropInt(SENDINFO(m_iActiveArmorType), 4, SPROP_UNSIGNED),
SendPropFloat(SENDINFO(m_flInfectionTimer), -1, SPROP_CHANGES_OFTEN, 0.0f, 2048.0f),
SendPropFloat(SENDINFO(m_flCarryWeight)),
SendPropFloat(SENDINFO(m_flPlayerRespawnTime), -1, SPROP_CHANGES_OFTEN, 0.0f, 2048.0f),
SendPropFloat(SENDINFO(m_flPerkTimer)),
SendPropBool(SENDINFO(m_bEnableAutoReload)),

SendPropFloat(SENDINFO(m_flPlayerSpeed)),
SendPropFloat(SENDINFO(m_flLeapLength)),
SendPropFloat(SENDINFO(m_flJumpHeight)),
SendPropFloat(SENDINFO(m_flSlideKickCooldownEnd)),
SendPropFloat(SENDINFO(m_flSlideKickCooldownStart)),
SendPropFloat(SENDINFO(m_flNewSurfaceFriction)),
SendPropBool(SENDINFO(m_bPlayerJumped)),

SendPropFloat(SENDINFO(m_flSlideTime), 12, SPROP_ROUNDDOWN | SPROP_CHANGES_OFTEN, 0.0f, 2048.0f),
SendPropBool(SENDINFO(m_bStandToSlide)),
SendPropBool(SENDINFO(m_bSliding)),

END_SEND_TABLE()

CBB2PlayerLocalData::CBB2PlayerLocalData()
{
	m_iSkill_XPCurrent = 0;
	m_iSkill_XPLeft = 0;
	m_iSkill_Talents = 0;

	m_iZombieCredits = 0;
	m_flZombieRageThresholdDamage = 0.0f;

	for (int i = 0; i < MAX_SKILL_ARRAY; i++)
		m_iPlayerSkills.Set(i, 0);

	m_iPerkTeamBonus = 0;
	m_bCanActivatePerk = false;
	m_bHasPlayerEscaped = false;
	m_bCanRespawnAsHuman = false;

	m_iActiveArmorType = 0;
	m_flInfectionTimer = 0.0f;
	m_flCarryWeight = 0.0f;
	m_flPlayerRespawnTime = 0.0f;
	m_flPerkTimer = 0.0f;
	m_bEnableAutoReload = false;

	m_flPlayerSpeed = 0.0f;
	m_flLeapLength = 0.0f;
	m_flJumpHeight = 0.0f;
	m_flSlideKickCooldownEnd = 0.0f;
	m_flSlideKickCooldownStart = 0.0f;
	m_flNewSurfaceFriction = 0.0f;
	m_bPlayerJumped = false;

	m_flSlideTime = 0.0f;
	m_bStandToSlide = false;
	m_bSliding = false;
}
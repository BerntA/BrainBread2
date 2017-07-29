//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Local Data for Players.
//
//========================================================================================//

#if !defined( C_BB2_PLAYERLOCALDATA )
#define C_BB2_PLAYERLOCALDATA
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "dt_recv.h"
#include "c_baseentity.h"
#include "skills_shareddefs.h"

EXTERN_RECV_TABLE(DT_BB2Local);

class C_BB2PlayerLocalData
{
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE(C_BB2PlayerLocalData);
	DECLARE_EMBEDDED_NETWORKVAR();

	C_BB2PlayerLocalData();

	// Human Skills:
	int m_iSkill_XPCurrent;
	int m_iSkill_XPLeft;
	int m_iSkill_Talents;

	// Zombie Skills
	int m_iZombieCredits;
	float m_flZombieRageThresholdDamage;

	// Shared Skills
	int m_iPlayerSkills[MAX_SKILL_ARRAY];
	int m_iPerkTeamBonus;
	bool m_bCanActivatePerk;
	bool m_bHasPlayerEscaped;
	bool m_bCanRespawnAsHuman;

	// Misc
	int m_iActiveArmorType;
	float m_flInfectionTimer;
	float m_flCarryWeight;
	float m_flPlayerRespawnTime;
	float m_flPerkTimer;

	// Movement:
	float m_flPlayerSpeed;
	float m_flLeapLength;
	float m_flJumpHeight;
	float m_flSlideKickCooldownEnd;
	float m_flSlideKickCooldownStart;
	float m_flNewSurfaceFriction;
	bool m_bPlayerJumped;

	// Slide
	float m_flSlideTime;
	bool m_bStandToSlide;
	bool m_bSliding;
};

#endif // C_BB2_PLAYERLOCALDATA
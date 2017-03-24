//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Local Data for Players.
//
//========================================================================================//

#include "cbase.h"
#include "c_bb2_playerlocaldata.h"
#include "dt_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE(C_BB2PlayerLocalData, DT_BB2Local)

RecvPropInt(RECVINFO(m_iSkill_XPCurrent)),
RecvPropInt(RECVINFO(m_iSkill_XPLeft)),
RecvPropInt(RECVINFO(m_iSkill_Talents)), 

RecvPropInt(RECVINFO(m_iZombieCredits)),

RecvPropArray3(RECVINFO_ARRAY(m_iPlayerSkills), RecvPropInt(RECVINFO(m_iPlayerSkills[0]))),
RecvPropInt(RECVINFO(m_iPerkTeamBonus)),
RecvPropBool(RECVINFO(m_bCanActivatePerk)),
RecvPropBool(RECVINFO(m_bHasPlayerEscaped)),
RecvPropBool(RECVINFO(m_bCanRespawnAsHuman)),

RecvPropInt(RECVINFO(m_iActiveArmorType)),
RecvPropFloat(RECVINFO(m_flInfectionTimer)),
RecvPropFloat(RECVINFO(m_flCarryWeight)),
RecvPropFloat(RECVINFO(m_flPlayerRespawnTime)),
RecvPropFloat(RECVINFO(m_flPerkTimer)),

RecvPropFloat(RECVINFO(m_flPlayerSpeed)),
RecvPropFloat(RECVINFO(m_flLeapLength)),
RecvPropFloat(RECVINFO(m_flJumpHeight)),
RecvPropFloat(RECVINFO(m_flSlideKickCooldownEnd)),
RecvPropFloat(RECVINFO(m_flSlideKickCooldownStart)),
RecvPropFloat(RECVINFO(m_flNewSurfaceFriction)),
RecvPropBool(RECVINFO(m_bPlayerJumped)),

RecvPropFloat(RECVINFO(m_flSlideTime)),
RecvPropBool(RECVINFO(m_bStandToSlide)),
RecvPropBool(RECVINFO(m_bSliding)),

END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE(C_BB2PlayerLocalData)
DEFINE_PRED_ARRAY(m_iPlayerSkills, FIELD_INTEGER, MAX_SKILL_ARRAY, FTYPEDESC_PRIVATE),

DEFINE_PRED_FIELD(m_flSlideTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bStandToSlide, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bSliding, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),

DEFINE_PRED_FIELD(m_bPlayerJumped, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),

DEFINE_PRED_FIELD(m_flNewSurfaceFriction, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flSlideKickCooldownEnd, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flSlideKickCooldownStart, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()

C_BB2PlayerLocalData::C_BB2PlayerLocalData()
{
	m_iSkill_XPCurrent = 0;
	m_iSkill_XPLeft = 0;
	m_iSkill_Talents = 0;

	m_iZombieCredits = 0;

	memset(m_iPlayerSkills, 0, sizeof(m_iPlayerSkills));

	m_iPerkTeamBonus = 0;
	m_bCanActivatePerk = false;
	m_bHasPlayerEscaped = false;
	m_bCanRespawnAsHuman = false;

	m_iActiveArmorType = 0;
	m_flInfectionTimer = 0.0f;
	m_flCarryWeight = 0.0f;
	m_flPlayerRespawnTime = 0.0f;
	m_flPerkTimer = 0.0f;

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
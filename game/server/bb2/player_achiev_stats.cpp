//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: Player Specific Achiev. Statistics.
//
//========================================================================================//

#include "cbase.h"
#include "player_achiev_stats.h"
#include "achievement_manager.h"
#include "GameBase_Server.h"

#define ACHMGR GameBaseShared()->GetAchievementManager()

CPlayerAchievStats::CPlayerAchievStats(CHL2MP_Player *pOuter)
{
	m_pOuter = pOuter;
}

CPlayerAchievStats::~CPlayerAchievStats()
{
	m_pOuter = NULL;
}

bool CPlayerAchievStats::IsValid(void)
{
	return (m_pOuter && !m_pOuter->IsBot() && GameBaseServer() && GameBaseShared() && GameBaseShared()->GetAchievementManager() && HL2MPRules() && HL2MPRules()->CanUseSkills() && (GameBaseServer()->CanStoreSkills() == PROFILE_GLOBAL));
}

void CPlayerAchievStats::OnSpawned(void)
{
	m_iHealthKitsUsed = m_iZombieKicks = m_iZombiePunches = m_iZombieUppercuts = m_iTotalHeadshots = m_iZombiePlayerHeadshots = 0;
}

void CPlayerAchievStats::OnDeath(const CTakeDamageInfo &info)
{
}

void CPlayerAchievStats::OnKilled(CBaseEntity *pVictim, CBaseEntity *pInflictor, const CTakeDamageInfo &info, int hitgroup)
{
	if (!IsValid() || !pVictim || !pInflictor || (pVictim == m_pOuter))
		return;

	CBaseCombatWeapon *pActiveWeapon = m_pOuter->GetActiveWeapon();
	int weaponID = info.GetForcedWeaponID();
	if (weaponID == WEAPON_ID_NONE)
		weaponID = pActiveWeapon->GetUniqueWeaponID();

	switch (weaponID)
	{

	case WEAPON_ID_HANDS:
	{
		if (!pVictim->IsPlayer() && pVictim->IsZombie(true))
		{
			if (pActiveWeapon && (pActiveWeapon->m_flMeleeCooldown.Get() > 0.0f))
				m_iZombieUppercuts++;
			else
				m_iZombiePunches++;
		}
		break;
	}

	case WEAPON_ID_KICK:
	{
		if (!pVictim->IsPlayer() && pVictim->IsZombie(true))
			m_iZombieKicks++;
		break;
	}

	}

	if ((m_iZombieKicks >= 25) && (m_iZombiePunches >= 20) && (m_iZombieUppercuts >= 15))
		ACHMGR->WriteToAchievement(m_pOuter, "ACH_RAZ_KUNG_FLU");

	if (hitgroup == HITGROUP_HEAD)
	{
		m_iTotalHeadshots++;
		if (pVictim->IsZombie())
			m_iZombiePlayerHeadshots++;

		if (pActiveWeapon && pActiveWeapon->IsMeleeWeapon() && (weaponID != WEAPON_ID_KICK))
			ACHMGR->WriteToStat(m_pOuter, "BBX_RZ_HEADSHOT");
	}

	if (m_iTotalHeadshots >= 69)
		ACHMGR->WriteToAchievement(m_pOuter, "ACH_RAZ_MAZELTOV");

	if (m_iZombiePlayerHeadshots >= 3)
		ACHMGR->WriteToAchievement(m_pOuter, "ACH_RAZ_SWEEPER");

	PrintDebugMsg();
}

void CPlayerAchievStats::OnTookDamage(const CTakeDamageInfo &info)
{
}

void CPlayerAchievStats::OnDidDamage(const CTakeDamageInfo &info)
{
	if (!IsValid())
		return;

	int currDamage = ((int)ceil(info.GetDamage()));
	ACHMGR->WriteToStat(m_pOuter, "BBX_RZ_PAIN", currDamage, true);

	PrintDebugMsg();
}

void CPlayerAchievStats::OnPickupItem(const DataInventoryItem_Base_t *item)
{
	if (!IsValid() || !item)
		return;

	int iType = item->iType, iSubType = item->iSubType;
	if (iType == TYPE_MISC)
	{
		if ((iSubType == TYPE_HEALTHKIT) || (iSubType == TYPE_HEALTHVIAL) || (iSubType == TYPE_FOOD))
			m_iHealthKitsUsed++;
	}

	if (m_iHealthKitsUsed >= 10)
		ACHMGR->WriteToAchievement(m_pOuter, "ACH_RAZ_HEALTH_ADDICT");

	PrintDebugMsg();
}

static ConVar bb2_debug_achievements("bb2_debug_achievements", "0", FCVAR_GAMEDLL, "Debug player achievements.");

void CPlayerAchievStats::PrintDebugMsg(void)
{
	if (!bb2_debug_achievements.GetBool())
		return;

	Warning("Achievement Status for %s:\n", m_pOuter->GetPlayerName());
	Warning("KUNGFLU: %i %i %i\n", m_iZombieKicks, m_iZombiePunches, m_iZombieUppercuts);
	Warning("HEALTHADDICT: %i\n", m_iHealthKitsUsed);
	Warning("MAZELTOV: %i\n", m_iTotalHeadshots);
	Warning("SWEEPER: %i\n", m_iZombiePlayerHeadshots);
	Msg("\n");
}
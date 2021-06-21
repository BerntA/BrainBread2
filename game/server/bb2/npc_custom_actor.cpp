//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom ally/enemy/villain actor NPC.
//
//========================================================================================//

#include "cbase.h"
#include "npc_custom_actor.h"
#include "ammodef.h"
#include "soundent.h"
#include "BasePropDoor.h"
#include "hl2_shared_misc.h"
#include "hl2_player.h"
#include "items.h"
#include "eventqueue.h"
#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "ai_senses.h"
#include "ai_looktarget.h"
#include "GameBase_Shared.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CBasePlayer *GetNearestMatchingPlayer(CAI_BaseNPC *pLooker, bool bVisibleMask = false, int mask = MASK_SOLID_BRUSHONLY)
{
	if (!pLooker)
		return NULL;

	float distToNearest = FLT_MAX;
	CBasePlayer *pNearest = NULL;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer)
			continue;

		if (!pPlayer->IsAlive() || pPlayer->IsObserver() || (pPlayer->GetTeamNumber() <= TEAM_SPECTATOR))
			continue;

		if (pLooker->IRelationType(pPlayer) != D_LI)
			continue;

		if (bVisibleMask)
		{
			if (!pLooker->FVisible(pPlayer, mask))
				continue;
		}

		float flDist = (pPlayer->GetAbsOrigin() - pLooker->GetAbsOrigin()).LengthSqr();
		if ((flDist < distToNearest))
		{
			pNearest = pPlayer;
			distToNearest = flDist;
		}
	}

	return pNearest;
}

LINK_ENTITY_TO_CLASS(npc_custom_actor, CNPC_CustomActor);

BEGIN_DATADESC(CNPC_CustomActor)

DEFINE_KEYFIELD(m_bNotifyNavFailBlocked, FIELD_BOOLEAN, "notifynavfailblocked"),

DEFINE_KEYFIELD(m_bIsAlly, FIELD_BOOLEAN, "friendly"),
DEFINE_KEYFIELD(m_bGender, FIELD_BOOLEAN, "gender"),
DEFINE_KEYFIELD(m_bBossState, FIELD_BOOLEAN, "boss"),
DEFINE_KEYFIELD(m_iTotalHP, FIELD_INTEGER, "totalhealth"),
DEFINE_KEYFIELD(m_iszNPCName, FIELD_STRING, "npcname"),

DEFINE_KEYFIELD(m_flDamageScaleFactor, FIELD_FLOAT, "damageScale"),
DEFINE_KEYFIELD(m_flHealthScaleFactor, FIELD_FLOAT, "healthScale"),

DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_OUTPUT(m_OnNavFailBlocked, "OnNavFailBlocked"),

DEFINE_USEFUNC(SimpleUse),

END_DATADESC()

CNPC_CustomActor::CNPC_CustomActor()
{
	m_bIsAlly = true;
	m_bGender = true;
	m_bBossState = false;
	m_iszNPCName = NULL_STRING;
	m_iTotalHP = 100;

	m_flDamageScaleValue = m_flHealthScaleValue = 0.0f;
	m_flDamageScaleFactor = m_flHealthScaleFactor = 1.0f;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::Precache()
{
	PrecacheModel(STRING(GetModelName()));
	UTIL_PrecacheOther("weapon_frag");
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::Spawn()
{
	Precache();
	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);
	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);
	SetUse(&CNPC_CustomActor::SimpleUse);

	m_flFieldOfView = -1.0;
	SetCollisionGroup(m_bIsAlly ? COLLISION_GROUP_NPC_MILITARY : COLLISION_GROUP_NPC_MERCENARY);
	m_iNumGrenades = 0;
	CapabilitiesRemove(bits_CAP_INNATE_MELEE_ATTACK1);
}

bool CNPC_CustomActor::ParseNPC(CBaseEntity *pEntity)
{
	m_flSpeedFactorValue = 1.0f;
	m_iXPToGive = 5;
	m_iDamageKick = 20;
	m_flRange = 60.0f;
	m_iModelSkin = m_nSkin.Get();

	// NO model? force default.
	const char *model = STRING(GetModelName());
	if (model && model[0])
		Q_strncpy(pszModelName, model, MAX_WEAPON_STRING);
	else
		Q_strncpy(pszModelName, "models/characters/marine/marine_soldier01.mdl", MAX_WEAPON_STRING);

	return true;
}

//-----------------------------------------------------------------------------
// Define our relationship towards the players here.
//-----------------------------------------------------------------------------
Class_T	CNPC_CustomActor::Classify()
{
	return (m_bIsAlly ? CLASS_COMBINE : CLASS_MILITARY);
}

void CNPC_CustomActor::PlaySound(const char *sound, float eventtime)
{
	char pchSoundScript[32];
	Q_strncpy(pchSoundScript, sound, 32);
	HL2MPRules()->EmitSoundToClient(this, pchSoundScript, GetNPCType(), GetGender());
}

void CNPC_CustomActor::AnnounceEnemyKill(CBaseEntity *pEnemy)
{
	if (!pEnemy)
		return;

	if (pEnemy->IsPlayer() && !m_bIsAlly)
		PlaySound("PlayerDown");
	else
		PlaySound("Taunt");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void CNPC_CustomActor::TaskFail(AI_TaskFailureCode_t code)
{
	if (code == FAIL_NO_ROUTE_BLOCKED && m_bNotifyNavFailBlocked)
		m_OnNavFailBlocked.FireOutput(this, this);
	BaseClass::TaskFail(code);
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
//-----------------------------------------------------------------------------
Activity CNPC_CustomActor::NPC_TranslateActivity(Activity activity)
{
	// Fixes faulty / missing ACT's for BB1 survivors.

	if (activity == ACT_MELEE_ATTACK1)
		return ACT_MELEE_ATTACK_SWING;

	if (activity == ACT_RUN_AIM_SHOTGUN)
		return ACT_RUN_AIM_RIFLE;
	if (activity == ACT_WALK_AIM_SHOTGUN)
		return ACT_WALK_AIM_RIFLE;
	if (activity == ACT_IDLE_ANGRY_SHOTGUN)
		return ACT_IDLE_ANGRY_SMG1;
	if (activity == ACT_RANGE_ATTACK_SHOTGUN_LOW)
		return ACT_RANGE_ATTACK_SMG1_LOW;

	if (GetActiveWeapon() && (GetActiveWeapon()->GetWeaponType() == WEAPON_TYPE_SHOTGUN))
	{
		switch (activity)
		{
		case ACT_RELOAD:
		case ACT_RELOAD_SHOTGUN:
		case ACT_RELOAD_SHOTGUN_LOW:
		case ACT_GESTURE_RELOAD_SHOTGUN:
			return ACT_RELOAD_SMG1;
		}
	}

	return BaseClass::NPC_TranslateActivity(activity);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::HandleAnimEvent(animevent_t *pEvent)
{
	switch (pEvent->event)
	{
	case AE_NPC_LEFTFOOT:
	case NPC_EVENT_LEFTFOOT:
	{
		PlaySound("FootstepLeft", pEvent->eventtime);
	}
	break;

	case AE_NPC_RIGHTFOOT:
	case NPC_EVENT_RIGHTFOOT:
	{
		PlaySound("FootstepRight", pEvent->eventtime);
	}
	break;

	case AE_NPC_BODYDROP_HEAVY: // Prevent console spamming for BB1 survivors.
		break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CustomActor::OnBeginMoveAndShoot()
{
	if (BaseClass::OnBeginMoveAndShoot())
	{
		if (m_iMySquadSlot == SQUAD_SLOT_ATTACK1 || m_iMySquadSlot == SQUAD_SLOT_ATTACK2)
			return true; // already have the slot I need

		if (m_iMySquadSlot == SQUAD_SLOT_NONE && OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2))
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::LocateEnemySound()
{
	PlaySound("EnemySpotted");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::OnChangeActiveWeapon(CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon)
{
	if (pNewWeapon)
		GetShotRegulator()->SetParameters(pNewWeapon->GetMinBurst(), pNewWeapon->GetMaxBurst(), pNewWeapon->GetMinRestTime(), pNewWeapon->GetMaxRestTime());
	BaseClass::OnChangeActiveWeapon(pOldWeapon, pNewWeapon);
}

//-----------------------------------------------------------------------------
// Purpose: Prevent teamkillin against friendlies.
//-----------------------------------------------------------------------------
int CNPC_CustomActor::OnTakeDamage(const CTakeDamageInfo &info)
{
	CBaseEntity *pAttacker = info.GetAttacker();
	if (pAttacker && (pAttacker->Classify() == Classify()))
		return 0;

	if (pAttacker && pAttacker->IsHuman() && m_bIsAlly)
	{
		PlaySound("FriendlyFire");
		return 0;
	}

	PlaySound("Pain");
	return BaseClass::OnTakeDamage(info);
}

void CNPC_CustomActor::FireBullets(const FireBulletsInfo_t &info)
{
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	FireBulletsInfo_t modinfo = info;

	float flDamage = info.m_flDamage;
	if (pWeapon)
	{
		modinfo.m_vecFirstStartPos = GetLocalOrigin();
		modinfo.m_flDropOffDist = pWeapon->GetWpnData().m_flDropOffDistance;
	}

	flDamage += ((flDamage / 100.0f) * m_flDamageScaleValue);

	modinfo.m_flDamage = flDamage;
	modinfo.m_iPlayerDamage = (int)flDamage;

	CAI_BaseActor::FireBullets(modinfo);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::SimpleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (IRelationType(pActivator) != D_LI)
		return;

	m_OnPlayerUse.FireOutput(pActivator, pCaller);

	if (pActivator == GetNearestMatchingPlayer(this))
	{
		if (m_iTimesGreeted >= 6)
		{
			PlaySound("Stop");
			m_iTimesGreeted = 0;
		}
		if (m_iTimesGreeted >= 3)
			PlaySound("WhatNow");
		else
			PlaySound("Greetings");

		m_iTimesGreeted++;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CustomActor::DeathSound(const CTakeDamageInfo &info)
{
	PlaySound("Death");
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CustomActor::FearSound(void)
{
	PlaySound("Scared");
}

void CNPC_CustomActor::UpdateNPCScaling()
{
	if ((!bb2_enable_scaling.GetBool() && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA)) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION))
	{
		m_flDamageScaleValue = m_flHealthScaleValue = 0.0f;
		return;
	}

	float flDamageScaleAmount = 0.0f, flHealthScaleAmount = 0.0f;

	flDamageScaleAmount = (bb2_npc_scaling.GetInt() * ((float)GameBaseShared()->GetNumActivePlayers()) * m_flDamageScaleFactor);
	flHealthScaleAmount = (bb2_npc_scaling.GetInt() * ((float)GameBaseShared()->GetNumActivePlayers()) * m_flHealthScaleFactor);

	float defaultTotalHP = ((float)m_iTotalHP);
	float flTotal = (flHealthScaleAmount * (defaultTotalHP / 100.0f)) + defaultTotalHP;

	float hpPercentLeft = (float)(((float)GetHealth()) / ((float)GetMaxHealth()));
	hpPercentLeft = clamp(hpPercentLeft, 0.0f, 1.0f);
	float newHP = clamp(round(flTotal * hpPercentLeft), 1.0f, flTotal);

	SetHealth((int)newHP);
	SetMaxHealth((int)flTotal);

	m_flDamageScaleValue = flDamageScaleAmount;
	m_flHealthScaleValue = flHealthScaleAmount;

	OnNPCScaleUpdated();
}
//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base properties for all npcs, all npcs will be linked to a script in data/npc. It contains basic values to make it easier to test & make modifications in general.
//
//========================================================================================//

#include "cbase.h"
#include "npc_base_properties.h"
#include "filesystem.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"
#include "hl2mp_player.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_show_active_military_activities("ai_show_active_military_activities", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Print out which activities are being executed by the military npcs.", true, 0.0f, true, 1.0f);

CNPCBaseProperties::CNPCBaseProperties()
{
	m_iXPToGive = 0;
	m_iDamageOneHand = 0;
	m_iDamageBothHands = 0;
	m_iDamageKick = 0;
	m_iTotalHP = 0;
	m_iModelSkin = 0;
	m_flRange = 50.0f;
	m_bGender = false;
	pszModelName[0] = 0;
	m_pNPCData = NULL;

	ListenForGameEvent("reload_game_data");
	ListenForGameEvent("player_connection");
	ListenForGameEvent("round_started");
}

bool CNPCBaseProperties::ParseNPC(int index)
{
	m_flSpeedFactorValue = 1.0f;

	if (!GameBaseShared() || !GameBaseShared()->GetNPCData())
		return false;

	CNPCDataItem *npcData = GameBaseShared()->GetNPCData()->GetNPCData(GetNPCName());
	if (npcData)
	{
		m_pNPCData = npcData;
		m_iEntIndex = index;

		const NPCModelItem_t *modelItem = npcData->GetModelItem();
		Assert(modelItem != NULL);

		Q_strncpy(pszModelName, modelItem ? modelItem->szModelPath : "", MAX_WEAPON_STRING);
		m_bGender = (Q_stristr(pszModelName, "female")) ? false : true;

		m_iDefaultHealth = random->RandomInt(npcData->iHealthMin, npcData->iHealthMax);
		m_iDefaultDamage1H = random->RandomInt(npcData->iSlashDamageMin, npcData->iSlashDamageMax);
		m_iDefaultDamage2H = random->RandomInt(npcData->iDoubleSlashDamageMin, npcData->iDoubleSlashDamageMax);
		m_iDefaultKickDamage = random->RandomInt(npcData->iKickDamageMin, npcData->iKickDamageMax);

		m_iXPToGive = npcData->iXP;
		m_iTotalHP = m_iDefaultHealth;
		m_iDamageOneHand = m_iDefaultDamage1H;
		m_iDamageBothHands = m_iDefaultDamage2H;
		m_iDamageKick = m_iDefaultKickDamage;
		m_iModelSkin = modelItem ? random->RandomInt(modelItem->iSkinMin, modelItem->iSkinMax) : 0;
		m_flRange = npcData->flRange;
		m_flSpeedFactorValue = abs(random->RandomFloat(npcData->flSpeedFactorMin, npcData->flSpeedFactorMax));
	}
	else
		Warning("Can't load the npc data!\nRemoving npc!\n");

	return (npcData != NULL);
}

float CNPCBaseProperties::GetScaleValue(bool bDamageScale)
{
	float value = (bb2_npc_scaling.GetInt() * (bDamageScale ? m_pNPCData->flDamageScale : m_pNPCData->flHealthScale));
	float avgLevel = ((float)GameBaseShared()->GetAveragePlayerLevel());
	if (HL2MPRules()->IsGamemodeFlagActive(GM_FLAG_EXTREME_SCALING) && (avgLevel > GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPScaleFactorMinAvgLvL))
	{
		float range = avgLevel - GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPScaleFactorMinAvgLvL;
		float fraction = range / (GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPScaleFactorMaxAvgLvL -
			GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPScaleFactorMinAvgLvL);
		fraction = clamp(fraction, 0.0f, 1.0f);
		value *= fraction * GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPScaleFactor;
	}

	return value;
}

void CNPCBaseProperties::UpdateNPCScaling()
{
	if ((!bb2_enable_scaling.GetBool() && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA)) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION) || (m_pNPCData == NULL))
	{
		m_flDamageScaleValue = 0.0f;
		m_flHealthScaleValue = 0.0f;
		return;
	}

	float flDamageScaleAmount = 0.0f, flHealthScaleAmount = 0.0f;

	flDamageScaleAmount = (((float)GameBaseShared()->GetNumActivePlayers()) * GetScaleValue(true));
	flHealthScaleAmount = (((float)GameBaseShared()->GetNumActivePlayers()) * GetScaleValue(false));

	float defaultTotalHP = ((float)m_iDefaultHealth);
	float flTotal = round((flHealthScaleAmount * (defaultTotalHP / 100.0f)) + defaultTotalHP);
	m_iTotalHP = (int)flTotal;

	float damageSingle = m_iDefaultDamage1H;
	float damageBoth = m_iDefaultDamage2H;
	float damageKick = m_iDefaultKickDamage;

	m_iDamageOneHand = (flDamageScaleAmount * (damageSingle / 100.0f)) + damageSingle;
	m_iDamageBothHands = (flDamageScaleAmount * (damageBoth / 100.0f)) + damageBoth;
	m_iDamageKick = (flDamageScaleAmount * (damageKick / 100.0f)) + damageKick;

	float defaultXP = ((float)m_pNPCData->iXP);
	float newXPValue = ((flHealthScaleAmount + flDamageScaleAmount) * (defaultXP / 100.0f)) + defaultXP;
	m_iXPToGive = (int)newXPValue;

	CBaseEntity *pEntity = UTIL_EntityByIndex(GetEntIndex());
	if (pEntity)
	{
		float newHP = 0.0f;
		float hpPercentLeft = (((float)pEntity->GetHealth()) / ((float)pEntity->GetMaxHealth()));

		if (hpPercentLeft >= 1.0f) // No HP lost.
			newHP = flTotal;
		else // HP lost
			newHP = ((float)m_iTotalHP) * hpPercentLeft;

		newHP = MAX(round(newHP), 1.0f);

		pEntity->SetHealth((int)newHP);
		pEntity->SetMaxHealth(m_iTotalHP);
	}

	m_flDamageScaleValue = flDamageScaleAmount;
	m_flHealthScaleValue = flHealthScaleAmount;

	OnNPCScaleUpdated();
}

void CNPCBaseProperties::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (!strcmp(type, "reload_game_data"))
	{
		m_pNPCData = GameBaseShared()->GetNPCData()->GetNPCData(GetNPCName());
		return;
	}
	else if (!strcmp(type, "player_connection"))
	{
		UpdateNPCScaling();
		return;
	}

	// Human Events:
	CBaseEntity *pEntity = UTIL_EntityByIndex(GetEntIndex());
	if (!pEntity)
		return;

	if (pEntity->Classify() != CLASS_COMBINE)
		return;

	if (!strcmp(type, "round_started"))
		HL2MPRules()->EmitSoundToClient(pEntity, "Ready", GetNPCType(), GetGender());
}
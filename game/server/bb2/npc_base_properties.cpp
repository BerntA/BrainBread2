//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base properties for all npcs, all npcs will be linked to a script in data/npc. It contains basic values to make it easier to test & make modifications in general.
//
//========================================================================================//

#include "cbase.h"
#include "npc_base_properties.h"
#include "filesystem.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"

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

	ListenForGameEvent("player_connection");
	ListenForGameEvent("round_started");
}

bool CNPCBaseProperties::ParseNPC(int index)
{
	bool bParsed = (GameBaseShared()->GetNPCData() && GameBaseShared()->GetNPCData()->DoesNPCExist(GetNPCName()));
	if (bParsed)
	{
		m_iEntIndex = index;

		Q_strncpy(pszModelName, GameBaseShared()->GetNPCData()->GetModel(GetNPCName()), MAX_WEAPON_STRING);
		m_bGender = (Q_stristr(pszModelName, "female")) ? false : true;

		m_iXPToGive = GameBaseShared()->GetNPCData()->GetXP(GetNPCName());
		m_iTotalHP = GameBaseShared()->GetNPCData()->GetHealth(GetNPCName());
		m_iDamageOneHand = GameBaseShared()->GetNPCData()->GetSlashDamage(GetNPCName());
		m_iDamageBothHands = GameBaseShared()->GetNPCData()->GetDoubleSlashDamage(GetNPCName());
		m_iDamageKick = GameBaseShared()->GetNPCData()->GetKickDamage(GetNPCName());
		m_iModelSkin = GameBaseShared()->GetNPCData()->GetSkin(GetNPCName(), pszModelName);
		m_flRange = GameBaseShared()->GetNPCData()->GetRange(GetNPCName());
	}
	else
		Warning("Can't load the npc data!\nRemoving npc!\n");

	return bParsed;
}

float CNPCBaseProperties::GetScaleValue(bool bDamageScale)
{
	return (bb2_npc_scaling.GetInt() * GameBaseShared()->GetNPCData()->GetScale(GetNPCName(), bDamageScale));
}

void CNPCBaseProperties::UpdateNPCScaling()
{
	if ((!bb2_enable_scaling.GetBool() && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA)) || (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION))
	{
		m_flDamageScaleValue = 0.0f;
		m_flHealthScaleValue = 0.0f;
		return;
	}

	float flDamageScaleAmount = 0.0f, flHealthScaleAmount = 0.0f;
	int iNumPlayers = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient)
			continue;

		if (!pClient->IsConnected())
			continue;

		iNumPlayers++;
	}

	if (iNumPlayers > 0)
		iNumPlayers--; // Everyone but the first player will affect scaling.

	flDamageScaleAmount = (iNumPlayers * GetScaleValue(true));
	flHealthScaleAmount = (iNumPlayers * GetScaleValue(false));

	float defaultTotalHP = GameBaseShared()->GetNPCData()->GetHealth(GetNPCName());
	float flTotal = (flHealthScaleAmount * (float)((float)defaultTotalHP / 100)) + defaultTotalHP;
	m_iTotalHP = (int)flTotal;

	float damageSingle = GameBaseShared()->GetNPCData()->GetSlashDamage(GetNPCName());
	float damageBoth = GameBaseShared()->GetNPCData()->GetDoubleSlashDamage(GetNPCName());
	float damageKick = GameBaseShared()->GetNPCData()->GetKickDamage(GetNPCName());

	m_iDamageOneHand = (flDamageScaleAmount * (float)(damageSingle / 100)) + damageSingle;
	m_iDamageBothHands = (flDamageScaleAmount * (float)(damageBoth / 100)) + damageBoth;
	m_iDamageKick = (flDamageScaleAmount * (float)(damageKick / 100)) + damageKick;

	float defaultXP = (float)GameBaseShared()->GetNPCData()->GetXP(GetNPCName());
	float newXPValue = ((flHealthScaleAmount + flDamageScaleAmount) * (defaultXP / 100.0f)) + defaultXP;
	m_iXPToGive = (int)newXPValue;

	CBaseEntity *pEntity = UTIL_EntityByIndex(GetEntIndex());
	if (pEntity)
	{
		float newHP = 0.0f;
		float hpPercentLeft = (float)(((float)pEntity->GetHealth()) / ((float)pEntity->GetMaxHealth()));

		if (hpPercentLeft >= 1) // No HP lost.
			newHP = flTotal;
		else // HP lost
			newHP = (float)((float)((float)m_iTotalHP / 100) * (hpPercentLeft * 100));

		newHP = round(newHP);

		if (newHP <= 0)
			newHP = 1;

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

	if (!strcmp(type, "player_connection"))
	{
		UpdateNPCScaling();
	}

	// Human Events:
	CBaseEntity *pEntity = UTIL_EntityByIndex(GetEntIndex());
	if (!pEntity)
		return;

	if (pEntity->Classify() != CLASS_COMBINE)
		return;

	if (!strcmp(type, "round_started"))
	{
		HL2MPRules()->EmitSoundToClient(pEntity, "Ready", GetNPCType(), GetGender());
	}
}
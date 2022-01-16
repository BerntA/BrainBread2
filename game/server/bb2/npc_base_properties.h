//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base properties for all npcs, all npcs will be linked to a script in data/npc. It contains basic values to make it easier to test & make modifications in general.
//
//========================================================================================//

#ifndef NPC_BASE_PROPERTIES_H
#define NPC_BASE_PROPERTIES_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"

class CNPCDataItem;
class CAI_BaseNPC;

enum NPC_CLASS_TYPES
{
	NPC_CLASS_WALKER = 0,
	NPC_CLASS_RUNNER,
	NPC_CLASS_FRED,
	NPC_CLASS_BANDIT,
	NPC_CLASS_BANDIT_LEADER,
	NPC_CLASS_BANDIT_JOHNSON,
	NPC_CLASS_MILITARY,
	NPC_CLASS_POLICE,
	NPC_CLASS_RIOT,
	NPC_CLASS_SWAT,
	NPC_CLASS_PRIEST,
};

abstract_class CNPCBaseProperties : public CGameEventListener
{
public:

	CNPCBaseProperties();

	// Parses the npc data from data/npc.
	virtual bool ParseNPC(CBaseEntity *pEntity);

	// Every NPC will scale up with the amount of players in game to make sure it always is a challenge! (also take the player's level into account?)
	virtual void UpdateNPCScaling();

	virtual void UpdateMeleeRange(const Vector &bounds);

	// Shared Accessors.
	virtual bool GetGender() { return m_bGender; }
	virtual int GetXP() { return m_iXPToGive; }

	static const char *GetNPCScript(int type);

	virtual const char *GetNPCName() { return (m_pNPCData ? m_pNPCData->szIndex : "N/A"); }
	virtual int GetNPCClassType() { return -1; }

	const char *GetNPCModelName() { return pszModelName; }
	const char *GetOverridenSoundSet() { return pszSoundsetOverride; }

	void OnTookDamage(const CTakeDamageInfo &info, const float flHealth); // Give XP to attackers.

protected:

	// Define what type of npcs this is: (a broad selection, classify is shared, = fred and other bosses go under the same classification as a walker, however this type is more specific for every 'kind'.
	virtual BB2_SoundTypes GetNPCType() { return TYPE_UNKNOWN; }

	virtual float GetScaleValue(bool bDamageScale);
	virtual void OnNPCScaleUpdated(void) { }
	virtual void FireGameEvent(IGameEvent *event);

	int m_iModelSkin;
	int m_iTotalHP;
	int m_iXPToGive;
	int m_iDamageOneHand;
	int m_iDamageBothHands;
	int m_iDamageKick;
	float m_flRange;
	bool m_bGender;

	float m_flSpeedFactorValue;
	float m_flDamageScaleValue;
	float m_flHealthScaleValue;

	char pszModelName[MAX_WEAPON_STRING];
	char pszSoundsetOverride[MAX_WEAPON_STRING];

	CNPCDataItem *m_pNPCData;

private:

	int m_iDefaultHealth;
	int m_iDefaultDamage1H;
	int m_iDefaultDamage2H;
	int m_iDefaultKickDamage;

	CBaseEntity *m_pOuter;
};

#endif // NPC_BASE_PROPERTIES_H
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

#include "ai_basenpc.h"
#include "ai_basehumanoid.h"
#include "ai_behavior.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_functank.h"
#include "ai_behavior_rappel.h"
#include "ai_behavior_actbusy.h"
#include "ai_sentence.h"
#include "ai_baseactor.h"
#include "hl2mp_gamerules.h"
#include "GameEventListener.h"
#include "hl2mp_player.h"

extern ConVar ai_show_active_military_activities;

abstract_class CNPCBaseProperties : public CGameEventListener
{
public:

	CNPCBaseProperties();

	// Parses the npc data from data/npc.
	virtual bool ParseNPC(int index);

	// Every NPC will scale up with the amount of players in game to make sure it always is a challenge! (also take the player's level into account?)
	virtual void UpdateNPCScaling();

	// Shared Accessors.
	virtual bool GetGender() { return m_bGender; }
	virtual int GetXP() { return m_iXPToGive; }

	// Every NPC should override this!
	virtual const char *GetNPCName() { return "UNKNOWN"; }

	virtual int GetEntIndex() { return m_iEntIndex; }

	const char *GetNPCModelName() { return pszModelName; }

	void SetEntIndex(int index) { m_iEntIndex = index; }

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
	int m_iEntIndex;
	float m_flRange;
	bool m_bGender;

	float m_flDamageScaleValue;
	float m_flHealthScaleValue;

	char pszModelName[MAX_WEAPON_STRING];
};

#endif // NPC_BASE_PROPERTIES_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Creates an objective icon handler - sent to every client.
//
//========================================================================================//

#ifndef OBJ_ICON_H
#define OBJ_ICON_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "baseentity.h"
#include "baseanimating.h"
#include "props.h"
#include "basecombatcharacter.h"

class CObjectiveIcon : public CBaseEntity
{
public:
	DECLARE_CLASS(CObjectiveIcon, CBaseEntity);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CObjectiveIcon();

	void Spawn();

	int UpdateTransmitState();
	int ShouldTransmit(const CCheckTransmitInfo *pInfo);

	void SetTeamLink(int team) { m_iTeamNumber.Set(team); }

	void HideIcon(bool bHide) { m_bShouldBeHidden = bHide; }
	bool ShouldHide() { return m_bShouldBeHidden; }

	// Linking icons to quests will add an ID (objective num) link:
	int GetRelatedQuestObjectiveID() { return m_iQuestIDLink; }
	void SetRelatedQuestObjectiveID(int id) { m_iQuestIDLink = id; }

	void SetObjectiveIconTexture(const char *szIconState, bool bVisible);

	void InputShowIcon(inputdata_t &data);
	void InputHideIcon(inputdata_t &data);

private:

	int m_iQuestIDLink;
	string_t c_szTextureFile;

	CNetworkVar(bool, m_bShouldBeHidden);
	CNetworkVar(int, m_iTeamNumber);
	CNetworkString(m_szTextureFile, MAX_WEAPON_STRING);
};

#endif // OBJ_ICON_H
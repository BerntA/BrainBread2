//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Creates an objective icon sprite.
//
//========================================================================================//

#include "cbase.h"
#include "objective_icon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CObjectiveIcon, DT_ObjectiveIcon)
SendPropBool(SENDINFO(m_bShouldBeHidden)),
SendPropInt(SENDINFO(m_iTeamNumber), 2, SPROP_UNSIGNED),
SendPropString(SENDINFO(m_szTextureFile)),
END_SEND_TABLE()

BEGIN_DATADESC(CObjectiveIcon)
DEFINE_KEYFIELD(c_szTextureFile, FIELD_STRING, "Texture"),
DEFINE_KEYFIELD(m_iTeamNumber, FIELD_INTEGER, "Team"),
DEFINE_KEYFIELD(m_bShouldBeHidden, FIELD_BOOLEAN, "Hidden"),

DEFINE_INPUTFUNC(FIELD_VOID, "ShowIcon", InputShowIcon),
DEFINE_INPUTFUNC(FIELD_VOID, "HideIcon", InputHideIcon),

DEFINE_ARRAY(m_szTextureFile, FIELD_CHARACTER, MAX_WEAPON_STRING),
END_DATADESC()

CObjectiveIcon::CObjectiveIcon()
{
	Q_strncpy(m_szTextureFile.GetForModify(), "", MAX_WEAPON_STRING);
	m_bShouldBeHidden = false;
	m_iTeamNumber = TEAM_HUMANS;
	c_szTextureFile = NULL_STRING;
}

void CObjectiveIcon::Spawn()
{
	BaseClass::Spawn();

	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	SetBlocksLOS(false);

	SetTransmitState(FL_EDICT_ALWAYS);
	DispatchUpdateTransmitState();

	if (c_szTextureFile != NULL_STRING)
		SetObjectiveIconTexture(STRING(c_szTextureFile), !m_bShouldBeHidden);
}

void CObjectiveIcon::SetObjectiveIconTexture(const char *szTexture, bool bVisible)
{
	if (!szTexture || !szTexture[0])
	{
		Warning("Invalid texture passed in to objective icon '%s'!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	Q_strncpy(m_szTextureFile.GetForModify(), szTexture, MAX_WEAPON_STRING);
	HideIcon(!bVisible);
}

int CObjectiveIcon::UpdateTransmitState(void)
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}

int CObjectiveIcon::ShouldTransmit(const CCheckTransmitInfo *pInfo)
{
	return FL_EDICT_ALWAYS;
}

void CObjectiveIcon::InputShowIcon(inputdata_t &data)
{
	HideIcon(false);
}

void CObjectiveIcon::InputHideIcon(inputdata_t &data)
{
	HideIcon(true);
}

LINK_ENTITY_TO_CLASS(objective_icon, CObjectiveIcon);
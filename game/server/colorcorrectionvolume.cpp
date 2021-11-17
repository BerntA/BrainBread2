//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CColorCorrectionVolume : public CBaseEntity
{
	DECLARE_CLASS(CColorCorrectionVolume, CBaseEntity);
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CColorCorrectionVolume();

	void Spawn(void);
	bool KeyValue(const char *szKeyName, const char *szValue);
	int  UpdateTransmitState();

	// Inputs
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);

private:

	CNetworkVar(bool, m_bDisabled);
	CNetworkVar(float, m_FadeDuration);
	CNetworkString(m_lookupFilename, MAX_PATH);

	CNetworkVector(m_vecBoundsMin);
	CNetworkVector(m_vecBoundsMax);
};

LINK_ENTITY_TO_CLASS(color_correction_volume, CColorCorrectionVolume);

BEGIN_DATADESC(CColorCorrectionVolume)

DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_FadeDuration, FIELD_FLOAT, "fadeDuration"),
DEFINE_AUTO_ARRAY_KEYFIELD(m_lookupFilename, FIELD_CHARACTER, "filename"),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST_NOBASE(CColorCorrectionVolume, DT_ColorCorrectionVolume)
SendPropBool(SENDINFO(m_bDisabled)),
SendPropFloat(SENDINFO(m_FadeDuration)),
SendPropString(SENDINFO(m_lookupFilename)),
SendPropVector(SENDINFO(m_vecBoundsMin), -1, SPROP_COORD),
SendPropVector(SENDINFO(m_vecBoundsMax), -1, SPROP_COORD),
END_SEND_TABLE()

CColorCorrectionVolume::CColorCorrectionVolume()
{
	m_bDisabled = false;
	m_FadeDuration = 10.0f;
	m_lookupFilename.GetForModify()[0] = 0;
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CColorCorrectionVolume::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_ALWAYS); // ALWAYS transmit to all clients.
}

bool CColorCorrectionVolume::KeyValue(const char *szKeyName, const char *szValue)
{
	if (FStrEq(szKeyName, "filename"))
	{
		Q_strncpy(m_lookupFilename.GetForModify(), szValue, MAX_PATH);
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

void CColorCorrectionVolume::Spawn(void)
{
	AddEFlags(EFL_FORCE_CHECK_TRANSMIT | EFL_DIRTY_ABSTRANSFORM);
	Precache();
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	SetModel(STRING(GetModelName()));
	SetBlocksLOS(false);
	m_nRenderMode = kRenderEnvironmental;

	m_vecBoundsMin = WorldSpaceCenter() + WorldAlignMins();
	m_vecBoundsMax = WorldSpaceCenter() + WorldAlignMaxs();
}

void CColorCorrectionVolume::InputEnable(inputdata_t &inputdata)
{
	m_bDisabled = false;
}

void CColorCorrectionVolume::InputDisable(inputdata_t &inputdata)
{
	m_bDisabled = true;
}
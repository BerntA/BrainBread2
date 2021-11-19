//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Client Simulated Color Correction
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CColorCorrection : public CBaseEntity
{
	DECLARE_CLASS(CColorCorrection, CBaseEntity);
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CColorCorrection();

	void Spawn(void);
	bool KeyValue(const char *szKeyName, const char *szValue);
	int  UpdateTransmitState();

	// Inputs
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);

private:

	CNetworkVar(bool, m_bDisabled);
	CNetworkVar(float, m_minFalloff);
	CNetworkVar(float, m_maxFalloff);
	CNetworkString(m_lookupFilename, MAX_PATH);
};

LINK_ENTITY_TO_CLASS(color_correction, CColorCorrection);

BEGIN_DATADESC(CColorCorrection)

DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_minFalloff, FIELD_FLOAT, "minfalloff"),
DEFINE_KEYFIELD(m_maxFalloff, FIELD_FLOAT, "maxfalloff"),
DEFINE_AUTO_ARRAY_KEYFIELD(m_lookupFilename, FIELD_CHARACTER, "filename"),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

END_DATADESC()

extern void SendProxy_Origin(const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);

IMPLEMENT_SERVERCLASS_ST_NOBASE(CColorCorrection, DT_ColorCorrection)
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
SendPropFloat(SENDINFO(m_minFalloff)),
SendPropFloat(SENDINFO(m_maxFalloff)),
SendPropString(SENDINFO(m_lookupFilename)),
SendPropBool(SENDINFO(m_bDisabled)),
END_SEND_TABLE()

CColorCorrection::CColorCorrection()
{
	m_bDisabled = false;
	m_lookupFilename.GetForModify()[0] = 0;
	m_minFalloff = 0.0f;
	m_maxFalloff = 1000.0f;
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CColorCorrection::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState(FL_EDICT_ALWAYS);
}

bool CColorCorrection::KeyValue(const char *szKeyName, const char *szValue)
{
	if (FStrEq(szKeyName, "filename"))
	{
		Q_strncpy(m_lookupFilename.GetForModify(), szValue, MAX_PATH);
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

void CColorCorrection::Spawn(void)
{
	AddEFlags(EFL_FORCE_CHECK_TRANSMIT | EFL_DIRTY_ABSTRANSFORM);
	Precache();
	SetSolid(SOLID_NONE);
	BaseClass::Spawn();
}

void CColorCorrection::InputEnable(inputdata_t &inputdata)
{
	m_bDisabled = false;
}

void CColorCorrection::InputDisable(inputdata_t &inputdata)
{
	m_bDisabled = true;
}
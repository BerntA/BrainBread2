//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Client Simulated Color Correction
//
//=============================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "cdll_client_int.h"
#include "colorcorrectionmgr.h"
#include "materialsystem/MaterialSystemUtil.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar mat_colcorrection_disableentities("mat_colcorrection_disableentities", "0", FCVAR_NONE, "Disable map color-correction entities");

//------------------------------------------------------------------------------
// Purpose : Color correction entity with radial falloff
//------------------------------------------------------------------------------
class C_ColorCorrection : public C_BaseEntity, public IColorCorrectionEntity
{
public:
	DECLARE_CLASS(C_ColorCorrection, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_ColorCorrection();
	virtual ~C_ColorCorrection();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw() { return false; }
	bool ShouldDrawColorCorrection();
	float GetColorCorrectionScale();
	float GetColorCorrectionMaxWeight() { return m_flMaxWeight; }

private:
	Vector	m_vecOrigin;
	bool	m_bDisabled;
	float	m_flMinFalloff;
	float	m_flMaxFalloff;
	float	m_flMaxWeight;
	char	m_lookupFilename[MAX_PATH];
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrection, DT_ColorCorrection, CColorCorrection)
RecvPropVector(RECVINFO(m_vecOrigin)),
RecvPropFloat(RECVINFO(m_flMinFalloff)),
RecvPropFloat(RECVINFO(m_flMaxFalloff)),
RecvPropFloat(RECVINFO(m_flMaxWeight)),
RecvPropBool(RECVINFO(m_bDisabled)),
RecvPropString(RECVINFO(m_lookupFilename)),
END_RECV_TABLE()

C_ColorCorrection::C_ColorCorrection()
{
	m_bLoadedColorCorrection = false;
}

C_ColorCorrection::~C_ColorCorrection()
{
	g_pColorCorrectionMgr->RemoveColorCorrection(this, m_lookupFilename);
}

void C_ColorCorrection::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if ((updateType == DATA_UPDATE_CREATED) && !m_bLoadedColorCorrection && m_lookupFilename && m_lookupFilename[0])
	{
		g_pColorCorrectionMgr->AddColorCorrection(this, m_lookupFilename);
		m_bLoadedColorCorrection = true;
	}
}

bool C_ColorCorrection::ShouldDrawColorCorrection()
{
	if (m_bDisabled || mat_colcorrection_disableentities.GetInt())
		return false;

	if (m_flMaxFalloff == -1) // Render everywhere!
		return true;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	const float flDist = (pPlayer->GetLocalOrigin() - m_vecOrigin).Length();
	return (flDist <= m_flMaxFalloff);
}

float C_ColorCorrection::GetColorCorrectionScale()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && (m_flMinFalloff != -1) && (m_flMaxFalloff != -1) && (m_flMinFalloff != m_flMaxFalloff))
	{
		const float dist = (pPlayer->GetLocalOrigin() - m_vecOrigin).Length();
		float weight = (dist - m_flMinFalloff) / (m_flMaxFalloff - m_flMinFalloff);
		if (weight < 0.0f) weight = 0.0f;
		if (weight > 1.0f) weight = 1.0f;
		return (1.0f - weight);
	}
	return 1.0f;
}
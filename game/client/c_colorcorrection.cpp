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
	bool ShouldDraw();
	bool ShouldDrawColorCorrection();
	float GetColorCorrectionScale();

private:
	Vector	m_vecOrigin;
	float	m_minFalloff;
	float	m_maxFalloff;
	char	m_lookupFilename[MAX_PATH];
	bool	m_bDisabled;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrection, DT_ColorCorrection, CColorCorrection)
RecvPropVector(RECVINFO(m_vecOrigin)),
RecvPropFloat(RECVINFO(m_minFalloff)),
RecvPropFloat(RECVINFO(m_maxFalloff)),
RecvPropString(RECVINFO(m_lookupFilename)),
RecvPropBool(RECVINFO(m_bDisabled)),
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

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_ColorCorrection::ShouldDraw()
{
	return false;
}

bool C_ColorCorrection::ShouldDrawColorCorrection()
{
	if (m_bDisabled || mat_colcorrection_disableentities.GetInt())
		return false;

	if (m_maxFalloff == -1) // Render everywhere!
		return true;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	const Vector &playerOrigin = pPlayer->GetLocalOrigin();
	const float flDist = (playerOrigin - m_vecOrigin).Length();

	return (flDist <= m_maxFalloff);
}

float C_ColorCorrection::GetColorCorrectionScale()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && (m_minFalloff != -1) && (m_maxFalloff != -1) && (m_minFalloff != m_maxFalloff))
	{
		const Vector &playerOrigin = pPlayer->GetLocalOrigin();
		const float dist = (playerOrigin - m_vecOrigin).Length();
		float weight = (dist - m_minFalloff) / (m_maxFalloff - m_minFalloff);
		if (weight < 0.0f) weight = 0.0f;
		if (weight > 1.0f) weight = 1.0f;
		return (1.0f - weight);
	}
	return 1.0f;
}
//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Client Simulated Color Correction
//
//=============================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "cdll_client_int.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "colorcorrectionmgr.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_ColorCorrectionVolume : public C_BaseEntity, public IColorCorrectionEntity
{
public:
	DECLARE_CLASS(C_ColorCorrectionVolume, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_ColorCorrectionVolume();
	virtual ~C_ColorCorrectionVolume();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw() { return false; }
	bool ShouldDrawColorCorrection();
	float GetColorCorrectionScale() { return 1.0f; }
	float GetColorCorrectionMaxWeight() { return m_flMaxWeight; }

private:
	bool	m_bDisabled;
	float	m_flMaxWeight;
	char	m_lookupFilename[MAX_PATH];
	Vector m_vecBoundsMin;
	Vector m_vecBoundsMax;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrectionVolume, DT_ColorCorrectionVolume, CColorCorrectionVolume)
RecvPropBool(RECVINFO(m_bDisabled)),
RecvPropFloat(RECVINFO(m_flMaxWeight)),
RecvPropVector(RECVINFO(m_vecBoundsMin)),
RecvPropVector(RECVINFO(m_vecBoundsMax)),
RecvPropString(RECVINFO(m_lookupFilename)),
END_RECV_TABLE()

C_ColorCorrectionVolume::C_ColorCorrectionVolume()
{
	m_bLoadedColorCorrection = false;
}

C_ColorCorrectionVolume::~C_ColorCorrectionVolume()
{
	g_pColorCorrectionMgr->RemoveColorCorrection(this, m_lookupFilename);
}

void C_ColorCorrectionVolume::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if ((updateType == DATA_UPDATE_CREATED) && !m_bLoadedColorCorrection && m_lookupFilename && m_lookupFilename[0])
	{
		g_pColorCorrectionMgr->AddColorCorrection(this, m_lookupFilename);
		m_bLoadedColorCorrection = true;
	}
}

bool C_ColorCorrectionVolume::ShouldDrawColorCorrection()
{
	if (m_bDisabled)
		return false;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	Vector playerOrigin = pPlayer->GetLocalOrigin();
	playerOrigin.z += 4.0f;
	if (!IsPointInBox(playerOrigin, m_vecBoundsMin, m_vecBoundsMax))
		return false;

	return true;
}
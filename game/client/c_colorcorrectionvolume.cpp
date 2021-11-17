//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "cdll_client_int.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "colorcorrectionmgr.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_ColorCorrectionVolume : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_ColorCorrectionVolume, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_ColorCorrectionVolume();
	virtual ~C_ColorCorrectionVolume();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();
	void ClientThink();

protected:
	bool ShouldDrawCorrection();

private:
	bool	m_bDisabled;
	float	m_FadeDuration;
	char	m_lookupFilename[MAX_PATH];

	float	m_Weight;
	bool	m_bCanDraw;

	float	m_LastEnterWeight;
	float	m_LastEnterTime;

	float	m_LastExitWeight;
	float	m_LastExitTime;

	ClientCCHandle_t m_CCHandle;
	Vector m_vecBoundsMin;
	Vector m_vecBoundsMax;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrectionVolume, DT_ColorCorrectionVolume, CColorCorrectionVolume)
RecvPropBool(RECVINFO(m_bDisabled)),
RecvPropFloat(RECVINFO(m_FadeDuration)),
RecvPropString(RECVINFO(m_lookupFilename)),
RecvPropVector(RECVINFO(m_vecBoundsMin)),
RecvPropVector(RECVINFO(m_vecBoundsMax)),
END_RECV_TABLE()

C_ColorCorrectionVolume::C_ColorCorrectionVolume()
{
	m_CCHandle = INVALID_CLIENT_CCHANDLE;
	m_bCanDraw = false;
	m_Weight = m_LastEnterWeight = m_LastExitWeight = m_LastEnterTime = m_LastExitTime = 0.0f;
}

C_ColorCorrectionVolume::~C_ColorCorrectionVolume()
{
	g_pColorCorrectionMgr->RemoveColorCorrection(m_CCHandle);
}

void C_ColorCorrectionVolume::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		if (m_CCHandle == INVALID_CLIENT_CCHANDLE)
		{
			char filename[MAX_PATH];
			Q_strncpy(filename, m_lookupFilename, MAX_PATH);

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection(filename);
			SetNextClientThink((m_CCHandle != INVALID_CLIENT_CCHANDLE) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER);
		}
	}
}

bool C_ColorCorrectionVolume::ShouldDraw()
{
	return false; // Nothing to draw!
}

bool C_ColorCorrectionVolume::ShouldDrawCorrection()
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

void C_ColorCorrectionVolume::ClientThink()
{
	const bool bShouldDraw = ShouldDrawCorrection();

	if (m_bCanDraw != bShouldDraw)
	{
		m_bCanDraw = bShouldDraw;
		if (m_bCanDraw)
		{
			m_LastEnterTime = gpGlobals->curtime;
			m_LastEnterWeight = m_Weight;
		}
		else
		{
			m_LastExitTime = gpGlobals->curtime;
			m_LastExitWeight = m_Weight;
		}
	}

	if (m_LastEnterTime > m_LastExitTime)
	{
		// we most recently entered the volume
		if (m_Weight < 1.0f)
		{
			float dt = gpGlobals->curtime - m_LastEnterTime;
			float weight = m_LastEnterWeight + dt / ((1.0f - m_LastEnterWeight)*m_FadeDuration);
			if (weight > 1.0f)
				weight = 1.0f;
			m_Weight = weight;
		}
	}
	else
	{
		// we most recently exitted the volume
		if (m_Weight > 0.0f)
		{
			float dt = gpGlobals->curtime - m_LastExitTime;
			float weight = (1.0f - m_LastExitWeight) + dt / (m_LastExitWeight*m_FadeDuration);
			if (weight > 1.0f)
				weight = 1.0f;
			m_Weight = 1.0f - weight;
		}
	}

	g_pColorCorrectionMgr->SetColorCorrectionWeight(m_CCHandle, m_Weight);
}
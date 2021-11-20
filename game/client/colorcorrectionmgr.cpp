//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Color Correction Manager
//
//=============================================================================//

#include "cbase.h"
#include "tier0/vprof.h"
#include "colorcorrectionmgr.h"

static ConVar mat_colcorrection_fade("mat_colcorrection_fade", "2.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set color correction fade in and fade out time.");

//------------------------------------------------------------------------------
// Singleton access
//------------------------------------------------------------------------------
static CColorCorrectionMgr s_ColorCorrectionMgr;
CColorCorrectionMgr *g_pColorCorrectionMgr = &s_ColorCorrectionMgr;

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------
CColorCorrectionMgr::CColorCorrectionMgr()
{
	m_nActiveWeightCount = 0;
}

//------------------------------------------------------------------------------
// Creates color corrections
//------------------------------------------------------------------------------
void CColorCorrectionMgr::AddColorCorrection(IColorCorrectionEntity *pEntity, const char *file)
{
	CColorCorrectionEntry *pColCorr = FindColorCorrection(file);
	if (pColCorr == NULL)
	{
		pColCorr = new CColorCorrectionEntry(file);
		m_pEntries.AddToTail(pColCorr);
	}
	pColCorr->m_pEntities.AddToTail(pEntity);
}

//------------------------------------------------------------------------------
// Destroys color corrections
//------------------------------------------------------------------------------
void CColorCorrectionMgr::RemoveColorCorrection(IColorCorrectionEntity *pEntity, const char *file)
{
	int index = -1;
	CColorCorrectionEntry *pColCorr = FindColorCorrection(file, &index);
	if (pColCorr == NULL)
		return;
	pColCorr->m_pEntities.FindAndRemove(pEntity);
	if (pColCorr->m_pEntities.Count() == 0)
	{
		delete m_pEntries[index];
		m_pEntries.Remove(index);
	}
}

//------------------------------------------------------------------------------
// Find color correction entry
//------------------------------------------------------------------------------
CColorCorrectionEntry *CColorCorrectionMgr::FindColorCorrection(const char *file, int *index)
{
	for (int i = (m_pEntries.Count() - 1); i >= 0; i--)
	{
		if (!strcmp(m_pEntries[i]->GetFileName(), file))
		{
			if (index)
				(*index) = i;
			return m_pEntries[i];
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------
// Modify color correction weights
//------------------------------------------------------------------------------
void CColorCorrectionMgr::SetColorCorrectionWeight(ClientCCHandle_t h, float flWeight)
{
	if (h != INVALID_CLIENT_CCHANDLE)
	{
		CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
		ColorCorrectionHandle_t ccHandle = (ColorCorrectionHandle_t)h;
		pRenderContext->SetLookupWeight(ccHandle, flWeight);

		// FIXME: NOTE! This doesn't work if the same handle has
		// its weight set twice with no intervening calls to ResetColorCorrectionWeights
		// which, at the moment, is true
		if (flWeight != 0.0f)
		{
			++m_nActiveWeightCount;
		}
	}
}

void CColorCorrectionMgr::UpdateColorCorrection()
{
	VPROF_("UpdateColorCorrection", 2, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0);
	CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
	pRenderContext->ResetLookupWeights();
	m_nActiveWeightCount = 0;

	for (int i = 0; i < m_pEntries.Count(); i++)
		m_pEntries[i]->Update();
}

void CColorCorrectionMgr::SetResetable(ClientCCHandle_t h, bool bResetable)
{
	// NOTE: Setting stuff to be not resettable doesn't work when in queued mode
	// because the logic that sets m_nActiveWeightCount to 0 in ResetColorCorrectionWeights
	// is no longer valid when stuff is not resettable.
	Assert(bResetable || !g_pMaterialSystem->GetThreadMode() == MATERIAL_SINGLE_THREADED);
	if (h != INVALID_CLIENT_CCHANDLE)
	{
		CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
		ColorCorrectionHandle_t ccHandle = (ColorCorrectionHandle_t)h;
		pRenderContext->SetResetable(ccHandle, bResetable);
	}
}

//------------------------------------------------------------------------------
// Is color correction active?
//------------------------------------------------------------------------------
bool CColorCorrectionMgr::HasNonZeroColorCorrectionWeights() const
{
	return (m_nActiveWeightCount != 0);
}

//
// Color Correction Entry Item
//

CColorCorrectionEntry::CColorCorrectionEntry(const char *file)
{
	m_bCanDraw = false;
	m_Weight = m_LastEnterWeight = m_LastExitWeight = m_LastEnterTime = m_LastExitTime = 0.0f;

	Q_strncpy(m_lookupFilename, file, sizeof(m_lookupFilename));
	CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
	ColorCorrectionHandle_t ccHandle = pRenderContext->AddLookup(m_lookupFilename);

	if (ccHandle)
	{
		pRenderContext->LockLookup(ccHandle);
		pRenderContext->LoadLookup(ccHandle, m_lookupFilename);
		pRenderContext->UnlockLookup(ccHandle);
	}
	else
		Warning("Cannot find color correction lookup file: '%s'\n", m_lookupFilename);

	m_CCHandle = ((ClientCCHandle_t)ccHandle);
}

CColorCorrectionEntry::~CColorCorrectionEntry()
{
	m_pEntities.RemoveAll();

	if (m_CCHandle == INVALID_CLIENT_CCHANDLE)
		return;

	CMatRenderContextPtr pRenderContext(g_pMaterialSystem);
	ColorCorrectionHandle_t ccHandle = ((ColorCorrectionHandle_t)m_CCHandle);
	pRenderContext->RemoveLookup(ccHandle);
}

void CColorCorrectionEntry::Update()
{
	if (m_CCHandle == INVALID_CLIENT_CCHANDLE)
		return;

	bool bShouldDraw = false;
	float flScale = 1.0f;
	float flMaxWeight = 1.0f;
	const float flFadeTime = mat_colcorrection_fade.GetFloat();

	for (int i = 0; i < m_pEntities.Count(); i++)
	{
		if (m_pEntities[i]->ShouldDrawColorCorrection())
		{
			bShouldDraw = true;
			const float flNewScale = m_pEntities[i]->GetColorCorrectionScale();
			const float flNewMaxWeight = m_pEntities[i]->GetColorCorrectionMaxWeight();

			if (flNewScale < flScale)
				flScale = flNewScale;

			if (flNewMaxWeight < flMaxWeight)
				flMaxWeight = flNewMaxWeight;
		}
	}

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
		// we most recently entered the bounds
		if (m_Weight < 1.0f)
		{
			float dt = gpGlobals->curtime - m_LastEnterTime;
			float weight = m_LastEnterWeight + dt / ((1.0f - m_LastEnterWeight)*flFadeTime);
			if (weight > 1.0f)
				weight = 1.0f;
			m_Weight = weight;
		}
	}
	else
	{
		// we most recently exitted the bounds
		if (m_Weight > 0.0f)
		{
			float dt = gpGlobals->curtime - m_LastExitTime;
			float weight = (1.0f - m_LastExitWeight) + dt / (m_LastExitWeight*flFadeTime);
			if (weight > 1.0f)
				weight = 1.0f;
			m_Weight = 1.0f - weight;
		}
	}

	g_pColorCorrectionMgr->SetColorCorrectionWeight(m_CCHandle, MIN(m_Weight * flScale, flMaxWeight));
}
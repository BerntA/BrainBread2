//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Color Correction Manager
//
//=============================================================================//

#ifndef COLORCORRECTIONMGR_H
#define COLORCORRECTIONMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

//------------------------------------------------------------------------------
// Purpose : Singleton manager for color correction on the client
//------------------------------------------------------------------------------

DECLARE_POINTER_HANDLE(ClientCCHandle_t);
#define INVALID_CLIENT_CCHANDLE ( (ClientCCHandle_t)0 )

class IColorCorrectionEntity
{
public:
	virtual bool ShouldDrawColorCorrection(bool &bNoFade) = 0;
	virtual float GetColorCorrectionScale() = 0;
	virtual float GetColorCorrectionMaxWeight() = 0;
protected:
	bool m_bLoadedColorCorrection;
};

class CColorCorrectionEntry
{
public:
	CColorCorrectionEntry(const char *file);
	virtual ~CColorCorrectionEntry();

	virtual void Update();
	virtual const char *GetFileName(void){ return m_lookupFilename; }

	CUtlVector<IColorCorrectionEntity*> m_pEntities;

private:
	char m_lookupFilename[MAX_PATH];
	float m_Weight;

	ClientCCHandle_t m_CCHandle;
	CColorCorrectionEntry(const CColorCorrectionEntry &); // undefined
};

class CColorCorrectionMgr : public CBaseGameSystem
{
public:
	virtual char const *Name() { return "Color Correction Mgr"; }

	CColorCorrectionMgr();

	void AddColorCorrection(IColorCorrectionEntity *pEntity, const char *file);
	void RemoveColorCorrection(IColorCorrectionEntity *pEntity, const char *file);
	CColorCorrectionEntry *FindColorCorrection(const char *file, int *index = NULL);

	// Modify color correction weights
	void SetColorCorrectionWeight(ClientCCHandle_t h, float flWeight);
	void SetResetable(ClientCCHandle_t h, bool bResetable);
	void UpdateColorCorrection();

	// Is color correction active?
	bool HasNonZeroColorCorrectionWeights() const;

private:
	int m_nActiveWeightCount;
	CUtlVector<CColorCorrectionEntry*> m_pEntries;
};

//------------------------------------------------------------------------------
// Singleton access
//------------------------------------------------------------------------------
extern CColorCorrectionMgr *g_pColorCorrectionMgr;

#endif // COLORCORRECTIONMGR_H
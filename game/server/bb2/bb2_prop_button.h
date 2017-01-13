//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: Button with model ( dynamic ) for skin change. ++ more - It also has a KeyPad mode for more fancy interaction!
//
//========================================================================================//

#ifndef BB2_PROP_BUTTON_H
#define BB2_PROP_BUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "baseanimating.h"
#include "items.h"

class CBB2Button : public CItem
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS(CBB2Button, CItem);

	CBB2Button(void);

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void ShowModel(inputdata_t &inputData);
	void HideModel(inputdata_t &inputData);
	void ShowGlow(inputdata_t &inputData);
	void HideGlow(inputdata_t &inputData);

	void EnableButton(inputdata_t &inputData);
	void DisableButton(inputdata_t &inputData);

	void SetGlowType(inputdata_t &inputData);

	void FireKeyPadOutput(CBasePlayer *pClient);

	COutputEvent m_OnUse;

	int ClassifyFor;
	int m_iGlowType;

	bool m_bStartGlowing;
	bool m_bShowModel;
	bool m_bIsEnabled;
	bool m_bIsKeyPad;
	string_t szKeyPadCode;

	color32 m_clrGlow;

	// Think
	void UpdateThink();
};

#endif // BB2_PROP_BUTTON_H
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
#include "BaseKeyPadEntity.h"

class CPropButton : public CItem, public CBaseKeyPadEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS(CPropButton, CItem);

	CPropButton(void);

	void Spawn(void);
	void Precache(void);

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void ShowModel(inputdata_t &inputData);
	void HideModel(inputdata_t &inputData);
	void ShowGlow(inputdata_t &inputData);
	void HideGlow(inputdata_t &inputData);

	void SetGlowType(inputdata_t &inputData);

	void UnlockSuccess(CHL2MP_Player *pUnlocker);
	void UnlockFail(CHL2MP_Player *pUnlocker);

	void UpdateThink();

private:

	int ClassifyFor;
	int m_iGlowType;

	bool m_bStartGlowing;
	bool m_bShowModel;
	bool m_bIsKeyPad;

	color32 m_clrGlow;

	COutputEvent m_OnUse;
	COutputEvent m_OnKeyPadSuccess;
	COutputEvent m_OnKeyPadFail;
};

#endif // BB2_PROP_BUTTON_H
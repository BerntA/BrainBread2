//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
// Client-side CBasePlayer

#ifndef C_STUDIOFLEX_H
#define C_STUDIOFLEX_H
#pragma once

#include "c_baseanimating.h"
#include "c_baseanimatingoverlay.h"
#include "utlvector.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BaseFlex : public C_BaseAnimatingOverlay
{
	DECLARE_CLASS(C_BaseFlex, C_BaseAnimatingOverlay);
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_BaseFlex();
	virtual			~C_BaseFlex();

	virtual CStudioHdr *OnNewModel(void);

	virtual void	StandardBlendingRules(CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask);

	virtual void	OnThreadedDrawSetup();

	// model specific
	virtual void	BuildTransformations(CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed);
	static void		LinkToGlobalFlexControllers(CStudioHdr *hdr);
	virtual	void	SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights);
	virtual	bool	SetupGlobalWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights);
	static void		RunFlexDelay(int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights, float &flFlexDelayTime);
	virtual	void	SetupLocalWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights);
	virtual bool	UsesFlexDelayedWeights();

	static	void	RunFlexRules(CStudioHdr *pStudioHdr, float *dest);

	virtual Vector	SetViewTarget(CStudioHdr *pStudioHdr);

	virtual bool	GetSoundSpatialization(SpatializationInfo_t& info);

	virtual void	GetToolRecordingState(KeyValues *msg);

	void			SetFlexWeight(LocalFlexController_t index, float value);
	float			GetFlexWeight(LocalFlexController_t index);

	// Look up flex controller index by global name
	LocalFlexController_t				FindFlexController(const char *szName);

public:
	Vector			m_viewtarget;
	CInterpolatedVar< Vector >	m_iv_viewtarget;
	// indexed by model local flexcontroller
	float			m_flexWeight[MAXSTUDIOFLEXCTRL];
	CInterpolatedVarArray< float, MAXSTUDIOFLEXCTRL >	m_iv_flexWeight;

	int				m_blinktoggle;

	static int		AddGlobalFlexController(const char *szName);
	static char const *GetGlobalFlexControllerName(int idx);

private:

	float			m_blinktime;
	int				m_prevblinktoggle;

	int				m_iBlink;
	LocalFlexController_t				m_iEyeUpdown;
	LocalFlexController_t				m_iEyeRightleft;
	bool			m_bSearchedForEyeFlexes;
	int				m_iMouthAttachment;

	float			m_flFlexDelayTime;
	float			*m_flFlexDelayedWeight;
	int				m_cFlexDelayedWeight;

	// shared flex controllers
	static int		g_numflexcontrollers;
	static char		*g_flexcontroller[MAXSTUDIOFLEXCTRL * 4]; // room for global set of flexcontrollers
	static float	g_flexweight[MAXSTUDIOFLEXDESC];

private:

	C_BaseFlex(const C_BaseFlex &); // not defined, not accessible

	enum
	{
		PHONEME_CLASS_WEAK = 0,
		PHONEME_CLASS_NORMAL,
		PHONEME_CLASS_STRONG,

		NUM_PHONEME_CLASSES
	};

#ifdef HL2_CLIENT_DLL
public:

	Vector			m_vecLean;
	CInterpolatedVar< Vector >	m_iv_vecLean;
	Vector			m_vecShift;
	CInterpolatedVar< Vector >	m_iv_vecShift;
#endif
};

EXTERN_RECV_TABLE(DT_BaseFlex);

#endif // C_STUDIOFLEX_H
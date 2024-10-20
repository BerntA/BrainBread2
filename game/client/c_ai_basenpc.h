//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_AI_BASENPC_H
#define C_AI_BASENPC_H
#ifdef _WIN32
#pragma once
#endif

#include "c_basecombatcharacter.h"

// NOTE: MOved all controller code into c_basestudiomodel
class C_AI_BaseNPC : public C_BaseCombatCharacter
{
	DECLARE_CLASS(C_AI_BaseNPC, C_BaseCombatCharacter);

public:
	DECLARE_CLIENTCLASS();

	C_AI_BaseNPC();
	virtual ~C_AI_BaseNPC();

	virtual unsigned int	PhysicsSolidMaskForEntity(void) const;
	virtual bool			IsNPC(void) { return true; }
	virtual bool			IsMoving(void){ return m_bIsMoving; }
	virtual bool			ShouldAvoidObstacle(void){ return m_bPerformAvoidance; }
	virtual bool			GetRagdollInitBoneArrays(matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt) OVERRIDE;
	virtual void			ClientThink(void);
	virtual void			OnDataChanged(DataUpdateType_t type);
	virtual int				DrawModel(int flags);
	virtual bool			IsBoss(void) { return m_bIsBoss; }
	virtual const char		*GetNPCName(void) { return m_szNPCName; }

	virtual bool IsTransparent(void)
	{
		return (m_bIsFading ? true : BaseClass::IsTransparent());
	}

	virtual unsigned char GetClientSideFade(void)
	{
		return (m_bIsFading ? m_cAlphaOverride : BaseClass::GetClientSideFade());
	}

	virtual ShadowType_t ShadowCastType(void)
	{
		return (m_bIsFading ? SHADOWS_NONE : BaseClass::ShadowCastType());
	}

protected:
	virtual					void RegisterHUDHealthBar();

	bool m_bPerformAvoidance;
	bool m_bIsMoving;

	char m_szNPCName[MAX_MAP_NAME_SAVE];
	bool m_bIsBoss;
	bool m_bHasFadedIn;

private:
	C_AI_BaseNPC(const C_AI_BaseNPC &); // not defined, not accessible

	bool m_bCreatedHealthBar;

	// Fade Logic
	bool m_bIsFading;
	float m_flFadeStart;
	unsigned char m_cAlphaOverride;
};


#endif // C_AI_BASENPC_H

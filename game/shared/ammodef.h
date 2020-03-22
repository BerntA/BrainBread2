//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Holds defintion for game ammo types
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef AI_AMMODEF_H
#define AI_AMMODEF_H

#ifdef _WIN32
#pragma once
#endif

class ConVar;

struct Ammo_t
{
	char 				*pName;
	int					nDamageType;
	int					eTracerType;
	float				physicsForceImpulse;
	int					nMinSplashSize;
	int					nMaxSplashSize;
};

enum AmmoTracer_t
{
	TRACER_NONE,
	TRACER_LINE,
	TRACER_RAIL,
	TRACER_BEAM,
	TRACER_LINE_AND_WHIZ,
};

#include "shareddefs.h"

//=============================================================================
//	>> CAmmoDef
//=============================================================================
class CAmmoDef
{

public:
	int					m_nAmmoIndex;

	Ammo_t				m_AmmoType[MAX_AMMO_TYPES];

	Ammo_t				*GetAmmoOfIndex(int nAmmoIndex);
	int					Index(const char *psz);
	int					DamageType(int nAmmoIndex);
	int					TracerType(int nAmmoIndex);
	float				DamageForce(int nAmmoIndex);
	int					MinSplashSize(int nAmmoIndex);
	int					MaxSplashSize(int nAmmoIndex);

	void				AddAmmoType(char const* name, int damageType, int tracerType, float physicsForceImpulse, int minSplashSize = 4, int maxSplashSize = 8);

	CAmmoDef(void);
	virtual ~CAmmoDef(void);

private:
	bool				AddAmmoTypeInternal(char const* name, int damageType, int tracerType, int minSplashSize, int maxSplashSize);
};

// Get the global ammodef object. This is usually implemented in each mod's game rules file somewhere,
// so the mod can setup custom ammo types.
CAmmoDef* GetAmmoDef();

#endif // AI_AMMODEF_H
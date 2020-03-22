//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the Ammo at the Index passed in
//-----------------------------------------------------------------------------
Ammo_t *CAmmoDef::GetAmmoOfIndex(int nAmmoIndex)
{
	if (nAmmoIndex >= m_nAmmoIndex)
		return NULL;

	return &m_AmmoType[nAmmoIndex];
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAmmoDef::Index(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < m_nAmmoIndex; i++)
	{
		if (stricmp(psz, m_AmmoType[i].pName) == 0)
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::DamageType(int nAmmoIndex)
{
	if (nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex)
		return 0;

	return m_AmmoType[nAmmoIndex].nDamageType;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::MinSplashSize(int nAmmoIndex)
{
	if (nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex)
		return 4;

	return m_AmmoType[nAmmoIndex].nMinSplashSize;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::MaxSplashSize(int nAmmoIndex)
{
	if (nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex)
		return 8;

	return m_AmmoType[nAmmoIndex].nMaxSplashSize;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAmmoDef::TracerType(int nAmmoIndex)
{
	if (nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex)
		return 0;

	return m_AmmoType[nAmmoIndex].eTracerType;
}

float CAmmoDef::DamageForce(int nAmmoIndex)
{
	if (nAmmoIndex < 1 || nAmmoIndex >= m_nAmmoIndex)
		return 0;

	return m_AmmoType[nAmmoIndex].physicsForceImpulse;
}

//-----------------------------------------------------------------------------
// Purpose: Create an Ammo type with the name, decal, and tracer.
// Does not increment m_nAmmoIndex because the functions below do so and 
//  are the only entry point.
//-----------------------------------------------------------------------------
bool CAmmoDef::AddAmmoTypeInternal(char const* name, int damageType, int tracerType, int minSplashSize, int maxSplashSize)
{
	if (m_nAmmoIndex == MAX_AMMO_TYPES)
		return false;

	int len = strlen(name);
	m_AmmoType[m_nAmmoIndex].pName = new char[len + 1];
	Q_strncpy(m_AmmoType[m_nAmmoIndex].pName, name, len + 1);
	m_AmmoType[m_nAmmoIndex].nDamageType = damageType;
	m_AmmoType[m_nAmmoIndex].eTracerType = tracerType;
	m_AmmoType[m_nAmmoIndex].nMinSplashSize = minSplashSize;
	m_AmmoType[m_nAmmoIndex].nMaxSplashSize = maxSplashSize;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Add an ammo type with it's damage capability specified via integers
//-----------------------------------------------------------------------------
void CAmmoDef::AddAmmoType(char const* name, int damageType, int tracerType, float physicsForceImpulse, int minSplashSize, int maxSplashSize)
{
	if (AddAmmoTypeInternal(name, damageType, tracerType, minSplashSize, maxSplashSize) == false)
		return;

	m_AmmoType[m_nAmmoIndex].physicsForceImpulse = physicsForceImpulse;
	m_nAmmoIndex++;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAmmoDef::CAmmoDef(void)
{
	// Start with an index of 1.  Client assumes 0 is an invalid ammo type
	m_nAmmoIndex = 1;
	memset(m_AmmoType, 0, sizeof(m_AmmoType));
}

CAmmoDef::~CAmmoDef(void)
{
	for (int i = 1; i < MAX_AMMO_TYPES; i++)
	{
		delete[] m_AmmoType[i].pName;
	}
}
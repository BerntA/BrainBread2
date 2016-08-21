//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: TempEnt : Bullet Shot, includes tracer and impact decal logic.
//
//========================================================================================//

#ifndef TE_BULLET_SHOT_H
#define TE_BULLET_SHOT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

void TE_HL2MPFireBullets(
	int	iWeaponIndex,
	const Vector &vOrigin,
	const Vector &vStart,
	const Vector &vDir,
	int	iAmmoID,
	bool bDoTracers,
	bool bDoImpacts,
	bool bPenetrationBullet,
	bool bUseTraceHull,
	bool bDoWiz,
	bool bDoMuzzleflash,
	bool bPrimaryAttack
	);

#endif // TE_BULLET_SHOT_H
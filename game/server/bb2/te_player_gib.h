//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: TempEnt : Player gib & ragdoll, the new client-side player models required a separate event for player related gibbings.
//
//========================================================================================//

#ifndef TE_PLAYER_GIB_H
#define TE_PLAYER_GIB_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

void TE_PlayerGibRagdoll(
	int	index,
	int flags,
	int type,
	const Vector &filterOrigin,
	const Vector &actualOrigin,
	const Vector &velocity,
	const QAngle &angles
	);

#endif // TE_PLAYER_GIB_H
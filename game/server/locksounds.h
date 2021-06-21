//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a structure common to buttons and doors for playing locked
//			and unlocked sounds.
//
// $NoKeywords: $
//=============================================================================//

#ifndef LOCKSOUNDS_H
#define LOCKSOUNDS_H
#ifdef _WIN32
#pragma once
#endif

#include "datamap.h"

struct locksound_t					// sounds that doors and buttons make when locked/unlocked
{
	string_t	sLockedSound;		// sound a door makes when it's locked
	string_t	sUnlockedSound;		// sound a door makes when it's unlocked
	float	flwaitSound;			// time delay between playing consecutive 'locked/unlocked' sounds
};

#endif // LOCKSOUNDS_H
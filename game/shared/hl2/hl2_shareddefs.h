//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2_SHAREDDEFS_H
#define HL2_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

#include "const.h"

//--------------------------------------------------------------------------
// Collision groups
//--------------------------------------------------------------------------

enum
{
	HL2COLLISION_GROUP_SPIT = LAST_SHARED_COLLISION_GROUP,
	HL2COLLISION_GROUP_FIRST_NPC,
	HL2COLLISION_GROUP_CROW,
	HL2COLLISION_GROUP_GUNSHIP,
	HL2COLLISION_GROUP_LAST_NPC,
};

#endif // HL2_SHAREDDEFS_H
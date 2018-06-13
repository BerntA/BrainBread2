//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef MP_SHAREDDEFS_H
#define MP_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

//-----------------------------------------------------------------------------
// TF Concepts
//-----------------------------------------------------------------------------
#define MP_CONCEPT_NONE		-1

enum
{
	MP_CONCEPT_START = 0,

	MP_TF_CONCEPT_COUNT

	// Other MP_CONCEPT_* start he using MP_TF_CONCEPT_COUNT + 1 as start.
};

extern const char *g_pszMPConcepts[];
int GetMPConceptIndexFromString( const char *pszConcept );

#endif // MP_SHAREDDEFS_H

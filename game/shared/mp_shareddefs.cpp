//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "mp_shareddefs.h"

const char *g_pszMPConcepts[] =
{
	"TLK_START",
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszMPConcepts ) == MP_TF_CONCEPT_COUNT );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetMPConceptIndexFromString( const char *pszConcept )
{
	for ( int iConcept = 0; iConcept < ARRAYSIZE( g_pszMPConcepts ); ++iConcept )
	{
		if ( !Q_stricmp( pszConcept, g_pszMPConcepts[iConcept] ) )
			return iConcept;
	}

	return MP_CONCEPT_NONE;
}

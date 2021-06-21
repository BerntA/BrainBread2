//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Dynamic light at the end of a spotlight 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef	SPOTLIGHTEND_H
#define	SPOTLIGHTEND_H

#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"

class CSpotlightEnd : public CBaseEntity
{
public:
	DECLARE_CLASS( CSpotlightEnd, CBaseEntity );

	void				Spawn( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( float, m_flLightScale );
	CNetworkVar( float, m_Radius );
//	CNetworkVector( m_vSpotlightDir );
//	CNetworkVector( m_vSpotlightOrg );
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightOrg;
};

#endif	//SPOTLIGHTEND_H



//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "entitylist.h"
#include "SkyCamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// automatically hooks in the system's callbacks
CEntityClassList<CSkyCamera> g_SkyList;
template <> CSkyCamera *CEntityClassList<CSkyCamera>::m_pClassList = NULL;

//-----------------------------------------------------------------------------
// Retrives the current skycamera
//-----------------------------------------------------------------------------
CSkyCamera*	GetCurrentSkyCamera()
{
	return g_SkyList.m_pClassList;
}

CSkyCamera*	GetSkyCameraList()
{
	return g_SkyList.m_pClassList;
}

//=============================================================================

LINK_ENTITY_TO_CLASS( sky_camera, CSkyCamera );

BEGIN_DATADESC( CSkyCamera )

	DEFINE_KEYFIELD( m_skyboxData.scale, FIELD_INTEGER, "scale" ),

	// fog data for 3d skybox
	DEFINE_KEYFIELD( m_bUseAngles,						FIELD_BOOLEAN,	"use_angles" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.enable,			FIELD_BOOLEAN, "fogenable" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.blend,			FIELD_BOOLEAN, "fogblend" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.dirPrimary,		FIELD_VECTOR, "fogdir" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.colorPrimary,		FIELD_COLOR32, "fogcolor" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.colorSecondary,	FIELD_COLOR32, "fogcolor2" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.start,			FIELD_FLOAT, "fogstart" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.end,				FIELD_FLOAT, "fogend" ),
	DEFINE_KEYFIELD( m_skyboxData.fog.maxdensity,		FIELD_FLOAT, "fogmaxdensity" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CSkyCamera::CSkyCamera()
{
	g_SkyList.Insert( this );
	m_skyboxData.fog.maxdensity = 1.0f;
}

CSkyCamera::~CSkyCamera()
{
	g_SkyList.Remove( this );
}

void CSkyCamera::Spawn( void ) 
{ 
	m_skyboxData.origin = GetLocalOrigin();
	m_skyboxData.area = engine->GetArea( m_skyboxData.origin );
	
	Precache();
}


//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CSkyCamera::Activate( ) 
{
	BaseClass::Activate();

	if ( m_bUseAngles )
	{
		AngleVectors( GetAbsAngles(), &m_skyboxData.fog.dirPrimary.GetForModify() );
		m_skyboxData.fog.dirPrimary.GetForModify() *= -1.0f; 
	}
}

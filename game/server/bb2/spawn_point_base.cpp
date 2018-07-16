//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Human & Zombie custom spawn point entities which allows the mapper to disable/enable the spawn point as well.
//
//========================================================================================//

#include "cbase.h"
#include "spawn_point_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CBaseSpawnPoint )

	DEFINE_KEYFIELD(m_bIsEnabled, FIELD_BOOLEAN, "StartEnabled"),
	DEFINE_KEYFIELD(m_bIsMaster, FIELD_BOOLEAN, "IsMasterSpawn"),

	DEFINE_OUTPUT( m_OnEnabled, "OnEnabledSpawn" ),
	DEFINE_OUTPUT( m_OnDisabled, "OnDisabledSpawn" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "ChangeState", InputState ),

END_DATADESC()

CBaseSpawnPoint::CBaseSpawnPoint( void )
{
	m_bIsEnabled = true;
	m_bIsMaster = false;
}

void CBaseSpawnPoint::Spawn()
{
	BaseClass::Spawn();
}

void CBaseSpawnPoint::InputState( inputdata_t &inputdata )
{
	int iValue = inputdata.value.Int();
	m_bIsEnabled = ( iValue >= 1 ) ? true : false;

	if ( m_bIsEnabled )
		m_OnEnabled.FireOutput( this, this );
	else
		m_OnDisabled.FireOutput( this, this );
}
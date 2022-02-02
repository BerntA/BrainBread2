//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Teleports a named entity to a given position and restores
//			it's physics state
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"


#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	SF_TELEPORT_TO_SPAWN_POS	0x00000001
#define	SF_TELEPORT_INTO_DUCK		0x00000002 ///< episodic only: player should be ducked after this teleport

class CPointTeleport : public CBaseEntity
{
	DECLARE_CLASS( CPointTeleport, CBaseEntity );
public:
	void	Activate( void );

	void InputTeleport( inputdata_t &inputdata );

	void InputTeleportHumans(inputdata_t &inputdata);
	void InputTeleportZombies(inputdata_t &inputdata);

private:
	
	bool	EntityMayTeleport( CBaseEntity *pTarget );
	void	TeleportTeam(int team);

	Vector m_vSaveOrigin;
	QAngle m_vSaveAngles;

	DECLARE_DATADESC();
};


LINK_ENTITY_TO_CLASS( point_teleport, CPointTeleport );


BEGIN_DATADESC( CPointTeleport )

	DEFINE_INPUTFUNC( FIELD_VOID, "Teleport", InputTeleport ),

	DEFINE_INPUTFUNC(FIELD_VOID, "TeleportHumans", InputTeleportHumans),
	DEFINE_INPUTFUNC(FIELD_VOID, "TeleportZombies", InputTeleportZombies),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
// Output : Returns true if the entity may be teleported
//-----------------------------------------------------------------------------
bool CPointTeleport::EntityMayTeleport(CBaseEntity *pTarget)
{
	if (pTarget->GetMoveParent() != NULL)
		return false;

	return true;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointTeleport::Activate( void )
{
	// Start with our origin point
	m_vSaveOrigin = GetAbsOrigin();
	m_vSaveAngles = GetAbsAngles();

	// Save off the spawn position of the target if instructed to do so
	if ( m_spawnflags & SF_TELEPORT_TO_SPAWN_POS )
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_target );
		if ( pTarget )
		{
			// If teleport object is in a movement hierarchy, remove it first
			if ( EntityMayTeleport( pTarget ) )
			{
				// Save the points
				m_vSaveOrigin = pTarget->GetAbsOrigin();
				m_vSaveAngles = pTarget->GetAbsAngles();
			}
			else
			{
				Warning("ERROR: (%s) can't teleport object (%s) as it has a parent (%s)!\n",GetDebugName(),pTarget->GetDebugName(),pTarget->GetMoveParent()->GetDebugName());
				BaseClass::Activate();
				return;
			}
		}
		else
		{
			Warning("ERROR: (%s) target '%s' not found. Deleting.\n", GetDebugName(), STRING(m_target));
			UTIL_Remove( this );
			return;
		}
	}

	BaseClass::Activate();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointTeleport::InputTeleport( inputdata_t &inputdata )
{
	// Attempt to find the entity in question
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_target, this, inputdata.pActivator, inputdata.pCaller );
	if ( pTarget == NULL )
		return;

	// If teleport object is in a movement hierarchy, remove it first
	if ( EntityMayTeleport( pTarget ) == false )
	{
		Warning("ERROR: (%s) can't teleport object (%s) as it has a parent (%s)!\n",GetDebugName(),pTarget->GetDebugName(),pTarget->GetMoveParent()->GetDebugName());
		return;
	}

	pTarget->Teleport( &m_vSaveOrigin, &m_vSaveAngles, NULL );
}

void CPointTeleport::InputTeleportHumans(inputdata_t &inputdata)
{
	TeleportTeam(TEAM_HUMANS);
}

void CPointTeleport::InputTeleportZombies(inputdata_t &inputdata)
{
	TeleportTeam(TEAM_DECEASED);
}

void CPointTeleport::TeleportTeam(int team)
{
	QAngle viewAngles;
	viewAngles.Init();

	trace_t trace;
	CTraceFilterWorldOnly filter;
	const Vector &vPos = GetLocalOrigin();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer || (pPlayer->GetTeamNumber() != team) || !pPlayer->IsAlive() || pPlayer->IsObserver() || !EntityMayTeleport(pPlayer))
			continue;

		if ((team == TEAM_HUMANS) && pPlayer->IsPlayerInfected())
			continue;

		UTIL_TraceLine(vPos, pPlayer->WorldSpaceCenter(), MASK_BLOCKLOS, &filter, &trace);
		if ((trace.fraction == 1.0f) && ((trace.endpos - trace.startpos).Length() < 600.0f)) continue;

		viewAngles[YAW] = random->RandomFloat(-180.0f, 180.0f);
		pPlayer->Teleport(&m_vSaveOrigin, &viewAngles, NULL);
	}
}
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "team.h"
#include "player.h"
#include "GameDefinitions_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUtlVector< CTeam * > g_Teams;

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
void SendProxy_PlayerList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTeam *pTeam = (CTeam*)pData;

	// If this assertion fails, then SendProxyArrayLength_PlayerArray must have failed.
	Assert( iElement < pTeam->m_aPlayers.Size() );

	CBasePlayer *pPlayer = pTeam->m_aPlayers[iElement];
	pOut->m_Int = pPlayer->entindex();
}


int SendProxyArrayLength_PlayerArray( const void *pStruct, int objectID )
{
	CTeam *pTeam = (CTeam*)pStruct;
	return pTeam->m_aPlayers.Count();
}


// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CTeam, DT_Team)
	SendPropInt( SENDINFO(m_iTeamNum), 5 ),
	SendPropInt( SENDINFO(m_iScore), 0 ),
	SendPropInt(SENDINFO(m_iExtraScore), 0),
	SendPropInt( SENDINFO(m_iRoundsWon), 8 ),
	SendPropString( SENDINFO( m_szTeamname ) ),

	SendPropArray2( 
		SendProxyArrayLength_PlayerArray,
		SendPropInt("player_array_element", 0, 4, 10, SPROP_UNSIGNED, SendProxy_PlayerList), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( team_manager, CTeam );

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified team manager
//-----------------------------------------------------------------------------
CTeam *GetGlobalTeam( int iIndex )
{
	if ( iIndex < 0 || iIndex >= GetNumberOfTeams() )
		return NULL;

	return g_Teams[ iIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of team managers
//-----------------------------------------------------------------------------
int GetNumberOfTeams( void )
{
	return g_Teams.Size();
}


//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
CTeam::CTeam( void )
{
	memset( m_szTeamname.GetForModify(), 0, sizeof(m_szTeamname) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeam::~CTeam( void )
{
	m_aPlayers.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame
//-----------------------------------------------------------------------------
void CTeam::Think( void )
{
	if (HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION)
	{
		int currentPerk = GetActivePerk();
		if (currentPerk)
		{
			if (m_flTeamPerkTime < gpGlobals->curtime)
			{
				m_flTeamPerkTime = 0.0f;
				m_nActivePerk = 0;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Teams are always transmitted to clients
//-----------------------------------------------------------------------------
int CTeam::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Visibility/scanners
//-----------------------------------------------------------------------------
bool CTeam::ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity )
{
	// Always transmit the observer target to players
	if ( pRecipient && pRecipient->IsObserver() && pRecipient->GetObserverTarget() == pEntity )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CTeam::Init( const char *pName, int iNumber )
{
	InitializePlayers();

	m_iScore = 0;
	m_iExtraScore = 0;

	m_nActivePerk = 0;
	m_flTeamPerkTime = 0.0f;

	Q_strncpy( m_szTeamname.GetForModify(), pName, MAX_TEAM_NAME_LENGTH );
	m_iTeamNum = iNumber;
}

//-----------------------------------------------------------------------------
// DATA HANDLING
//-----------------------------------------------------------------------------
int CTeam::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: Get the team's name
//-----------------------------------------------------------------------------
const char *CTeam::GetName( void )
{
	return m_szTeamname;
}


//-----------------------------------------------------------------------------
// Purpose: Update the player's client data
//-----------------------------------------------------------------------------
void CTeam::UpdateClientData( CBasePlayer *pPlayer )
{
}

//------------------------------------------------------------------------------------------------------------------
// PLAYERS
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::InitializePlayers( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified player to this team. Remove them from their current team, if any.
//-----------------------------------------------------------------------------
void CTeam::AddPlayer( CBasePlayer *pPlayer )
{
	m_aPlayers.AddToTail( pPlayer );
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from the team
//-----------------------------------------------------------------------------
void CTeam::RemovePlayer( CBasePlayer *pPlayer )
{
	m_aPlayers.FindAndRemove( pPlayer );
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of players in this team.
//-----------------------------------------------------------------------------
int CTeam::GetNumPlayers( void )
{
	return m_aPlayers.Size();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific player
//-----------------------------------------------------------------------------
CBasePlayer *CTeam::GetPlayer( int iIndex )
{
	Assert( iIndex >= 0 && iIndex < m_aPlayers.Size() );
	return m_aPlayers[ iIndex ];
}

//------------------------------------------------------------------------------------------------------------------
// SCORING
//-----------------------------------------------------------------------------
// Purpose: Add / Remove score for this team
//-----------------------------------------------------------------------------
void CTeam::AddScore( int iScore )
{
	m_iScore += iScore;
	m_iExtraScore++;
	CanActivateTeamPerk();
}

void CTeam::SetScore( int iScore )
{
	m_iScore = iScore;
}

//-----------------------------------------------------------------------------
// Purpose: Get this team's score
//-----------------------------------------------------------------------------
int CTeam::GetScore( void )
{
	return m_iScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::ResetScores( void )
{
	SetScore(0);
}

int CTeam::GetAliveMembers( void )
{
	int iAlive = 0;

	int iNumPlayers = GetNumPlayers();

	for ( int i=0;i<iNumPlayers;i++ )
	{
		if ( GetPlayer(i) && GetPlayer(i)->IsAlive() )
		{
			iAlive++;
		}
	}

	return iAlive;
}

bool CTeam::CanActivateTeamPerk(void)
{
	if (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)
		return false;

	if (m_iExtraScore < bb2_elimination_teamperk_kills_required.GetInt())
		return false;

	m_iExtraScore = 0;
	m_flTeamPerkTime = gpGlobals->curtime + bb2_elimination_teamperk_duration.GetFloat();

	const char *teamName = "Human";
	if (m_iTeamNum == TEAM_DECEASED)
	{
		teamName = "Zombie";
		m_nActivePerk = random->RandomInt(teamDeceasedEliminationPerks_t::TEAM_DECEASED_PERK_INCREASED_DAMAGE, teamDeceasedEliminationPerks_t::TEAM_DECEASED_PERK_INCREASED_DAMAGE);
	}
	else if (m_iTeamNum == TEAM_HUMANS)
	{
		m_nActivePerk = random->RandomInt(teamHumanEliminationPerks_t::TEAM_HUMAN_PERK_UNLIMITED_AMMO, teamHumanEliminationPerks_t::TEAM_HUMAN_PERK_UNLIMITED_AMMO);
	}

	// Announce this to everyone:
	CRecipientFilter filter;
	filter.AddAllPlayers();
	filter.MakeReliable();
	UTIL_ClientPrintFilter(filter, HUD_PRINTTALK, "#GRANT_TEAM_PERK", teamName, GetTeamPerkName(m_nActivePerk), bb2_elimination_teamperk_duration.GetString());

	return true;
}

int CTeam::GetActivePerk(void)
{
	if (HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION)
		return 0;

	return m_nActivePerk;
}

void CTeam::ResetTeamPerks(void)
{
	m_nActivePerk = 0;
	m_flTeamPerkTime = 0.0f;
	m_iExtraScore = 0;
}
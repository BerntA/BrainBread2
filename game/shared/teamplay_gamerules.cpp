//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "KeyValues.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "hl2mp_gamerules.h"

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
	#include "c_team.h"
#else
	#include "player.h"
	#include "game.h"
	#include "gamevars_shared.h"
	#include "team.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
REGISTER_GAMERULES_CLASS( CTeamplayRules );

CTeamplayRules::CTeamplayRules()
{	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRules::Precache( void )
{
	// Call the Team Manager's precaches
	for ( int i = 0; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		pTeam->Precache();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamplayRules::Think ( void )
{
	BaseClass::Think();
}

//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
bool CTeamplayRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;

	return false;
}

//=========================================================
// InitHUD
//=========================================================
void CTeamplayRules::InitHUD( CBasePlayer *pPlayer )
{
	BaseClass::InitHUD( pPlayer );
}

void CTeamplayRules::ChangePlayerTeam(CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib)
{
	CHL2MP_Player *pOther = ToHL2MPPlayer(pPlayer);

	if (pOther == NULL || pTeamName == NULL || (strlen(pTeamName) <= 0))
		return;

	CTeam *team = pOther->GetTeam();
	if (team == NULL || strcmp(pTeamName, team->GetName()))
		return;

	if (bKill)
		pOther->CommitSuicide(bGib, true);

	pOther->HandleCommand_JoinTeam(team->GetTeamNumber());
}

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CTeamplayRules::ClientDisconnected( edict_t *pClient )
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( pPlayer )
	{
		pPlayer->SetConnected( PlayerDisconnecting );
		// Remove the player from his team
		if ( pPlayer->GetTeam() )
		{
			pPlayer->ChangeTeam( 0 );
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

//=========================================================
// ClientUserInfoChanged
//=========================================================
void CTeamplayRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );
	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pszName );
			gameeventmanager->FireEvent( event );
		}
		
		pPlayer->SetPlayerName( pszName );
	}

	// NVNT see if this user is still or has began using a haptic device
	const char *pszHH = engine->GetClientConVarValue( pPlayer->entindex(), "hap_HasDevice" );
	if(pszHH)
	{
		int iHH = atoi(pszHH);
		pPlayer->SetHaptics(iHH!=0);
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CTeamplayRules::DeathNotice(CBaseEntity *pVictim, const CTakeDamageInfo &info)
{
}

//=========================================================
//=========================================================
void CTeamplayRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::PlayerKilled(pVictim, info);
}


//=========================================================
// IsTeamplay
//=========================================================
bool CTeamplayRules::IsTeamplay( void )
{
	return true;
}

bool CTeamplayRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
	if ( pAttacker && (PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE) && !info.IsMiscFlagActive(TAKEDMGINFO_FORCE_FRIENDLYFIRE) )
	{
		// my teammate hit me.
		if ( (friendlyfire.GetInt() == 0) && (pAttacker != pPlayer) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return false;
		}
	}

	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker, info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pListener - 
//			*pSpeaker - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTeamplayRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
{
	return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
}

//=========================================================
//=========================================================
int CTeamplayRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}
#endif // GAME_DLL
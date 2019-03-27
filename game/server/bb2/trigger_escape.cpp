//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Throws the player over to the spec team and prints out a message. In classic the human team will win with at least one human extractor.
//
//========================================================================================//

#include "cbase.h"
#include "trigger_escape.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CTriggerEscape )

	DEFINE_OUTPUT( m_OnAddedPlayer, "OnAddPlayer" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_escape, CTriggerEscape);

CTriggerEscape::CTriggerEscape( void )
{
	m_bDisabled = false;
}

CTriggerEscape::~CTriggerEscape( void )
{
}

void CTriggerEscape::Spawn()
{
	BaseClass::Spawn();
}

void CTriggerEscape::Touch( CBaseEntity *pOther )
{
	if ( m_bDisabled )
		return;

	if ( !pOther )
		return;

	if ( !pOther->IsHuman() )
		return;

	if (HL2MPRules()->IsGameoverOrScoresVisible())
		return;

	CHL2MP_Player *pClient = ToHL2MPPlayer( pOther );
	if ( !pClient )
		return;

	if ( ( pClient->GetTeamNumber() != TEAM_HUMANS ) )
		return;

	if (pClient->IsPlayerInfected() || pClient->HasPlayerEscaped())
		return;

	pClient->SetPlayerEscaped(true);
	GameBaseShared()->RemoveInventoryItem(pClient->entindex(), pClient->GetAbsOrigin());
	pClient->ChangeTeam(TEAM_SPECTATOR); // Drop us into the spec team = escape team.
	m_OnAddedPlayer.FireOutput(this, this);
	GameBaseServer()->SendToolTip("#TOOLTIP_ESCAPE_SUCCESS", GAME_TIP_DEFAULT, pClient->entindex());

	BaseClass::Touch(pOther);
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CTriggerEscape::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CTriggerEscape::InputDisable(inputdata_t &inputdata)
{
	Disable();
}
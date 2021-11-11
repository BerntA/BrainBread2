//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2mp_player.h"
#include "player_resource.h"
#include <coordsize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CPlayerResource, DT_PlayerResource)
SendPropArray3(SENDINFO_ARRAY3(m_nGroupID), SendPropInt(SENDINFO_ARRAY(m_nGroupID), 3, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_iLevel), SendPropInt(SENDINFO_ARRAY(m_iLevel), 9, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_iTotalScore), SendPropInt(SENDINFO_ARRAY(m_iTotalScore), 12)),
SendPropArray3(SENDINFO_ARRAY3(m_iTotalDeaths), SendPropInt(SENDINFO_ARRAY(m_iTotalDeaths), 12)),
SendPropArray3(SENDINFO_ARRAY3(m_iRoundScore), SendPropInt(SENDINFO_ARRAY(m_iRoundScore), 12)),
SendPropArray3(SENDINFO_ARRAY3(m_iRoundDeaths), SendPropInt(SENDINFO_ARRAY(m_iRoundDeaths), 12)),
SendPropArray3(SENDINFO_ARRAY3(m_iSelectedTeam), SendPropInt(SENDINFO_ARRAY(m_iSelectedTeam), 3, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_iPing), SendPropInt(SENDINFO_ARRAY(m_iPing), 10, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_bInfected), SendPropInt(SENDINFO_ARRAY(m_bInfected), 1, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_bConnected), SendPropInt(SENDINFO_ARRAY(m_bConnected), 1, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_iTeam), SendPropInt(SENDINFO_ARRAY(m_iTeam), 4)),
SendPropArray3(SENDINFO_ARRAY3(m_bAlive), SendPropInt(SENDINFO_ARRAY(m_bAlive), 1, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_bAdmin), SendPropInt(SENDINFO_ARRAY(m_bAdmin), 1, SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_iHealth), SendPropInt(SENDINFO_ARRAY(m_iHealth), -1, SPROP_VARINT | SPROP_UNSIGNED)),
SendPropArray3(SENDINFO_ARRAY3(m_vecPosition), SendPropVector(SENDINFO_ARRAY(m_vecPosition), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT)),
END_SEND_TABLE()

BEGIN_DATADESC(CPlayerResource)
DEFINE_FUNCTION(ResourceThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(player_manager, CPlayerResource);

CPlayerResource *g_pPlayerResource;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::Spawn(void)
{
	for (int i = 0; i < MAX_PLAYERS + 1; i++)
	{
		m_nGroupID.Set(i, 0);
		m_iLevel.Set(i, 1);
		m_iTotalScore.Set(i, 0);
		m_iTotalDeaths.Set(i, 0);
		m_iRoundScore.Set(i, 0);
		m_iRoundDeaths.Set(i, 0);
		m_iSelectedTeam.Set(i, 0);
		m_iPing.Set(i, 0);
		m_bInfected.Set(i, 0);
		m_bConnected.Set(i, 0);
		m_iTeam.Set(i, 0);
		m_bAlive.Set(i, 0);
		m_bAdmin.Set(i, 0);
		m_vecPosition.Set(i, vec3_origin);
	}

	SetThink(&CPlayerResource::ResourceThink);
	SetNextThink(gpGlobals->curtime);
	m_nUpdateCounter = 0;
}

//-----------------------------------------------------------------------------
// Purpose: The Player resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CPlayerResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState(FL_EDICT_ALWAYS);
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper for the virtual GrabPlayerData Think function
//-----------------------------------------------------------------------------
void CPlayerResource::ResourceThink(void)
{
	m_nUpdateCounter++;

	UpdatePlayerData();

	SetNextThink(gpGlobals->curtime + 0.1f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerResource::UpdatePlayerData(void)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (pPlayer && pPlayer->IsConnected())
		{
			m_nGroupID.Set(i, pPlayer->GetGroupIDFlags());
			m_iLevel.Set(i, pPlayer->GetPlayerLevel());
			m_iTotalScore.Set(i, pPlayer->GetTotalScore());
			m_iTotalDeaths.Set(i, pPlayer->GetTotalDeaths());
			m_iRoundScore.Set(i, pPlayer->GetRoundScore());
			m_iRoundDeaths.Set(i, pPlayer->GetRoundDeaths());
			m_iSelectedTeam.Set(i, pPlayer->GetSelectedTeam());
			m_bInfected.Set(i, (pPlayer->IsPlayerInfected() ? 1 : 0));
			m_bConnected.Set(i, 1);
			m_iTeam.Set(i, pPlayer->GetTeamNumber());
			m_bAlive.Set(i, pPlayer->IsAlive() ? 1 : 0);
			m_bAdmin.Set(i, pPlayer->GetAdminLevel() ? 1 : 0);
			m_iHealth.Set(i, MAX(0, pPlayer->GetHealth()));

			if ((m_nUpdateCounter % 4) == 0)
				m_vecPosition.Set(i, pPlayer->GetLocalOrigin());

			// Don't update ping / packetloss everytime
			if (!(m_nUpdateCounter % 20))
			{
				// update ping all 20 think ticks = (20*0.1=2seconds)
				int ping, packetloss;
				UTIL_GetPlayerConnectionInfo(i, ping, packetloss);

				// calc avg for scoreboard so it's not so jittery
				ping = 0.8f * m_iPing.Get(i) + 0.2f * ping;

				m_iPing.Set(i, ping);
			}
		}
		else
		{
			m_bConnected.Set(i, 0);
		}
	}
}
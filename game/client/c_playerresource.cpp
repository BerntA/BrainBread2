//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_playerresource.h"
#include "c_team.h"
#include "gamestringpool.h"

#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float PLAYER_RESOURCE_THINK_INTERVAL = 0.2f;

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
RecvPropArray3(RECVINFO_ARRAY(m_iPing), RecvPropInt(RECVINFO(m_iPing[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_nGroupID), RecvPropInt(RECVINFO(m_nGroupID[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iLevel), RecvPropInt(RECVINFO(m_iLevel[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iTotalScore), RecvPropInt(RECVINFO(m_iTotalScore[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iTotalDeaths), RecvPropInt(RECVINFO(m_iTotalDeaths[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iRoundScore), RecvPropInt(RECVINFO(m_iRoundScore[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iRoundDeaths), RecvPropInt(RECVINFO(m_iRoundDeaths[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iSelectedTeam), RecvPropInt(RECVINFO(m_iSelectedTeam[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_bInfected), RecvPropInt(RECVINFO(m_bInfected[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_bConnected), RecvPropInt(RECVINFO(m_bConnected[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iTeam), RecvPropInt(RECVINFO(m_iTeam[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_bAlive), RecvPropInt(RECVINFO(m_bAlive[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_bAdmin), RecvPropInt(RECVINFO(m_bAdmin[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_iHealth), RecvPropInt(RECVINFO(m_iHealth[0]))),
RecvPropArray3(RECVINFO_ARRAY(m_vecPosition), RecvPropVector(RECVINFO(m_vecPosition[0]))),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA(C_PlayerResource)

DEFINE_PRED_ARRAY(m_szName, FIELD_STRING, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iPing, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_nGroupID, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iLevel, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iTotalScore, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iTotalDeaths, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iRoundScore, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iRoundDeaths, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iSelectedTeam, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_bInfected, FIELD_BOOLEAN, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_bConnected, FIELD_BOOLEAN, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iTeam, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_bAlive, FIELD_BOOLEAN, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_bAdmin, FIELD_BOOLEAN, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_iHealth, FIELD_INTEGER, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),
DEFINE_PRED_ARRAY(m_vecPosition, FIELD_VECTOR, MAX_PLAYERS + 1, FTYPEDESC_PRIVATE),

END_PREDICTION_DATA()

C_PlayerResource *g_PR;

IGameResources * GameResources(void) { return g_PR; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::C_PlayerResource()
{
	memset(m_iPing, 0, sizeof(m_iPing));
	//	memset( m_iPacketloss, 0, sizeof( m_iPacketloss ) );
	memset(m_nGroupID, 0, sizeof(m_nGroupID));
	memset(m_iLevel, 0, sizeof(m_iLevel));
	memset(m_iTotalScore, 0, sizeof(m_iTotalScore));
	memset(m_iTotalDeaths, 0, sizeof(m_iTotalDeaths));
	memset(m_iRoundScore, 0, sizeof(m_iRoundScore));
	memset(m_iRoundDeaths, 0, sizeof(m_iRoundDeaths));
	memset(m_iSelectedTeam, 0, sizeof(m_iSelectedTeam));
	memset(m_bInfected, 0, sizeof(m_bInfected));
	memset(m_bConnected, 0, sizeof(m_bConnected));
	memset(m_iTeam, 0, sizeof(m_iTeam));
	memset(m_bAlive, 0, sizeof(m_bAlive));
	memset(m_bAdmin, 0, sizeof(m_bAdmin));
	memset(m_iHealth, 0, sizeof(m_iHealth));
	memset(m_vecPosition, 0, sizeof(m_vecPosition));
	m_szUnconnectedName = 0;

	for (int i = 0; i < MAX_TEAMS; i++)
	{
		m_Colors[i] = COLOR_GREY;
	}

#ifdef HL2MP
	m_Colors[TEAM_HUMANS] = COLOR_BLUE;
	m_Colors[TEAM_DECEASED] = COLOR_RED;
	m_Colors[TEAM_UNASSIGNED] = COLOR_YELLOW;
#endif

	g_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::~C_PlayerResource()
{
	g_PR = NULL;
}

void C_PlayerResource::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if (updateType == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL);
	}
}

void C_PlayerResource::UpdatePlayerName(int slot)
{
	if (slot < 1 || slot > MAX_PLAYERS)
	{
		Error("UpdatePlayerName with bogus slot %d\n", slot);
		return;
	}
	if (!m_szUnconnectedName)
		m_szUnconnectedName = AllocPooledString(PLAYER_UNCONNECTED_NAME);

	player_info_t sPlayerInfo;
	if (IsConnected(slot) && engine->GetPlayerInfo(slot, &sPlayerInfo))
	{
		m_szName[slot] = AllocPooledString(sPlayerInfo.name);
	}
	else
	{
		m_szName[slot] = m_szUnconnectedName;
	}
}

void C_PlayerResource::ClientThink()
{
	BaseClass::ClientThink();

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		UpdatePlayerName(i);
	}

	SetNextClientThink(gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_PlayerResource::GetPlayerName(int iIndex)
{
	if (iIndex < 1 || iIndex > MAX_PLAYERS)
	{
		Assert(false);
		return PLAYER_ERROR_NAME;
	}

	if (!IsConnected(iIndex))
		return PLAYER_UNCONNECTED_NAME;

	// X360TBD: Network - figure out why the name isn't set
	if (!m_szName[iIndex] || !Q_stricmp(m_szName[iIndex], PLAYER_UNCONNECTED_NAME))
	{
		// If you get a full "reset" uncompressed update from server, then you can have NULLNAME show up in the scoreboard
		UpdatePlayerName(iIndex);
	}

	// This gets updated in ClientThink, so it could be up to 1 second out of date, oh well.
	return m_szName[iIndex];
}

bool C_PlayerResource::IsAlive(int iIndex)
{
	return m_bAlive[iIndex];
}

bool C_PlayerResource::IsAdmin(int iIndex)
{
	if (!IsConnected(iIndex))
		return false;

	return m_bAdmin[iIndex];
}

int C_PlayerResource::GetTeam(int iIndex)
{
	if (iIndex < 1 || iIndex > MAX_PLAYERS)
	{
		Assert(false);
		return 0;
	}
	else
	{
		return m_iTeam[iIndex];
	}
}

const char * C_PlayerResource::GetTeamName(int index)
{
	C_Team *team = GetGlobalTeam(index);
	if (!team)
		return "Unknown";

	return team->Get_Name();
}

int C_PlayerResource::GetTeamScore(int index)
{
	C_Team *team = GetGlobalTeam(index);
	if (!team)
		return 0;

	return team->Get_Score();
}

bool C_PlayerResource::IsLocalPlayer(int index)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	return (index == pPlayer->entindex());
}

bool C_PlayerResource::IsHLTV(int index)
{
	if (!IsConnected(index))
		return false;

	player_info_t sPlayerInfo;

	if (engine->GetPlayerInfo(index, &sPlayerInfo))
	{
		return sPlayerInfo.ishltv;
	}

	return false;
}

bool C_PlayerResource::IsReplay(int index)
{
#if defined( REPLAY_ENABLED )
	if ( !IsConnected( index ) )
		return false;

	player_info_t sPlayerInfo;

	if ( engine->GetPlayerInfo( index, &sPlayerInfo ) )
	{
		return sPlayerInfo.isreplay;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsFakePlayer(int iIndex)
{
	if (!IsConnected(iIndex))
		return false;

	// Yuck, make sure it's up to date
	player_info_t sPlayerInfo;
	if (engine->GetPlayerInfo(iIndex, &sPlayerInfo))
	{
		return sPlayerInfo.fakeplayer;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPing(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iPing[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetTotalScore(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iTotalScore[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetTotalDeaths(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iTotalDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetRoundScore(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iRoundScore[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetRoundDeaths(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iRoundDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetHealth(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iHealth[iIndex];
}

Vector C_PlayerResource::GetPosition(int index)
{
	if (!IsConnected(index))
		return Vector(0, 0, 0);

	return m_vecPosition[index];
}

const Color &C_PlayerResource::GetTeamColor(int index)
{
	if (index < 0 || index >= MAX_TEAMS)
	{
		Assert(false);
		static Color blah;
		return blah;
	}
	else
	{
		return m_Colors[index];
	}
}

bool C_PlayerResource::IsConnected(int iIndex)
{
	if (iIndex < 1 || iIndex > MAX_PLAYERS)
		return false;
	else
		return m_bConnected[iIndex];
}

bool C_PlayerResource::IsInfected(int iIndex)
{
	if (iIndex < 1 || iIndex > MAX_PLAYERS)
		return false;
	else
		return m_bInfected[iIndex];
}

bool C_PlayerResource::IsGroupIDFlagActive(int iIndex, int flag)
{
	if (iIndex < 1 || iIndex > MAX_PLAYERS)
		return false;

	return ((m_nGroupID[iIndex] & flag) != 0);
}

int	C_PlayerResource::GetLevel(int iIndex)
{
	if (!IsConnected(iIndex))
		return 1;

	return m_iLevel[iIndex];
}

int	C_PlayerResource::GetGroupIDFlags(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_nGroupID[iIndex];
}

int	C_PlayerResource::GetSelectedTeam(int iIndex)
{
	if (!IsConnected(iIndex))
		return 0;

	return m_iSelectedTeam[iIndex];
}
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_hl2_playerlocaldata.h"
#include "dt_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE( C_HL2PlayerLocalData, DT_HL2Local )
	RecvPropInt( RECVINFO(m_iSquadMemberCount) ),
	RecvPropBool( RECVINFO(m_fSquadInFollowMode) ),
	RecvPropEHandle( RECVINFO(m_hLadder) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( C_HL2PlayerLocalData )
	DEFINE_PRED_FIELD( m_hLadder, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

C_HL2PlayerLocalData::C_HL2PlayerLocalData()
{
	m_iSquadMemberCount = 0;
	m_fSquadInFollowMode = false;
	m_hLadder = NULL;
}


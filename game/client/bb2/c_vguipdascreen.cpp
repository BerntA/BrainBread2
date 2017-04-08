//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: PDA VGUI Screen Client Ent. Displays KeyPad codes.
//
//========================================================================================//

#include "cbase.h"
#include "c_vguipdascreen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_VGuiPDAScreen, DT_VGuiPDAScreen, CVGuiPDAScreen)
RecvPropString(RECVINFO(m_szKeyPadCode)),
END_RECV_TABLE()
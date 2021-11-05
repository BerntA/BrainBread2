//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: Leaderboard Definitions!
//
//========================================================================================//

#include "cbase.h"
#include "c_leaderboard_handler.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CLeaderboardPvE : public CLeaderboardHandler
{
public:
	DECLARE_CLASS(CLeaderboardPvE, CLeaderboardHandler);

	CLeaderboardPvE();
	void GetLeaderboardStats(int32 &arg1, int32 &arg2, int32 &arg3);
};

CLeaderboardPvE::CLeaderboardPvE() : CLeaderboardHandler("Global")
{
}

void CLeaderboardPvE::GetLeaderboardStats(int32 &arg1, int32 &arg2, int32 &arg3)
{
	steamapicontext->SteamUserStats()->GetStat("BBX_ST_LEVEL", &arg1);
	steamapicontext->SteamUserStats()->GetStat("BBX_ST_KILLS", &arg2);
	steamapicontext->SteamUserStats()->GetStat("BBX_ST_DEATHS", &arg3);
}

static CLeaderboardPvE g_pPvELeaderboard; // Global Leaderboard
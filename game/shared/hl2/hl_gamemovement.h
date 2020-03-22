//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Special handling for hl2 usable ladders
//
//=============================================================================//

#include "gamemovement.h"

#if defined( CLIENT_DLL )

#include "c_basehlplayer.h"
#define CHL2_Player C_BaseHLPlayer
#else

#include "hl2_player.h"

#endif

//-----------------------------------------------------------------------------
// Purpose: HL2 specific movement code
//-----------------------------------------------------------------------------
class CHL2GameMovement : public CGameMovement
{
	typedef CGameMovement BaseClass;
public:

	CHL2GameMovement();

// Overrides
	virtual void FullLadderMove();
	virtual bool LadderMove( void );
	virtual bool OnLadder( trace_t &trace );
	virtual int GetCheckInterval( IntervalType_t type );
	virtual void	SetGroundEntity( trace_t *pm );
	virtual bool CanAccelerate( void );
	virtual unsigned int PlayerSolidMask(bool brushOnly = false);

private:

	CHL2_Player	*GetHL2Player();
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CHL2_Player	*CHL2GameMovement::GetHL2Player()
{
	return static_cast< CHL2_Player * >( player );
}

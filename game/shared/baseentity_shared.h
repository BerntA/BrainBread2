//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEENTITY_SHARED_H
#define BASEENTITY_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#define		FCAP_IMPULSE_USE			0x00000010		// can be used by the player
#define		FCAP_CONTINUOUS_USE			0x00000020		// can be used by the player
#define		FCAP_ONOFF_USE				0x00000040		// can be used by the player
#define		FCAP_DIRECTIONAL_USE		0x00000080		// Player sends +/- 1 when using (currently only tracktrains)
// NOTE: Normally +USE only works in direct line of sight.  Add these caps for additional searches
#define		FCAP_USE_ONGROUND			0x00000100
#define		FCAP_USE_IN_RADIUS			0x00000200
#define		FCAP_SAVE_NON_NETWORKABLE	0x00000400
#define		FCAP_MASTER					0x10000000		// Can be used to "master" other entities (like multisource)

// How many bits are used to transmit parent attachment indices?
#define NUM_PARENTATTACHMENT_BITS	6

// Maximum number of vphysics objects per entity
#define VPHYSICS_MAX_OBJECT_LIST_COUNT	1024

//-----------------------------------------------------------------------------
// For invalidate physics recursive
//-----------------------------------------------------------------------------
enum InvalidatePhysicsBits_t
{
	POSITION_CHANGED	= 0x1,
	ANGLES_CHANGED		= 0x2,
	VELOCITY_CHANGED	= 0x4,
	ANIMATION_CHANGED	= 0x8,
};

#if defined( CLIENT_DLL )
#include "c_baseentity.h"
#include "c_baseanimating.h"
#else
#include "baseentity.h"

#endif

#if !defined( NO_ENTITY_PREDICTION )
// CBaseEntity inlines
inline bool CBaseEntity::IsPlayerSimulated( void ) const
{
	return m_bIsPlayerSimulated;
}

inline CBasePlayer *CBaseEntity::GetSimulatingPlayer( void )
{
	return m_hPlayerSimulationOwner;
}
#endif

inline MoveType_t CBaseEntity::GetMoveType() const
{
	return (MoveType_t)(unsigned char)m_MoveType;
}

inline MoveCollide_t CBaseEntity::GetMoveCollide() const
{
	return (MoveCollide_t)(unsigned char)m_MoveCollide;
}

//-----------------------------------------------------------------------------
// Collision group accessors
//-----------------------------------------------------------------------------
inline int CBaseEntity::GetCollisionGroup() const
{
	return m_CollisionGroup;
}

inline int	CBaseEntity::GetFlags( void ) const
{
	return m_fFlags;
}

inline bool CBaseEntity::IsAlive( void )
{
	return m_lifeState == LIFE_ALIVE; 
}

inline CBaseEntity	*CBaseEntity::GetOwnerEntity() const
{
	return m_hOwnerEntity.Get();
}

inline CBaseEntity	*CBaseEntity::GetEffectEntity() const
{
	return m_hEffectEntity.Get();
}

inline int CBaseEntity::GetPredictionRandomSeed( bool bUseUnSyncedServerPlatTime )
{
#ifdef GAME_DLL
	return bUseUnSyncedServerPlatTime ? m_nPredictionRandomSeedServer : m_nPredictionRandomSeed;
#else
	return m_nPredictionRandomSeed;
#endif
}

inline CBasePlayer *CBaseEntity::GetPredictionPlayer( void )
{
	return m_pPredictionPlayer;
}

inline void CBaseEntity::SetPredictionPlayer( CBasePlayer *player )
{
	m_pPredictionPlayer = player;
}


inline bool CBaseEntity::IsSimulatedEveryTick() const
{
	return m_bSimulatedEveryTick;
}

inline bool CBaseEntity::IsAnimatedEveryTick() const
{
	return m_bAnimatedEveryTick;
}

inline void CBaseEntity::SetSimulatedEveryTick( bool sim )
{
	if ( m_bSimulatedEveryTick != sim )
	{
		m_bSimulatedEveryTick = sim;
#ifdef CLIENT_DLL
		Interp_UpdateInterpolationAmounts( GetVarMapping() );
#endif
	}
}

inline void CBaseEntity::SetAnimatedEveryTick( bool anim )
{
	if ( m_bAnimatedEveryTick != anim )
	{
		m_bAnimatedEveryTick = anim;
#ifdef CLIENT_DLL
		Interp_UpdateInterpolationAmounts( GetVarMapping() );
#endif
	}
}

inline float CBaseEntity::GetAnimTime() const
{
	return m_flAnimTime;
}

inline float CBaseEntity::GetSimulationTime() const
{
	return m_flSimulationTime;
}

inline void CBaseEntity::SetAnimTime( float at )
{
	m_flAnimTime = at;
}

inline void CBaseEntity::SetSimulationTime( float st )
{
	m_flSimulationTime = st;
}

inline int CBaseEntity::GetEffects( void ) const
{ 
	return m_fEffects; 
}

inline void CBaseEntity::RemoveEffects( int nEffects ) 
{ 
	m_fEffects &= ~nEffects;
	if ( nEffects & EF_NODRAW )
	{
#ifndef CLIENT_DLL
		NetworkProp()->MarkPVSInformationDirty();
		DispatchUpdateTransmitState();
#else
		UpdateVisibility();
#endif
	}
}

inline void CBaseEntity::ClearEffects( void ) 
{ 
	m_fEffects = 0;
#ifndef CLIENT_DLL
		DispatchUpdateTransmitState();
#else
		UpdateVisibility();
#endif
}

inline bool CBaseEntity::IsEffectActive( int nEffects ) const
{ 
	return (m_fEffects & nEffects) != 0; 
}

// Shared EntityMessage between game and client .dlls
#define BASEENTITY_MSG_REMOVE_DECALS	1

extern float k_flMaxEntityPosCoord;
extern float k_flMaxEntityEulerAngle;
extern float k_flMaxEntitySpeed;
extern float k_flMaxEntitySpinRate;

inline bool IsEntityCoordinateReasonable ( const vec_t c )
{
	float r = k_flMaxEntityPosCoord;
	return c > -r && c < r;
}

inline bool IsEntityPositionReasonable( const Vector &v )
{
	float r = k_flMaxEntityPosCoord;
	return
		v.x > -r && v.x < r &&
		v.y > -r && v.y < r &&
		v.z > -r && v.z < r;
}

// Returns:
//   -1 - velocity is really, REALLY bad and probably should be rejected.
//   0  - velocity was suspicious and clamped.
//   1  - velocity was OK and not modified
extern int CheckEntityVelocity( Vector &v );

inline bool IsEntityQAngleReasonable( const QAngle &q )
{
	float r = k_flMaxEntityEulerAngle;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

// Angular velocity in exponential map form
inline bool IsEntityAngularVelocityReasonable( const Vector &q )
{
	float r = k_flMaxEntitySpinRate;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

// Angular velocity of each Euler angle.
inline bool IsEntityQAngleVelReasonable( const QAngle &q )
{
	float r = k_flMaxEntitySpinRate;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

extern bool CheckEmitReasonablePhysicsSpew();

// HELPER CLASS
class CIgnorePredictionSuppression
{
public:
	explicit CIgnorePredictionSuppression()
	{
		CBaseEntity::m_bDontSuppressEvents = true;
	}

	~CIgnorePredictionSuppression()
	{
		CBaseEntity::m_bDontSuppressEvents = false;
	}
private:
	CIgnorePredictionSuppression(const CIgnorePredictionSuppression &);
};

#define IGNORE_PREDICTION_SUPPRESSION CIgnorePredictionSuppression c_IgnorePredSuppression
extern bool ShouldIgnorePredictionSuppression();

#endif // BASEENTITY_SHARED_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hooks and classes for the support of humanoid NPCs with 
//			groovy facial animation capabilities, aka, "Actors"
//
//=============================================================================//

#ifndef AI_BASEACTOR_H
#define AI_BASEACTOR_H

#include "ai_basehumanoid.h"
#include "AI_Interest_Target.h"
#include <limits.h>

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// CAI_BaseActor
//
// Purpose: The base class for all head/body/eye expressive NPCS.
//
//-----------------------------------------------------------------------------
enum PoseParameter_t { POSE_END=INT_MAX };
enum FlexWeight_t { FLEX_END=INT_MAX };

struct AILookTargetArgs_t
{
	EHANDLE 			hTarget;
	Vector				vTarget;
	float				flDuration;
	float				flInfluence;
	float				flRamp;
	bool 				bExcludePlayers;
	CAI_InterestTarget *pQueue;
};

class CAI_BaseActor : public CAI_BaseHumanoid
{
	DECLARE_CLASS(CAI_BaseActor, CAI_BaseHumanoid);

	//friend CPoseParameter;
	//friend CFlexWeight;

public:

	// FIXME: this method is lame, isn't there some sort of template thing that would get rid of the Outer pointer?

	void	Init( PoseParameter_t &index, const char *szName ) { index = (PoseParameter_t)LookupPoseParameter( szName ); };
	void	Set( PoseParameter_t index, float flValue ) { SetPoseParameter( (int)index, flValue ); }
	float	Get( PoseParameter_t index ) { return GetPoseParameter( (int)index ); }

	float	ClampWithBias( PoseParameter_t index, float value, float base );

public:
	CAI_BaseActor()
	 :	m_fLatchedPositions( 0 ),
		m_latchedEyeOrigin( vec3_origin ),
		m_latchedEyeDirection( vec3_origin ),
		m_latchedHeadDirection( vec3_origin ),
		m_hLookTarget( NULL )
	{
	}

	~CAI_BaseActor()
	{
	}

	virtual void			StudioFrameAdvance();
	virtual void			SetModel( const char *szModelName );

	Vector					EyePosition( );
	virtual Vector			HeadDirection2D( void );
	virtual Vector			HeadDirection3D( void );
	virtual Vector			EyeDirection2D( void );
	virtual Vector			EyeDirection3D( void );

	CBaseEntity				*GetLooktarget() { return m_hLookTarget.Get(); }
	virtual void			OnNewLookTarget() {};

	// CBaseFlex
	virtual	void			SetViewtarget( const Vector &viewtarget );
	
	// CAI_BaseNPC
	virtual float			PickLookTarget( bool bExcludePlayers = false, float minTime = 1.5, float maxTime = 2.5 );
	virtual float			PickLookTarget( CAI_InterestTarget &queue, bool bExcludePlayers = false, float minTime = 1.5, float maxTime = 2.5 );
	virtual bool 			PickTacticalLookTarget( AILookTargetArgs_t *pArgs );
	virtual bool 			PickRandomLookTarget( AILookTargetArgs_t *pArgs );
	virtual void			MakeRandomLookTarget( AILookTargetArgs_t *pArgs, float minTime, float maxTime );
	virtual bool			HasActiveLookTargets( void );
	virtual void 			OnSelectedLookTarget( AILookTargetArgs_t *pArgs ) { return; }
	virtual void 			ClearLookTarget( CBaseEntity *pTarget );
	virtual void			ExpireCurrentRandomLookTarget() { m_flNextRandomLookTime = gpGlobals->curtime - 0.1f; }

	virtual void			StartTaskRangeAttack1( const Task_t *pTask );

	virtual void			AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void			AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );

	virtual void			SetHeadDirection( const Vector &vTargetPos, float flInterval );

	void					UpdateBodyControl( void );
	void					UpdateHeadControl( const Vector &vHeadTarget, float flHeadInfluence );
	virtual	float			GetHeadDebounce( void ) { return 0.3; } // how much of previous head turn to use

	virtual void			MaintainLookTargets( float flInterval );
	virtual bool			ValidEyeTarget(const Vector &lookTargetPos);
	virtual bool			ValidHeadTarget(const Vector &lookTargetPos);
	virtual float			HeadTargetValidity(const Vector &lookTargetPos);

	virtual bool			ShouldBruteForceFailedNav()	{ return true; }

	void					AccumulateIdealYaw( float flYaw, float flIntensity );
	bool					SetAccumulatedYawAndUpdate( void );

	float					m_flAccumYawDelta;
	float					m_flAccumYawScale;

private:
	enum
	{
		HUMANOID_LATCHED_EYE	= 0x0001,
		HUMANOID_LATCHED_HEAD	= 0x0002,
		HUMANOID_LATCHED_ALL	= 0x0003,
	};

	//---------------------------------

	void					UpdateLatchedValues( void );

	//---------------------------------

	int						m_fLatchedPositions;
	Vector					m_latchedEyeOrigin;
	Vector 					m_latchedEyeDirection;		// direction eyes are looking
	Vector 					m_latchedHeadDirection;		// direction head is aiming

	void					ClearHeadAdjustment( void );
	Vector					m_goalHeadDirection;
	float					m_goalHeadInfluence;

	//---------------------------------

	float					m_goalSpineYaw;
	float					m_goalBodyYaw;
	Vector					m_goalHeadCorrection;

	//---------------------------------

	EHANDLE					m_hLookTarget;
	CAI_InterestTarget		m_lookQueue;
	CAI_InterestTarget		m_syntheticLookQueue;

	CAI_InterestTarget		m_randomLookQueue;
	float					m_flNextRandomLookTime;	// FIXME: move to scene

private:
	//---------------------------------

	//PoseParameter_t			m_ParameterBodyTransY;		// "body_trans_Y"
	//PoseParameter_t			m_ParameterBodyTransX;		// "body_trans_X"
	//PoseParameter_t			m_ParameterBodyLift;		// "body_lift"
	PoseParameter_t			m_ParameterBodyYaw;			// "body_yaw"
	//PoseParameter_t			m_ParameterBodyPitch;		// "body_pitch"
	//PoseParameter_t			m_ParameterBodyRoll;		// "body_roll"
	PoseParameter_t			m_ParameterSpineYaw;		// "spine_yaw"
	//PoseParameter_t			m_ParameterSpinePitch;		// "spine_pitch"
	//PoseParameter_t			m_ParameterSpineRoll;		// "spine_roll"
	PoseParameter_t			m_ParameterNeckTrans;		// "neck_trans"
	PoseParameter_t			m_ParameterHeadYaw;			// "head_yaw"
	PoseParameter_t			m_ParameterHeadPitch;		// "head_pitch"
	PoseParameter_t			m_ParameterHeadRoll;		// "head_roll"
};

#endif // AI_BASEACTOR_H
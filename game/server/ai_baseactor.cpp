//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "ai_baseactor.h"
#include "ai_navigator.h"
#include "bone_setup.h"
#include "physics_npc_solver.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_debug_looktargets( "ai_debug_looktargets", "0" );

//-----------------------------------------------------------------------------
// Purpose: clear out latched state
//-----------------------------------------------------------------------------
void CAI_BaseActor::StudioFrameAdvance ()
{
	// clear out head and eye latched values
	m_fLatchedPositions &= ~(HUMANOID_LATCHED_ALL);

	BaseClass::StudioFrameAdvance();
}

void CAI_BaseActor::SetModel( const char *szModelName )
{
	BaseClass::SetModel( szModelName );

	//Init( m_ParameterBodyTransY, "body_trans_Y" );
	//Init( m_ParameterBodyTransX, "body_trans_X" );
	//Init( m_ParameterBodyLift, "body_lift" );
	Init( m_ParameterBodyYaw, "body_yaw" );
	//Init( m_ParameterBodyPitch, "body_pitch" );
	//Init( m_ParameterBodyRoll, "body_roll" );
	Init( m_ParameterSpineYaw, "spine_yaw" );
	//Init( m_ParameterSpinePitch, "spine_pitch" );
	//Init( m_ParameterSpineRoll, "spine_roll" );
	Init( m_ParameterNeckTrans, "neck_trans" );
	Init( m_ParameterHeadYaw, "head_yaw" );
	Init( m_ParameterHeadPitch, "head_pitch" );
	Init( m_ParameterHeadRoll, "head_roll" );
}

//-----------------------------------------------------------------------------
// Purpose: clear out latched state
//-----------------------------------------------------------------------------
void CAI_BaseActor::SetViewtarget( const Vector &viewtarget )
{
	// clear out eye latch
	m_fLatchedPositions &= ~HUMANOID_LATCHED_EYE;

	BaseClass::SetViewtarget( viewtarget );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true position of the eyeballs
//-----------------------------------------------------------------------------
void CAI_BaseActor::UpdateLatchedValues( ) 
{ 
	if (!(m_fLatchedPositions & HUMANOID_LATCHED_HEAD))
	{
		// set head latch
		m_fLatchedPositions |= HUMANOID_LATCHED_HEAD;

		if (!HasCondition( COND_IN_PVS ) || !GetAttachment( "eyes", m_latchedEyeOrigin, &m_latchedHeadDirection ))
		{
			m_latchedEyeOrigin = BaseClass::EyePosition( );
			AngleVectors( GetLocalAngles(), &m_latchedHeadDirection );
		}
		// clear out eye latch
		m_fLatchedPositions &= ~(HUMANOID_LATCHED_EYE);
		// DevMsg( "eyeball %4f %4f %4f  : %3f %3f %3f\n", origin.x, origin.y, origin.z, angles.x, angles.y, angles.z );
	}

	if (!(m_fLatchedPositions & HUMANOID_LATCHED_EYE))
	{
		m_fLatchedPositions |= HUMANOID_LATCHED_EYE;

		if ( CapabilitiesGet() & bits_CAP_ANIMATEDFACE )
		{
			m_latchedEyeDirection = GetViewtarget() - m_latchedEyeOrigin; 
			VectorNormalize( m_latchedEyeDirection );
		}
		else
		{
			m_latchedEyeDirection = m_latchedHeadDirection;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true position of the eyeballs
//-----------------------------------------------------------------------------
Vector CAI_BaseActor::EyePosition( )
{ 
	UpdateLatchedValues();

	return m_latchedEyeOrigin;
}

#define MIN_LOOK_TARGET_DIST 1.0f
#define MAX_FULL_LOOK_TARGET_DIST 10.0f

//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of eye movement for the current head position
//-----------------------------------------------------------------------------
bool CAI_BaseActor::ValidEyeTarget(const Vector &lookTargetPos)
{
	Vector vHeadDir = HeadDirection3D( );
	Vector lookTargetDir	= lookTargetPos - EyePosition();
	float flDist = VectorNormalize(lookTargetDir);

	if (flDist < MIN_LOOK_TARGET_DIST)
	{
		return false;
	}

	// Only look if it doesn't crank my eyeballs too far
	float dotPr = DotProduct(lookTargetDir, vHeadDir);
	// DevMsg( "ValidEyeTarget( %4f %4f %4f )  %3f\n", lookTargetPos.x, lookTargetPos.y, lookTargetPos.z, dotPr );

	if (dotPr > 0.259) // +- 75 degrees
	// if (dotPr > 0.86) // +- 30 degrees
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if target is in legal range of possible head movements
//-----------------------------------------------------------------------------
bool CAI_BaseActor::ValidHeadTarget(const Vector &lookTargetPos)
{
	Vector vFacing = BodyDirection3D();
	Vector lookTargetDir = lookTargetPos - EyePosition();
	float flDist = VectorNormalize(lookTargetDir);

	if (flDist < MIN_LOOK_TARGET_DIST)
	{
		return false;
	}

	// Only look if it doesn't crank my head too far
	float dotPr = DotProduct(lookTargetDir, vFacing);
	if (dotPr > 0 && fabs( lookTargetDir.z ) < 0.7) // +- 90 degrees side to side, +- 45 up/down
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns how much to try to look at the target
//-----------------------------------------------------------------------------
float CAI_BaseActor::HeadTargetValidity(const Vector &lookTargetPos)
{
	Vector vFacing = BodyDirection3D();

	int iForward = LookupAttachment( "forward" );
	if ( iForward > 0 )
	{
		Vector tmp1;
		GetAttachment( iForward, tmp1, &vFacing, NULL, NULL );
	}

	Vector lookTargetDir = lookTargetPos - EyePosition();
	float flDist = lookTargetDir.Length2D();
	VectorNormalize(lookTargetDir);

	if (flDist <= MIN_LOOK_TARGET_DIST)
	{
		return 0;
	}

	// Only look if it doesn't crank my head too far
	float dotPr = DotProduct(lookTargetDir, vFacing);
	// only look if target is within +-135 degrees
	// scale 1..-0.707 == 1..1,  -.707..-1 == 1..0
	// 	X * b + b = 1 == 1 / (X + 1) = b, 3.4142
	float flInterest = clamp( 3.4142f + 3.4142f * dotPr, 0.f, 1.f );

	// stop looking when point too close 
	if (flDist < MAX_FULL_LOOK_TARGET_DIST)
	{
		flInterest = flInterest * (flDist - MIN_LOOK_TARGET_DIST ) / (MAX_FULL_LOOK_TARGET_DIST - MIN_LOOK_TARGET_DIST);
	}

	return flInterest;
}

//-----------------------------------------------------------------------------
// Purpose: Integrate head turn over time
//-----------------------------------------------------------------------------
void CAI_BaseActor::SetHeadDirection( const Vector &vTargetPos, float flInterval)
{
	Assert(0); // Actors shouldn't be calling this, it doesn't do anything
}

float CAI_BaseActor::ClampWithBias( PoseParameter_t index, float value, float base )
{
	return EdgeLimitPoseParameter( (int)index, value, base );
}

//-----------------------------------------------------------------------------
// Purpose: Accumulate all the wanted yaw changes
//-----------------------------------------------------------------------------

void CAI_BaseActor::AccumulateIdealYaw( float flYaw, float flIntensity )
{
	float diff = AngleDiff( flYaw, GetLocalAngles().y );
	m_flAccumYawDelta += diff * flIntensity;
	m_flAccumYawScale += flIntensity;
}

//-----------------------------------------------------------------------------
// Purpose: do any pending yaw movements
//-----------------------------------------------------------------------------

bool CAI_BaseActor::SetAccumulatedYawAndUpdate( void )
{
	if (m_flAccumYawScale > 0.0)
	{
		float diff = m_flAccumYawDelta / m_flAccumYawScale;
		float facing = GetLocalAngles().y + diff;

		m_flAccumYawDelta = 0.0;
		m_flAccumYawScale = 0.0;

		if (IsCurSchedule( SCHED_SCENE_GENERIC ))
		{
			if (!IsMoving())
			{
				GetMotor()->SetIdealYawAndUpdate( facing );
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: match actors "forward" attachment to point in direction of vHeadTarget
//-----------------------------------------------------------------------------

void CAI_BaseActor::UpdateBodyControl( )
{
	Set( m_ParameterBodyYaw, m_goalBodyYaw );
	Set(m_ParameterSpineYaw, m_goalSpineYaw);
}

static ConVar scene_clamplookat( "scene_clamplookat", "1", FCVAR_NONE, "Clamp head turns to a max of 20 degrees per think." );

void CAI_BaseActor::UpdateHeadControl( const Vector &vHeadTarget, float flHeadInfluence )
{
	float flTarget;
	float flLimit;

	if (!(CapabilitiesGet() & bits_CAP_TURN_HEAD))
	{
		return;
	}

	// calc current animation head bias, movement needs to clamp accumulated with this
	QAngle angBias;
	QAngle vTargetAngles;

	int iEyes = LookupAttachment( "eyes" );
	int iChest = LookupAttachment( "chest" );
	int iForward = LookupAttachment( "forward" );

	matrix3x4_t eyesToWorld;
	matrix3x4_t forwardToWorld, worldToForward;

	if (iEyes <= 0 || iForward <= 0)
	{
		// Head control on model without "eyes" or "forward" attachment
		// Most likely this is a cheaple or a generic_actor set to a model that doesn't support head/eye turning.
		// DevWarning( "%s using model \"%s\" that doesn't support head turning\n", GetClassname(), STRING( GetModelName() ) );
		CapabilitiesRemove( bits_CAP_TURN_HEAD );
		return;
	}

	GetAttachment( iEyes, eyesToWorld );

	GetAttachment( iForward, forwardToWorld );
	MatrixInvert( forwardToWorld, worldToForward );

	// Lookup chest attachment to do compounded range limit checks
	if (iChest > 0)
	{
		matrix3x4_t chestToWorld, worldToChest;
		GetAttachment( iChest, chestToWorld );
		MatrixInvert( chestToWorld, worldToChest );
		matrix3x4_t tmpM;
		ConcatTransforms( worldToChest, eyesToWorld, tmpM );
		MatrixAngles( tmpM, angBias );

		angBias.y -= Get( m_ParameterHeadYaw );
		angBias.x -= Get( m_ParameterHeadPitch );
		angBias.z -= Get( m_ParameterHeadRoll );

		/*
		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
		{
			// Msg("bias %f %f %f\n", angBias.x, angBias.y, angBias.z );

			Vector tmp1, tmp2;
			
			VectorTransform( Vector( 0, 0, 0), chestToWorld, tmp1 );
			VectorTransform( Vector( 100, 0, 0), chestToWorld, tmp2 );
			NDebugOverlay::Line( tmp1, tmp2, 0,0,255, false, 0.12 );

			VectorTransform( Vector( 0, 0, 0), eyesToWorld, tmp1 );
			VectorTransform( Vector( 100, 0, 0), eyesToWorld, tmp2 );
			NDebugOverlay::Line( tmp1, tmp2, 0,0,255, false, 0.12 );

			// NDebugOverlay::Line( EyePosition(), pEntity->EyePosition(), 0,0,255, false, 0.5);
		}
		*/
	}
	else
	{
		angBias.Init( 0, 0, 0 );
	}

	matrix3x4_t targetXform;
	targetXform = forwardToWorld;
	Vector vTargetDir = vHeadTarget - EyePosition();

	if (scene_clamplookat.GetBool())
	{
		// scale down pitch when the target is behind the head
		Vector vTargetLocal;
		VectorNormalize( vTargetDir );
		VectorIRotate( vTargetDir, forwardToWorld, vTargetLocal );
		vTargetLocal.z *= clamp( vTargetLocal.x, 0.1f, 1.0f );
		VectorNormalize( vTargetLocal );
		VectorRotate( vTargetLocal, forwardToWorld, vTargetDir );

		// clamp local influence when target is behind the head
		flHeadInfluence = flHeadInfluence * clamp( vTargetLocal.x * 2.0f + 2.0f, 0.0f, 1.0f );
	}

	Studio_AlignIKMatrix( targetXform, vTargetDir );

	matrix3x4_t headXform;
	ConcatTransforms( worldToForward, targetXform, headXform );
	MatrixAngles( headXform, vTargetAngles );

	// partially debounce head goal
	float s0 = 1.0 - flHeadInfluence + GetHeadDebounce() * flHeadInfluence;
	float s1 = (1.0 - s0);
	// limit velocity of head turns
	m_goalHeadCorrection.x = UTIL_Approach( m_goalHeadCorrection.x * s0 + vTargetAngles.x * s1, m_goalHeadCorrection.x, 10.0 );
	m_goalHeadCorrection.y = UTIL_Approach( m_goalHeadCorrection.y * s0 + vTargetAngles.y * s1, m_goalHeadCorrection.y, 30.0 );
	m_goalHeadCorrection.z = UTIL_Approach( m_goalHeadCorrection.z * s0 + vTargetAngles.z * s1, m_goalHeadCorrection.z, 10.0 );

	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		// Msg( "yaw %.1f (%f) pitch %.1f (%.1f)\n", m_goalHeadCorrection.y, vTargetAngles.y, vTargetAngles.x, m_goalHeadCorrection.x );
		// Msg( "yaw %.2f (goal %.2f) (influence %.2f) (flex %.2f)\n", flLimit, m_goalHeadCorrection.y, flHeadInfluence, Get( m_FlexweightHeadRightLeft ) );
	}
	*/

	flTarget = m_goalHeadCorrection.y;
	flLimit = ClampWithBias( m_ParameterHeadYaw, flTarget, angBias.y );
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		Msg( "yaw  %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.y, Get( m_FlexweightHeadRightLeft ), angBias.y, Get( m_ParameterHeadYaw ) );
	}
	*/
	Set( m_ParameterHeadYaw, flLimit );

	flTarget = m_goalHeadCorrection.x;
	flLimit = ClampWithBias( m_ParameterHeadPitch, flTarget, angBias.x );
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		Msg( "pitch %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.x, Get( m_FlexweightHeadUpDown ), angBias.x, Get( m_ParameterHeadPitch ) );
	}
	*/
	Set( m_ParameterHeadPitch, flLimit );

	flTarget = m_goalHeadCorrection.z;
	flLimit = ClampWithBias( m_ParameterHeadRoll, flTarget, angBias.z );
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		Msg( "roll  %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.z, Get( m_FlexweightHeadTilt ), angBias.z, Get( m_ParameterHeadRoll ) );
	}
	*/
	Set( m_ParameterHeadRoll, flLimit );
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::EyeDirection2D( void )
{
	Vector vEyeDirection = EyeDirection3D( );
	vEyeDirection.z = 0;

	vEyeDirection.AsVector2D().NormalizeInPlace();

	return vEyeDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's eye direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::EyeDirection3D( void )
{
	UpdateLatchedValues( );

	return m_latchedEyeDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 2D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::HeadDirection2D( void )
{	
	Vector vHeadDirection = HeadDirection3D();
	vHeadDirection.z = 0;
	vHeadDirection.AsVector2D().NormalizeInPlace();
	return vHeadDirection;
}

//------------------------------------------------------------------------------
// Purpose : Calculate the NPC's head direction in 3D world space
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseActor::HeadDirection3D( )
{	
	UpdateLatchedValues( );

	return m_latchedHeadDirection;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAI_BaseActor::HasActiveLookTargets( void )
{
	return m_lookQueue.Count() != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Clear any active look targets for the specified entity
//-----------------------------------------------------------------------------
void CAI_BaseActor::ClearLookTarget( CBaseEntity *pTarget )
{
	int iIndex = m_lookQueue.Find( pTarget );
	if ( iIndex != m_lookQueue.InvalidIndex() )
	{
		m_lookQueue.Remove(iIndex);
	}

	iIndex = m_randomLookQueue.Find( pTarget );
	if ( iIndex != m_randomLookQueue.InvalidIndex() )
	{
		m_randomLookQueue.Remove(iIndex);

		// Figure out the new random look time
		m_flNextRandomLookTime = gpGlobals->curtime + 1.0;
		for (int i = 0; i < m_randomLookQueue.Count(); i++)
		{
			if ( m_randomLookQueue[i].m_flEndTime > m_flNextRandomLookTime )
			{
				m_flNextRandomLookTime = m_randomLookQueue[i].m_flEndTime + 0.4;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look at other NPCs and clients from time to time
//-----------------------------------------------------------------------------
float CAI_BaseActor::PickLookTarget( bool bExcludePlayers, float minTime, float maxTime )
{
	return PickLookTarget( m_randomLookQueue, bExcludePlayers, minTime, maxTime );
}

float CAI_BaseActor::PickLookTarget( CAI_InterestTarget &queue, bool bExcludePlayers, float minTime, float maxTime )
{
	AILookTargetArgs_t args;
	
	args.vTarget			= vec3_invalid;
	args.flDuration			= random->RandomFloat( minTime, maxTime );
	args.flInfluence		= random->RandomFloat( 0.3, 0.5 );
	args.flRamp				= random->RandomFloat( 0.2, 0.4 );
	args.bExcludePlayers	= bExcludePlayers;
	args.pQueue				= &queue;
	
	bool foundLookTarget = true;
	
	if ( !PickTacticalLookTarget( &args ) )
	{
		if ( !PickRandomLookTarget( &args ) )
		{
			foundLookTarget = false;
		}
	}
	
	if ( !foundLookTarget )
	{
		// DevMsg("nothing to see\n" );
		MakeRandomLookTarget( &args, minTime, maxTime );
	}
	
	// See if derived NPCs want to do anything with this look target before I use it
	OnSelectedLookTarget( &args );

	if ( args.hTarget != NULL )
	{
		Assert( args.vTarget == vec3_invalid );
		queue.Add( args.hTarget, args.flInfluence, args.flDuration, args.flRamp );
	}
	else
	{
		Assert( args.vTarget != vec3_invalid );
		queue.Add( args.vTarget, args.flInfluence, args.flDuration, args.flRamp );
	}

	return args.flDuration;
	
}

bool CAI_BaseActor::PickTacticalLookTarget( AILookTargetArgs_t *pArgs )
{
	CBaseEntity *pEnemy = GetEnemy();

	if (pEnemy != NULL)
	{
		if ( ( FVisible( pEnemy ) || random->RandomInt(0, 3) == 0 ) && ValidHeadTarget(pEnemy->EyePosition()))
		{
			// look at enemy closer
			pArgs->hTarget = pEnemy;
			pArgs->flInfluence = random->RandomFloat( 0.7, 1.0 );
			pArgs->flRamp = 0;
			return true;
		}
		else
		{
			// look at something else for a shorter time
			pArgs->flDuration = random->RandomFloat( 0.5, 0.8 );
			// move head faster
			pArgs->flRamp = 0.2;
		}
	}
	return false;
}

bool CAI_BaseActor::PickRandomLookTarget( AILookTargetArgs_t *pArgs )
{
	bool bIsNavigating = ( GetNavigator()->IsGoalActive() && GetNavigator()->IsGoalSet() );
	
	if ( bIsNavigating && random->RandomInt(1, 10) <= 3 )
	{
		Vector navLookPoint;
		Vector delta;
		if ( GetNavigator()->GetPointAlongPath( &navLookPoint, 12 * 12 ) && (delta = navLookPoint - GetAbsOrigin()).Length() > 8.0 * 12.0 )
		{
			if ( random->RandomInt(1, 10) <= 5 )
			{
				pArgs->vTarget = navLookPoint;
				pArgs->flDuration = random->RandomFloat( 0.2, 0.4 );
			}
			else
			{
				pArgs->hTarget = this;
				pArgs->flDuration = random->RandomFloat( 1.0, 2.0 );
			}
			pArgs->flRamp = 0.2;
			return true;
		}
	}

	if ( GetState() == NPC_STATE_COMBAT && random->RandomInt(1, 10) <= 8 )
	{
		// if in combat, look forward 80% of the time?
		pArgs->hTarget = this;
		return true;
	}

	CBaseEntity *pBestEntity = NULL;
	CBaseEntity *pEntity = NULL;
	int iHighestImportance = 0;
	int iConsidered = 0;
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), 30 * 12, 0 ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		if (pEntity == this)
		{
			continue;
		}

		if ( pArgs->bExcludePlayers && pEntity->GetFlags() & FL_CLIENT )
		{
			// Don't look at any players.
			continue;
		}

		if (!pEntity->IsViewable())
		{
			// Don't look at things without a model, or aren't tagged as interesting
			continue;
		}

		if ( pEntity->GetOwnerEntity() && !pEntity->GetOwnerEntity()->IsViewable() )
		{
			// Don't look at things that are associated with non-viewable owners. 
			// Specifically, this prevents NPC's looking at beams or sprites that
			// are part of a viewmodel. (sjb)
			continue;
		}

		// Don't look at any object that is ultimately parented to the player.
		// These objects will almost always be at the player's origin (feet), and it
		// looks bad when an actor looks at the player's feet. (sjb)
		CBaseEntity *pParent = pEntity->GetParent();
		bool bObjectParentedToPlayer = false;
		while( pParent )
		{
			if( pParent->IsPlayer() )
			{
				bObjectParentedToPlayer = true;
				break;
			}

			pParent = pParent->GetParent();
		}

		if( bObjectParentedToPlayer )
			continue;
		
		// skip entities we're already looking at
		if ( pArgs->pQueue->Find( pEntity ) != pArgs->pQueue->InvalidIndex() )
			continue;

		// keep track of number of interesting things
		iConsidered++;

		if ((pEntity->GetFlags() & FL_CLIENT) && (pEntity->IsMoving() || random->RandomInt( 0, 2) == 0))
		{
			if (FVisible( pEntity ) && ValidHeadTarget(pEntity->EyePosition()))
			{
				pArgs->flDuration = random->RandomFloat( 1.0, 4.0 );
				pBestEntity = pEntity;
				break;
			}
		}
	
		Vector delta = (pEntity->EyePosition() - EyePosition());
		VectorNormalize( delta );

		int iImportance;
#if 0
		// consider things in front to be more important than things to the sides
		iImportance = (DotProduct( delta, HeadDirection3D() );
#else
		// No, for now, give all targets random priority (as long as they're in front)
		iImportance = random->RandomInt( 1, 100 );
		
#endif
		// make other npcs, and moving npc's far more important
		if (pEntity->MyNPCPointer())
		{
			iImportance *= 10;
			if (pEntity->IsMoving())
			{
				iImportance *= 10;
			}
		}

		if ( iImportance > iHighestImportance )
		{
			if (FVisible( pEntity ) && ValidHeadTarget(pEntity->EyePosition()))
			{
				iHighestImportance = iImportance;
				pBestEntity	= pEntity; 
				// NDebugOverlay::Line( EyePosition(), pEntity->EyePosition(), 0,0,255, false, 0.5);
			}
		}
	}

	// if there were too few things to look at, don't trust the item
	if (iConsidered < random->RandomInt( 0, 5))
	{
		pBestEntity = NULL;
	}

	if (pBestEntity)
	{
		//Msg("looking at %s\n", pBestEntity->GetClassname() );
		//NDebugOverlay::Line( EyePosition(), pBestEntity->WorldSpaceCenter(), 255, 0, 0, false, 5 );
		pArgs->hTarget = pBestEntity;
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// All attempts to find a target have failed, so just make something up.
//-----------------------------------------------------------------------------
void CAI_BaseActor::MakeRandomLookTarget( AILookTargetArgs_t *pArgs, float minTime, float maxTime )
{
	Vector forward, right, up;
	GetVectors( &forward, &right, &up );
	pArgs->vTarget = EyePosition() + forward * 128 + right * random->RandomFloat(-32, 32) + up * random->RandomFloat(-16, 16);
	pArgs->flDuration = random->RandomFloat( minTime, maxTime );
	pArgs->flInfluence = 0.01;
	pArgs->flRamp = random->RandomFloat( 0.8, 2.8 );
}

//-----------------------------------------------------------------------------
// Purpose: Make sure we're looking at what we're shooting at
//-----------------------------------------------------------------------------

void CAI_BaseActor::StartTaskRangeAttack1( const Task_t *pTask )
{
	BaseClass::StartTaskRangeAttack1( pTask );
	if (GetEnemy())
	{
		AddLookTarget( GetEnemy(), 1.0, 0.5, 0.2 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set direction that the NPC is looking
//-----------------------------------------------------------------------------
void CAI_BaseActor::AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	m_lookQueue.Add( pTarget, flImportance, flDuration, flRamp );
}


void CAI_BaseActor::AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_lookQueue.Add( vecPosition, flImportance, flDuration, flRamp );
}

//-----------------------------------------------------------------------------
// Purpose: Maintain eye, head, body postures, etc.
//-----------------------------------------------------------------------------
void CAI_BaseActor::MaintainLookTargets( float flInterval )
{
	int i;

	// decay body/spine yaw
	m_goalSpineYaw = m_goalSpineYaw * 0.8;
	m_goalBodyYaw = m_goalBodyYaw * 0.8;
	m_goalHeadCorrection = m_goalHeadCorrection * 0.8;

	// ARRGGHHH, this needs to be moved!!!!
	SetAccumulatedYawAndUpdate( );
	MaintainTurnActivity( );
	DoBodyLean( );
	UpdateBodyControl( );
	InvalidateBoneCache();

	// cached versions of the current eye position
	Vector vEyePosition = EyePosition( );

	// initialize goal head direction to be current direction - this frames animation layering/pose parameters -  
	// but with the head controlls removed.
	Vector vHead = HeadDirection3D( );
	float flHeadInfluence = 0.0;

	// NDebugOverlay::Line( vEyePosition, vEyePosition + vHead * 16, 0,0,255, false, 0.1);

	// clean up look targets
	m_lookQueue.Cleanup();

	// clean up random look targets
	m_randomLookQueue.Cleanup();

	// clean up synthetic look targets
	m_syntheticLookQueue.Cleanup();

	// if there's real things to look at, turn off the random targets
	if (m_lookQueue.Count() != 0 || m_syntheticLookQueue.Count() != 0)
	{
		for (i = 0; i < m_randomLookQueue.Count(); i++)
		{
			if (gpGlobals->curtime < m_randomLookQueue[i].m_flEndTime - m_randomLookQueue[i].m_flRamp - 0.2)
			{
				m_randomLookQueue[i].m_flEndTime = gpGlobals->curtime + m_randomLookQueue[i].m_flRamp + 0.2;
			}
		}
		m_flNextRandomLookTime = gpGlobals->curtime + 1.0;
	}
	else if (gpGlobals->curtime >= m_flNextRandomLookTime && GetState() != NPC_STATE_SCRIPT)
	{
		// Look at whatever!
		m_flNextRandomLookTime = gpGlobals->curtime + PickLookTarget( m_randomLookQueue ) - 0.4;
	}

	// don't bother with any of the rest if the player can't see you
	if (!HasCondition( COND_IN_PVS ))
	{
		return;
	}

	CUtlVector<CAI_InterestTarget_t *> active;
	// clean up random look targets
	for (i = 0; i < m_randomLookQueue.Count(); i++)
	{
		active.AddToTail( &m_randomLookQueue[i] );
	}
	for (i = 0; i < m_lookQueue.Count(); i++)
	{
		active.AddToTail( &m_lookQueue[i] );
	}
	for (i = 0; i < m_syntheticLookQueue.Count(); i++)
	{
		active.AddToTail( &m_syntheticLookQueue[i] );
	}
		
	// figure out ideal head yaw
	bool bValidHeadTarget = false;
	bool bExpectedHeadTarget = false;
	for (i = 0; i < active.Count();i++)
	{
		Vector dir;
		float flDist = 100.0f;
		
		bExpectedHeadTarget = true;
		float flInterest = active[i]->Interest( );

		if (active[i]->IsThis( this ))
		{
			int iForward = LookupAttachment( "forward" );
			if ( iForward > 0)
			{
				Vector tmp1;
				GetAttachment( iForward, tmp1, &dir, NULL, NULL );
			}
			else
			{
				dir = HeadDirection3D();
			}
		}
		else
		{
			dir = active[i]->GetPosition() - vEyePosition;
			flDist = VectorNormalize( dir );
			flInterest = flInterest * HeadTargetValidity( active[i]->GetPosition() );
		}
		
		/*
		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
		{
			DevMsg( "head (%d) %.2f : %s : %.1f %.1f %.1f\n", i, flInterest, active[i]->m_hTarget->GetClassname(), active[i]->GetPosition().x, active[i]->GetPosition().y, active[i]->GetPosition().z );
		}
		*/
		
		if (flInterest > 0.0)
		{
			if (flHeadInfluence == 0.0)
			{
				vHead = dir;
				flHeadInfluence = flInterest;
			}
			else
			{
				flHeadInfluence = flHeadInfluence * (1 - flInterest) + flInterest;
				float w = flInterest / flHeadInfluence;
				vHead = vHead * (1 - w) + dir * w;
			}

			bValidHeadTarget = true;

			// NDebugOverlay::Line( vEyePosition, vEyePosition + dir * 64, 0,255,0, false, 0.1);
		}
		else
		{
			// NDebugOverlay::Line( vEyePosition, active[i]->GetPosition(), 255,0,0, false, 0.1);
		}
	}

	Assert( flHeadInfluence <= 1.0 );

	// turn head toward target
	if (bValidHeadTarget)
	{
		UpdateHeadControl( vEyePosition + vHead * 100, flHeadInfluence );
		m_goalHeadDirection = vHead;
		m_goalHeadInfluence = flHeadInfluence;
	}
	else
	{
		// no target, decay all head control direction
		m_goalHeadDirection = m_goalHeadDirection * 0.8 + vHead * 0.2;

		m_goalHeadInfluence = MAX( m_goalHeadInfluence - 0.2, 0 );

		VectorNormalize( m_goalHeadDirection );
		UpdateHeadControl( vEyePosition + m_goalHeadDirection * 100, m_goalHeadInfluence );
		// NDebugOverlay::Line( vEyePosition, vEyePosition + m_goalHeadDirection * 100, 255,0,0, false, 0.1);
	}

	// DevMsg( "%.1f %.1f ", GetPoseParameter( "head_pitch" ), GetPoseParameter( "head_roll" ) );

	// figure out eye target
	// eyes need to look directly at a target, even if the head doesn't quite aim there yet.
	bool bFoundTarget = false;
	EHANDLE	hTarget = NULL;

	for (i = active.Count() - 1; i >= 0; i--)
	{
		if (active[i]->IsThis( this ))
		{
			// DevMsg( "eyes (%d) %s\n", i, STRING( active[i]->m_hTarget->GetEntityName().ToCStr() ) );
			bFoundTarget = true;
			hTarget = this;
			SetViewtarget( vEyePosition + HeadDirection3D() * 100 );
			// NDebugOverlay::Line( vEyePosition, vEyePosition + HeadDirection3D() * 100, 255,0,0, false, 0.1);
			break;
		}
		else
		{
			// E3 Hack
			if (ValidEyeTarget(active[i]->GetPosition()))
			{
				// DevMsg( "eyes (%d) %s\n", i, STRING( pTarget->GetEntityName().ToCStr() ) );

				bFoundTarget = true;
				hTarget = active[i]->m_hTarget;
				SetViewtarget( active[i]->GetPosition() );
				break;
			}
		}
	}

	if (m_hLookTarget != hTarget)
	{
		m_hLookTarget = hTarget;

		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) && ai_debug_looktargets.GetInt() == 2 && m_hLookTarget.Get() )
		{
			if ( m_hLookTarget != this )
			{
				Vector vecEyePos = m_hLookTarget->EyePosition();
				NDebugOverlay::Box( vecEyePos, -Vector(5,5,5), Vector(5,5,5), 0, 255, 0, 255, 20 );
				NDebugOverlay::Line( EyePosition(), vecEyePos, 0,255,0, true, 20 );
				NDebugOverlay::Text( vecEyePos, UTIL_VarArgs( "%s (%s)", m_hLookTarget->GetClassname(), m_hLookTarget->GetDebugName() ), false, 20 );
			}
		}

		OnNewLookTarget();
	}

	// this should take into acount where it will try to be....
	if (!bFoundTarget && !ValidEyeTarget( GetViewtarget() ))
	{
		Vector right, up;
		VectorVectors( HeadDirection3D(), right, up );
		// DevMsg("random view\n");
		SetViewtarget( EyePosition() + HeadDirection3D() * 128 + right * random->RandomFloat(-32,32) + up * random->RandomFloat(-16,16) );
	}

	if ( m_hLookTarget != NULL )
	{
		Vector absVel = m_hLookTarget->GetAbsVelocity();
		CBaseEntity *ground = m_hLookTarget->GetGroundEntity();
		if ( ground && ground->GetMoveType() == MOVETYPE_PUSH)
		{
			absVel = absVel + ground->GetAbsVelocity();
		}

		if ( !VectorCompare( absVel, vec3_origin ) )
		{
			Vector viewTarget = GetViewtarget();

			// Forward one think cycle
			viewTarget += absVel * flInterval;

			SetViewtarget( viewTarget );
		}
	}

	// NDebugOverlay::Triangle( vEyePosition, GetViewtarget(), GetAbsOrigin(), 255, 255, 255, 10, false, 0.1 );
	// DevMsg("pitch %.1f yaw %.1f\n", GetFlexWeight( "eyes_updown" ), GetFlexWeight( "eyes_rightleft" ) );

	if ( ai_debug_looktargets.GetInt() == 1 && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
		NDebugOverlay::Box( GetViewtarget(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, 0, 20 );
		NDebugOverlay::Line( EyePosition(),GetViewtarget(), 0,255,0, false, .1 );
	}
}
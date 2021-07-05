//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "game.h"			
#include "ndebugoverlay.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_navigator.h"
#include "scripted.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// PATHING & HIGHER LEVEL MOVEMENT
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectHighPrioSchedule()
{
	CBaseEntity *pTargetOverride = m_hTargetSchedEntity.Get();
	if (!IsAlive() || (m_schedInterruptability < Interruptability_t::DEATH_INTERRUPTABILITY) || !pTargetOverride)
		return SCHED_NONE;

	const Vector &vTargetPos = pTargetOverride->GetAbsOrigin();
	const float flDist = (vTargetPos - WorldSpaceCenter()).Length();
	if ((flDist <= 70.0f) && CBaseCombatCharacter::FVisible(vTargetPos)) // Can we see the target, and are we close enuff?	
	{
		m_hTargetSchedEntity = NULL;
		return SCHED_NONE;
	}

	return SCHED_MOVE_TO_TARGET_VITAL;
}

bool CAI_BaseNPC::ScheduledMoveToGoalEntity(int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity)
{
	m_hTargetSchedEntity = pGoalEntity;
	m_actTargetMovement = movementActivity;

	if (m_NPCState == NPC_STATE_NONE)
	{
		// More than likely being grabbed before first think. Set ideal state to prevent schedule stomp
		m_NPCState = m_IdealNPCState;
	}

	SetSchedule(scheduleType);
	SetGoalEnt(pGoalEntity);

	// HACKHACK: Call through TranslateNavGoal to fixup this goal position
	// UNDONE: Remove this and have NPCs that need this functionality fix up paths in the 
	// movement system instead of when they are specified.
	AI_NavGoal_t goal(pGoalEntity->GetAbsOrigin(), movementActivity, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST);
	TranslateNavGoal(pGoalEntity, goal.dest);

	return GetNavigator()->SetGoal(goal);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity )
{
	if ( m_NPCState == NPC_STATE_NONE )
	{
		// More than likely being grabbed before first think. Set ideal state to prevent schedule stomp
		m_NPCState = m_IdealNPCState;
	}

	SetSchedule( scheduleType );

	SetGoalEnt( pPathStart );

	// HACKHACK: Call through TranslateNavGoal to fixup this goal position
	AI_NavGoal_t goal(GOALTYPE_PATHCORNER, pPathStart->GetLocalOrigin(), movementActivity, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST);

	TranslateNavGoal( pPathStart, goal.dest );

	return GetNavigator()->SetGoal( goal );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsMoving( void ) 
{ 
	return GetNavigator()->IsGoalSet(); 
}


//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsCurTaskContinuousMove()
{
	const Task_t* pTask = GetTask();

	// This bit of logic strikes me funny, but the case does exist. (sjb)
	if( !pTask )
		return true;

	switch( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_MOVE_TO_GOAL_RANGE:
	case TASK_PLAY_SCENE:
	case TASK_RUN_PATH_TIMED:
	case TASK_WALK_PATH_TIMED:
	case TASK_RUN_PATH_FOR_UNITS:
	case TASK_WALK_PATH_FOR_UNITS:
	case TASK_RUN_PATH_FLEE:
	case TASK_WALK_PATH_WITHIN_DIST:
	case TASK_RUN_PATH_WITHIN_DIST:
		return true;
		break;

	default:
		return false;
		break;
	}
}

//----------------------------------------------------------------------------------
// Purpose: Returns z value of floor below given point (up to fMaxDrop inches below)
// Input  :
// Output :
//----------------------------------------------------------------------------------
static float GetFloorZ(const Vector &origin, float fMaxDrop)
{
	// trace to the ground, then pop up 8 units and place node there to make it
	// easier for them to connect (think stairs, chairs, and bumps in the floor).
	// After the routing is done, push them back down.
	//
	trace_t	tr;
	AI_TraceLine(origin,
		origin - Vector(0, 0, fMaxDrop),
		MASK_NPCSOLID_BRUSHONLY,
		NULL,
		COLLISION_GROUP_NONE,
		&tr);

	// This trace is ONLY used if we hit an entity flagged with FL_WORLDBRUSH
	trace_t	trEnt;
	AI_TraceLine(origin,
		origin - Vector(0, 0, fMaxDrop),
		MASK_NPCSOLID,
		NULL,
		COLLISION_GROUP_NONE,
		&trEnt);


	// Did we hit something closer than the floor?
	if (trEnt.fraction < tr.fraction)
	{
		// If it was a world brush entity, copy the node location
		if (trEnt.m_pEnt)
		{
			CBaseEntity *e = trEnt.m_pEnt;
			if (e && (e->GetFlags() & FL_WORLDBRUSH))
			{
				tr.endpos = trEnt.endpos;
			}
		}
	}

	return tr.endpos.z;
}

//-----------------------------------------------------------------------------
// Purpose: Checks the validity of the given route's goaltype
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ValidateNavGoal()
{
	if (GetNavigator()->GetGoalType() == GOALTYPE_COVER)
	{
		// Check if this location will block my enemy's line of sight to me
		if (GetEnemy())
		{
			Vector	 vCoverLocation = GetNavigator()->GetGoalPos();

			// For now we have to drop the node to the floor so we can
			// get an accurate postion of the NPC.  Should change once Ken checks in
			float floorZ = GetFloorZ(vCoverLocation, 384);
			vCoverLocation.z = floorZ;

			Vector vEyePos = vCoverLocation + EyeOffset(ACT_COVER);

			if (!IsCoverPosition( GetEnemy()->EyePosition(), vEyePos ) )
			{
				TaskFail(FAIL_BAD_PATH_GOAL);
				return false;
			}
		}
	}
	return true;
}



//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_BaseNPC::OpenDoorAndWait( CBaseEntity *pDoor )
{
	float flTravelTime = 0;

	//DevMsg( 2, "A door. ");
	if (pDoor && !pDoor->IsLockedByMaster())
	{
		pDoor->Use(this, this, USE_ON, 0.0);
		flTravelTime = pDoor->GetMoveDoneTime();
		if ( pDoor->GetEntityName() != NULL_STRING )
		{
			CBaseEntity *pTarget = NULL;
			for (;;)
			{
				pTarget = gEntList.FindEntityByName( pTarget, pDoor->GetEntityName() );

				if ( pTarget != pDoor )
				{
					if ( !pTarget )
						break;

					if ( FClassnameIs( pTarget, pDoor->GetClassname() ) )
					{
						pTarget->Use(this, this, USE_ON, 0.0);
					}
				}
			}
		}
	}

	return gpGlobals->curtime + flTravelTime;
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::CanStandOn( CBaseEntity *pSurface ) const
{
	if ( !pSurface->IsAIWalkable() )
	{
		return false;
	}

	CAI_Navigator *pNavigator = const_cast<CAI_Navigator *>(GetNavigator());

	if ( pNavigator->IsGoalActive() && 
		 pSurface == pNavigator->GetGoalTarget() )
		return false;

	return BaseClass::CanStandOn( pSurface );
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, 
							   float maxUp, float maxDown, float maxDist ) const
{
	if ((endPos.z - startPos.z) > maxUp + 0.1) 
		return false;
	if ((startPos.z - endPos.z) > maxDown + 0.1) 
		return false;

	if ((apex.z - startPos.z) > maxUp * 1.25 ) 
		return false;

	float dist = (startPos - endPos).Length();
	if ( dist > maxDist + 0.1) 
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	return IsJumpLegal(startPos, apex, endPos, GetMaxJumpRise(), GetMaxJumpDrop(), GetMaxJumpDistance());
}

//-----------------------------------------------------------------------------
// Purpose: Returns a throw velocity from start to end position
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CAI_BaseNPC::CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize) 
{
	// Get the height I have to throw to get to the target
	float stepHeight = endPos.z - startPos.z;
	float throwHeight = 0;

	// -----------------------------------------------------------------
	// Now calcluate the distance to a point halfway between our current
	// and target position.  (the apex of our throwing arc)
	// -----------------------------------------------------------------
	Vector targetDir2D = endPos - startPos;
	targetDir2D.z = 0;

	float distance = VectorNormalize(targetDir2D);


	// If jumping up we want to throw a bit higher than the height diff	
	if (stepHeight > 0) 
	{
		throwHeight = stepHeight + fArcSize;	
	}	
	else
	{
		throwHeight = fArcSize;
	}
	// Make sure that I at least catch some air
	if (throwHeight < fArcSize)
	{
		throwHeight = fArcSize;
	}


	// -------------------------------------------------------------
	// calculate the vertical and horizontal launch velocities
	// -------------------------------------------------------------
	float velVert = (float)sqrt(2.0f*fGravity*throwHeight);

	float divisor = velVert;
	divisor += (float)sqrt((2.0f*(-fGravity)*(stepHeight-throwHeight)));

	float velHorz = (distance * fGravity)/divisor;

	// -----------------------------------------------------------
	// Make the horizontal throw vector and add vertical component
	// -----------------------------------------------------------
	Vector throwVel = targetDir2D * velHorz;
	throwVel.z = velVert;

	return throwVel;
}

bool CAI_BaseNPC::ShouldMoveWait()
{
	return (m_flMoveWaitFinished > gpGlobals->curtime);
}

float CAI_BaseNPC::GetStepDownMultiplier() const
{
	return m_pNavigator->GetStepDownMultiplier();
}


//-----------------------------------------------------------------------------
// Purpose: execute any movement this sequence may have
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::AutoMovement( CBaseEntity *pTarget, AIMoveTrace_t *pTraceResult )
{
	return AutoMovement( GetAnimTimeInterval(), pTarget, pTraceResult );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//			 - 
//			*pTraceResult - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::AutoMovement( float flInterval, CBaseEntity *pTarget, AIMoveTrace_t *pTraceResult )
{
	bool ignored;
	Vector newPos;
	QAngle newAngles;

	if (flInterval <= 0.0)
		return true;

	m_ScheduleState.bTaskRanAutomovement = true;

	if (GetIntervalMovement( flInterval, ignored, newPos, newAngles ))
	{
		// DevMsg( "%.2f : (%.1f) %.1f %.1f %.1f\n", gpGlobals->curtime, (newPos - GetLocalOrigin()).Length(), newPos.x, newPos.y, newAngles.y );
	
		if ( m_hCine )
		{
			m_hCine->ModifyScriptedAutoMovement( &newPos );
		}

		if (GetMoveType() == MOVETYPE_STEP)
		{
			if (!(GetFlags() & FL_FLY))
			{
				if ( !pTarget )
				{
					pTarget = GetNavTargetEntity();
				}

				return ( GetMotor()->MoveGroundStep( newPos, pTarget, newAngles.y, false, true, pTraceResult ) == AIM_SUCCESS );
			}
			else
			{
				// FIXME: here's no direct interface to a fly motor, plus this needs to support a state where going through the world is okay.
				// FIXME: add callbacks into the script system for validation
				// FIXME: add function on scripts to force only legal movements
				// FIXME: GetIntervalMovement deals in Local space, nor global.  Currently now way to communicate that through these interfaces.
				SetLocalOrigin( newPos );
				SetLocalAngles( newAngles );
				return true;
			}
		}
		else if (GetMoveType() == MOVETYPE_FLY)
		{
			Vector dist = newPos - GetLocalOrigin();

			VectorScale( dist, 1.0 / flInterval, dist );

			SetLocalVelocity( dist );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: return max 1/10 second rate of turning
// Input  :
// Output :
//-----------------------------------------------------------------------------

float CAI_BaseNPC::MaxYawSpeed( void )
{
	return 45;
}

//-----------------------------------------------------------------------------
// Returns the estimate in seconds before we reach our nav goal.
// -1 means we don't know / haven't calculated it yet.
//-----------------------------------------------------------------------------
float CAI_BaseNPC::GetTimeToNavGoal()
{
	float flDist = GetNavigator()->BuildAndGetPathDistToGoal();
	if ( flDist < 0 )
	{
		return -1.0f;
	}

	float flSpeed = GetIdealSpeed();

	// FIXME: needs to consider stopping time!
	if (flSpeed > 0 && flDist > 0)
	{
		return flDist / flSpeed;
	}
	return 0.0;
}

//=============================================================================

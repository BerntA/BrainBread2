//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_PATHFINDER_H
#define AI_PATHFINDER_H

#include "ai_component.h"
#include "ai_navtype.h"

#ifdef BB2_USE_NAVMESH
#include "nav_pathfind.h"
#include "nav_mesh.h"
#endif // BB2_USE_NAVMESH

#if defined( _WIN32 )
#pragma once
#endif

struct AIMoveTrace_t;
struct OverlayLine_t;
struct AI_Waypoint_t;

//-----------------------------------------------------------------------------
// The type of route to build
enum RouteBuildFlags_e 
{
	bits_BUILD_GROUND		=			0x00000001, // 
	bits_BUILD_JUMP			=			0x00000002, //
	bits_BUILD_FLY			=			0x00000004, // 
	bits_BUILD_CLIMB		=			0x00000008, //
	bits_BUILD_GIVEWAY		=			0x00000010, //
	bits_BUILD_TRIANG		=			0x00000020, //
	bits_BUILD_IGNORE_NPCS	=			0x00000040, // Ignore collisions with NPCs
	bits_BUILD_COLLIDE_NPCS	=			0x00000080, // Use    collisions with NPCs (redundant for argument clarity)
	bits_BUILD_GET_CLOSE	=			0x00000100, // the route will be built even if it can't reach the destination
};

//-----------------------------------------------------------------------------
// CAI_Pathfinder
//
// Purpose: Executes pathfinds through an associated network.
//
//-----------------------------------------------------------------------------

class CAI_Pathfinder : public CAI_Component
{
public:
	CAI_Pathfinder( CAI_BaseNPC *pOuter )
	 :	CAI_Component(pOuter)
	{
	}

	void Init();

	// --------------------------------
	
	AI_Waypoint_t *BuildRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType = NAV_NONE, bool bLocalSucceedOnWithinTolerance = false );

#ifdef BB2_USE_NAVMESH
	virtual AI_Waypoint_t *BuildNavRoute(CBaseEntity *pTarget, CNavArea *area, const Vector &start, const Vector &end, int buildFlags);
	virtual AI_Waypoint_t *BuildJumpRoute(CBaseEntity *pTarget, const Vector &areaStart, const Vector &areaEnd, const Vector &dir, Vector &jumpStart, Vector &jumpEnd, const float &heightDiff, const float &hullWide, bool bDirect = false);
	virtual bool BuildJumpRoute(CBaseEntity *pTarget, const Vector &from, const Vector &to, Vector &start, Vector &end);
	virtual AI_Waypoint_t *BuildDirectJumpRoute(CBaseEntity *pTarget, const Vector &start, const Vector &end, int buildFlags);
	virtual bool CanClimbToPoint(float dir, const Vector &vecStart, const Vector &vecEnd);
#endif

	// --------------------------------
	
	virtual AI_Waypoint_t *BuildLocalRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float goalTolerance);
	virtual AI_Waypoint_t *BuildRadialRoute( const Vector &vStartPos, const Vector &vCenterPos, const Vector &vGoalPos, float flRadius, float flArc, float flStepDist, bool bClockwise, float goalTolerance, bool bAirRoute );	
	
	virtual AI_Waypoint_t *BuildTriangulationRoute( const Vector &vStart, 
													const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID,
													float flYaw, float flDistToBlocker, Navigation_t navType);

	virtual AI_Waypoint_t *BuildOBBAvoidanceRoute(  const Vector &vStart, const Vector &vEnd, 
													const CBaseEntity *pObstruction, const CBaseEntity *pTarget, 
													Navigation_t navType );

	// --------------------------------
	
	bool Triangulate( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, 
						float flDistToBlocker, CBaseEntity const *pTargetEnt, Vector *pApex );
						
	// --------------------------------
	
	void DrawDebugGeometryOverlays( int m_debugOverlays );

protected:
	virtual bool	CanUseLocalNavigation() { return true; }

private:

	AI_Waypoint_t*	BuildRouteThroughPoints( Vector *vecPoints, int nNumPoints, int nDirection, int nStartIndex, int nEndIndex, Navigation_t navType, CBaseEntity *pTarget );

	// --------------------------------
	
	// Builds a simple route (no triangulation, no making way)
	AI_Waypoint_t	*BuildSimpleRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd, 
		const CBaseEntity *pTarget, int endFlags, float flYaw);

	// Builds a complex route (triangulation, making way)
	AI_Waypoint_t	*BuildComplexRoute( Navigation_t navType, const Vector &vStart, 
		const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, 
		int buildFlags, float flYaw, float goalTolerance, float maxLocalNavDistance );

	AI_Waypoint_t	*BuildGroundRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildFlyRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildJumpRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw );
	AI_Waypoint_t	*BuildClimbRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw );

	// --------------------------------
	
	bool TestTriangulationRoute( Navigation_t navType, const Vector& vecStart, 
		const Vector &vecApex, const Vector &vecEnd, const CBaseEntity *pTargetEnt, AIMoveTrace_t *pStartTrace );

	// --------------------------------
	
	bool			CanGiveWay( const Vector& vStart, const Vector& vEnd, CBaseEntity *pNPCBlocker );

	// --------------------------------

	bool			UseStrongOptimizations();

	// --------------------------------
	// Debugging fields and functions

	class CTriDebugOverlay
	{
	public:
		CTriDebugOverlay()
		 :	m_debugTriOverlayLine( NULL )
		{
		}
		void AddTriOverlayLines( const Vector &vecStart, const Vector &vecApex, const Vector &vecEnd, const AIMoveTrace_t &startTrace, const AIMoveTrace_t &endTrace, bool bPathClear );
		void ClearTriOverlayLines(void);
		void FadeTriOverlayLines(void);

		void Draw(int npcDebugOverlays);
	private:
		void AddTriOverlayLine(const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest);

		OverlayLine_t	**m_debugTriOverlayLine;
	};

	CTriDebugOverlay m_TriDebugOverlay;
};

//-----------------------------------------------------------------------------

#endif // AI_PATHFINDER_H

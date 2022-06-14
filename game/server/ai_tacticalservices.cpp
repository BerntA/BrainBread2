//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#include "cbase.h"
#include "bitstring.h"
#include "ai_tacticalservices.h"
#include "ai_basenpc.h"
#include "ai_moveprobe.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"

#ifdef BB2_USE_NAVMESH
#include "nav.h"
#include "nav_pathfind.h"
#include "nav_area.h"
#endif // BB2_USE_NAVMESH

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_find_lateral_cover("ai_find_lateral_cover", "1");
ConVar ai_find_lateral_los("ai_find_lateral_los", "1");

//-------------------------------------

void CAI_TacticalServices::Init()
{
	m_pPathfinder = GetOuter()->GetPathfinder();
	Assert(m_pPathfinder);
}

//-------------------------------------

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam, Vector *pResult)
{
	AI_PROFILE_SCOPE(CAI_TacticalServices_FindLos);

	MARK_TASK_EXPENSIVE();

#ifdef BB2_USE_NAVMESH
	// Try to find a position using navmesh first:
	if (GetOuter()->UsesNavMesh() && TheNavMesh)
	{
		const Vector pLosPos = FindLosNavArea(threatPos, threatEyePos,
			minThreatDist, maxThreatDist, eFlankType, vecFlankRefPos, flFlankParam);
		if (pLosPos != vec3_invalid)
		{
			*pResult = pLosPos;
			return true;
		}
	}
#endif

	return false;
}

//-------------------------------------

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, Vector *pResult)
{
	return FindLos(threatPos, threatEyePos, minThreatDist, maxThreatDist, blockTime, FLANKTYPE_NONE, vec3_origin, 0, pResult);
}

//-------------------------------------

bool CAI_TacticalServices::FindBackAwayPos(const Vector &vecThreat, Vector *pResult)
{
	MARK_TASK_EXPENSIVE();

	Vector vMoveAway = GetAbsOrigin() - vecThreat;
	vMoveAway.NormalizeInPlace();

	if (GetOuter()->GetNavigator()->FindVectorGoal(pResult, vMoveAway, 10 * 12, 10 * 12, true))
		return true;

#ifdef BB2_USE_NAVMESH
	if (GetOuter()->UsesNavMesh() && TheNavMesh)
	{
		const Vector pBackAwayPos = FindBackAwayNavArea(vecThreat);
		if (pBackAwayPos != vec3_invalid)
		{
			*pResult = pBackAwayPos;
			return true;
		}
	}
#endif

	if (GetOuter()->GetNavigator()->FindVectorGoal(pResult, vMoveAway, GetHullWidth() * 4, GetHullWidth() * 2, true))
		return true;

	return false;
}

//-------------------------------------

bool CAI_TacticalServices::FindCoverPos(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult)
{
	return FindCoverPos(GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist, pResult);
}

//-------------------------------------

bool CAI_TacticalServices::FindCoverPos(const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult)
{
	AI_PROFILE_SCOPE(CAI_TacticalServices_FindCoverPos);

	MARK_TASK_EXPENSIVE();

#ifdef BB2_USE_NAVMESH
	if (GetOuter()->UsesNavMesh() && TheNavMesh)
	{
		const Vector pCoverPos = FindCoverNavArea(vNearPos, vThreatPos, vThreatEyePos, flMinDist, flMaxDist);
		if (pCoverPos != vec3_invalid)
		{
			*pResult = pCoverPos;
			return true;
		}
	}
#endif

	return false;
}

//-------------------------------------
// Checks lateral cover
//-------------------------------------
bool CAI_TacticalServices::TestLateralCover(const Vector& vecCheckStart, const Vector& vecCheckEnd, float flMinDist)
{
	trace_t	tr;

	if ((vecCheckStart - vecCheckEnd).LengthSqr() > Square(flMinDist))
	{
		if (GetOuter()->IsCoverPosition(vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset()))
		{
			if (GetOuter()->IsValidCover(vecCheckEnd))
			{
				AIMoveTrace_t moveTrace;
				GetOuter()->GetMoveProbe()->MoveLimit(NAV_GROUND, GetLocalOrigin(), vecCheckEnd, MASK_NPCSOLID, NULL, &moveTrace);
				if (moveTrace.fStatus == AIMR_OK)
					return true;
			}
		}
	}

	return false;
}

//-------------------------------------
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//-------------------------------------

#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks

bool CAI_TacticalServices::FindLateralCover(const Vector &vecThreat, float flMinDist, Vector *pResult)
{
	return FindLateralCover(vecThreat, flMinDist, COVER_CHECKS * COVER_DELTA, COVER_CHECKS, pResult);
}

bool CAI_TacticalServices::FindLateralCover(const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult)
{
	return FindLateralCover(GetAbsOrigin(), vecThreat, flMinDist, distToCheck, numChecksPerDir, pResult);
}

bool CAI_TacticalServices::FindLateralCover(const Vector &vNearPos, const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult)
{
	AI_PROFILE_SCOPE(CAI_TacticalServices_FindLateralCover);

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	int		i;

	if (TestLateralCover(vecThreat, vNearPos, flMinDist))
	{
		*pResult = GetLocalOrigin();
		return true;
	}

	if (!ai_find_lateral_cover.GetBool())
	{
		// Force the NPC to use the nodegraph to find cover. NOTE: We let the above code run
		// to detect the case where the NPC may already be standing in cover, but we don't 
		// make any additional lateral checks.
		return false;
	}

	Vector right = vecThreat - vNearPos;
	float temp;

	right.z = 0;
	VectorNormalize(right);
	temp = right.x;
	right.x = -right.y;
	right.y = temp;

	vecStepRight = right * (distToCheck / (float)numChecksPerDir);
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = vNearPos;
	vecCheckStart = vecThreat;

	for (i = 0; i < numChecksPerDir; i++)
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralCover(vecCheckStart, vecLeftTest, flMinDist))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralCover(vecCheckStart, vecRightTest, flMinDist))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}

#ifdef BB2_USE_NAVMESH
const Vector CAI_TacticalServices::FindLosNavArea(const Vector& vThreatPos, const Vector& vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, FlankType_t eFlankType, const Vector& vecFlankRefPos, float flFlankParam)
{
	if (!TheNavMesh->IsLoaded())
		return vec3_invalid;

	AI_PROFILE_SCOPE(CAI_TacticalServices_FindLosNavArea);

	MARK_TASK_EXPENSIVE();

	CNavArea* pArea = TheNavMesh->GetNearestNavArea(GetOuter());
	if (pArea == NULL)
		return vec3_invalid;

	CUtlVector<CNavArea*> pNearbyAreas;
	CollectSurroundingAreas(&pNearbyAreas, pArea, (flMaxThreatDist * 1.5f));
	for (int i = 0; i < pNearbyAreas.Count(); i++)
	{
		CNavArea* pCurrArea = pNearbyAreas[i];
		if (!pCurrArea)
			continue;

		bool skip = false;
		Vector navOrigin = pCurrArea->GetCenter();

		// See if the node satisfies the flanking criteria.
		switch (eFlankType)
		{

		case FLANKTYPE_NONE:
			navOrigin = pCurrArea->GetRandomPoint(vThreatPos, (flMinThreatDist + 1.0f), (flMaxThreatDist - 1.0f));
			break;

		case FLANKTYPE_RADIUS:
		{
			navOrigin = pCurrArea->GetRandomPoint(vecFlankRefPos, flFlankParam, flMaxThreatDist);
			if (navOrigin == vec3_invalid)
				skip = true;

			break;
		}

		case FLANKTYPE_ARC:
		{
			Vector vecEnemyToRef = vecFlankRefPos - vThreatPos;
			VectorNormalize(vecEnemyToRef);

			Vector vecEnemyToNode = navOrigin - vThreatPos;
			VectorNormalize(vecEnemyToNode);

			float flDot = DotProduct(vecEnemyToRef, vecEnemyToNode);

			if (RAD2DEG(acos(flDot)) < flFlankParam)
				skip = true;

			break;
		}

		}

		if (skip)
			continue;

		// Now check its distance and only accept if in range
		float flThreatDist = (navOrigin - vThreatPos).Length();
		if (flThreatDist < flMaxThreatDist &&
			flThreatDist > flMinThreatDist)
		{
			if (GetOuter()->TestShootPosition(navOrigin, vThreatEyePos))
				return navOrigin;
		}
	}

	return vec3_invalid;
}

const Vector CAI_TacticalServices::FindBackAwayNavArea(const Vector &vecFrom)
{
	if (!TheNavMesh->IsLoaded())
		return vec3_invalid;

	CNavArea *pNearestFromNPC = TheNavMesh->GetNearestNavArea(GetOuter());
	if (pNearestFromNPC == NULL)
		return vec3_invalid;

	// A vector pointing to the threat.
	Vector vecToThreat;
	vecToThreat = vecFrom - GetLocalOrigin();

	// Get my current distance from the threat
	float flCurDist = vecToThreat.Length();
	VectorNormalize(vecToThreat);

	CUtlVector<CNavArea *> pNearbyAreas;
	CollectSurroundingAreas(&pNearbyAreas, pNearestFromNPC, GetOuter()->CoverRadius());
	for (int i = 0; i < pNearbyAreas.Count(); i++)
	{
		CNavArea *pCurrArea = pNearbyAreas[i];
		if (!pCurrArea)
			continue;

		Vector navOrigin = pCurrArea->GetRandomPoint(vecFrom, (flCurDist + 1.0f), (GetOuter()->CoverRadius() * 1.5f));
		if (navOrigin == vec3_invalid)
			continue;

		// Make sure this navarea doesn't take me past the enemy's position.
		Vector vecToArea;
		vecToArea = navOrigin - GetLocalOrigin();
		VectorNormalize(vecToArea);

		if (DotProduct(vecToArea, vecToThreat) < 0.0)
			return navOrigin;
	}

	return vec3_invalid;
}

const Vector CAI_TacticalServices::FindCoverNavArea(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist)
{
	return FindCoverNavArea(GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist);
}

// We don't care about hintgroups nor ducking/cover, we just want to find a potential 'safe' spot / a spot far away from our enemy. (fallback spot)
const Vector CAI_TacticalServices::FindCoverNavArea(const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist)
{
	if (!TheNavMesh->IsLoaded())
		return vec3_invalid;

	AI_PROFILE_SCOPE(CAI_TacticalServices_FindCoverNavArea);
	MARK_TASK_EXPENSIVE();

	CNavArea *pNearestFromNPC = TheNavMesh->GetNearestNavArea(vNearPos);
	if (pNearestFromNPC == NULL)
		return vec3_invalid;

	if (!flMaxDist)
		flMaxDist = 784;

	if (flMinDist > 0.5 * flMaxDist)
		flMinDist = 0.5 * flMaxDist;

	CUtlVector<CNavArea *> pNearbyAreas;
	CollectSurroundingAreas(&pNearbyAreas, pNearestFromNPC, flMaxDist);
	for (int i = 0; i < pNearbyAreas.Count(); i++)
	{
		CNavArea *pCurrArea = pNearbyAreas[i];
		if (!pCurrArea)
			continue;

		Vector navOrigin = pCurrArea->GetRandomPoint(vNearPos, flMinDist, flMaxDist);
		if (navOrigin == vec3_invalid)
			continue;

		const HidingSpotVector *extraSpots = pCurrArea->GetHidingSpots();
		if (extraSpots && extraSpots->Count())		
			navOrigin = extraSpots->Element(random->RandomInt(0, (extraSpots->Count() - 1)))->GetPosition();		
		
		return navOrigin;
	}

	return vec3_invalid;
}
#endif // BB2_USE_NAVMESH

//-------------------------------------
// Checks lateral LOS
//-------------------------------------
bool CAI_TacticalServices::TestLateralLos(const Vector &vecCheckStart, const Vector &vecCheckEnd)
{
	trace_t	tr;

	// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
	AI_TraceLOS(vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset(), NULL, &tr);

	if (tr.fraction == 1.0)
	{
		if (GetOuter()->TestShootPosition(vecCheckEnd, vecCheckStart))
		{
			AIMoveTrace_t moveTrace;
			GetOuter()->GetMoveProbe()->MoveLimit(NAV_GROUND, GetLocalOrigin(), vecCheckEnd, MASK_NPCSOLID, NULL, &moveTrace);
			if (moveTrace.fStatus == AIMR_OK)
			{
				return true;
			}
		}
	}

	return false;
}

//-------------------------------------

bool CAI_TacticalServices::FindLateralLos(const Vector &vecThreat, Vector *pResult)
{
	AI_PROFILE_SCOPE(CAI_TacticalServices_FindLateralLos);

	if (!m_bAllowFindLateralLos)
	{
		return false;
	}

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	bool	bLookingForEnemy = GetEnemy() && VectorsAreEqual(vecThreat, GetEnemy()->EyePosition(), 0.1f);
	int		i;

	if (!bLookingForEnemy || GetOuter()->HasCondition(COND_SEE_ENEMY) || GetOuter()->HasCondition(COND_HAVE_ENEMY_LOS) ||
		GetOuter()->GetTimeScheduleStarted() == gpGlobals->curtime) // Conditions get nuked before tasks run, assume should try
	{
		// My current position might already be valid.
		if (TestLateralLos(vecThreat, GetLocalOrigin()))
		{
			*pResult = GetLocalOrigin();
			return true;
		}
	}

	if (!ai_find_lateral_los.GetBool())
	{
		// Allows us to turn off lateral LOS at the console. Allow the above code to run 
		// just in case the NPC has line of sight to begin with.
		return false;
	}

	int iChecks = COVER_CHECKS;
	int iDelta = COVER_DELTA;

	// If we're limited in how far we're allowed to move laterally, don't bother checking past it
	int iMaxLateralDelta = GetOuter()->GetMaxTacticalLateralMovement();
	if (iMaxLateralDelta != MAXTACLAT_IGNORE && iMaxLateralDelta < iDelta)
	{
		iChecks = 1;
		iDelta = iMaxLateralDelta;
	}

	Vector right;
	AngleVectors(GetLocalAngles(), NULL, &right, NULL);
	vecStepRight = right * iDelta;
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = GetLocalOrigin();
	vecCheckStart = vecThreat;

	for (i = 0; i < iChecks; i++)
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralLos(vecCheckStart, vecLeftTest))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralLos(vecCheckStart, vecRightTest))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ILAGCOMPENSATIONMANAGER_H
#define ILAGCOMPENSATIONMANAGER_H
#ifdef _WIN32
#pragma once
#endif

class CBasePlayer;
class CUserCmd;

//-----------------------------------------------------------------------------
// Purpose: This is also an IServerSystem
//-----------------------------------------------------------------------------
abstract_class ILagCompensationManager
{
public:
	virtual void TraceRealtime(
		CBaseCombatCharacter *pTracer,
		const Vector& vecAbsStart,
		const Vector& vecAbsEnd,
		const Vector &hullMin,
		const Vector &hullMax,
		ITraceFilter *pFilter,
		trace_t *ptr,
		float maxrange = MAX_TRACE_LENGTH,
		bool bRevertToHullTrace = false,
		bool bOnlyDoBoxCheck = false
		) = 0;

	virtual void TraceRealtime(
		CBaseCombatCharacter *pTracer,
		const Vector& vecAbsStart,
		const Vector& vecAbsEnd,
		const Vector &hullMin,
		const Vector &hullMax,
		const IHandleEntity *ignore,
		int collisionGroup,
		trace_t *ptr,
		float maxrange = MAX_TRACE_LENGTH,
		bool bRevertToHullTrace = false,
		bool bOnlyDoBoxCheck = false
		) = 0;

	virtual void TraceRealtime(CBaseCombatCharacter *pTracer) = 0;

#ifdef BB2_AI
	virtual void RemoveNpcData(int index) = 0;
#endif // BB2_AI
};

extern ILagCompensationManager *lagcompensation;

#endif // ILAGCOMPENSATIONMANAGER_H

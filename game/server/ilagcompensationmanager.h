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

enum LagCompensationFlags
{
	LAGCOMP_BULLET = 0x01, // Lag comping firearm attack.
	LAGCOMP_TRACE_BOX = 0x02, // Allow doing box traces, regardless.
	LAGCOMP_TRACE_REVERT_HULL = 0x04, // Revert to hull check if trace fails.
	LAGCOMP_TRACE_BOX_ONLY = 0x08, // Use simple box/intersect checks -> no fancy hitbox checks.
};

//-----------------------------------------------------------------------------
// Purpose: This is also an IServerSystem
//-----------------------------------------------------------------------------
abstract_class ILagCompensationManager
{
public:
	virtual bool IsLagCompFlagActive(int flag) = 0;

	virtual void BuildLagCompList(
		CBasePlayer* player,
		int flags
	) = 0;

	virtual void ClearLagCompList(void) = 0;

	virtual void TraceRealtime(
		CBasePlayer* player,
		const Vector& vecAbsStart,
		const Vector& vecAbsEnd,
		const Vector& hullMin,
		const Vector& hullMax,
		trace_t* ptr,
		float maxrange = MAX_TRACE_LENGTH
	) = 0;

	virtual void TraceRealtime(
		CBasePlayer* player,
		const Vector& vecAbsStart,
		const Vector& vecAbsEnd,
		const Vector& hullMin,
		const Vector& hullMax,
		trace_t* ptr,
		int flags,
		float maxrange = MAX_TRACE_LENGTH
	) = 0;

#ifdef BB2_AI
	virtual void RemoveNpcData(int index) = 0;
#endif // BB2_AI
};

extern ILagCompensationManager* lagcompensation;

#endif // ILAGCOMPENSATIONMANAGER_H

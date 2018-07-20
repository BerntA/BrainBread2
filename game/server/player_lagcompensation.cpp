//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Accurate, Stable and FAST lag compensation logic!
//
//========================================================================================//

#include "cbase.h"
#include "usercmd.h"
#include "igamesystem.h"
#include "ilagcompensationmanager.h"
#include "inetchannelinfo.h"
#include "utllinkedlist.h"
#include "BaseAnimatingOverlay.h"
#include "collisionutils.h"
#include "GameBase_Shared.h"

#ifdef BB2_AI
#include "ai_basenpc.h" 
#endif //BB2_AI

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LC_ALIVE			(1<<0)

static ConVar sv_lagcompensation_teleport_dist("sv_lagcompensation_teleport_dist", "64", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "How far a player got moved by game code before we can't lag compensate their position back");
#define HITBOX_MAX_DEVIATION 8.3f // How far can we move off the nearest hitbox and still call it a successful hit? 7-9 units is fair..

ConVar sv_unlag("sv_unlag", "1", FCVAR_DEVELOPMENTONLY, "Enables player lag compensation");
ConVar sv_maxunlag("sv_maxunlag", "1.0", FCVAR_DEVELOPMENTONLY, "Maximum lag compensation in seconds", true, 0.0f, true, 1.0f);
ConVar sv_lagflushbonecache("sv_lagflushbonecache", "1", FCVAR_DEVELOPMENTONLY, "Flushes entity bone cache on lag compensation");
ConVar sv_showlagcompensation("sv_showlagcompensation", "0", FCVAR_CHEAT, "Show lag compensated hitboxes whenever a player is lag compensated.");

static Vector g_vecPlayerStartPos = vec3_origin;

struct LagCompEntry
{
	CHandle<CBaseCombatCharacter> m_hEntity;
	Vector originalPos;
	Vector lagCompedPos;
	Vector differencePos;
	Vector boundsMin;
	Vector boundsMax;
	Vector endDirection;
	int hitgroup;

	int distanceFromCaller() const
	{
		return ((int)((lagCompedPos - g_vecPlayerStartPos).Length()));
	}

	LagCompEntry(CBaseCombatCharacter *pEntity)
	{
		Assert(pEntity != NULL);
		m_hEntity = pEntity;
		originalPos = lagCompedPos = pEntity->GetLocalOrigin();
		boundsMin = (pEntity->IsPlayer() ? pEntity->CollisionProp()->OBBMinsPreScaled() : pEntity->WorldAlignMins());
		boundsMax = (pEntity->IsPlayer() ? pEntity->CollisionProp()->OBBMaxsPreScaled() : pEntity->WorldAlignMaxs());
		differencePos = vec3_origin;
		endDirection = vec3_invalid;
		hitgroup = HITGROUP_GENERIC;
	}

	~LagCompEntry()
	{
		m_hEntity = NULL;
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

struct LagRecord
{
public:
	LagRecord()
	{
		m_fFlags = 0;
		m_vecOrigin.Init();
		m_vecMinsPreScaled.Init();
		m_vecMaxsPreScaled.Init();
		m_flSimulationTime = -1;
	}

	LagRecord(const LagRecord& src)
	{
		m_fFlags = src.m_fFlags;
		m_vecOrigin = src.m_vecOrigin;
		m_vecMinsPreScaled = src.m_vecMinsPreScaled;
		m_vecMaxsPreScaled = src.m_vecMaxsPreScaled;
		m_flSimulationTime = src.m_flSimulationTime;
	}

	// Did player die this frame
	int						m_fFlags;

	// Player position, orientation and bbox
	Vector					m_vecOrigin;
	Vector					m_vecMinsPreScaled;
	Vector					m_vecMaxsPreScaled;

	float					m_flSimulationTime;
};

ConVar sv_unlag_debug("sv_unlag_debug", "0", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLagCompensationManager : public CAutoGameSystemPerFrame, public ILagCompensationManager
{
public:
	CLagCompensationManager(char const *name) : CAutoGameSystemPerFrame(name), m_flTeleportDistanceSqr(64 * 64)
	{
	}

	// IServerSystem stuff
	virtual void Shutdown()
	{
		ClearHistory();
	}

	virtual void LevelShutdownPostEntity()
	{
		ClearHistory();
	}

	// called after entities think
	virtual void FrameUpdatePostEntityThink();

	void TraceRealtime(
		CBaseCombatCharacter *pTracer,
		const Vector& vecAbsStart,
		const Vector& vecAbsEnd,
		const Vector &hullMin,
		const Vector &hullMax,
		ITraceFilter *pFilter,
		trace_t *ptr,
		float maxrange = MAX_TRACE_LENGTH,
		bool bRevertToHullTrace = false,
		bool bOnlyDoBoxCheck = false,
		bool bFirearm = false
		);

	void TraceRealtime(
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
		bool bOnlyDoBoxCheck = false,
		bool bFirearm = false
		);

	void TraceRealtime(CBaseCombatCharacter *pTracer);

#ifdef BB2_AI	
	void RemoveNpcData(int index) // clear specific NPC's history 
	{
		CUtlFixedLinkedList< LagRecord > *track = &m_EntityTrack[index];
		track->Purge();
	}
#endif //BB2_AI

private:
	float GetSimulationTime(CBasePlayer *player);

	void DoFastBacktrack(CBaseCombatCharacter *pEntity, float flTargetTime, LagCompEntry &entry);
	void AnalyzeFastBacktracks(
		CBasePlayer *player,
		CUtlVector<LagCompEntry> &list,
		const Vector& vecAbsStart,
		const Vector& vecAbsEnd,
		const Vector &hullMin,
		const Vector &hullMax,
		ITraceFilter *pFilter,
		trace_t *ptr,
		float maxrange = MAX_TRACE_LENGTH,
		bool bRevertToHullTrace = false,
		bool bOnlyDoBoxCheck = false,
		bool bFirearm = false
		);

	Vector GetNearestHitboxPos(CBaseCombatCharacter *pEntity, const Vector &from, Vector &chestHBOXPos, int &hitgroup);
	Vector GetChestHitboxPos(CBaseCombatCharacter *pEntity);

	virtual void AutoCenterVector(const LagCompEntry &entry, Vector &result)
	{
		Vector vecTargetPos = (entry.lagCompedPos + Vector(0, 0, (0.55f * ((entry.boundsMax - entry.boundsMin).z))));
		result = ((vecTargetPos - g_vecPlayerStartPos) + entry.differencePos);
	}

	void ClearHistory()
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
			m_PlayerTrack[i].Purge();

#ifdef BB2_AI
		for (int j = 0; j < MAX_AIS; j++)
			m_EntityTrack[j].Purge();
#endif //BB2_AI
	}

	// keep a list of lag records for each player
	CUtlFixedLinkedList< LagRecord > m_PlayerTrack[MAX_PLAYERS];
#ifdef BB2_AI
	CUtlFixedLinkedList< LagRecord > m_EntityTrack[MAX_AIS];
#endif //BB2_AI

	float m_flTeleportDistanceSqr;
};

static CLagCompensationManager g_LagCompensationManager("CLagCompensationManager");
ILagCompensationManager *lagcompensation = &g_LagCompensationManager;

//-----------------------------------------------------------------------------
// Purpose: Called once per frame after all entities have had a chance to think
//-----------------------------------------------------------------------------
void CLagCompensationManager::FrameUpdatePostEntityThink()
{
	if ((gpGlobals->maxClients <= 1) || !sv_unlag.GetBool())
	{
		ClearHistory();
		return;
	}

	m_flTeleportDistanceSqr = sv_lagcompensation_teleport_dist.GetFloat() * sv_lagcompensation_teleport_dist.GetFloat();

	VPROF_BUDGET("FrameUpdatePostEntityThink", "CLagCompensationManager");

	// remove all records before that time:
	int flDeadtime = gpGlobals->curtime - sv_maxunlag.GetFloat();

	// Iterate all active players
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);

		CUtlFixedLinkedList< LagRecord > *track = &m_PlayerTrack[i - 1];

		if (!pPlayer)
		{
			if (track->Count() > 0)
				track->RemoveAll();

			continue;
		}

		Assert(track->Count() < 1000); // insanity check

		// remove tail records that are too old
		int tailIndex = track->Tail();
		while (track->IsValidIndex(tailIndex))
		{
			LagRecord &tail = track->Element(tailIndex);

			// if tail is within limits, stop
			if (tail.m_flSimulationTime >= flDeadtime)
				break;

			// remove tail, get new tail
			track->Remove(tailIndex);
			tailIndex = track->Tail();
		}

		// check if head has same simulation time
		if (track->Count() > 0)
		{
			LagRecord &head = track->Element(track->Head());

			// check if player changed simulation time since last time updated
			if (head.m_flSimulationTime >= pPlayer->GetSimulationTime())
				continue; // don't add new entry for same or older time
		}

		// add new record to player track
		LagRecord &record = track->Element(track->AddToHead());

		record.m_fFlags = 0;
		if (pPlayer->IsAlive())
			record.m_fFlags |= LC_ALIVE;

		record.m_flSimulationTime = pPlayer->GetSimulationTime();
		record.m_vecOrigin = pPlayer->GetLocalOrigin();
		record.m_vecMinsPreScaled = pPlayer->CollisionProp()->OBBMinsPreScaled();
		record.m_vecMaxsPreScaled = pPlayer->CollisionProp()->OBBMaxsPreScaled();
	}

#ifdef BB2_AI
	// Iterate all active NPCs
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	for (int i = 0; i < nAIs; i++)
	{
		CAI_BaseNPC *pNPC = ppAIs[i];
		if (!pNPC)
			continue;

		CUtlFixedLinkedList< LagRecord > *track = &m_EntityTrack[i];

		Assert(track->Count() < 1000); // insanity check

		// remove tail records that are too old
		int tailIndex = track->Tail();
		while (track->IsValidIndex(tailIndex))
		{
			LagRecord &tail = track->Element(tailIndex);

			// if tail is within limits, stop
			if (tail.m_flSimulationTime >= flDeadtime)
				break;

			// remove tail, get new tail
			track->Remove(tailIndex);
			tailIndex = track->Tail();
		}

		// check if head has same simulation time
		if (track->Count() > 0)
		{
			LagRecord &head = track->Element(track->Head());

			// check if entity changed simulation time since last time updated
			if (head.m_flSimulationTime >= pNPC->GetSimulationTime())
				continue; // don't add new entry for same or older time

			// Simulation Time is set when an entity moves or rotates ...
			// this error occurs when whatever entity it is that breaks it moves or rotates then, presumably?
		}

		// add new record to track
		LagRecord &record = track->Element(track->AddToHead());

		record.m_fFlags = 0;
		if (pNPC->IsAlive())
			record.m_fFlags |= LC_ALIVE;

		record.m_flSimulationTime = pNPC->GetSimulationTime();
		record.m_vecOrigin = pNPC->GetLocalOrigin();
		record.m_vecMaxsPreScaled = pNPC->WorldAlignMaxs();
		record.m_vecMinsPreScaled = pNPC->WorldAlignMins();
	}
#endif //BB2_AI
}

void CLagCompensationManager::TraceRealtime(
	CBaseCombatCharacter *pTracer,
	const Vector& vecAbsStart,
	const Vector& vecAbsEnd,
	const Vector &hullMin,
	const Vector &hullMax,
	ITraceFilter *pFilter,
	trace_t *ptr,
	float maxrange,
	bool bRevertToHullTrace,
	bool bOnlyDoBoxCheck,
	bool bFirearm)
{
	CHL2MP_Player *player = ToHL2MPPlayer(pTracer);
	if (player)
		player->SetLagCompVecPos(vec3_invalid);

	// Only players can make use of the lag comp features!
	if ((gpGlobals->maxClients <= 1) ||
		!sv_unlag.GetBool() ||
		(player == NULL) ||
		(player->GetTeamNumber() <= TEAM_SPECTATOR) ||
		player->IsObserver() ||
		!player->IsAlive() ||
		player->IsBot() ||
		(player->GetCurrentCommand() == NULL))
	{
		if (ptr && pFilter)
		{
			AI_TraceLine(vecAbsStart, vecAbsEnd, MASK_SHOT, pFilter, ptr);
			if (bRevertToHullTrace && (ptr->fraction == 1.0f))
				AI_TraceHull(vecAbsStart, vecAbsEnd, hullMin, hullMax, MASK_SHOT_HULL, pFilter, ptr);
		}

		return;
	}

	CUserCmd *cmd = player->GetCurrentCommand();
	g_vecPlayerStartPos = vecAbsStart;

	VPROF_BUDGET("TraceRealtime", "CLagCompensationManager");

	float tick = GetSimulationTime(player);
	CUtlVector<LagCompEntry> potentialEntries;

	trace_t trVerify;
	CTraceFilterNoNPCsOrPlayer trFilter(player, player->GetCollisionGroup());

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || (player->entindex() == pPlayer->entindex()))
			continue;

		if (!pPlayer->IsAlive() || (pPlayer->GetTeamNumber() <= TEAM_SPECTATOR) || pPlayer->IsObserver() ||
			!player->WantsLagCompensationOnEntity(pPlayer, cmd))
			continue;

		LagCompEntry item(pPlayer);
		DoFastBacktrack(pPlayer, tick, item);

		// Do a simple double check if this ent can be reached:
		AI_TraceLine(vecAbsStart, item.lagCompedPos, MASK_SHOT, &trFilter, &trVerify);
		if (trVerify.DidHit())
		{
			if ((bFirearm == false) || (TryPenetrateSurface(&trVerify, &trFilter) == vec3_invalid))
				continue;
		}

		potentialEntries.AddToTail(item);
	}

#ifdef BB2_AI
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();
	for (int i = 0; i < nAIs; i++)
	{
		CAI_BaseNPC *pNPC = ppAIs[i];
		if (!pNPC)
			continue;

		// Only compensate 'valid' npcs.
		if ((pNPC->Classify() == CLASS_NONE) ||
			(pNPC->GetCollisionGroup() == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING) ||
			!pNPC->IsAlive() ||
			(pNPC->GetAIIndex() <= -1))
			continue;

		Disposition_t rel = pNPC->IRelationType(player);
		if (rel == D_LI)
		{
			// If we like this player, continue, however if we're infected we still want to lag comp enemy zombies!! Despite them considering us as allies until we turn fully.
			if (!(player->IsHuman() && pNPC->IsZombie(true)))
				continue;
		}

		if (!player->WantsLagCompensationOnEntity(pNPC, cmd))
			continue;

		LagCompEntry item(pNPC);
		DoFastBacktrack(pNPC, tick, item);

		// Do a simple double check if this ent can be reached:
		AI_TraceLine(vecAbsStart, item.lagCompedPos, MASK_SHOT, &trFilter, &trVerify);
		if (trVerify.DidHit())
		{
			if ((bFirearm == false) || (TryPenetrateSurface(&trVerify, &trFilter) == vec3_invalid))
				continue;
		}

		potentialEntries.AddToTail(item);
	}
#endif //BB2_AI

	AnalyzeFastBacktracks(player, potentialEntries, vecAbsStart, vecAbsEnd, hullMin, hullMax, pFilter, ptr, maxrange, bRevertToHullTrace, bOnlyDoBoxCheck, bFirearm);

	potentialEntries.Purge();

	// We were unable to hit anything, revert to default:
	if (ptr && pFilter && ptr->m_pEnt == NULL)
	{
		AI_TraceLine(vecAbsStart, vecAbsEnd, MASK_SHOT, pFilter, ptr);
		if (bRevertToHullTrace && (ptr->fraction == 1.0f))
			AI_TraceHull(vecAbsStart, vecAbsEnd, hullMin, hullMax, MASK_SHOT_HULL, pFilter, ptr);
	}
}

void CLagCompensationManager::TraceRealtime(
	CBaseCombatCharacter *pTracer,
	const Vector& vecAbsStart,
	const Vector& vecAbsEnd,
	const Vector &hullMin,
	const Vector &hullMax,
	const IHandleEntity *ignore,
	int collisionGroup,
	trace_t *ptr,
	float maxrange,
	bool bRevertToHullTrace,
	bool bOnlyDoBoxCheck,
	bool bFirearm)
{
	CTraceFilterSimple filter(ignore, collisionGroup);
	TraceRealtime(pTracer,
		vecAbsStart, vecAbsEnd,
		hullMin, hullMax,
		&filter, ptr, maxrange, bRevertToHullTrace, bOnlyDoBoxCheck, bFirearm);
}

void CLagCompensationManager::TraceRealtime(CBaseCombatCharacter *pTracer)
{
	Assert(pTracer != NULL);
	Vector vecStart = pTracer->Weapon_ShootPosition();
	Vector vecForward;
	AngleVectors(pTracer->EyeAngles(), &vecForward);
	VectorNormalize(vecForward);

	TraceRealtime(
		pTracer,
		vecStart,
		(vecStart + vecForward * MAX_TRACE_LENGTH),
		-Vector(3, 3, 3),
		Vector(3, 3, 3),
		NULL,
		NULL,
		MAX_TRACE_LENGTH,
		false,
		false,
		true);
}

float CLagCompensationManager::GetSimulationTime(CBasePlayer *player)
{
	Assert(player != NULL);
	CUserCmd *cmd = player->GetCurrentCommand();
	Assert(cmd != NULL);

	// Get true latency
	// correct is the amout of time we have to correct game time
	float correct = 0.0f;

	INetChannelInfo *nci = engine->GetPlayerNetInfo(player->entindex());
	if (nci)
	{
		// add network latency
		correct += nci->GetLatency(FLOW_OUTGOING);
	}

	// calc number of view interpolation ticks - 1
	int lerpTicks = TIME_TO_TICKS(player->m_fLerpTime);

	// add view interpolation latency see C_BaseEntity::GetInterpolationAmount()
	correct += TICKS_TO_TIME(lerpTicks);

	// check bouns [0,sv_maxunlag]
	correct = clamp(correct, 0.0f, sv_maxunlag.GetFloat());

	// correct tick send by player 
	int targettick = cmd->tick_count - lerpTicks;

	// calc difference between tick send by player and our latency based tick
	float deltaTime = correct - TICKS_TO_TIME(gpGlobals->tickcount - targettick);

	if (fabs(deltaTime) > 0.2f)
	{
		// difference between cmd time and latency is too big > 200ms, use time correction based on latency
		targettick = gpGlobals->tickcount - TIME_TO_TICKS(correct);
	}

	return (TICKS_TO_TIME(targettick));
}

void CLagCompensationManager::DoFastBacktrack(CBaseCombatCharacter *pEntity, float flTargetTime, LagCompEntry &entry)
{
	VPROF_BUDGET("DoFastBacktrack", "CLagCompensationManager");

	Vector org, mins, maxs;

	// get track history of this entity
	int index = pEntity->entindex() - 1; // PLR indx.
	if (pEntity->IsNPC() && pEntity->MyNPCPointer()) // NPC? Use npc indx.
		index = pEntity->MyNPCPointer()->GetAIIndex();

	CUtlFixedLinkedList< LagRecord > *track = NULL;

	if (pEntity->IsPlayer())
		track = &m_PlayerTrack[index];
	else
		track = &m_EntityTrack[index];

	// check if we have at leat one entry
	if (!track || track->Count() <= 0)	
		return;	

	int curr = track->Head();

	LagRecord *prevRecord = NULL;
	LagRecord *record = NULL;

	Vector prevOrg = pEntity->GetLocalOrigin();

	// Walk context looking for any invalidating event
	while (track->IsValidIndex(curr))
	{
		// remember last record
		prevRecord = record;

		// get next record
		record = &track->Element(curr);

		if (!(record->m_fFlags & LC_ALIVE))
		{
			// entity must be alive, lost track
			return;
		}

		Vector delta = record->m_vecOrigin - prevOrg;
		if (delta.Length2DSqr() > m_flTeleportDistanceSqr)
		{
			// lost track, moved too far (may have teleported)
			return;
		}

		// did we find a context smaller than target time ?
		if (record->m_flSimulationTime <= flTargetTime)
			break; // hurra, stop

		prevOrg = record->m_vecOrigin;

		// go one step back in time
		curr = track->Next(curr);
	}

	Assert(record);

	if (!record)
	{
		if (sv_unlag_debug.GetBool())
			DevMsg("No valid positions in history for BacktrackEntity ( %s )\n", pEntity->GetClassname());

		return; // that should never happen
	}

	float frac = 0.0f;
	if (prevRecord &&
		(record->m_flSimulationTime < flTargetTime) &&
		(record->m_flSimulationTime < prevRecord->m_flSimulationTime))
	{
		// we didn't find the exact time but have a valid previous record
		// so interpolate between these two records;

		Assert(prevRecord->m_flSimulationTime > record->m_flSimulationTime);
		Assert(flTargetTime < prevRecord->m_flSimulationTime);

		// calc fraction between both records
		frac = (flTargetTime - record->m_flSimulationTime) /
			(prevRecord->m_flSimulationTime - record->m_flSimulationTime);

		Assert(frac > 0 && frac < 1); // should never extrapolate

		org = Lerp(frac, record->m_vecOrigin, prevRecord->m_vecOrigin);
		mins = Lerp(frac, record->m_vecMinsPreScaled, prevRecord->m_vecMinsPreScaled);
		maxs = Lerp(frac, record->m_vecMaxsPreScaled, prevRecord->m_vecMaxsPreScaled);
	}
	else
	{
		// we found the exact record or no other record to interpolate with
		// just copy these values since they are the best we have
		org = record->m_vecOrigin;
		mins = record->m_vecMinsPreScaled;
		maxs = record->m_vecMaxsPreScaled;
	}

	entry.lagCompedPos = org;
	entry.differencePos = pEntity->GetLocalOrigin() - org;
	entry.boundsMin = mins;
	entry.boundsMax = maxs;
}

int __cdecl SortLagCompEntriesPredicate(const LagCompEntry *data1, const LagCompEntry *data2)
{
	int dist1 = data1->distanceFromCaller();
	int dist2 = data2->distanceFromCaller();

	if (dist1 == dist2)
		return 0;

	if (dist1 < dist2)
		return -1;

	return 1;
}

void CLagCompensationManager::AnalyzeFastBacktracks(
	CBasePlayer *player,
	CUtlVector<LagCompEntry> &list,
	const Vector& vecAbsStart,
	const Vector& vecAbsEnd,
	const Vector &hullMin,
	const Vector &hullMax,
	ITraceFilter *pFilter,
	trace_t *ptr,
	float maxrange,
	bool bRevertToHullTrace,
	bool bOnlyDoBoxCheck,
	bool bFirearm)
{
	VPROF_BUDGET("AnalyzeFastBacktracks", "CLagCompensationManager");

	if (ptr)
		ptr->m_pEnt = NULL;

	if (list.Count() <= 0)
	{
		if (sv_unlag_debug.GetBool())
			DevMsg("No available entries in fast backtrack list!\n");

		return;
	}

	Vector vecRight, vecUp;
	AngleVectors(player->EyeAngles(), NULL, &vecRight, &vecUp);

	Vector vecForward = (vecAbsEnd - vecAbsStart);
	Vector vecStart = vecAbsStart;

	VectorNormalize(vecForward);
	VectorNormalize(vecRight);
	VectorNormalize(vecUp);

	Vector vecMeleeBoundsTemp = (vecForward * (abs(maxrange) * 0.5f) + vecRight * 20.0f + vecUp * 8.0f), vecMeleeBounds = vec3_origin;
	VectorMax(-vecMeleeBoundsTemp, vecMeleeBoundsTemp, vecMeleeBounds);

	CBaseCombatWeapon *pActiveWeapon = player->GetActiveWeapon();
	bool bCanUseBiggerHull = pActiveWeapon ?
		(pActiveWeapon->m_iMeleeAttackType.Get() == MELEE_TYPE_SLASH || pActiveWeapon->m_iMeleeAttackType.Get() == MELEE_TYPE_BASH_SLASH) : false;

	int numEnts = list.Count();
	for (int i = (numEnts - 1); i >= 0; i--) // Keep valid items, remove the rest.
	{
		LagCompEntry *entry = &list[i];
		CBaseCombatCharacter *pEntity = entry->m_hEntity.Get();
		if (!pEntity)
		{
			list.Remove(i);
			continue;
		}

		Vector vecWepPos = vecStart + vecForward * (abs(maxrange) * 0.5f);

		if (bOnlyDoBoxCheck)
		{
			if (IsBoxIntersectingBox(
				entry->lagCompedPos + entry->boundsMin,
				entry->lagCompedPos + entry->boundsMax,
				vecWepPos - vecMeleeBounds,
				vecWepPos + vecMeleeBounds) || 
				IsBoxIntersectingBox(
				entry->originalPos + entry->boundsMin,
				entry->originalPos + entry->boundsMax,
				vecWepPos - vecMeleeBounds,
				vecWepPos + vecMeleeBounds))
			{
				if (sv_lagflushbonecache.GetBool())
					pEntity->InvalidateBoneCache();

				if (sv_showlagcompensation.GetInt() >= 1)
					pEntity->DrawServerHitboxes((entry->lagCompedPos - entry->originalPos), 2.0f, true);

				AutoCenterVector(*entry, entry->endDirection);
				continue;
			}

			list.Remove(i);
			continue;
		}

		CBaseTrace tr;
		Vector vecCurrentForward = vecForward * abs(maxrange);
		IntersectRayWithBox(vecStart, vecCurrentForward, entry->lagCompedPos + entry->boundsMin, entry->lagCompedPos + entry->boundsMax, 0.0f, &tr);
		if (tr.fraction < 1.0f)
		{
			if (sv_lagflushbonecache.GetBool())
				pEntity->InvalidateBoneCache();

			if (sv_showlagcompensation.GetInt() >= 1)
				pEntity->DrawServerHitboxes((entry->lagCompedPos - entry->originalPos), 2.0f, true);

			Vector chestHitboxPos = vec3_invalid;
			Vector vecNearestPosToHitbox = GetNearestHitboxPos(pEntity, (tr.endpos + entry->differencePos), chestHitboxPos, entry->hitgroup);
			if (vecNearestPosToHitbox != vec3_invalid)
				entry->endDirection = vecNearestPosToHitbox - tr.startpos;
			else
			{
				entry->endDirection = (tr.endpos - tr.startpos) + entry->differencePos;
				if (bCanUseBiggerHull)
				{
					if (chestHitboxPos != vec3_invalid)
					{
						entry->endDirection = (chestHitboxPos - tr.startpos);
						entry->hitgroup = HITGROUP_CHEST;
					}
					else
						AutoCenterVector(*entry, entry->endDirection);
				}
			}

			continue;
		}
		else if (bCanUseBiggerHull &&
			(IsBoxIntersectingBox(
			entry->lagCompedPos + entry->boundsMin,
			entry->lagCompedPos + entry->boundsMax,
			vecWepPos - vecMeleeBounds,
			vecWepPos + vecMeleeBounds) || IsBoxIntersectingBox(
			entry->originalPos + entry->boundsMin,
			entry->originalPos + entry->boundsMax,
			vecWepPos - vecMeleeBounds,
			vecWepPos + vecMeleeBounds)))
		{
			if (sv_lagflushbonecache.GetBool())
				pEntity->InvalidateBoneCache();

			if (sv_showlagcompensation.GetInt() >= 1)
				pEntity->DrawServerHitboxes((entry->lagCompedPos - entry->originalPos), 2.0f, true);

			Vector chestHitboxPos = GetChestHitboxPos(pEntity);
			if (chestHitboxPos != vec3_invalid)
			{
				entry->endDirection = chestHitboxPos - vecStart;
				entry->hitgroup = HITGROUP_CHEST;
			}
			else
				AutoCenterVector(*entry, entry->endDirection);			

			continue;
		}

		list.Remove(i);
	}

	// Sort the list if possible. From near to far, focus on the nearest ents first!
	if (list.Count() > 1)
		list.Sort(SortLagCompEntriesPredicate);

	trace_t trace;
	CTraceFilterRealtime filter(player, player->GetCollisionGroup(), pActiveWeapon);

	numEnts = list.Count();
	for (int i = 0; i < numEnts; i++)
	{
		if ((i + 1) >= BB2_LAGCOMP_HIT_MAX)
			break;

		LagCompEntry *entry = &list[i];
		CBaseCombatCharacter *pEntity = entry->m_hEntity.Get();
		if (!pEntity)
			continue;

		Vector vecEndPos = (vecStart + entry->endDirection);
		VectorNormalize(entry->endDirection);

		// Draw hitboxes + hit pos.
		if (sv_showlagcompensation.GetInt() >= 1)
		{
			NDebugOverlay::BoxDirection(vecEndPos, hullMin, hullMax, entry->endDirection, 100, 255, 255, 20, 4.0f);
			pEntity->DrawServerHitboxes(4.0f, true);
		}

		if (bFirearm)
		{
			player->SetLagCompVecPos((vecEndPos - vecAbsStart));
			break;
		}

		// Check if this pos is invalid:
		if (ptr == NULL)
			UTIL_TraceLine(vecStart, vecEndPos, MASK_SHOT, &filter, &trace);
		else
			UTIL_TraceHull(vecStart, vecEndPos, hullMin, hullMax, MASK_SHOT_HULL, &filter, &trace);

		if (trace.DidHit() && (trace.m_pEnt != pEntity))
			continue;

		// Force a 'hit':
		if (ptr)
		{
			ptr->m_pEnt = (CBaseEntity*)pEntity;
			ptr->fraction = 0.0f;
			ptr->allsolid = ptr->startsolid = false;
			ptr->hitgroup = entry->hitgroup;
			ptr->endpos = vecEndPos;
			ptr->startpos = vecAbsStart;
		}

		player->SetLagCompVecPos((vecEndPos - vecAbsStart));
		break;
	}
}

Vector CLagCompensationManager::GetNearestHitboxPos(CBaseCombatCharacter *pEntity, const Vector &from, Vector &chestHBOXPos, int &hitgroup)
{
	if (!pEntity)
		return vec3_invalid;

	CStudioHdr *pStudioHdr = pEntity->GetModelPtr();
	if (!pStudioHdr)
		return vec3_invalid;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(pEntity->m_nHitboxSet);
	if (!set)
		return vec3_invalid;

	Vector vecNearest = vec3_invalid;
	float nearestDist = FLT_MAX;

	Vector tempPos;
	QAngle tempAngles;

	mstudiobbox_t *pTargetHBOX = NULL;
	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		if (!pbox)
			continue;

		pEntity->GetBonePosition(pbox->bone, tempPos, tempAngles);

		if (pbox->group == HITGROUP_CHEST)
			chestHBOXPos = tempPos;

		float distFromIntersectionPnt = (tempPos - from).Length();
		if (distFromIntersectionPnt < nearestDist)
		{
			nearestDist = distFromIntersectionPnt;
			vecNearest = tempPos;
			pTargetHBOX = pbox;
		}
	}

	if (pTargetHBOX == NULL)
		return vec3_invalid;

	Vector newBounds = (0.5f * (pTargetHBOX->bbmax - pTargetHBOX->bbmin)) + HITBOX_MAX_DEVIATION * Vector(1, 1, 1);
	Vector newMins = -1.0f * newBounds;
	Vector newMaxs = newBounds;
	if (!IsPointInBox(from, vecNearest + newMins, vecNearest + newMaxs))
		return vec3_invalid;

	hitgroup = pTargetHBOX->group;
	return vecNearest;
}

Vector CLagCompensationManager::GetChestHitboxPos(CBaseCombatCharacter *pEntity)
{
	if (!pEntity)
		return vec3_invalid;

	CStudioHdr *pStudioHdr = pEntity->GetModelPtr();
	if (!pStudioHdr)
		return vec3_invalid;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(pEntity->m_nHitboxSet);
	if (!set)
		return vec3_invalid;

	Vector tempPos;
	QAngle tempAngles;

	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		if (!pbox)
			continue;

		if (pbox->group != HITGROUP_CHEST)
			continue;

		pEntity->GetBonePosition(pbox->bone, tempPos, tempAngles);
		return tempPos;
	}

	return vec3_invalid;
}
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

#define LC_NONE				0
#define LC_ALIVE			(1<<0)
#define LC_ORIGIN_CHANGED	(1<<8)
#define LC_SIZE_CHANGED		(1<<9)

static ConVar sv_lagcompensation_teleport_dist("sv_lagcompensation_teleport_dist", "64", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "How far a player got moved by game code before we can't lag compensate their position back");
#define LAG_COMPENSATION_EPS_SQR ( 0.1f * 0.1f )
#define HITBOX_MAX_DEVIATION 8.3f // How far can we move off the nearest hitbox and still call it a successful hit? 7-9 units is fair..
#define MELEE_BBOX_MAX_DEVIATION 12.0f

ConVar sv_unlag("sv_unlag", "1", FCVAR_DEVELOPMENTONLY, "Enables player lag compensation");
ConVar sv_maxunlag("sv_maxunlag", "1.0", FCVAR_DEVELOPMENTONLY, "Maximum lag compensation in seconds", true, 0.0f, true, 1.0f);
ConVar sv_lagflushbonecache("sv_lagflushbonecache", "1", FCVAR_DEVELOPMENTONLY, "Flushes entity bone cache on lag compensation");
ConVar sv_showlagcompensation("sv_showlagcompensation", "0", FCVAR_CHEAT, "Show lag compensated hitboxes whenever a player is lag compensated.");

struct LagCompEntry
{
	CHandle<CBaseCombatCharacter> m_hEntity;
	Vector originalPos;
	Vector lagCompedPos;
	Vector differencePos;
	Vector boundsMin;
	Vector boundsMax;
	Vector callerPos;
	Vector endDirection;

	int distanceFromCaller() const
	{
		return ((int)((lagCompedPos - callerPos).Length()));
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

//
// Try to take the player from his current origin to vWantedPos.
// If it can't get there, leave the player where he is.
// 

ConVar sv_unlag_debug("sv_unlag_debug", "0", FCVAR_GAMEDLL | FCVAR_DEVELOPMENTONLY);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLagCompensationManager : public CAutoGameSystemPerFrame, public ILagCompensationManager
{
public:
	CLagCompensationManager(char const *name) : CAutoGameSystemPerFrame(name), m_flTeleportDistanceSqr(64 * 64)
	{
		m_isCurrentlyDoingCompensation = false;
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

	void StartLagCompensation(CBasePlayer *player, CUserCmd *cmd, float maxrange = MAX_TRACE_LENGTH);
	void FinishLagCompensation(CBasePlayer *player);

#ifdef BB2_AI	
	void RemoveNpcData(int index) // clear specific NPC's history 
	{
		CUtlFixedLinkedList< LagRecord > *track = &m_EntityTrack[index];
		track->Purge();
	}
#endif //BB2_AI

	bool IsCurrentlyDoingLagCompensation() const OVERRIDE{ return m_isCurrentlyDoingCompensation; }

private:
	void DoFastBacktrack(CBasePlayer *player, CBaseCombatCharacter *pEntity, float flTargetTime, CUtlVector<LagCompEntry> &list);
	void AnalyzeFastBacktracks(CBasePlayer *player, CUtlVector<LagCompEntry> &list, float maxrange);
	Vector GetNearestHitboxPos(CBaseCombatCharacter *pEntity, const Vector &from, Vector &chestHBOXPos);
	Vector GetChestHitboxPos(CBaseCombatCharacter *pEntity);

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

	CBasePlayer *m_pCurrentPlayer;	// The player we are doing lag compensation for
	float m_flTeleportDistanceSqr;
	bool m_isCurrentlyDoingCompensation;
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
		{
			record.m_fFlags |= LC_ALIVE;
		}

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
		{
			record.m_fFlags |= LC_ALIVE;
		}

		record.m_flSimulationTime = pNPC->GetSimulationTime();
		record.m_vecOrigin = pNPC->GetLocalOrigin();
		record.m_vecMaxsPreScaled = pNPC->WorldAlignMaxs();
		record.m_vecMinsPreScaled = pNPC->WorldAlignMins();
	}
#endif //BB2_AI

	//Clear the current player.
	m_pCurrentPlayer = NULL;
	m_isCurrentlyDoingCompensation = false;
}

// Called during player movement to set up/restore after lag compensation
void CLagCompensationManager::StartLagCompensation(CBasePlayer *player, CUserCmd *cmd, float maxrange)
{
	if (!player || !cmd)
		return;

	// DONT LAG COMP AGAIN THIS FRAME IF THERES ALREADY ONE IN PROGRESS
	// IF YOU'RE HITTING THIS THEN IT MEANS THERES A CODE BUG
	if (m_pCurrentPlayer || m_isCurrentlyDoingCompensation)
	{
		Warning("Trying to start a new lag compensation session while one is already active!\n");
		return;
	}

	if (!player->m_bLagCompensation		// Player not wanting lag compensation
		|| (gpGlobals->maxClients <= 1)	// no lag compensation in single player
		|| !sv_unlag.GetBool()				// disabled by server admin
		|| player->IsBot() 				// not for bots
		|| player->IsObserver()			// not for spectators
		)
		return;

	m_pCurrentPlayer = player;
	m_isCurrentlyDoingCompensation = true;

	// NOTE: Put this here so that it won't show up in single player mode.
	VPROF_BUDGET("StartLagCompensation", VPROF_BUDGETGROUP_OTHER_NETWORKING);

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
		// DevMsg("StartLagCompensation: delta too big (%.3f)\n", deltaTime );
		targettick = gpGlobals->tickcount - TIME_TO_TICKS(correct);
	}

	CUtlVector<LagCompEntry> potentialEntries;
	const CBitVec<MAX_EDICTS> *pEntityTransmitBits = engine->GetEntityTransmitBitsForClient(player->entindex() - 1);

	// Iterate all active players
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || player == pPlayer)
			continue;

		if (!pPlayer->IsAlive() || (pPlayer->GetTeamNumber() <= TEAM_SPECTATOR))
			continue;

		// Custom checks for if things should lag compensate (based on things like what team the player is on).
		if (!player->WantsLagCompensationOnEntity(pPlayer, cmd, pEntityTransmitBits))
			continue;

		// Move other player back in time
		DoFastBacktrack(player, pPlayer, TICKS_TO_TIME(targettick), potentialEntries);
	}

#ifdef BB2_AI
	// also iterate all monsters 
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();
	for (int i = 0; i < nAIs; i++)
	{
		CAI_BaseNPC *pNPC = ppAIs[i];
		if (!pNPC)
			continue;

		// Only compensate 'valid' npcs.
		if ((pNPC->GetSleepState() != AISS_AWAKE) ||
			(pNPC->Classify() == CLASS_NONE) ||
			(pNPC->GetCollisionGroup() == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING) ||
			!pNPC->IsAlive() ||
			pNPC->IsStaticNPC() ||
			(pNPC->GetAIIndex() <= -1))
			continue;

		Disposition_t rel = pNPC->IRelationType(player);
		if (rel == D_LI)
		{
			// If we like this player, continue, however if we're infected we still want to lag comp enemy zombies!! Despite them considering us as allies until we turn fully.
			if (!(player->IsHuman() && pNPC->IsZombie(true)))
				continue;
		}

		if (!player->WantsLagCompensationOnEntity(pNPC, cmd, pEntityTransmitBits))
			continue;

		// Move NPC back in time 
		DoFastBacktrack(player, pNPC, TICKS_TO_TIME(targettick), potentialEntries);
	}
#endif //BB2_AI

	AnalyzeFastBacktracks(player, potentialEntries, maxrange);

	potentialEntries.Purge();
}

void CLagCompensationManager::DoFastBacktrack(CBasePlayer *player, CBaseCombatCharacter *pEntity, float flTargetTime, CUtlVector<LagCompEntry> &list)
{
	Vector org, mins, maxs;

	VPROF_BUDGET("DoFastBacktrack", "CLagCompensationManager");

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

	// See if this represents a change for the entity
	int flags = 0;

	Vector orgdiff = pEntity->GetLocalOrigin() - org;

	if (pEntity->IsPlayer())
	{
		if (mins != pEntity->CollisionProp()->OBBMinsPreScaled() || maxs != pEntity->CollisionProp()->OBBMaxsPreScaled())
			flags |= LC_SIZE_CHANGED;
	}
	else
	{
		if ((mins != pEntity->WorldAlignMins()) || (maxs != pEntity->WorldAlignMaxs()))
			flags |= LC_SIZE_CHANGED;
	}

	if (orgdiff.LengthSqr() > LAG_COMPENSATION_EPS_SQR)
		flags |= LC_ORIGIN_CHANGED;

	if (!flags)
		return; // we didn't change anything

	LagCompEntry item;
	item.m_hEntity = pEntity;
	item.originalPos = pEntity->GetLocalOrigin();
	item.lagCompedPos = org;
	item.differencePos = pEntity->GetLocalOrigin() - org;
	item.boundsMin = mins;
	item.boundsMax = maxs;
	item.callerPos = player->Weapon_ShootPosition();
	item.endDirection = vec3_invalid;

	list.AddToTail(item);
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

void CLagCompensationManager::AnalyzeFastBacktracks(CBasePlayer *player, CUtlVector<LagCompEntry> &list, float maxrange)
{
	VPROF_BUDGET("AnalyzeFastBacktracks", "CLagCompensationManager");

	if (!player)
		return;

	if (list.Count() <= 0)
	{
		if (sv_unlag_debug.GetBool())
			DevMsg("No available entries in fast backtrack list!\n");

		return;
	}

	Vector vecForward = player->GetAutoaimVector(1.0f);
	Vector vecStart = player->Weapon_ShootPosition();
	VectorNormalize(vecForward);

	Vector traceCheckHullMins = Vector(-3, -3, -3);
	Vector traceCheckHullMaxs = Vector(3, 3, 3);
	bool bCanUseBiggerHull = false;
	CBaseCombatWeapon *pActiveWeapon = player->GetActiveWeapon();
	if (pActiveWeapon)
	{
		int meleeAttackType = pActiveWeapon->m_iMeleeAttackType.Get();
		bCanUseBiggerHull = (meleeAttackType == MELEE_TYPE_SLASH) || (meleeAttackType == MELEE_TYPE_BASH_SLASH);
		if (meleeAttackType > 0)
		{
			traceCheckHullMins = Vector(-5, -5, -5);
			traceCheckHullMaxs = Vector(5, 5, 5);
		}
	}

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

		float rangeMax = maxrange;
		float extraDeviation = MELEE_BBOX_MAX_DEVIATION;
		if (rangeMax <= MAX_MELEE_LAGCOMP_DIST)
			rangeMax += extraDeviation;

		Vector vecCurrentForward = vecForward * rangeMax;
		Vector vecWepPos = vecStart + (vecCurrentForward * 0.5f);

		CBaseTrace tr;
		IntersectRayWithBox(vecStart, vecCurrentForward, entry->lagCompedPos + entry->boundsMin, entry->lagCompedPos + entry->boundsMax, 0.0f, &tr);
		if (tr.fraction < 1.0f)
		{
			if (sv_lagflushbonecache.GetBool())
				pEntity->InvalidateBoneCache();

			if (sv_showlagcompensation.GetInt() >= 1)
				pEntity->DrawServerHitboxes((entry->lagCompedPos - entry->originalPos), 2.0f, true);

			Vector chestHitboxPos = vec3_invalid;
			Vector vecNearestPosToHitbox = GetNearestHitboxPos(pEntity, (tr.endpos + entry->differencePos), chestHitboxPos);
			if (vecNearestPosToHitbox != vec3_invalid)
				entry->endDirection = vecNearestPosToHitbox - tr.startpos;
			else
			{
				entry->endDirection = (tr.endpos - tr.startpos) + entry->differencePos;
				if (bCanUseBiggerHull)
				{
					if (chestHitboxPos != vec3_invalid)
						entry->endDirection = (chestHitboxPos - tr.startpos);
					else
					{
						Vector vecAutoCorrector = (entry->lagCompedPos - tr.startpos);
						vecAutoCorrector.z += (0.55f * (entry->boundsMax - entry->boundsMin).z);
						entry->endDirection = vecAutoCorrector + entry->differencePos;
					}
				}
			}

			continue;
		}
		else if (bCanUseBiggerHull)
		{
			if (IsBoxIntersectingBox(
				vecWepPos + pActiveWeapon->GetMeleeBoundsMin() + Vector(-extraDeviation, -extraDeviation, 0),
				vecWepPos + pActiveWeapon->GetMeleeBoundsMax() + Vector(extraDeviation, extraDeviation, 0),
				entry->lagCompedPos + entry->boundsMin,
				entry->lagCompedPos + entry->boundsMax))
			{
				if (sv_lagflushbonecache.GetBool())
					pEntity->InvalidateBoneCache();

				if (sv_showlagcompensation.GetInt() >= 1)
					pEntity->DrawServerHitboxes((entry->lagCompedPos - entry->originalPos), 2.0f, true);

				Vector chestHitboxPos = GetChestHitboxPos(pEntity);
				if (chestHitboxPos != vec3_invalid)
					entry->endDirection = chestHitboxPos - vecStart;
				else
				{
					Vector vecAutoCorrector = (entry->lagCompedPos - vecStart);
					vecAutoCorrector.z += (0.55f * (entry->boundsMax - entry->boundsMin).z);
					entry->endDirection = vecAutoCorrector + entry->differencePos;
				}

				continue;
			}
		}

		list.Remove(i);
	}

	// Sort the list if possible. From near to far, focus on the nearest ents first!
	if (list.Count() > 1)
		list.Sort(SortLagCompEntriesPredicate);

	Vector vecWantedLagPosVec = vec3_invalid;
	trace_t trace;
	CTraceFilterOnlyNPCsAndPlayer filter(player, COLLISION_GROUP_NONE);

	numEnts = list.Count();
	for (int i = 0; i < numEnts; i++)
	{
		if (i >= BB2_LAGCOMP_HIT_MAX)
			break;

		LagCompEntry *entry = &list[i];
		CBaseCombatCharacter *pEntity = entry->m_hEntity.Get();
		if (!pEntity)
			continue;

		VectorNormalize(entry->endDirection);
		Vector vecStartPos = entry->callerPos;
		Vector vecEndPos = vecStartPos + entry->endDirection * MAX_TRACE_LENGTH;

		if (maxrange <= MAX_MELEE_LAGCOMP_DIST) // Melee will not allow 'bullet pene'. <.<
		{
			UTIL_TraceLine(vecStartPos, vecEndPos, MASK_SHOT, player, COLLISION_GROUP_NONE, &trace); // A simple trace against hitboxes.
			if ((trace.fraction == 1.0f) || (trace.m_pEnt != pEntity)) // Nothing? Try a hull.
				UTIL_TraceHull(vecStartPos, vecEndPos, traceCheckHullMins, traceCheckHullMaxs, MASK_SHOT_HULL, player, COLLISION_GROUP_NONE, &trace);
		}
		else
		{
			UTIL_TraceLine(vecStartPos, vecEndPos, MASK_SHOT, &filter, &trace); // A simple trace against hitboxes.
			if ((trace.fraction == 1.0f) || (trace.m_pEnt != pEntity)) // Nothing? Try a hull.
				UTIL_TraceHull(vecStartPos, vecEndPos, traceCheckHullMins, traceCheckHullMaxs, MASK_SHOT_HULL, &filter, &trace);
		}

		// Draw hitboxes + hit pos.
		if (sv_showlagcompensation.GetInt() >= 1)
		{
			NDebugOverlay::BoxDirection(trace.endpos, -Vector(5, 5, 5), Vector(5, 5, 5), entry->endDirection, 100, 255, 255, 20, 4.0f);
			pEntity->DrawServerHitboxes(4.0f, true);
		}

		if ((trace.fraction == 1.0f) || (trace.m_pEnt != pEntity))
			continue;

		vecWantedLagPosVec = (trace.endpos - trace.startpos);
		break;
	}

	player->SetLagCompVecPos(vecWantedLagPosVec);
}

Vector CLagCompensationManager::GetNearestHitboxPos(CBaseCombatCharacter *pEntity, const Vector &from, Vector &chestHBOXPos)
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

void CLagCompensationManager::FinishLagCompensation(CBasePlayer *player)
{
	VPROF_BUDGET_FLAGS("FinishLagCompensation", VPROF_BUDGETGROUP_OTHER_NETWORKING, BUDGETFLAG_CLIENT | BUDGETFLAG_SERVER);

	if (player && player->IsBot())
		return;

	if (player == NULL || m_isCurrentlyDoingCompensation == false || m_pCurrentPlayer == NULL || (player != m_pCurrentPlayer))
	{
		Warning("Unable to finish lag compensation!\n");
		return;
	}

	player->SetLagCompVecPos(vec3_invalid);
	m_isCurrentlyDoingCompensation = false;
	m_pCurrentPlayer = NULL;
}
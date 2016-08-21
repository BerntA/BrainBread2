//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Client Gib Class. Used for prop gibs / npc / player gibs and ragdolls.
//
//========================================================================================//

#include "cbase.h"
#include "c_client_gib.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "physpropclientside.h"
#include "vcollide_parse.h"
#include "mapentities_shared.h"
#include "gamestringpool.h"
#include "GameBase_Shared.h"
#include "GameBase_Client.h"
#include "props_shared.h"
#include "c_te_effect_dispatch.h"
#include "datacache/imdlcache.h"
#include "weapon_hl2mpbase.h"
#include "view.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar bb2_gibs_spawn_blood_puddle("bb2_gibs_spawn_blood_puddle", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Spawn blood puddles with ragdolls? Eventually this may decrease performance, if the level has a lot of puddles.", true, 0.0f, true, 1.0f);
ConVar bb2_gibs_spawn_blood("bb2_gibs_spawn_blood", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "When a gib take damage/touch stuff should it spray/spawn blood?", true, 0.0f, true, 1.0f);
ConVar bb2_gibs_max("bb2_gibs_max", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the max amount of gibs and ragdolls to be created on the client. When you reach the limit new gibs will fade out automatically.", true, 0.0f, true, 128.0f);
ConVar bb2_gibs_enable_fade("bb2_gibs_enable_fade", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Should gibs and ragdolls fade out?", true, 0.0f, true, 1.0f);
ConVar bb2_gibs_fadeout_time("bb2_gibs_fadeout_time", "4", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the time in seconds before a ragdoll or gib will fade out.", true, 0.0f, true, 30.0f);
ConVar bb2_gibs_blood_chance("bb2_gibs_blood_chance", "100", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the chance in % to spawn blood when you attack an entity.", true, 0.0f, true, 100.0f);

extern ConVar g_ragdoll_fadespeed;
extern ConVar r_propsmaxdist;

static CUtlVector<C_ClientSideGibBase*> s_ClientGibList;

bool CanSpawnClientGib(int type)
{
	if (type != CLIENT_GIB_PROP)
	{
		int iGibsInWorld = 0;
		for (int i = 0; i < s_ClientGibList.Count(); i++)
		{
			if (s_ClientGibList[i]->GetGibType() != CLIENT_GIB_PROP)
				iGibsInWorld++;
		}

		if (iGibsInWorld >= bb2_gibs_max.GetInt())
			return false;
	}

	return true;
}

bool ShouldFadeClientGib(int type)
{
	if (type != CLIENT_GIB_PROP && (!bb2_gibs_enable_fade.GetBool()))
		return false;

	return true;
}

bool RemoveAllClientGibs()
{
	if (!s_ClientGibList.Count())
		return false;

	while (s_ClientGibList.Count() > 0)
	{
		C_ClientSideGibBase *p = s_ClientGibList[0];
		p->Release();
	}

	return true;
}

#define TOUCH_SURFACE_DELAY 0.1f
#define FORCE_FADE_TIME 2.0f // If we've reached the gib limit then this is the time until we fade out the gib!
#define PROP_FADE_TIME 4.0f // Prop Gibs will fade out after 4 sec.

//========================================================================================//
//
// Purpose: Client Side GIB Base Class:
//
//========================================================================================//

C_ClientSideGibBase::C_ClientSideGibBase(void)
{
	m_iPlayerIndexLink = 0;
	m_flTouchDelta = 0;
	m_iGibType = 0;
	m_bReleaseGib = false;
	m_bFadingOut = false;
	m_bNoModelParticles = false;
	m_bForceFade = false;
	m_flFadeOutDelay = gpGlobals->curtime + PROP_FADE_TIME;

	s_ClientGibList.AddToTail(this);
}

C_ClientSideGibBase::~C_ClientSideGibBase()
{
	PhysCleanupFrictionSounds(this);
	VPhysicsDestroyObject();

	s_ClientGibList.FindAndRemove(this);
}

bool C_ClientSideGibBase::Initialize(int type)
{
	if (InitializeAsClientEntity(STRING(GetModelName()), RENDER_GROUP_OPAQUE_ENTITY) == false)
		return false;

	if (engine->IsInEditMode())
		return false;

	m_iGibType = type;
	return true;
}

bool C_ClientSideGibBase::LoadRagdoll()
{
	if (!IsClientRagdoll())
		return false;

	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;
	GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);

	return InitAsClientRagdoll(boneDelta0, boneDelta1, currentBones, boneDt);
}

void C_ClientSideGibBase::SetForceFade(bool value)
{
	m_bForceFade = value;
	if (m_bForceFade)
		m_flFadeOutDelay = gpGlobals->curtime + FORCE_FADE_TIME;
	else
	{
		m_flFadeOutDelay = gpGlobals->curtime + (m_iGibType == CLIENT_GIB_PROP ? PROP_FADE_TIME : bb2_gibs_fadeout_time.GetFloat());
	}
}

void C_ClientSideGibBase::OnBecomeRagdoll(void)
{
	if ((m_iGibType == CLIENT_RAGDOLL) && IsClientRagdoll())
	{
		if (bb2_gibs_spawn_blood_puddle.GetBool())
			GameBaseShared()->DispatchBleedout(this);
	}
}

IRagdoll* C_ClientSideGibBase::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_ClientSideGibBase::ClientThink(void)
{
	if (m_bReleaseGib == true)
	{
		Release();
		return;
	}

	// Do we want to fade out?
	if (ShouldFadeClientGib(m_iGibType) || m_bForceFade)
	{
		if (gpGlobals->curtime >= m_flFadeOutDelay)
			SUB_Remove();
	}

	FadeOut();
}

void C_ClientSideGibBase::SUB_Remove(void)
{
	m_bFadingOut = true;
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_ClientSideGibBase::Release(void)
{
	C_BaseEntity *pChild = GetEffectEntity();
	if (pChild && pChild->IsMarkedForDeletion() == false)
	{
		pChild->Release();
	}

	if (GetThinkHandle() != INVALID_THINK_HANDLE)
	{
		ClientThinkList()->RemoveThinkable(GetClientHandle());
	}
	ClientEntityList().RemoveEntity(GetClientHandle());

	partition->Remove(PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, CollisionProp()->GetPartitionHandle());
	RemoveFromLeafSystem();

	BaseClass::Release();
}

void C_ClientSideGibBase::FadeOut(void)
{
	if (m_bFadingOut == false || (!ShouldFadeClientGib(m_iGibType) && !m_bForceFade))
		return;

	int iAlpha = GetRenderColor().a;
	int iFadeSpeed = g_ragdoll_fadespeed.GetInt();

	iAlpha = MAX(iAlpha - (iFadeSpeed * gpGlobals->frametime), 0);

	SetRenderMode(kRenderTransAlpha);
	SetRenderColorA(iAlpha);

	if (iAlpha == 0)
		m_bReleaseGib = true;
}

void C_ClientSideGibBase::Spawn()
{
	BaseClass::Spawn();

	m_takedamage = DAMAGE_EVENTS_ONLY;
}

void C_ClientSideGibBase::StartTouch(C_BaseEntity *pOther)
{
	// Limit the amount of times we can bounce
	if (m_flTouchDelta < gpGlobals->curtime)
	{
		HitSurface(pOther);
		m_flTouchDelta = gpGlobals->curtime + TOUCH_SURFACE_DELAY;
	}

	BaseClass::StartTouch(pOther);
}

void C_ClientSideGibBase::HitSurface(C_BaseEntity *pOther)
{
	if (!bb2_gibs_spawn_blood.GetBool())
		return;

	if (m_iGibType != CLIENT_GIB_PROP && m_iGibType != CLIENT_RAGDOLL)
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();
		if (tr.m_pEnt)
		{
			UTIL_DecalTrace(&tr, GameBaseClient->IsExtremeGore() ? "ExtremeBlood" : "Blood");
		}
	}
}

void C_ClientSideGibBase::DoBloodSpray(trace_t *pTrace)
{
	if (!bb2_gibs_spawn_blood.GetBool())
		return;

	if (random->RandomInt(0, 100) > bb2_gibs_blood_chance.GetInt())
		return;

	if (m_iGibType > CLIENT_GIB_PROP)
	{
		DispatchParticleEffect(GameBaseShared()->GetSharedGameDetails()->GetBloodParticle(GameBaseClient->IsExtremeGore()), pTrace->endpos, this->GetAbsAngles(), Vector(0, 0, 0), Vector(0, 0, 0), false, this, PATTACH_ABSORIGIN_FOLLOW);
		trace_t tr;
		UTIL_TraceLine(pTrace->endpos + Vector(0, 0, 50), pTrace->endpos - Vector(0, 0, 200), MASK_ALL, this, COLLISION_GROUP_NONE, &tr);
		UTIL_DecalTrace(&tr, GameBaseClient->IsExtremeGore() ? "ExtremeBlood" : "Blood");
	}
}

void C_ClientSideGibBase::ImpactTrace(trace_t *pTrace, int iDamageType, const char *pCustomImpactName)
{
	VPROF("C_ClientSideGibBase::ImpactTrace");
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if (!pPhysicsObject)
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;
	int iDamage = 0;

	if (iDamageType == DMG_BLAST)
	{
		iDamage = VectorLength(dir);
		dir *= 500;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter(dir);
	}
	else
	{
		Vector hitpos;

		VectorMA(pTrace->startpos, pTrace->fraction, dir, hitpos);
		VectorNormalize(dir);

		// guess avg damage
		if (iDamageType == DMG_BULLET)
		{
			iDamage = 30;
		}
		else
		{
			iDamage = 50;
		}

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset(dir, hitpos);

		// Build the impact data
		CEffectData data;
		data.m_vOrigin = pTrace->endpos;
		data.m_vStart = pTrace->startpos;
		data.m_nSurfaceProp = pTrace->surface.surfaceProps;
		data.m_nDamageType = iDamageType;
		data.m_nHitBox = pTrace->hitbox;
		data.m_hEntity = GetRefEHandle();

		// Send it on its way
		if (!pCustomImpactName)
		{
			DispatchEffect("Impact", data);
		}
		else
		{
			DispatchEffect(pCustomImpactName, data);
		}
	}

	DoBloodSpray(pTrace);
}

//========================================================================================//
//
// Purpose: Client Ragdoll GIB:
//
//========================================================================================//

C_ClientRagdollGib::C_ClientRagdollGib(void)
{
	m_iHealth = 0;
	m_nGibFlags = 0;
	Q_strncpy(pchNPCName, "", 32);
}

C_ClientRagdollGib::~C_ClientRagdollGib()
{
}

bool C_ClientRagdollGib::Initialize(int type)
{
	bool ret = BaseClass::Initialize(type);
	if (!ret)
		return false;

	Spawn();
	return true;
}

bool C_ClientRagdollGib::LoadRagdoll()
{
	bool ret = BaseClass::LoadRagdoll();
	if (!ret)
		return false;

	IPhysicsObject *pPhysicsObj = VPhysicsGetObject();
	if (pPhysicsObj)
	{
		// We want touch calls when we hit the world
		unsigned int flags = pPhysicsObj->GetCallbackFlags();
		pPhysicsObj->SetCallbackFlags(flags | CALLBACK_GLOBAL_TOUCH_STATIC);
	}

	// start fading out at 75% of r_propsmaxdist
	m_fadeMaxDist = r_propsmaxdist.GetFloat();
	m_fadeMinDist = r_propsmaxdist.GetFloat() * 0.75f;

	UpdatePartitionListEntry();
	CollisionProp()->UpdatePartition();
	SetBlocksLOS(false);
	UpdateVisibility();
	SetNextClientThink(CLIENT_THINK_ALWAYS);
	return true;
}

void C_ClientRagdollGib::ImpactTrace(trace_t *pTrace, int iDamageType, const char *pCustomImpactName)
{
	VPROF("C_ClientRagdollGib::ImpactTrace");

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if (!pPhysicsObject)
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if (iDamageType == DMG_BLAST)
	{
		dir *= 500;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter(dir);
	}
	else
	{
		Vector hitpos;

		VectorMA(pTrace->startpos, pTrace->fraction, dir, hitpos);
		VectorNormalize(dir);

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset(dir, hitpos);
	}

	DoBloodSpray(pTrace);
	CanGibEntity(dir, pTrace->hitgroup, iDamageType);

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

void C_ClientRagdollGib::OnGibbedGroup(int hitgroup, bool bExploded)
{
	// We only care about this for plr. models...
	if (GetPlayerIndexLink() <= 0)
		return;

	if (bExploded)
	{
		for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
		{
			int accessoryGroup = FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[i]);
			if (accessoryGroup == -1)
				continue;

			SetBodygroup(accessoryGroup, 0);
		}

		return;
	}

	int iAccessoryGroup = -1;
	if (hitgroup == HITGROUP_HEAD)
		iAccessoryGroup = 0;
	else if (hitgroup == HITGROUP_LEFTLEG)
		iAccessoryGroup = 2;
	else if (hitgroup == HITGROUP_RIGHTLEG)
		iAccessoryGroup = 3;

	if (iAccessoryGroup == -1 || (iAccessoryGroup < 0) || (iAccessoryGroup >= PLAYER_ACCESSORY_MAX))
		return;

	int accessoryGroup = FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[iAccessoryGroup]);
	if (accessoryGroup == -1)
		return;

	SetBodygroup(accessoryGroup, 0);
}

void C_ClientRagdollGib::OnBecomeRagdoll(void)
{
	BaseClass::OnBecomeRagdoll();

	// Ragdolls have different indexes so we need to spawn blood here... 
	if ((m_iGibType == CLIENT_RAGDOLL) && IsClientRagdoll())
	{
		if (IsGibFlagActive(GIB_FULL_EXPLODE))
			DispatchParticleEffect(GameBaseShared()->GetSharedGameDetails()->GetBloodExplosionMist(GameBaseClient->IsExtremeGore()), PATTACH_ABSORIGIN_FOLLOW, this);
		else if (IsGibFlagActive(GIB_NO_HEAD))
			DispatchParticleEffect(GameBaseShared()->GetSharedGameDetails()->GetGibParticleForLimb("head", GameBaseClient->IsExtremeGore()), PATTACH_POINT_FOLLOW, this, "gore_head");
	}
}

//========================================================================================//
//
// Purpose: Client Physics GIB:
//
//========================================================================================//

C_ClientPhysicsGib::C_ClientPhysicsGib(void)
{
	m_iHealth = 0;
}

C_ClientPhysicsGib::~C_ClientPhysicsGib()
{
}

bool C_ClientPhysicsGib::Initialize(int type)
{
	bool ret = BaseClass::Initialize(type);
	if (!ret)
		return false;

	Spawn();
	SetMoveType(MOVETYPE_PUSH);

	solid_t tmpSolid;
	PhysModelParseSolid(tmpSolid, this, GetModelIndex());

	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal(SOLID_VPHYSICS, 0, false, &tmpSolid);
	if (!pPhysicsObject)
	{
		SetSolid(SOLID_NONE);
		SetMoveType(MOVETYPE_NONE);
		Warning("ERROR!: Can't create physics object for %s\n", STRING(GetModelName()));
	}

	IPhysicsObject *pPhysicsObj = VPhysicsGetObject();
	if (pPhysicsObj)
	{
		// We want touch calls when we hit the world
		unsigned int flags = pPhysicsObj->GetCallbackFlags();
		pPhysicsObj->SetCallbackFlags(flags | CALLBACK_GLOBAL_TOUCH_STATIC);
	}

	// start fading out at 75% of r_propsmaxdist
	m_fadeMaxDist = r_propsmaxdist.GetFloat();
	m_fadeMinDist = r_propsmaxdist.GetFloat() * 0.75f;

	UpdatePartitionListEntry();
	CollisionProp()->UpdatePartition();
	SetBlocksLOS(false);
	UpdateVisibility();
	SetNextClientThink(CLIENT_THINK_ALWAYS);

	return true;
}
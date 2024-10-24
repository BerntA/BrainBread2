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
ConVar bb2_gibs_max("bb2_gibs_max", "64", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the max amount of gibs and ragdolls to be created on the client. When you reach the limit old gibs will fade out automatically.", true, 0.0f, true, 256.0f);
ConVar bb2_gibs_enable_fade("bb2_gibs_enable_fade", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Should gibs and ragdolls fade out?", true, 0.0f, true, 1.0f);
ConVar bb2_gibs_fadeout_time("bb2_gibs_fadeout_time", "4", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the time in seconds before a ragdoll or gib will fade out.", true, 0.0f, true, 30.0f);
ConVar bb2_gibs_blood_chance("bb2_gibs_blood_chance", "100", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Set the chance in % to spawn blood when you attack an entity.", true, 0.0f, true, 100.0f);

extern ConVar g_ragdoll_fadespeed;
extern ConVar r_propsmaxdist;

static CUtlVector<C_ClientSideGibBase*> s_ClientGibList;

int GetCurrentGibCount()
{
	int count = 0;
	for (int i = 0; i < s_ClientGibList.Count(); i++)
	{
		if ((s_ClientGibList[i]->GetGibType() <= CLIENT_GIB_PROP) || s_ClientGibList[i]->IsForceFading())
			continue;

		count++;
	}

	return count;
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
	m_iPlayerTeam = m_iPlayerIndex = m_iGibType = 0;
	m_szSurvivor[0] = 0;
	m_flTouchDelta = 0.0f;
	m_bReleaseGib = m_bFadingOut = m_bNoModelParticles = m_bForceFade = m_bDispatchedBleedout = false;
	m_flFadeOutDelay = gpGlobals->curtime + PROP_FADE_TIME;
	m_takedamage = DAMAGE_EVENTS_ONLY;
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

	AddEFlags(EFL_USE_PARTITION_WHEN_NOT_SOLID);
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

	m_iGibType = type;
	return true;
}

bool C_ClientSideGibBase::Initialize(int type, const model_t* model)
{
	if (InitializeAsClientEntity(NULL, RENDER_GROUP_OPAQUE_ENTITY) == false)
		return false;

	SetModelName(modelinfo->GetModelName(model));
	SetModelPointer(model);

	AddEFlags(EFL_USE_PARTITION_WHEN_NOT_SOLID);
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

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

void C_ClientSideGibBase::LoadPhysics()
{
	SetMoveType(MOVETYPE_PUSH);
	solid_t tmpSolid;
	PhysModelParseSolid(tmpSolid, this, GetModelIndex());
	VPhysicsInitNormal(SOLID_VPHYSICS, 0, false, &tmpSolid);
	IPhysicsObject* pPhysicsObj = VPhysicsGetObject();
	if (!pPhysicsObj)
	{
		SetSolid(SOLID_NONE);
		SetMoveType(MOVETYPE_NONE);
		Warning("ERROR!: Can't create physics object for %s\n", STRING(GetModelName()));
		return;
	}

	// We want touch calls when we hit the world
	pPhysicsObj->SetCallbackFlags(pPhysicsObj->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH | CALLBACK_GLOBAL_TOUCH_STATIC);
}

void C_ClientSideGibBase::SetForceFade(bool value)
{
	m_bForceFade = value;
	if (m_bForceFade)
		m_flFadeOutDelay = gpGlobals->curtime + FORCE_FADE_TIME;
	else
		m_flFadeOutDelay = gpGlobals->curtime + (m_iGibType == CLIENT_GIB_PROP ? PROP_FADE_TIME : bb2_gibs_fadeout_time.GetFloat());
}

void C_ClientSideGibBase::OnBecomeRagdoll(void)
{
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

	if (!m_bDispatchedBleedout && (m_iGibType == CLIENT_RAGDOLL) && IsClientRagdoll() && !IsGibFlagActive(GIB_RAGDOLL_SUICIDE) && bb2_gibs_spawn_blood_puddle.GetBool())
	{
		IPhysicsObject* pPhysics = VPhysicsGetObject();
		if (pPhysics == NULL)
			return;

		Vector vecVelocity, origin;
		AngularImpulse angVelocity;
		QAngle angles;

		pPhysics->GetVelocity(&vecVelocity, &angVelocity);
		pPhysics->GetPosition(&origin, &angles);

		if (vecVelocity.Length() < 10.0f)
		{
			m_bDispatchedBleedout = true;
			GameBaseShared()->DispatchBleedout(origin);
		}
	}
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
		pChild->Release();

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

	if (iAlpha <= 0)
		m_bReleaseGib = true;
}

void C_ClientSideGibBase::OnFullyInitialized(void)
{
	if (GetGibType() != CLIENT_GIB_PROP)
	{
		int items = GetCurrentGibCount();
		if (items > bb2_gibs_max.GetInt()) // Remove older gibs if we reached the limit!
		{
			for (int i = 0; i < s_ClientGibList.Count(); i++)
			{
				if ((s_ClientGibList[i]->GetGibType() <= CLIENT_GIB_PROP) ||
					s_ClientGibList[i]->IsForceFading() ||
					(s_ClientGibList[i] == this))
					continue;

				s_ClientGibList[i]->SetForceFade(true);
				items--;

				if (items > bb2_gibs_max.GetInt())
					continue;

				break;
			}
		}
	}

	AddEffects(EF_NOINTERP);
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
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
			UTIL_DecalTrace(&tr, GameBaseClient->IsExtremeGore() ? "ExtremeBlood" : "Blood");
	}
}

void C_ClientSideGibBase::DoBloodSpray(trace_t *pTrace)
{
	if (!bb2_gibs_spawn_blood.GetBool() || (random->RandomInt(0, 100) > bb2_gibs_blood_chance.GetInt()))
		return;

	if (m_iGibType > CLIENT_GIB_PROP)
	{
		QAngle angles = GetLocalAngles();
		DispatchParticleEffect(GameBaseShared()->GetSharedGameDetails()->GetBloodParticle(GameBaseClient->IsExtremeGore()), pTrace->endpos, angles, Vector(0, 0, 0), Vector(0, 0, 0), false, this, PATTACH_ABSORIGIN_FOLLOW);

		trace_t tr;
		Vector vecInitial = pTrace->endpos, vecDown;
		AngleVectors(angles, NULL, NULL, &vecDown);
		VectorNormalize(vecDown);
		vecDown *= -1;

		UTIL_TraceLine(vecInitial, vecInitial + vecDown * MAX_TRACE_LENGTH, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
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

	if (iDamageType == DMG_BLAST)
	{
		dir *= 500;  // adjust impact strenght
		pPhysicsObject->ApplyForceCenter(dir); // apply force
	}
	else
	{
		Vector hitpos;
		VectorMA(pTrace->startpos, pTrace->fraction, dir, hitpos);
		VectorNormalize(dir);
		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset(dir, hitpos);

		CEffectData data;
		data.m_vOrigin = pTrace->endpos;
		data.m_vStart = pTrace->startpos;
		data.m_nSurfaceProp = pTrace->surface.surfaceProps;
		data.m_nDamageType = iDamageType;
		data.m_nHitBox = pTrace->hitbox;
		data.m_hEntity = GetRefEHandle();

		DispatchEffect(((pCustomImpactName == NULL) ? "Impact" : pCustomImpactName), data);
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
	if (m_iPlayerIndex == GetLocalPlayerIndex())
		m_pPlayerRagdoll = NULL;
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
		pPhysicsObj->SetCallbackFlags(pPhysicsObj->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH | CALLBACK_GLOBAL_TOUCH_STATIC);
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
		pPhysicsObject->ApplyForceCenter(dir);
	}
	else
	{
		Vector hitpos;
		VectorMA(pTrace->startpos, pTrace->fraction, dir, hitpos);
		VectorNormalize(dir);
		dir *= 4000;  // adjust impact strenght
		pPhysicsObject->ApplyForceOffset(dir, hitpos);
	}

	DoBloodSpray(pTrace);
	CanGibEntity(dir, pTrace->hitgroup, iDamageType);

	if (m_pRagdoll)
		m_pRagdoll->ResetRagdollSleepAfterTime();
}

void C_ClientRagdollGib::OnGibbedGroup(int hitgroup, bool bExploded)
{
	// We only care about this for plr. models...
	if (m_iPlayerTeam <= 0)
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
	if (GetIRagdoll() == NULL)
		LoadPhysics(); // Fallback

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

	LoadPhysics();
	return true;
}

bool C_ClientPhysicsGib::Initialize(int type, const model_t *model)
{
	bool ret = BaseClass::Initialize(type, model);
	if (!ret)
		return false;

	LoadPhysics();
	return true;
}

void C_ClientPhysicsGib::LoadPhysics()
{
	SetMoveType(MOVETYPE_PUSH);
	solid_t tmpSolid;
	PhysModelParseSolid(tmpSolid, this, GetModelIndex());
	VPhysicsInitNormal(SOLID_VPHYSICS, 0, false, &tmpSolid);

	IPhysicsObject *pPhysicsObj = VPhysicsGetObject();
	if (pPhysicsObj)
	{
		// We want touch calls when we hit the world
		pPhysicsObj->SetCallbackFlags(pPhysicsObj->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH | CALLBACK_GLOBAL_TOUCH_STATIC);
	}
	else
	{
		SetSolid(SOLID_NONE);
		SetMoveType(MOVETYPE_NONE);
		Warning("ERROR!: Can't create physics object for %s\n", STRING(GetModelName()));
	}

	// start fading out at 75% of r_propsmaxdist
	m_fadeMaxDist = r_propsmaxdist.GetFloat();
	m_fadeMinDist = r_propsmaxdist.GetFloat() * 0.75f;

	UpdatePartitionListEntry();
	CollisionProp()->UpdatePartition();
	SetBlocksLOS(false);
	UpdateVisibility();
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_ClientPhysicsGib::OnFullyInitialized(void)
{
	BaseClass::OnFullyInitialized();

	if (m_iGibType > CLIENT_GIB_PROP)
	{
		if (!bb2_gibs_spawn_blood.GetBool())
			return;

		DispatchParticleEffect(GameBaseShared()->GetSharedGameDetails()->GetBloodParticle(GameBaseClient->IsExtremeGore()), PATTACH_ABSORIGIN_FOLLOW, this);
	}
}
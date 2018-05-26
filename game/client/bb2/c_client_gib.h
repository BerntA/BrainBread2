//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Client Gib Class. Used for prop gibs / npc / player gibs and ragdolls.
//
//========================================================================================//

#ifndef C_CLIENT_GIB_H
#define C_CLIENT_GIB_H
#ifdef _WIN32
#pragma once
#endif

#include "gibs_shared.h"
#include "takedamageinfo.h"

class C_ClientSideGibBase : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_ClientSideGibBase, C_BaseAnimating);

	C_ClientSideGibBase(void);
	virtual ~C_ClientSideGibBase();

	virtual bool Initialize(int type);
	virtual bool LoadRagdoll();
	virtual void Spawn();

	virtual bool IsDormant(void) { return false; }
	virtual CollideType_t GetCollideType(void) { return ENTITY_SHOULD_RESPOND; }
	virtual void StartTouch(C_BaseEntity *pOther);
	virtual void HitSurface(C_BaseEntity *pOther);
	virtual void ImpactTrace(trace_t *pTrace, int iDamageType, const char *pCustomImpactName);
	virtual bool IsClientCreated(void) const { return true; }

	virtual void ClientThink(void);
	virtual void SUB_Remove(void);
	virtual void ReleaseGib(void) { m_bReleaseGib = true; }
	virtual void Release(void);

	virtual bool IsClientRagdoll(void) { return false; }

	virtual int GetGibType(void) { return m_iGibType; }
	virtual void SetGibType(int type) { m_iGibType = type; }
	virtual void OnGibbedGroup(int hitgroup, bool bExploded) { }

	virtual void SetForceFade(bool value);

	virtual void OnBecomeRagdoll(void);

	virtual IRagdoll* GetIRagdoll() const;

	virtual void SetPlayerIndexLink(int index) { m_iPlayerIndexLink = index; }
	virtual int GetPlayerIndexLink(void) { return m_iPlayerIndexLink; }

	virtual void DoBloodSpray(trace_t *pTrace);

private:
	void FadeOut(void);

protected:
	float m_flTouchDelta; // Amount of time that must pass before another touch function can be called
	float m_flFadeOutDelay;
	bool m_bFadingOut;
	bool m_bReleaseGib;
	bool m_bForceFade;
	int m_iPlayerIndexLink;
	int m_iGibType;
};

class C_ClientRagdollGib : public C_ClientSideGibBase
{
public:
	DECLARE_CLASS(C_ClientRagdollGib, C_ClientSideGibBase);

	C_ClientRagdollGib(void);
	virtual ~C_ClientRagdollGib();

	bool Initialize(int type);
	bool IsClientRagdoll(void) { return true; }
	bool LoadRagdoll();

	void ImpactTrace(trace_t *pTrace, int iDamageType, const char *pCustomImpactName);
	bool CanGibEntity(const Vector &velocity, int hitgroup, int damageType);
	void OnGibbedGroup(int hitgroup, bool bExploded);
	void OnBecomeRagdoll(void);

	int GetGibFlags(void) { return m_nGibFlags; }
	void SetGibFlag(int nFlag) { m_nGibFlags = nFlag; }
	void AddGibFlag(int nFlag) { m_nGibFlags |= nFlag; }
	void RemoveGibFlag(int nFlag) { m_nGibFlags &= ~nFlag; }
	void ClearGibFlags(void) { m_nGibFlags = 0; }
	bool IsGibFlagActive(int nFlag) { return (m_nGibFlags & nFlag) != 0; }

	int m_nGibFlags;
	char pchNPCName[32];
};

class C_ClientPhysicsGib : public C_ClientSideGibBase
{
public:
	DECLARE_CLASS(C_ClientPhysicsGib, C_ClientSideGibBase);

	C_ClientPhysicsGib(void);
	virtual ~C_ClientPhysicsGib();

	bool Initialize(int type);
};

extern ConVar bb2_gibs_fadeout_time;
extern ConVar bb2_gibs_blood_chance;

extern bool CanSpawnClientGib(int type);
extern bool ShouldFadeClientGib(int type);
extern bool RemoveAllClientGibs();

#endif // C_CLIENT_GIB_H
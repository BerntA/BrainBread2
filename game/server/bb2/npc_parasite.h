//=========       Copyright © Reperio Studios 2024 @ Bernt Andreas Eide!       ============//
//
// Purpose: Parasite - Jumper & Humper!
//
//========================================================================================//

#ifndef NPC_PARASITE_H
#define NPC_PARASITE_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_squadslot.h"
#include "ai_basenpc.h"
#include "soundent.h"
#include "npc_base_properties.h"

class CBaseParasite : public CAI_BaseNPC, public CNPCBaseProperties
{
	DECLARE_CLASS(CBaseParasite, CAI_BaseNPC);

public:
	virtual void	Spawn(void);
	virtual void	Precache(void);

	virtual void	RunTask(const Task_t* pTask);
	virtual void	StartTask(const Task_t* pTask);

	virtual void	OnChangeActivity(Activity NewActivity);

	virtual bool	IsFirmlyOnGround();
	virtual void	MoveOrigin(const Vector& vecDelta);
	virtual void	ThrowAt(const Vector& vecPos);
	virtual void	ThrowThink(void);
	virtual void	JumpAttack(bool bRandomJump, const Vector& vecPos = vec3_origin, bool bThrown = false);

	virtual bool	HasHeadroom();
	virtual void	LeapTouch(CBaseEntity* pOther);
	virtual void	TouchDamage(CBaseEntity* pOther);
	virtual Vector	BodyTarget(const Vector& posSrc, bool bNoisy = true);

	virtual float	MaxYawSpeed(void);
	virtual void	GatherConditions(void);
	virtual void	PrescheduleThink(void);
	virtual Class_T Classify(void);
	virtual void	HandleAnimEvent(animevent_t* pEvent);
	virtual int		RangeAttack1Conditions(float flDot, float flDist);
	virtual int		OnTakeDamage_Alive(const CTakeDamageInfo& info);
	virtual void	ClampRagdollForce(const Vector& vecForceIn, Vector* vecForceOut);
	virtual void	Event_Killed(const CTakeDamageInfo& info);
	virtual void	BuildScheduleTestBits(void);

	virtual bool	IsJumping(void) { return m_bMidJump; }

	virtual void	BiteSound(void) {};
	virtual void	AttackSound(void) {};
	virtual void	ImpactSound(void) {};
	virtual void	TelegraphSound(void) {};

	virtual int		SelectSchedule(void);
	virtual int		SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode);
	virtual int		TranslateSchedule(int scheduleType);

	virtual float	GetReactionDelay(CBaseEntity* pEnemy) { return 0.0; }
	virtual	bool	AllowedToIgnite(void) { return false; }

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

protected:
	void Leap(const Vector& vecVel);

	int CalcDamageInfo(CTakeDamageInfo* pInfo);

	float InnateRange1MinRange(void);
	float InnateRange1MaxRange(void);

protected:
	Vector	m_vecCommittedJumpPos;	// The position of our enemy when we locked in our jump attack.

	float	m_flNextNPCThink;
	float	m_flIgnoreWorldCollisionTime;

	bool	m_bCommittedToJump;		// Whether we have 'locked in' to jump at our enemy.
	bool	m_bMidJump;
	bool	m_bAttackFailed;		// whether we ran into a wall during a jump.

	COutputEvent m_OnLeap;
};

class CTurtleLord : public CBaseParasite
{
	DECLARE_CLASS(CTurtleLord, CBaseParasite);

public:
	void Precache(void);
	void Spawn(void);

	float	MaxYawSpeed(void);
	Activity NPC_TranslateActivity(Activity eNewActivity);

	void	BiteSound(void);
	void	PainSound(const CTakeDamageInfo& info);
	void	DeathSound(const CTakeDamageInfo& info);
	void	IdleSound(void);
	void	AlertSound(void);
	void	AttackSound(void);
	void	TelegraphSound(void);

	int		GetNPCClassType() { return NPC_CLASS_TURTLE; }
	BB2_SoundTypes GetNPCType() { return TYPE_CUSTOM; }

	bool	CanAlwaysSeePlayers() { return true; }
	bool	GetGender() { return true; }
};

#endif // NPC_PARASITE_H
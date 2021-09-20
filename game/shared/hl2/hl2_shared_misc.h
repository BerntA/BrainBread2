//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared HL2 Stuff...
//
//========================================================================================//

#ifndef HL2_SHARED_MISC_H
#define HL2_SHARED_MISC_H
#ifdef _WIN32
#pragma once
#endif 

#ifdef CLIENT_DLL
#include "c_baseentity.h"
#include "iviewrender_beams.h"
#else
#include "baseentity.h"
#include "Sprite.h"
#include "npcevent.h"
#include "beam_shared.h"
#endif

bool PlayerPickupControllerIsHoldingEntity(CBaseEntity *pPickupController, CBaseEntity *pHeldEntity);
float PlayerPickupGetHeldObjectMass(CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject);

#ifndef CLIENT_DLL

class RocketTrail;

//###########################################################################
//	>> CMissile		(missile launcher class is below this one!)
//###########################################################################
class CMissile : public CBaseCombatCharacter
{
	DECLARE_CLASS(CMissile, CBaseCombatCharacter);

public:

	static const int EXPLOSION_RADIUS = 200;
	static const int EXPLOSION_DAMAGE = 200;

	CMissile();
	virtual ~CMissile();

	virtual Class_T Classify(void);

	virtual void	Spawn(void);
	virtual void	Precache(void);
	virtual void	MissileTouch(CBaseEntity *pOther);
	virtual void	Explode(void);
	virtual void	IgniteThink(void);
	virtual void	SeekThink(void);
	virtual void	DumbFire(void);
	virtual void	SetGracePeriod(float flGracePeriod);

	virtual int		OnTakeDamage_Alive(const CTakeDamageInfo &info);

	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	virtual float GetRadius() { return m_flRadius; }
	virtual void SetRadius(float flRadius) { m_flRadius = flRadius; }

	virtual void SetFriendly(bool value) { m_bShouldBeFriendly = value; }

	unsigned int PhysicsSolidMaskForEntity(void) const;

	EHANDLE m_hOwner;

	static CMissile *Create(const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner, bool bFriendly = false);

	void CreateDangerSounds(bool bState){ m_bCreateDangerSounds = bState; }

protected:
	virtual void DoExplosion();
	virtual void CreateSmokeTrail(void);

	CHandle<RocketTrail>	m_hRocketTrail;
	float					m_flDamage;
	float                   m_flRadius;

	bool					m_bShouldBeFriendly;

private:
	float					m_flGracePeriodEndsAt;
	bool					m_bCreateDangerSounds;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// Specialized mizzizzile
//-----------------------------------------------------------------------------
class CAPCMissile : public CMissile
{
	DECLARE_CLASS(CMissile, CMissile);
	DECLARE_DATADESC();

public:
	static CAPCMissile *Create(const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner);

	CAPCMissile();
	virtual ~CAPCMissile();
	virtual void	IgniteDelay(void);
	virtual void	ExplodeDelay(float flDelayTime);
	virtual Class_T Classify(void) { return CLASS_COMBINE; }

	CAPCMissile *m_pNext;

protected:
	virtual void DoExplosion();

private:
	virtual void Init();
	virtual void BeginSeekThink();
	virtual void ExplodeThink();
};

#endif

#endif // HL2_SHARED_MISC_H
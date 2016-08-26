//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: TempEnt : Bullet Shot, includes tracer and impact decal logic.
//
//========================================================================================//

#include "cbase.h"
#include "c_basetempentity.h"
#include <cliententitylist.h>
#include "ammodef.h"
#include "c_te_effect_dispatch.h"
#include "shot_manipulator.h"
#include "util_shared.h"
#include "input.h"

ConVar bb2_enable_particle_gunfx("bb2_enable_particle_gunfx", "1", FCVAR_ARCHIVE, "Enable particle based gun effects like: Muzzleflashes, Bullet tracers and Smoke.", true, 0.0f, true, 1.0f);

class C_TEBulletShot : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEBulletShot, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	void PostDataUpdate(DataUpdateType_t updateType);
	void CreateEffects(void);

	int		m_iWeaponIndex;
	Vector	m_vecOrigin;
	Vector  m_vecDir;
	int		m_iAmmoID;
	bool	m_bDoImpacts;
	bool	m_bDoTracers;
	bool    m_bIsPenetrationBullet;
	bool    m_bUseTraceHull;
	bool    m_bDoWiz;
	bool    m_bDoMuzzleflash;
	bool    m_bPrimaryAttack;
};

class CTraceFilterClientBulletFire : public CTraceFilterSimple
{
public:
	DECLARE_CLASS(CTraceFilterClientBulletFire, CTraceFilterSimple);

	CTraceFilterClientBulletFire(const IHandleEntity *passentity, int collisionGroup) : CTraceFilterSimple(passentity, collisionGroup)
	{ 
	}

	virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask)
	{
		C_BaseEntity *pEntity = EntityFromEntityHandle(pServerEntity);
		if (pEntity)
		{
			if (dynamic_cast<C_BaseViewModel *>(pEntity) != NULL)
				return false;

			C_BaseCombatWeapon *pWeapon = dynamic_cast<C_BaseCombatWeapon *>(pEntity);
			if (pWeapon && pWeapon->GetOwner())
				return false;
		}

		return BaseClass::ShouldHitEntity(pServerEntity, contentsMask);
	}
};

void C_TEBulletShot::CreateEffects(void)
{
	CAmmoDef *pAmmoDef = GetAmmoDef();
	if (pAmmoDef == NULL)
		return;

	C_BaseEntity *pEnt = ClientEntityList().GetEnt(m_iWeaponIndex);
	if (!pEnt)
		return;

	C_BaseCombatWeapon *pWpn = dynamic_cast<C_BaseCombatWeapon *>(pEnt);
	if (!pWpn)
		return;

	C_BaseCombatCharacter *pOwnerOfWep = pWpn->GetOwner();
	if (!pOwnerOfWep)
		return;

	bool bParticleGunFX = bb2_enable_particle_gunfx.GetBool();

	if (m_bDoTracers || m_bDoImpacts)
	{
		Vector vecEnd = m_vecOrigin + m_vecDir * MAX_TRACE_LENGTH;
		trace_t tr;
		CTraceFilterClientBulletFire traceFilter(pOwnerOfWep, COLLISION_GROUP_NONE);

		if (m_bUseTraceHull)
			UTIL_TraceHull(m_vecOrigin, vecEnd, Vector(-3, -3, -3), Vector(3, 3, 3), MASK_SHOT, &traceFilter, &tr);
		else
			UTIL_TraceLine(m_vecOrigin, vecEnd, MASK_SHOT, &traceFilter, &tr);

		if (m_bDoTracers)
		{
			bool bDoParticleTracer = false;
			CEffectData data;
			data.m_vStart = tr.startpos;
			data.m_vOrigin = tr.endpos;
			data.m_hEntity = pWpn->GetRefEHandle();
			data.m_flScale = 0.0f;

			if (!m_bIsPenetrationBullet)
			{
				data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
				data.m_nAttachmentIndex = pWpn->GetTracerAttachment();
			}

			if (bParticleGunFX)
			{
				const char *pParticleEffect = pWpn->GetParticleEffect(PARTICLE_TYPE_TRACER);
				int iParticleIndex = GetParticleSystemIndex(pParticleEffect);
				if (iParticleIndex)
				{
					bDoParticleTracer = true;
					data.m_nHitBox = iParticleIndex;
					DispatchEffect("ParticleTracer", data);
				}
			}

			if (!bDoParticleTracer)
			{
				if (m_bDoWiz)
					data.m_fFlags |= TRACER_FLAG_WHIZ;

				DispatchEffect("Tracer", data);
			}
		}

		if (m_bDoImpacts)
			pWpn->DoImpactEffect(tr, pAmmoDef->DamageType(m_iAmmoID));
	}

	if (!m_bDoMuzzleflash)
		return;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pLocalPlayer)
		return;

	const char *particleAttachment = pWpn->GetMuzzleflashAttachment(m_bPrimaryAttack);
	bool bThirdpersonDispatch = true;
	C_BaseAnimating *pDispatcher = pWpn;
	C_BasePlayer *pPlayer = ToBasePlayer(pOwnerOfWep);
	if (pPlayer)
	{
		int entindexplr = pPlayer->entindex();
		int specTargetIndex = GetSpectatorTarget();
		C_BaseViewModel *pvm = pPlayer->GetViewModel();
		if (pvm &&
			(((entindexplr == GetLocalPlayerIndex()) && (!input->CAM_IsThirdPerson())) ||
			((pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE) && (specTargetIndex == entindexplr)))
			)
		{
			pDispatcher = pvm;
			bThirdpersonDispatch = false;

			if (!bParticleGunFX)
				pvm->DoMuzzleFlash();
		}

		if (entindexplr == GetLocalPlayerIndex())
			pWpn->GetBaseAnimating()->ProcessMuzzleFlashEvent();
	}

	if (bParticleGunFX)
		DispatchParticleEffect(pWpn->GetParticleEffect(PARTICLE_TYPE_MUZZLE, bThirdpersonDispatch), PATTACH_POINT_FOLLOW, pDispatcher, particleAttachment);
	else
		pWpn->DoMuzzleFlash();
}

void C_TEBulletShot::PostDataUpdate(DataUpdateType_t updateType)
{
	if (m_bDoTracers || m_bDoImpacts || m_bDoMuzzleflash)
	{
		CreateEffects();
	}
}

IMPLEMENT_CLIENTCLASS_EVENT(C_TEBulletShot, DT_TEBulletShot, CTEBulletShot);

BEGIN_RECV_TABLE_NOBASE(C_TEBulletShot, DT_TEBulletShot)
RecvPropInt(RECVINFO(m_iWeaponIndex)),
RecvPropVector(RECVINFO(m_vecOrigin)),
RecvPropVector(RECVINFO(m_vecDir)),
RecvPropInt(RECVINFO(m_iAmmoID)),
RecvPropBool(RECVINFO(m_bDoImpacts)),
RecvPropBool(RECVINFO(m_bDoTracers)),
RecvPropBool(RECVINFO(m_bIsPenetrationBullet)),
RecvPropBool(RECVINFO(m_bUseTraceHull)),
RecvPropBool(RECVINFO(m_bDoWiz)),
RecvPropBool(RECVINFO(m_bDoMuzzleflash)),
RecvPropBool(RECVINFO(m_bPrimaryAttack)),
END_RECV_TABLE()
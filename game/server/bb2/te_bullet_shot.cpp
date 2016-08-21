//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: TempEnt : Bullet Shot, includes tracer and impact decal logic.
//
//========================================================================================//

#include "cbase.h"
#include "basetempentity.h"

class CTEBulletShot : public CBaseTempEntity
{
public:
	DECLARE_CLASS(CTEBulletShot, CBaseTempEntity);
	DECLARE_SERVERCLASS();

	CTEBulletShot(const char *name);

	CNetworkVar(int, m_iWeaponIndex);
	CNetworkVector(m_vecOrigin);
	CNetworkVector(m_vecDir);
	CNetworkVar(int, m_iAmmoID);
	CNetworkVar(bool, m_bDoImpacts);
	CNetworkVar(bool, m_bDoTracers);
	CNetworkVar(bool, m_bIsPenetrationBullet);
	CNetworkVar(bool, m_bUseTraceHull);
	CNetworkVar(bool, m_bDoWiz);
	CNetworkVar(bool, m_bDoMuzzleflash);
	CNetworkVar(bool, m_bPrimaryAttack);
};

CTEBulletShot::CTEBulletShot(const char *name) : CBaseTempEntity(name)
{
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEBulletShot, DT_TEBulletShot)
SendPropInt(SENDINFO(m_iWeaponIndex), 12, SPROP_UNSIGNED),
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD),
SendPropVector(SENDINFO(m_vecDir), -1),
SendPropInt(SENDINFO(m_iAmmoID), 5, SPROP_UNSIGNED),
SendPropBool(SENDINFO(m_bDoImpacts)),
SendPropBool(SENDINFO(m_bDoTracers)),
SendPropBool(SENDINFO(m_bIsPenetrationBullet)),
SendPropBool(SENDINFO(m_bUseTraceHull)),
SendPropBool(SENDINFO(m_bDoWiz)),
SendPropBool(SENDINFO(m_bDoMuzzleflash)),
SendPropBool(SENDINFO(m_bPrimaryAttack)),
END_SEND_TABLE()

static CTEBulletShot g_TEBulletShot("BulletShot");

void TE_HL2MPFireBullets(
	int	iWeaponIndex,
	const Vector &vOrigin,
	const Vector &vStart,
	const Vector &vDir,
	int	iAmmoID,
	bool bDoTracers,
	bool bDoImpacts,
	bool bPenetrationBullet,
	bool bUseTraceHull,
	bool bDoWiz,
	bool bDoMuzzleflash,
	bool bPrimaryAttack
	)
{
	CPASFilter filter(vOrigin);
	g_TEBulletShot.m_iWeaponIndex = iWeaponIndex;
	g_TEBulletShot.m_vecOrigin = vStart;
	g_TEBulletShot.m_vecDir = vDir;
	g_TEBulletShot.m_iAmmoID = iAmmoID;
	g_TEBulletShot.m_bDoTracers = bDoTracers;
	g_TEBulletShot.m_bDoImpacts = bDoImpacts;
	g_TEBulletShot.m_bIsPenetrationBullet = bPenetrationBullet;
	g_TEBulletShot.m_bUseTraceHull = bUseTraceHull;
	g_TEBulletShot.m_bDoWiz = bDoWiz;
	g_TEBulletShot.m_bDoMuzzleflash = bDoMuzzleflash;
	g_TEBulletShot.m_bPrimaryAttack = bPrimaryAttack;
	g_TEBulletShot.Create(filter, 0);
}
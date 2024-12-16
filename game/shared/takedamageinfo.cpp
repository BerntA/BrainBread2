//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "takedamageinfo.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar phys_pushscale( "phys_pushscale", "1", FCVAR_REPLICATED );

void CTakeDamageInfo::Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iCustomDamage )
{
	m_hInflictor = pInflictor;

	if ( pAttacker )
	{
		m_hAttacker = pAttacker;
	}
	else
	{
		m_hAttacker = pInflictor;
	}

	m_hWeapon = pWeapon;
	m_flDamage = flDamage;

	m_bitsDamageType = bitsDamageType;
	m_iDamageCustom = iCustomDamage;

	m_vecDamageForce = damageForce;
	m_vecDamagePosition = damagePosition;
	m_vecReportedPosition = reportedPosition;
	m_iAmmoType = -1;

	m_iWeaponIDForced = WEAPON_ID_NONE;
	m_cRelationshipLink = m_nMiscFlags = m_nSkillFlags = 0;
}

CTakeDamageInfo::CTakeDamageInfo()
{
	Init( NULL, NULL, NULL, vec3_origin, vec3_origin, vec3_origin, 0, 0, 0 );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, pWeapon, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, NULL, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, pWeapon, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, NULL, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Vector vecReported = vec3_origin;
	if ( reportedPosition )
	{
		vecReported = *reportedPosition;
	}
	Init( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, vecReported, flDamage, bitsDamageType, iKillType );
}

//-----------------------------------------------------------------------------
// Purpose: get the name of the ammo that caused damage
// Note: returns the ammo name, or the classname of the object, or the model name in the case of physgun ammo.
//-----------------------------------------------------------------------------
const char *CTakeDamageInfo::GetAmmoName() const
{
	const char *pszAmmoType;

	if ( m_iAmmoType >= 0 )
	{
		pszAmmoType = GetAmmoDef()->GetAmmoOfIndex( m_iAmmoType )->pName;
	}
	// no ammoType, so get the ammo name from the inflictor
	else if ( m_hInflictor != NULL )
	{
		pszAmmoType = m_hInflictor->GetClassname();

		// check for physgun ammo.  unfortunate that this is in game_shared.
		if ( Q_strcmp( pszAmmoType, "prop_physics" ) == 0 )
		{
			pszAmmoType = STRING( m_hInflictor->GetModelName() );
		}
	}
	else
	{
		pszAmmoType = "Unknown";
	}

	return pszAmmoType;
}

// -------------------------------------------------------------------------------------------------- //
// MultiDamage
// Collects multiple small damages into a single damage
// -------------------------------------------------------------------------------------------------- //
CMultiDamage g_MultiDamage;

CMultiDamage::CMultiDamage()
{
	m_hTarget = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiDamage::Init( CBaseEntity *pTarget, CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType )
{
	m_hTarget = pTarget;
	BaseClass::Init( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, reportedPosition, flDamage, bitsDamageType, iKillType );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the global multi damage accumulator
//-----------------------------------------------------------------------------
void ClearMultiDamage( void )
{
	g_MultiDamage.Init( NULL, NULL, NULL, NULL, vec3_origin, vec3_origin, vec3_origin, 0, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: inflicts contents of global multi damage register on gMultiDamage.pEntity
//-----------------------------------------------------------------------------
void ApplyMultiDamage( void )
{
	Vector		vecSpot1;//where blood comes from
	Vector		vecDir;//direction blood should go
	trace_t		tr;

	if ( !g_MultiDamage.GetTarget() )
		return;

#ifndef CLIENT_DLL
	const CBaseEntity *host = te->GetSuppressHost();
	te->SetSuppressHost( NULL );
		
	g_MultiDamage.GetTarget()->TakeDamage( g_MultiDamage );

	te->SetSuppressHost( (CBaseEntity*)host );
#endif

	// Damage is done, clear it out
	ClearMultiDamage();
}

//-----------------------------------------------------------------------------
// Purpose: Add damage to the existing multidamage, and apply if it won't fit
//-----------------------------------------------------------------------------
void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity )
{
	if ( !pEntity )
		return;

	if ( pEntity != g_MultiDamage.GetTarget() )
	{
		ApplyMultiDamage();
		g_MultiDamage.Init( pEntity, info.GetInflictor(), info.GetAttacker(), info.GetWeapon(), vec3_origin, vec3_origin, vec3_origin, 0.0, info.GetDamageType(), info.GetDamageCustom() );
	}

	g_MultiDamage.AddDamageType( info.GetDamageType() );
	g_MultiDamage.SetDamage( g_MultiDamage.GetDamage() + info.GetDamage() );
	g_MultiDamage.SetDamageForce( g_MultiDamage.GetDamageForce() + info.GetDamageForce() );
	g_MultiDamage.SetDamagePosition( info.GetDamagePosition() );
	g_MultiDamage.SetReportedPosition( info.GetReportedPosition() );
	g_MultiDamage.SetAmmoType( info.GetAmmoType() );
	g_MultiDamage.SetSkillFlags(info.GetSkillFlags());
	g_MultiDamage.SetForcedWeaponID(info.GetForcedWeaponID());
	g_MultiDamage.SetRelationshipLink(info.GetRelationshipLink());
	g_MultiDamage.SetMiscFlag(info.GetMiscFlag());

	bool bHasPhysicsForceDamage = !g_pGameRules->Damage_NoPhysicsForce( info.GetDamageType() );
	if ( bHasPhysicsForceDamage && g_MultiDamage.GetDamageType() != DMG_GENERIC )
	{
		// If you hit this assert, you've called TakeDamage with a damage type that requires a physics damage
		// force & position without specifying one or both of them. Decide whether your damage that's causing 
		// this is something you believe should impart physics force on the receiver. If it is, you need to 
		// setup the damage force & position inside the CTakeDamageInfo (Utility functions for this are in
		// takedamageinfo.cpp. If you think the damage shouldn't cause force (unlikely!) then you can set the 
		// damage type to DMG_GENERIC, or | DMG_CRUSH if you need to preserve the damage type for purposes of HUD display.
		if ( g_MultiDamage.GetDamageForce() == vec3_origin || g_MultiDamage.GetDamagePosition() == vec3_origin )
		{
			static int warningCount = 0;
			if ( ++warningCount < 10 )
			{
				if ( g_MultiDamage.GetDamageForce() == vec3_origin )
				{
					Warning( "AddMultiDamage:  g_MultiDamage.GetDamageForce() == vec3_origin\n" );
				}

				if ( g_MultiDamage.GetDamagePosition() == vec3_origin)
				{
					Warning( "AddMultiDamage:  g_MultiDamage.GetDamagePosition() == vec3_origin\n" );
				}
			}
		}
	}
}


//============================================================================================================
// Utility functions for physics damage force calculation 
//============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: Returns an impulse scale required to push an object.
// Input  : flTargetMass - Mass of the target object, in kg
//			flDesiredSpeed - Desired speed of the target, in inches/sec.
//-----------------------------------------------------------------------------
float ImpulseScale( float flTargetMass, float flDesiredSpeed )
{
	return (flTargetMass * flDesiredSpeed);
}

//-----------------------------------------------------------------------------
// Purpose: Fill out a takedamageinfo with a damage force for an explosive
//-----------------------------------------------------------------------------
void CalculateExplosiveDamageForce( CTakeDamageInfo *info, const Vector &vecDir, const Vector &vecForceOrigin, float flScale )
{
	info->SetDamagePosition( vecForceOrigin );

	float flClampForce = ImpulseScale( 75, 400 );

	// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
	float flForceScale = info->GetDamage() * ImpulseScale( 75, 4 );

	if( flForceScale > flClampForce )
		flForceScale = flClampForce;

	// Fudge blast forces a little bit, so that each
	// victim gets a slightly different trajectory. 
	// This simulates features that usually vary from
	// person-to-person variables such as bodyweight,
	// which are all indentical for characters using the same model.
	flForceScale *= random->RandomFloat( 0.85, 1.15 );

	// Calculate the vector and stuff it into the takedamageinfo
	Vector vecForce = vecDir;
	VectorNormalize( vecForce );
	vecForce *= flForceScale;
	vecForce *= phys_pushscale.GetFloat();
	vecForce *= flScale;
	info->SetDamageForce( vecForce );
}

//-----------------------------------------------------------------------------
// Purpose: Fill out a takedamageinfo with a damage force for a bullet impact
//-----------------------------------------------------------------------------
void CalculateBulletDamageForce( CTakeDamageInfo *info, int iBulletType, const Vector &vecBulletDir, const Vector &vecForceOrigin, float flScale )
{
	info->SetDamagePosition( vecForceOrigin );
	Vector vecForce = vecBulletDir;
	VectorNormalize( vecForce );
	vecForce *= GetAmmoDef()->DamageForce( iBulletType );
	vecForce *= phys_pushscale.GetFloat();
	vecForce *= flScale;
	info->SetDamageForce( vecForce );
	Assert(vecForce!=vec3_origin);
}

//-----------------------------------------------------------------------------
// Purpose: Fill out a takedamageinfo with a damage force for a melee impact
//-----------------------------------------------------------------------------
void CalculateMeleeDamageForce( CTakeDamageInfo *info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale )
{
	info->SetDamagePosition( vecForceOrigin );

	// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
	float flForceScale = info->GetDamage() * ImpulseScale( 75, 4 );
	Vector vecForce = vecMeleeDir;
	VectorNormalize( vecForce );
	vecForce *= flForceScale;
	vecForce *= phys_pushscale.GetFloat();
	vecForce *= flScale;
	info->SetDamageForce( vecForce );
}

//-----------------------------------------------------------------------------
// Purpose: Try and guess the physics force to use.
//			This shouldn't be used for any damage where the damage force is unknown.
//			i.e. only use it for mapmaker specified damages.
//-----------------------------------------------------------------------------
void GuessDamageForce( CTakeDamageInfo *info, const Vector &vecForceDir, const Vector &vecForceOrigin, float flScale )
{
	if ( info->GetDamageType() & DMG_BULLET )
	{
		CalculateBulletDamageForce( info, GetAmmoDef()->Index("SMG"), vecForceDir, vecForceOrigin, flScale );
	}
	else if ( info->GetDamageType() & DMG_BLAST )
	{
		CalculateExplosiveDamageForce( info, vecForceDir, vecForceOrigin, flScale );
	}
	else
	{
		CalculateMeleeDamageForce( info, vecForceDir, vecForceOrigin, flScale );
	}
}
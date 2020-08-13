//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: HL2MP Base Weapon.
//
//==============================================================================================//

#ifndef WEAPON_HL2MPBASE_H
#define WEAPON_HL2MPBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "hl2mp_player_shared.h"
#include "basecombatweapon_shared.h"
#include "hl2mp_weapon_parse.h"

#ifndef CLIENT_DLL 
#include "ai_basenpc.h" 
#endif 

#if defined( CLIENT_DLL )
	#define CWeaponHL2MPBase C_WeaponHL2MPBase
#endif

class CHL2MP_Player;

class CWeaponHL2MPBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponHL2MPBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponHL2MPBase();

	#ifdef GAME_DLL
		DECLARE_DATADESC();
		void Materialize( void );
	#endif

	// All predicted weapons need to implement and return true
	virtual bool	IsPredicted() const;

	CBasePlayer* GetPlayerOwner() const;
	CHL2MP_Player* GetHL2MPPlayerOwner() const;

	void WeaponSound(WeaponSound_t sound_type, float soundtime = 0.0f, bool bSkipPrediction = false) OVERRIDE;
	
	CHL2MPSWeaponInfo const	&GetHL2MPWpnData() const;

	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual void FallInit( void );
	virtual bool Reload();

	virtual void	AddViewmodelBob(CBaseViewModel *viewmodel, Vector &origin, QAngle &angles);
	virtual	float	CalcViewmodelBob(void);
	
public:
	#if defined( CLIENT_DLL )
		
		virtual bool	ShouldPredict();
		virtual void	OnDataChanged( DataUpdateType_t type );

	#else

		virtual void	Spawn();

	#endif

	float m_flPrevAnimTime;

	virtual const Vector &GetOriginalSpawnOrigin(void) { return m_vOriginalSpawnOrigin; }
	virtual const QAngle &GetOriginalSpawnAngles( void ) { return m_vOriginalSpawnAngles;	}

private:

	CWeaponHL2MPBase( const CWeaponHL2MPBase & );

	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;
};


#endif // WEAPON_HL2MPBASE_H

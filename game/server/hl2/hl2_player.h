//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2_PLAYER_H
#define HL2_PLAYER_H
#pragma once

#include "player.h"
#include "hl2_playerlocaldata.h"
#include "simtimer.h"
#include "soundenvelope.h"

extern int TrainSpeed(int iSpeed, int iMax);

class CAI_Squad;
class IPhysicsPlayerController;
class CLogicPlayerProxy;

//=============================================================================
// >> HL2_PLAYER
//=============================================================================
class CHL2_Player : public CBasePlayer
{
public:
	DECLARE_CLASS(CHL2_Player, CBasePlayer);

	CHL2_Player();
	virtual ~CHL2_Player( void );
	
	static CHL2_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CHL2_Player::s_PlayerEdict = ed;
		return (CHL2_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void		Precache( void );
	virtual void		Spawn(void);
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper);
	virtual void		PlayerUse ( void );
	virtual void		SuspendUse( float flDuration ) { m_flTimeUseSuspended = gpGlobals->curtime + flDuration; }
	virtual void		UpdateClientData( void );
	virtual void		StopLoopingSounds( void );
	virtual void		Splash( void );

	virtual void		DrawDebugGeometryOverlays(void);

	virtual Vector		EyeDirection2D( void );
	virtual Vector		EyeDirection3D( void );

	virtual bool		ClientCommand( const CCommand &args );

	// from cbasecombatcharacter
	void				InitVCollision( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity );

	virtual Class_T				Classify ( void );

	// from CBasePlayer
	virtual void		SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize );
	
	void SetFlashlightEnabled( bool bState );
	void NotifyFriendsOfDamage( CBaseEntity *pAttackerEntity );

	// Walking
	void StartWalking( void );
	void StopWalking( void );
	bool IsWalking( void ) { return m_fIsWalking; }

	// Aiming heuristics accessors
	virtual float		GetLastDamageTime( void ) const { return m_flLastDamageTime; }
	virtual bool		IsDucking(void) const { return ((GetFlags() & FL_DUCKING) != 0); }

	virtual bool		PassesDamageFilter( const CTakeDamageInfo &info );
	void				InputIgnoreFallDamage( inputdata_t &inputdata );
	void				InputIgnoreFallDamageWithoutReset( inputdata_t &inputdata );
	void				InputEnableFlashlight( inputdata_t &inputdata );
	void				InputDisableFlashlight( inputdata_t &inputdata );

	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void		OnDamagedByExplosion( const CTakeDamageInfo &info );
	bool				ShouldShootMissTarget( CBaseCombatCharacter *pAttacker );

	virtual int			GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound);
	
	virtual bool		Weapon_CanUse( CBaseCombatWeapon *pWeapon );
	virtual void		Weapon_Equip( CBaseCombatWeapon *pWeapon );
	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, bool bWantDraw = false, int viewmodelindex = 0 );
	virtual bool		Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );

	// Flashlight Device
	int					FlashlightIsOn( void );
	void				FlashlightTurnOn( void );
	void				FlashlightTurnOff( void );

	// Underwater breather device
	virtual void		SetPlayerUnderwater( bool state );
	virtual bool		CanBreatheUnderwater() const { return false; }

	// physics interactions
	virtual void		PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual	bool		IsHoldingEntity( CBaseEntity *pEnt );
	virtual void		ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldindThis );
	virtual float		GetHeldObjectMass( IPhysicsObject *pHeldObject );

	virtual bool		IsFollowingPhysics( void ) { return false; }
	void				InputForceDropPhysObjects( inputdata_t &data );

	// override the test for getting hit
	virtual bool		TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	LadderMove_t		*GetLadderMove() { return &m_HL2Local.m_LadderMove; }
	virtual void		ExitLadder();
	virtual surfacedata_t *GetLadderSurface( const Vector &origin );
	
	void  HandleSpeedChanges( void );

	void SetControlClass( Class_T controlClass ) { m_nControlClass = controlClass; }
	
	void StartWaterDeathSounds( void );
	void StopWaterDeathSounds( void );

	CSoundPatch *m_sndLeeches;
	CSoundPatch *m_sndWaterSplashes;

	// This player's HL2 specific data that should only be replicated to 
	//  the player and not to other players.
	CNetworkVarEmbedded( CHL2PlayerLocalData, m_HL2Local ); // moved from private.

protected:
	virtual void		PreThink( void );
	virtual	void		PostThink( void );
	virtual bool		HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	virtual void		ItemPostFrame();
	virtual void		PlayUseDenySound();

private:

	Class_T				m_nControlClass;			// Class when player is controlling another entity

	CNetworkVarForDerived( bool, m_fIsWalking );

	float				m_flNextEntityTraceCheck;

protected:	// Jeep: Portal_Player needs access to this variable to overload PlayerUse for picking up objects through portals
	bool				m_bPlayUseDenySound;		// Signaled by PlayerUse, but can be unset by HL2 ladder code...

private:

	float				m_flTimeIgnoreFallDamage;
	bool				m_bIgnoreFallDamageResetAfterImpact;

	// Aiming heuristics code
	float				m_flLastDamageTime;	//Last time we took damage
	float				m_flTargetFindTime;

	bool				m_bFlashlightDisabled;

	float				m_flTimeUseSuspended;
	
	float				m_flLastTimeReplenishedAmmo; // From a bbc_ammo_box!
	float				m_flNextAmmoReplenishTime; // Penalty
	int					m_iNumAmmoReplenishes; // Ammo replenishes throughout a minute, if this value > X we will be punished for camping the ammo box!
	
	friend class CHL2GameMovement;
};

#endif	//HL2_PLAYER_H
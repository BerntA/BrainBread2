//=========       Copyright © Reperio-Studios 2015 @ Bernt A Eide!       ============//
//
// Purpose: Base Weapon Handling - Handles FX, Bash, Special stuff...
//
//==================================================================================//

#ifndef COMBATWEAPON_SHARED_H
#define COMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "sharedInterface.h"
#include "vphysics_interface.h"
#include "predictable_entity.h"
#include "soundflags.h"
#include "weapon_parse.h"
#include "baseviewmodel_shared.h"
#include "utlmap.h"
#include "skills_shareddefs.h"
#include "hl2mp_gamerules.h"

#if defined( CLIENT_DLL )
#define CBaseCombatWeapon C_BaseCombatWeapon
#endif

#if !defined( CLIENT_DLL )
void *SendProxy_SendLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );
#endif

class CBasePlayer;
class CBaseCombatCharacter;
class IPhysicsConstraint;
class CUserCmd;

#define SF_WEAPON_NO_PLAYER_PICKUP	(1<<1)
#define SF_WEAPON_NO_PHYSCANNON_PUNT (1<<2)
#define SF_WEAPON_NO_MOTION (1<<3)

// Put this in your derived class definition to declare it's activity table
// UNDONE: Cascade these?
#define DECLARE_ACTTABLE()		static acttable_t m_acttable[];\
	acttable_t *ActivityList( void );\
	int ActivityListCount( void );

// You also need to include the activity table itself in your class' implementation:
// e.g.
//	acttable_t	CWeaponStunstick::m_acttable[] = 
//	{
//		{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, TRUE },
//	};
//
// The stunstick overrides the ACT_MELEE_ATTACK1 activity, replacing it with ACT_MELEE_ATTACK_SWING.
// This animation is required for this weapon's operation.
//

// Put this after your derived class' definition to implement the accessors for the
// activity table.
// UNDONE: Cascade these?
#define IMPLEMENT_ACTTABLE(className) \
	acttable_t *className::ActivityList( void ) { return m_acttable; } \
	int className::ActivityListCount( void ) { return ARRAYSIZE(m_acttable); } \

typedef struct
{
	int			baseAct;
	int			weaponAct;
	bool		required;
} acttable_t;

class CHudTexture;
class Color;

namespace vgui2
{
	typedef unsigned long HFont;
}

// -----------------------------------------
//	Vector cones
// -----------------------------------------
// VECTOR_CONE_PRECALCULATED - this resolves to vec3_origin, but adds some
// context indicating that the person writing the code is not allowing
// FireBullets() to modify the direction of the shot because the shot direction
// being passed into the function has already been modified by another piece of
// code and should be fired as specified. See GetActualShotTrajectory(). 

// NOTE: The way these are calculated is that each component == sin (degrees/2)
#define VECTOR_CONE_PRECALCULATED	vec3_origin
#define VECTOR_CONE_1DEGREES		Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES		Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES		Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES		Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES		Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES		Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES		Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES		Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES		Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES		Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES		Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES		Vector( 0.17365, 0.17365, 0.17365 )

//-----------------------------------------------------------------------------
// Purpose: Base weapon class, shared on client and server
//-----------------------------------------------------------------------------

#define BASECOMBATWEAPON_DERIVED_FROM		CBaseAnimating

//-----------------------------------------------------------------------------
// Collect trace attacks for weapons that fire multiple projectiles per attack that also penetrate
//-----------------------------------------------------------------------------
class CDmgAccumulator
{
public:
	CDmgAccumulator( void );
	~CDmgAccumulator();

#ifdef GAME_DLL
	virtual void Start( void ) { m_bActive = true; }
	virtual void AccumulateMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );
	virtual void Process( void );

private:
	CTakeDamageInfo					m_updatedInfo;
	CUtlMap< int, CTakeDamageInfo >	m_TargetsDmgInfo;
#endif	// GAME_DLL

private:
	bool							m_bActive;

};

//-----------------------------------------------------------------------------
// Purpose: Client side rep of CBaseTFCombatWeapon 
//-----------------------------------------------------------------------------
// Hacky
class CBaseCombatWeapon : public BASECOMBATWEAPON_DERIVED_FROM
{
public:
	DECLARE_CLASS( CBaseCombatWeapon, BASECOMBATWEAPON_DERIVED_FROM );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

							CBaseCombatWeapon();
	virtual 				~CBaseCombatWeapon();

	virtual bool			IsBaseCombatWeapon( void ) const { return true; }
	virtual CBaseCombatWeapon *MyCombatWeaponPointer( void ) { return this; }

	// A derived weapon class should return true here so that weapon sounds, etc, can
	//  apply the proper filter
	virtual bool			IsPredicted( void ) const { return false; }

	virtual void			Spawn( void );
	virtual void			Precache( void );

	virtual void			MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual void			Equip( CBaseCombatCharacter *pOwner );
	virtual void			Drop( const Vector &vecVelocity );

	virtual	int				UpdateClientData( CBasePlayer *pPlayer );

	virtual bool			CanBeSelected( void );
	virtual bool			VisibleInWeaponSelection( void );
	virtual bool			HasAmmo( void );

	// Weapon Pickup For Player
	virtual void			SetPickupTouch( void );
	virtual void 			DefaultTouch( CBaseEntity *pOther );	// default weapon touch

	// Weapon client handling
	virtual bool			SendWeaponAnim( int iActivity );
	virtual void			SendViewModelAnim( int nSequence );
	float					GetViewModelSequenceDuration();	// Return how long the current view model sequence is.
	bool					IsViewModelSequenceFinished( void ); // Returns if the viewmodel's current animation is finished

	// BB2
	virtual bool			CheckShouldReload(CBasePlayer *pOwner);
	virtual bool			CanRespawnWeapon() { return true; }
	virtual bool			CanPickupWeaponAsAmmo() { return false; }
	virtual void			SetViewModel();
	 
	virtual bool			HasWeaponIdleTimeElapsed( void );
	virtual void			SetWeaponIdleTime( float time );
	virtual float			GetWeaponIdleTime( void );

	// Weapon selection
	virtual bool			HasAnyAmmo( void );							// Returns true is weapon has ammo
	void					GiveDefaultAmmo( void );
	
	virtual bool			CanHolster( void );		// returns true if the weapon can be holstered
	virtual bool			DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity );
	virtual bool			CanDeploy( void );			// return true if the weapon's allowed to deploy
	virtual bool			Deploy( void );								// returns true is deploy was successful
	virtual bool			Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool            FullHolster(void);
	virtual void			SetWeaponVisible( bool visible );
	virtual bool			IsWeaponVisible( void );
	virtual void			OnActiveStateChanged( int iOldState ) { return; }
	virtual bool			HolsterOnDetach() { return false; }
	virtual bool			IsHolstered(){ return false; }
	virtual void			Detach() {}

	// Weapon behaviour
	virtual void            GenericBB2Animations( void );
	virtual void            HandleWeaponSelectionTime(void);
	virtual void			ItemPreFrame( void );					// called each frame by the player PreThink
	virtual void			ItemPostFrame( void );					// called each frame by the player PostThink
	virtual void			ItemBusyFrame( void );					// called each frame by the player PostThink, if the player's not ready to attack yet
	virtual void			ItemHolsterFrame( void ) {};			// called each frame by the player PreThink, if the weapon is holstered
	virtual void			WeaponIdle( void );						// called when no buttons pressed
	virtual void			HandleFireOnEmpty(void); // Called when they have the attack button down
																	// but they are out of ammo. The default implementation
																	// either reloads, switches weapons, or plays an empty sound.

	virtual int GetReloadActivity(bool bCanDoEmpty = true);
	virtual const char *GetMuzzleflashAttachment(bool bPrimaryAttack) { return "muzzle"; }

	// Each wep should override this one to tell us how many bullets before this wep sort of overheats?
	virtual int GetOverloadCapacity();
	virtual void DoWeaponFX( void );
	virtual void OnWeaponOverload(void) { }

	// Allow weapon to respawn back at orig pos after drop.
	void ForceOriginalPosition();

#ifdef CLIENT_DLL
	virtual void			CreateMove( float flInputSampleTime, CUserCmd *pCmd, const QAngle &vecOldViewAngles ) {}
	virtual int				CalcOverrideModelIndex() OVERRIDE;
#endif

	// Reloading
	virtual	void			CheckReload( void );
	virtual void			FinishReload( void );
	virtual void			AbortReload( void );
	virtual bool			Reload( void );
	bool					DefaultReload(int iClipSize, int iActivity);
	bool					ReloadsSingly( void ) const;

	// Weapon firing
	virtual void			PrimaryAttack( void );						// do "+ATTACK"
	virtual void			SecondaryAttack( void ) { return; }			// do "+ATTACK2"

	// BB2
	virtual const char *GetParticleEffect(int iType, bool bThirdperson = false);
	virtual bool UsesEmptyAnimation() { return false; }
	virtual int GetWeaponType(void) { return WEAPON_TYPE_RIFLE; }
	virtual bool IsAkimboWeapon(void) { return false; }
	virtual const char *GetAmmoEntityLink(void) { return ""; }
	virtual int GetUniqueWeaponID() { return WEAPON_ID_NONE; }

	// Melee Base:
	virtual bool CanPerformMeleeAttacks() { return true; }
	virtual void MeleeAttackStart(int type);
	virtual void MeleeAttackEnd(void);
	virtual void MeleeAttackUpdate(void);
	virtual void MeleeAttackTrace(void);
	virtual bool ImpactWater(const Vector &start, const Vector &end);
	virtual float GetRange(void);
	virtual float GetDamageForActivity(Activity hitActivity);
	virtual int GetMeleeDamageType(void) { return DMG_SLASH; }

#ifndef CLIENT_DLL
	virtual bool WantsLagCompensation(const CBaseEntity *pEntity);
	virtual bool CanHitThisTarget(int index);
	virtual void StruckTarget(int index) { m_pEnemiesStruck.AddToTail(index); }
	virtual int GetMeleeSkillFlags(void) { return 0; }

	float m_flLastTraceTime;
	CUtlVector<int> m_pEnemiesStruck;
#endif
 
	CNetworkVar(int, m_iMeleeAttackType);

	virtual void StartHolsterSequence(); // Starts holster, Holster() just hides and finalizes the prev wep state.

	// Animation related
	void    PlayAnimation( int iActivity );

	// Firing animations
	virtual Activity		GetPrimaryAttackActivity(void);
	virtual Activity		GetSecondaryAttackActivity(void);
	virtual Activity		GetDrawActivity(void);
	virtual Activity		GetHolsterActivity(void);
	virtual float			GetDefaultAnimSpeed(void) { return 1.0; }

	// Bullet launch information
	virtual const Vector&	GetBulletSpread(void);
	virtual float			GetFireRate( void );
	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }
	virtual float			GetMinRestTime() { return 0.2; }
	virtual float			GetMaxRestTime() { return 0.4; }
	virtual int				GetRandomBurst() { return random->RandomInt( GetMinBurst(), GetMaxBurst() ); }
	virtual void			WeaponSound(WeaponSound_t sound_type, float soundtime = 0.0f, bool bSkipPrediction = false);
	virtual void			StopWeaponSound( WeaponSound_t sound_type );

	virtual void			SetActivity( Activity act, float duration );
	inline void				SetActivity( Activity eActivity ) { m_Activity = eActivity; }
	inline Activity			GetActivity( void ) { return m_Activity; }

	virtual void			AddViewKick( void );	// Add in the view kick for the weapon

	CBaseCombatCharacter	*GetOwner() const;
	void					SetOwner( CBaseCombatCharacter *owner );
	virtual void			OnPickedUp( CBaseCombatCharacter *pNewOwner );

	virtual void			AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles ) {};
	virtual float			CalcViewmodelBob( void ) { return 0.0f; };

	// Returns information about the various control panels
	virtual void 			GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	virtual void			GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );

	virtual bool			ShouldShowControlPanels( void ) { return true; }

	void					Lock( float lockTime, CBaseEntity *pLocker );
	bool					IsLocked( CBaseEntity *pAsker );

public:

	// Weapon info accessors for data in the weapon's data file
	const FileWeaponInfo_t	&GetWpnData( void ) const;
	virtual const char		*GetViewModel(void) const;
	virtual const char		*GetWorldModel( void ) const;
	virtual int				GetMaxClip( void ) const;
	virtual int				GetDefaultClip( void ) const;
	virtual int				GetWeight( void ) const;
	virtual int				GetWeaponFlags( void ) const;
	virtual int				GetSlot( void ) const;
	virtual char const		*GetName( void ) const;
	virtual char const		*GetPrintName( void ) const;
	virtual char const		*GetShootSound( int iIndex ) const;
	virtual int				GetRumbleEffect() const;
	virtual bool			UsesClipsForAmmo( void ) const;
	bool					IsMeleeWeapon() const;
	float                   GetSpecialDamage() const;

	virtual const char		*GetAmmoTypeName(void) { return NULL; }
	virtual int				GetAmmoMaxCarry(void) { return 0; }

	int				GetAmmoTypeID(void) { return m_iAmmoType.Get(); }
	void			SetAmmoTypeID(int id) { m_iAmmoType.Set(id); }

	char const		*GetAttachmentLink( void ) const;
	const Vector &GetAttachmentPositionOffset( void ) const;
	const QAngle &GetAttachmentAngleOffset( void ) const;

	virtual int				Clip() { return m_iClip; }

	virtual bool			DidFirePrimary(void) { return true; }

	virtual void SetupWeaponRanges(void);
	virtual void AffectedByPlayerSkill(int skill) {}

	// Ammo quantity queries for weapons that do not use clips. These are only
	// used to determine how much ammo is in a weapon that does not have an owner.
	// That is, a weapon that's on the ground for the player to get ammo out of.
	int GetAmmoCount() { return m_iAmmoCount.Get(); }
	void SetAmmoCount(int count) { m_iAmmoCount.Set(count); }
	void RemoveAmmo(int count);
	int GiveAmmo(int count, bool bSuppressSound = false);

#ifndef CLIENT_DLL
	void SetWeaponStartAmmo(int count) { m_iDefaultAmmoCount = count; GiveDefaultAmmo(); }
	int GetWeaponStartAmmo() { return m_iDefaultAmmoCount; }
#endif

	virtual CHudTexture const	*GetSpriteActive( void ) const;
	virtual CHudTexture const	*GetSpriteInactive( void ) const;
	virtual CHudTexture const	*GetSpriteAmmo( void ) const;

	virtual Activity		ActivityOverride(Activity baseAct);
	virtual	acttable_t*		ActivityList( void ) { return NULL; }
	virtual	int				ActivityListCount( void ) { return 0; }

	virtual void			Activate( void );

	virtual bool ShouldUseLargeViewModelVROverride() { return false; }
public:
// Server Only Methods
#if !defined( CLIENT_DLL )

	DECLARE_DATADESC();
	virtual void			FallInit( void );						// prepare to fall to the ground
	virtual void			FallThink( void );						// make the weapon fall to the ground after spawning

	// Weapon spawning
	bool m_bSuppressRespawn;
	virtual void			Materialize( void );					// make a weapon visible and tangible
	void					AttemptToMaterialize( void );			// see if the game rules will let the weapon become visible and tangible
	virtual void			CheckRespawn( void );					// see if this weapon should respawn after being picked up
	CBaseEntity				*Respawn ( void );						// copy a weapon

	// Weapon dropping / destruction
	virtual void			Delete( void );
	void					DestroyItem( void );
	virtual void			Kill( void );

	virtual int				CapabilitiesGet( void ) { return 0; }
	virtual	int				ObjectCaps( void );
	
	// Returns bits for	weapon conditions
	virtual bool			WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	
	virtual	int				WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual	int				WeaponRangeAttack2Condition( float flDot, float flDist );
	virtual	int				WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual	int				WeaponMeleeAttack2Condition( float flDot, float flDist );

	virtual void			Operator_FrameUpdate( CBaseCombatCharacter  *pOperator );
	virtual void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual void			Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary ) { return; }
	// NOTE: This should never be called when a character is operating the weapon.  Animation events should be
	// routed through the character, and then back into CharacterAnimEvent() 
	void					HandleAnimEvent( animevent_t *pEvent );

	virtual int				UpdateTransmitState( void );

	void					InputHideWeapon( inputdata_t &inputdata );
	void                    Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual CDmgAccumulator	*GetDmgAccumulator( void ) { return NULL; }

// Client only methods
#else

	virtual void			BoneMergeFastCullBloat( Vector &localMins, Vector &localMaxs, const Vector &thisEntityMins, const Vector &thisEntityMaxs  ) const;

	virtual bool			OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options ) 
	{ 
		return false; 
	}

	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType();
	virtual void			SetDormant( bool bDormant );
	virtual void			OnDataChanged( DataUpdateType_t updateType );

	virtual unsigned char	GetClientSideFade(void);

	virtual void			RestartParticleEffect( void ) {}

	virtual void			ViewModelDrawn( CBaseViewModel *pViewModel );
	// Get the position that bullets are seen coming out. Note: the returned values are different
	// for first person and third person.
	bool					GetShootPosition( Vector &vOrigin, QAngle &vAngles );
	virtual bool			ShouldDrawCrosshair( void ) { return true; }
	
	// Weapon state checking
	virtual bool			IsCarriedByLocalPlayer( void );
	virtual bool			ShouldDrawUsingViewModel( void );
	virtual bool			IsActiveByLocalPlayer( void );

	bool					IsBeingCarried() const;

	// Is the carrier alive?
	bool					IsCarrierAlive() const;

	// Returns the aiment render origin + angles
	virtual int				DrawModel( int flags );
	virtual int				InternalDrawModel(int flags);
	virtual bool			ShouldDraw( void );
	virtual bool			ShouldDrawPickup( void );
	virtual void			HandleInput( void ) { return; };
	virtual void			OverrideMouseInput( float *x, float *y ) { return; };
	virtual int				KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding ) { return 1; }
	virtual bool			AddLookShift( void ) { return true; };

	virtual void			GetViewmodelBoneControllers(C_BaseViewModel *pViewModel, float controllers[MAXSTUDIOBONECTRLS]) { return; }

	virtual void			NotifyShouldTransmit( ShouldTransmitState_t state );
	WEAPON_FILE_INFO_HANDLE	GetWeaponFileInfoHandle() { return m_hWeaponFileInfo; }

	virtual int				GetWorldModelIndex( void );

	virtual void			GetToolRecordingState( KeyValues *msg );

	virtual void			GetWeaponCrosshairScale( float &flScale ) { flScale = 1.f; }

	// BB2 Fixes for mirror rendering:
	virtual bool EnsureCorrectRenderingModel();
	virtual bool ShouldDoAnimEvents();

	// Viewmodel overriding
	virtual bool			ViewModel_IsTransparent(void) { return IsTransparent(); }
	virtual bool			ViewModel_IsUsingFBTexture(void) { return UsesPowerOfTwoFrameBufferTexture(); }
	virtual bool			IsOverridingViewmodel(void) { return false; };
	virtual int				DrawOverriddenViewmodel(C_BaseViewModel *pViewmodel, int flags) { return 0; };
	bool					WantsToOverrideViewmodelAttachments(void) { return false; }

#endif // End client-only methods

	virtual void			HideThink( void );

private:
	Vector GetBulletConeByAccLevel(void);
	typedef CHandle< CBaseCombatCharacter > CBaseCombatCharacterHandle;
	CNetworkVar( CBaseCombatCharacterHandle, m_hOwner );				// Player carrying this weapon

public:

	// Weapon firing
	CNetworkVar( float, m_flNextPrimaryAttack );						// soonest time ItemPostFrame will call PrimaryAttack
	CNetworkVar( float, m_flNextSecondaryAttack );					// soonest time ItemPostFrame will call SecondaryAttack
	CNetworkVar(float, m_flMeleeCooldown);
	CNetworkVar(float, m_flNextBashAttack);                         // soonest time you will be able to call bash.
	CNetworkVar( float, m_flTimeWeaponIdle );							// soonest time ItemPostFrame will call WeaponIdle
	// BB2
	CNetworkVar( float, m_flHolsteredTime );
	CNetworkVar( bool, m_bWantsHolster );
	CNetworkVar(bool, m_bIsBloody);

#ifndef CLIENT_DLL
	void SetBloodState(bool value);
#endif

	// Weapon state
	bool					m_bInReload;			// Are we in the middle of a reload;

	// Weapon art
	CNetworkVar( int, m_iViewModelIndex );
	CNetworkVar( int, m_iWorldModelIndex );
	// Sounds
	float					m_flNextEmptySoundTime;				// delay on empty sound playing

	Activity				GetIdealActivity( void ) { return m_IdealActivity; }
	int						GetIdealSequence( void ) { return m_nIdealSequence; }

	bool					SetIdealActivity( Activity ideal );
	void					MaintainIdealActivity( void );

	// Viewpunch / Viewkick logic:
	virtual QAngle GetViewKickAngle(void);

	CNetworkVar(float, m_flViewKickTime); // Will be set everytime you do a viewkick, when this becomes less than curtime we reset a 'bullet' shooting counter, we will increase the X kick as we shoot to move up as we shoot.
	CNetworkVar(float, m_flViewKickPenalty); // As you shoot this increases and adds to you kick view.
	CNetworkVar(int, m_iShotsFired);

private:
	Activity				m_Activity;
	int						m_nIdealSequence;
	Activity				m_IdealActivity;

	CNetworkVar(int, m_iAmmoCount);

#ifndef CLIENT_DLL
	bool m_bCanRemoveWeapon;
#endif

public:

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_nNextThinkTick );

	int WeaponState() const { return m_iState; }

	// Weapon data
	CNetworkVar( int, m_iState );				// See WEAPON_* definition
	string_t				m_iszName;				// Classname of this weapon.
	CNetworkVar( int, m_iAmmoType );		// "primary" ammo index into the ammo info array 
	CNetworkVar(int, m_iClip);				// number of shots left in the primary weapon clip, -1 it not used

	bool					m_bFiresUnderwater;		// true if this weapon can fire underwater
	float					m_fMinRange1;			// What's the closest this weapon can be used?
	float					m_fMinRange2;			// What's the closest this weapon can be used?
	float					m_fMaxRange1;			// What's the furthest this weapon can be used?
	float					m_fMaxRange2;			// What's the furthest this weapon can be used?
	bool					m_bReloadsSingly;		// True if this weapon reloads 1 round at a time
	float					m_fFireDuration;		// The amount of time that the weapon has sustained firing

	float					m_flUnlockTime;
	EHANDLE					m_hLocker;				// Who locked this weapon.

private:
	WEAPON_FILE_INFO_HANDLE	m_hWeaponFileInfo;
	
	// Server only
#if !defined( CLIENT_DLL )

	// Outputs
protected:
	COutputEvent			m_OnPlayerUse;		// Fired when the player uses the weapon.
	COutputEvent			m_OnPlayerPickup;	// Fired when the player picks up the weapon.
	COutputEvent			m_OnNPCPickup;		// Fired when an NPC picks up the weapon.

	int                     m_iDefaultAmmoCount; // The ammo this weapon spawns with.

#else // Client .dll only
	// Allow weapons resource to access m_hWeaponFileInfo directly
	friend class			WeaponsResource;

protected:	
	int						m_iOldState;

#endif // End Client .dll only
};

#endif // COMBATWEAPON_SHARED_H

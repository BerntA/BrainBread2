//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side CBasePlayer.
//
//			- Manages the player's flashlight effect.
//
//=============================================================================//

#ifndef C_BASEPLAYER_H
#define C_BASEPLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_playerlocaldata.h"
#include "c_basecombatcharacter.h"
#include "PlayerState.h"
#include "usercmd.h"
#include "shareddefs.h"
#include "timedevent.h"
#include "smartptr.h"
#include "fx_water.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "c_env_fog_controller.h"
#include "igameevents.h"
#include "GameEventListener.h"

class C_BaseCombatWeapon;
class C_BaseViewModel;
class CFlashlightEffect;
class C_EconWearable;

extern bool g_bShouldRenderLocalPlayerExternally;

class C_CommandContext
{
public:
	bool			needsprocessing;

	CUserCmd		cmd;
	int				command_number;
};

class C_PredictionError
{
public:
	float	time;
	Vector	error;
};

#define CHASE_CAM_DISTANCE_MIN	16.0f
#define CHASE_CAM_DISTANCE_MAX	96.0f
#define WALL_OFFSET				6.0f

bool IsInFreezeCam( void );

//-----------------------------------------------------------------------------
// Purpose: Base Player class
//-----------------------------------------------------------------------------
class C_BasePlayer : public C_BaseCombatCharacter, public CGameEventListener
{
public:
	DECLARE_CLASS( C_BasePlayer, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_BasePlayer();
	virtual			~C_BasePlayer();

	virtual void	Spawn( void );
	virtual void	SharedSpawn(); // Shared between client and server.
	virtual void	SetNewSolidFlags( bool bNonSolid );
	virtual bool	GetSteamID( CSteamID *pID );

	// IClientEntity overrides.
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	PostDataUpdate( DataUpdateType_t updateType );
	
	virtual void	ReceiveMessage( int classID, bf_read &msg );

	virtual void	AddEntity( void );

	virtual void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual void	GetToolRecordingState( KeyValues *msg );

	virtual float GetPlayerMaxSpeed();

	C_BaseViewModel		*GetViewModel(bool bObserverOK = true);
	C_BaseCombatWeapon	*GetActiveWeapon( void ) const;
	const char			*GetTracerType( void );

	// View model prediction setup
	virtual void		CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual void		CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles);
	
	// Handle view smoothing when going up stairs
	void				SmoothViewOnStairs( Vector& eyeOrigin );
	virtual float		CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);
	void				CalcViewRoll( QAngle& eyeAngles );
	void				CreateWaterEffects( void );

	virtual void			SetPlayerUnderwater( bool state );
	void					UpdateUnderwaterState( void );
	bool					IsPlayerUnderwater( void ) { return m_bPlayerUnderwater; }

	virtual Vector			Weapon_ShootPosition();
	virtual void			Weapon_DropPrimary( void ) {}

	virtual Vector			GetAutoaimVector(void);

	// Input handling
	virtual bool	CreateMove(float flInputSampleTime, CUserCmd *pCmd, bool bFakeInput);
	virtual void	AvoidPhysicsProps( CUserCmd *pCmd );
	
	virtual void	PlayerUse( void );
	CBaseEntity		*FindUseEntity( void );
	virtual bool	IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps );

	// Data handlers
	virtual bool	IsPlayer( void ) const { return true; }
	virtual int		GetHealth() const { return m_iHealth; }
	virtual int     GetMaxHealth() const { return m_iMaxHealth; }
	int GetArmorValue() { return m_ArmorValue; }

	// observer mode
	virtual int			GetObserverMode() const;
	void				SetObserverMode ( int iNewMode );
	virtual CBaseEntity	*GetObserverTarget() const;
	void			SetObserverTarget( EHANDLE hObserverTarget );

	bool			AudioStateIsUnderwater( Vector vecMainViewOrigin );

	bool IsObserver() const;
	bool IsHLTV() const;
	bool IsReplay() const;
	void ResetObserverMode();
	bool IsBot( void ) const { return false; }

	// Eye position..
	virtual Vector		 EyePosition();
	virtual const QAngle &EyeAngles();		// Direction of eyes
	void				 EyePositionAndVectors( Vector *pPosition, Vector *pForward, Vector *pRight, Vector *pUp );
	virtual const QAngle &LocalEyeAngles();		// Direction of eyes
	
	// This can be overridden to return something other than m_pRagdoll if the mod uses separate 
	// entities for ragdolls.
	virtual IRagdoll* GetRepresentativeRagdoll() const;

	// override the initial bone position for ragdolls
	virtual bool GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt ) OVERRIDE;
	
	// Returns eye vectors
	void			EyeVectors( Vector *pForward, Vector *pRight = NULL, Vector *pUp = NULL );

	// Team handlers
	virtual void	TeamChange( int iNewTeam );

	// Flashlight
	void	Flashlight( void );
	void	UpdateFlashlight( void );

	// Weapon selection code
	virtual bool				IsAllowedToSwitchWeapons( void ) { return !IsObserver(); }
	virtual C_BaseCombatWeapon	*GetActiveWeaponForSelection( void );

	// Returns the view model if this is the local player. If you're in third person or 
	// this is a remote player, it returns the active weapon
	// (and its appropriate left/right weapon if this is TF2).
	virtual C_BaseAnimating*	GetRenderedWeaponModel();

	virtual bool				IsOverridingViewmodel( void ) { return false; };
	virtual int					DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags ) { return 0; };

	virtual float				GetDefaultAnimSpeed( void ) { return 1.0; }

	void						SetMaxSpeed( float flMaxSpeed ) { m_flMaxspeed = flMaxSpeed; }
	float						MaxSpeed() const		{ return m_flMaxspeed; }

	void SetMaxAirSpeed(float flMaxAirSpeed) { m_flMaxAirSpeed = flMaxAirSpeed; }
	float MaxAirSpeed() const { return m_flMaxAirSpeed; }

	// Should this object cast shadows?
	virtual ShadowType_t		ShadowCastType() { return SHADOWS_NONE; }

	virtual bool				ShouldReceiveProjectedTextures( int flags )
	{
		return false;
	}


	bool						IsLocalPlayer( void ) const;

	// Global/static methods
	virtual void				ThirdPersonSwitch( bool bThirdperson );
	static bool					LocalPlayerInFirstPersonView();
	static bool					ShouldDrawLocalPlayer();
	static C_BasePlayer			*GetLocalPlayer( void );
	int							GetUserID( void );
	virtual bool				CanSetSoundMixer( void );
	virtual int					GetVisionFilterFlags( bool bWeaponsCheck = false ) { return 0x00; }
	bool						HasVisionFilterFlags( int nFlags, bool bWeaponsCheck = false ) { return ( GetVisionFilterFlags( bWeaponsCheck ) & nFlags ) == nFlags; }
	virtual void				CalculateVisionUsingCurrentFlags( void ) {}

	// Specific queries about this player.
	bool						InFirstPersonView();
	bool						ShouldDrawThisPlayer();

	// Called by the view model if its rendering is being overridden.
	virtual bool				ViewModel_IsTransparent( void );
	virtual bool				ViewModel_IsUsingFBTexture( void );

#if !defined( NO_ENTITY_PREDICTION )
	void						AddToPlayerSimulationList( C_BaseEntity *other );
	void						SimulatePlayerSimulatedEntities( void );
	void						RemoveFromPlayerSimulationList( C_BaseEntity *ent );
	void						ClearPlayerSimulationList( void );
#endif

	virtual void				PhysicsSimulate( void );
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const { return MASK_PLAYERSOLID; }

	// Prediction stuff
	virtual bool				ShouldPredict( void );

	virtual void				PreThink( void );
	virtual void				PostThink( void );

	virtual void				ItemPreFrame( void );
	virtual void				ItemPostFrame( void );

	// Movement:
	virtual float GetPlayerSpeed() { return 240.0f; }
	virtual float GetLeapLength() { return 0.0f; }
	virtual float GetJumpHeight() { return 20.0f; }

	// Armor
	int m_ArmorValue;

	bool m_bIsSelectingWeapons;

	virtual void				AbortReload( void );

	virtual void				SelectLastItem(void);
	virtual void				Weapon_SetLast( C_BaseCombatWeapon *pWeapon );
	virtual void                Weapon_SetNext( C_BaseCombatWeapon *pWeapon );
	virtual void                Weapon_DeployNextWeapon(void);
	virtual bool				Weapon_ShouldSetLast( C_BaseCombatWeapon *pOldWeapon, C_BaseCombatWeapon *pNewWeapon ) { return true; }
	virtual bool				Weapon_ShouldSelectItem( C_BaseCombatWeapon *pWeapon );
	virtual	bool				Weapon_Switch(C_BaseCombatWeapon *pWeapon, bool bWantDraw = false);		// Switch to given weapon if has ammo (false if failed)
	virtual C_BaseCombatWeapon *GetLastWeapon( void ) { return m_hLastWeapon.Get(); }
	virtual C_BaseCombatWeapon *GetNextWeapon( void ) { return m_hNextWeapon.Get(); }
	virtual void 				SelectItem(const char *pstr);

	virtual void				UpdateClientData( void );

	virtual float				GetFOV( void );	
	int							GetDefaultFOV( void ) const;
	virtual bool				IsZoomed( void )	{ return false; }
	bool						SetFOV(int FOV, float zoomRate = 0.0f, int iZoomStart = 0);

	float						GetFOVDistanceAdjustFactor();

	virtual void				ViewPunch( const QAngle &angleOffset );
	void						ViewPunchReset( float tolerance = 0 );

	void						UpdateButtonState( int nUserCmdButtonMask );
	int							GetImpulse( void ) const;

	virtual void				Simulate();

	virtual bool				ShouldInterpolate();

	virtual bool				ShouldDraw();
	virtual int					DrawModel( int flags );

	// Called when not in tactical mode. Allows view to be overriden for things like driving a tank.
	virtual void				OverrideView( CViewSetup *pSetup );

	// returns the player name
	const char *				GetPlayerName();
	virtual const Vector		GetPlayerMins( void ) const; // uses local player
	virtual const Vector		GetPlayerMaxs( void ) const; // uses local player

	// Is the player dead?
	bool				IsPlayerDead();

	C_BaseEntity				*GetUseEntity();

	float					GetTimeBase( void ) const;
	float					GetFinalPredictedTime() const;

	bool					IsInVGuiInputMode() const;
	bool					IsInViewModelVGuiInputMode() const;

	C_CommandContext		*GetCommandContext();

	// Get the command number associated with the current usercmd we're running (if in predicted code).
	int CurrentCommandNumber() const;
	const CUserCmd *GetCurrentUserCommand() const;

	const QAngle& GetPunchAngle();
	void SetPunchAngle( const QAngle &angle );

	float					GetWaterJumpTime() const;
	void					SetWaterJumpTime( float flWaterJumpTime );
	float					GetSwimSoundTime( void ) const;
	void					SetSwimSoundTime( float flSwimSoundTime );

	float					GetDeathTime( void ) { return m_flDeathTime; }

	void		SetPreviouslyPredictedOrigin( const Vector &vecAbsOrigin );
	const Vector &GetPreviouslyPredictedOrigin() const;

	// CS wants to allow small FOVs for zoomed-in AWPs.
	virtual float GetMinFOV() const;

	virtual void DoMuzzleFlash();

	virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity  );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual surfacedata_t * GetFootstepSurface( const Vector &origin, const char *surfaceName );
	virtual void GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual const char *GetOverrideStepSound( const char *pszBaseStepSoundName ) { return pszBaseStepSoundName; }

	virtual void OnEmitFootstepSound( const CSoundParameters& params, const Vector& vecOrigin, float fVolume ) {}

	// Called by prediction when it detects a prediction correction.
	// vDelta is the line from where the client had predicted the player to at the usercmd in question,
	// to where the server says the client should be at said usercmd.
	void NotePredictionError( const Vector &vDelta );
	
	// Called by the renderer to apply the prediction error smoothing.
	void GetPredictionErrorSmoothingVector( Vector &vOffset ); 

	virtual void ExitLadder() {}
	surfacedata_t *GetLadderSurface( const Vector &origin );

	surfacedata_t *GetSurfaceData( void ) { return m_pSurfaceData; }

	virtual	IMaterial *GetHeadLabelMaterial( void );

	// Fog
	fogparams_t				*GetFogParams( void ) { return &m_CurrentFog; }
	void					FogControllerChanged( bool bSnap );
	void					UpdateFogController( void );
	void					UpdateFogBlend( void );
	float					GetFOVTime( void ){ return m_flFOVTime; }

	virtual bool			CanUseFirstPersonCommand( void ){ return true; }
	
protected:
	fogparams_t				m_CurrentFog;
	EHANDLE					m_hOldFogController;

public:
	int m_StuckLast;
	
	// Data for only the local player
	CNetworkVarEmbedded( CPlayerLocalData, m_Local );

	// Data common to all other players, too
	CPlayerState			pl;

	// Player FOV values
	int						m_iFOV;				// field of view
	int						m_iFOVStart;		// starting value of the FOV changing over time (client only)
	float					m_flFOVTime;		// starting time of the FOV zoom
	int						m_iDefaultFOV;		// default FOV if no other zooms are occurring

	int				m_afButtonLast;
	int				m_afButtonPressed;
	int				m_afButtonReleased;

	int				m_nButtons;

	CUserCmd		*m_pCurrentCommand;

	// Movement constraints
	EHANDLE			m_hConstraintEntity;
	Vector			m_vecConstraintCenter;
	float			m_flConstraintRadius;
	float			m_flConstraintWidth;
	float			m_flConstraintSpeedFactor;

protected:

	void				CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void		CalcObserverView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual Vector		GetChaseCamViewOffset( CBaseEntity *target );
	void				CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void		CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	virtual float		GetDeathCamInterpolationTime();

	virtual void		CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	void				CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov);
	virtual void		CalcFreezeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

	// Check to see if we're in vgui input mode...
	void DetermineVguiInputMode( CUserCmd *pCmd );

	// Used by prediction, sets the view angles for the player
	virtual void SetLocalViewAngles( const QAngle &viewAngles );
	virtual void SetViewAngles( const QAngle& ang );

	// used by client side player footsteps 
	surfacedata_t* GetGroundSurface();
	
	virtual void	FireGameEvent( IGameEvent *event );

protected:

// DATA
	int				m_iObserverMode;	// if in spectator mode != 0
	EHANDLE			m_hObserverTarget;	// current observer target
	float			m_flObserverChaseDistance; // last distance to observer traget
	Vector			m_vecFreezeFrameStart;
	float			m_flFreezeFrameStartTime;	// Time at which we entered freeze frame observer mode
	float			m_flFreezeFrameDistance;
	bool			m_bWasFreezeFraming; 
	float			m_flDeathTime;		// last time player died

	float			m_flStepSoundTime;
	bool			m_IsFootprintOnLeft;

private:
	// Make sure no one calls this...
	C_BasePlayer& operator=( const C_BasePlayer& src );
	C_BasePlayer( const C_BasePlayer & ); // not defined, not accessible

	EHANDLE			m_hUseEntity;
	
	float			m_flMaxspeed;
	float m_flMaxAirSpeed;

	CInterpolatedVar< Vector >	m_iv_vecViewOffset;

	// Not replicated
	Vector			m_vecWaterJumpVel;
	float			m_flWaterJumpTime;  // used to be called teleport_time
	int				m_nImpulse;

	float			m_flSwimSoundTime;
	Vector			m_vecLadderNormal;
	
	QAngle			m_vecOldViewAngles;

	bool			m_bWasFrozen;
	int				m_flPhysics;

	int				m_nTickBase;
	int				m_nFinalPredictedTick;

	EHANDLE			m_pCurrentVguiScreen;

	// Player flashlight dynamic light pointers
	CFlashlightEffect *m_pFlashlight;

	typedef CHandle<C_BaseCombatWeapon> CBaseCombatWeaponHandle;
	CNetworkVar( CBaseCombatWeaponHandle, m_hLastWeapon );
	CNetworkVar( CBaseCombatWeaponHandle, m_hNextWeapon );

#if !defined( NO_ENTITY_PREDICTION )
	CUtlVector< CHandle< C_BaseEntity > > m_SimulatedByThisPlayer;
#endif

	CHandle<C_BaseViewModel> m_hViewModel;
	
	float					m_flOldPlayerZ;
	float					m_flOldPlayerViewOffsetZ;

	C_CommandContext		m_CommandContext;

	// For underwater effects
	float							m_flWaterSurfaceZ;
	bool							m_bResampleWaterSurface;
	TimedEvent						m_tWaterParticleTimer;
	CSmartPtr<WaterDebrisEffect>	m_pWaterEmitter;

	bool							m_bPlayerUnderwater;

	friend class CPrediction;

	// HACK FOR TF2 Prediction
	friend class CGameMovement;
	friend class CHL2GameMovement;
	
	// Accessors for gamemovement
	float GetStepSize( void ) const { return m_Local.m_flStepSize; }

	float m_flNextAvoidanceTime;
	float m_flAvoidanceRight;
	float m_flAvoidanceForward;
	float m_flAvoidanceDotForward;
	float m_flAvoidanceDotRight;

protected:
	virtual bool IsDucked( void ) const { return m_Local.m_bDucked; }
	virtual bool IsDucking( void ) const { return m_Local.m_bDucking; }
	virtual float GetFallVelocity( void ) { return m_Local.m_flFallVelocity; }
	bool ForceSetupBonesAtTimeFakeInterpolation( matrix3x4_t *pBonesOut, float curtimeOffset );

	float m_flLaggedMovementValue;

	// These are used to smooth out prediction corrections. They're most useful when colliding with
	// vphysics objects. The server will be sending constant prediction corrections, and these can help
	// the errors not be so jerky.
	Vector m_vecPredictionError;
	float m_flPredictionErrorTime;
	
	Vector m_vecPreviouslyPredictedOrigin; // Used to determine if non-gamemovement game code has teleported, or tweaked the player's origin

	// Texture names and surface data, used by CGameMovement
	int				m_surfaceProps;
	surfacedata_t*	m_pSurfaceData;
	float			m_surfaceFriction;
	char			m_chTextureType;

	bool			m_bSentFreezeFrame;
	float			m_flFreezeZOffset;

	int				m_nForceVisionFilterFlags; // Force our vision filter to a specific setting
	int				m_nLocalPlayerVisionFlags;

private:

	struct StepSoundCache_t
	{
		StepSoundCache_t() : m_usSoundNameIndex( 0 ) {}
		CSoundParameters	m_SoundParameters;
		unsigned short		m_usSoundNameIndex;
	};
	// One for left and one for right side of step
	StepSoundCache_t		m_StepSoundCache[ 2 ];

public:

	float GetLaggedMovementValue(void) { return m_flLaggedMovementValue; }
	void SetOldPlayerZ( float flOld ) { m_flOldPlayerZ = flOld;	}
};

EXTERN_RECV_TABLE(DT_BasePlayer);

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline C_BasePlayer *ToBasePlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

#if _DEBUG
	Assert( dynamic_cast<C_BasePlayer *>( pEntity ) != NULL );
#endif

	return static_cast<C_BasePlayer *>( pEntity );
}

inline C_BaseEntity *C_BasePlayer::GetUseEntity() 
{ 
	return m_hUseEntity;
}

inline bool C_BasePlayer::IsObserver() const 
{ 
	return (GetObserverMode() != OBS_MODE_NONE); 
}

inline int C_BasePlayer::GetImpulse( void ) const 
{ 
	return m_nImpulse; 
}


inline C_CommandContext* C_BasePlayer::GetCommandContext()
{
	return &m_CommandContext;
}

inline int CBasePlayer::CurrentCommandNumber() const
{
	Assert( m_pCurrentCommand );
	return m_pCurrentCommand->command_number;
}

inline const CUserCmd *CBasePlayer::GetCurrentUserCommand() const
{
	Assert( m_pCurrentCommand );
	return m_pCurrentCommand;
}

#endif // C_BASEPLAYER_H

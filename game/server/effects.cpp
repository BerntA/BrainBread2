//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a grab bag of visual effects entities.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "effects.h"
#include "beam_shared.h"
#include "decals.h"
#include "func_break.h"
#include "EntityFlame.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "model_types.h"
#include "player.h"
#include "physics.h"
#include "baseparticleentity.h"
#include "ndebugoverlay.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "env_wind_shared.h"
#include "filesystem.h"
#include "engine/IEngineSound.h"
#include "fire.h"
#include "te_effect_dispatch.h"
#include "Sprite.h"
#include "precipitation_shared.h"
#include "shot_manipulator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_FUNNEL_REVERSE			1 // funnel effect repels particles instead of attracting them.

// UNDONE: This should be client-side and not use TempEnts
class CBubbling : public CBaseEntity
{
public:
	DECLARE_CLASS( CBubbling, CBaseEntity );

	virtual  void	Spawn( void );
	virtual void	Precache( void );

	void	FizzThink( void );

	// Input handlers.
	void	InputActivate( inputdata_t &inputdata );
	void	InputDeactivate( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

	void	InputSetCurrent( inputdata_t &inputdata );
	void	InputSetDensity( inputdata_t &inputdata );
	void	InputSetFrequency( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:

	void TurnOn();
	void TurnOff();
	void Toggle();

	int		m_density;
	int		m_frequency;
	int		m_bubbleModel;
	int		m_state;
};

LINK_ENTITY_TO_CLASS( env_bubbles, CBubbling );

BEGIN_DATADESC( CBubbling )

	DEFINE_KEYFIELD( m_flSpeed, FIELD_FLOAT, "current" ),
	DEFINE_KEYFIELD( m_density, FIELD_INTEGER, "density" ),
	DEFINE_KEYFIELD( m_frequency, FIELD_INTEGER, "frequency" ),

	// Function Pointers
	DEFINE_FUNCTION( FizzThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCurrent", InputSetCurrent ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDensity", InputSetDensity ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFrequency", InputSetFrequency ),

END_DATADESC()

#define SF_BUBBLES_STARTOFF		0x0001

void CBubbling::Spawn( void )
{
	Precache( );
	SetModel( STRING( GetModelName() ) );		// Set size

	// Make it invisible to client
	SetRenderColorA( 0 );

	SetSolid( SOLID_NONE );						// Remove model & collisions

	if ( !HasSpawnFlags(SF_BUBBLES_STARTOFF) )
	{
		SetThink( &CBubbling::FizzThink );
		SetNextThink( gpGlobals->curtime + 2.0 );
		m_state = 1;
	}
	else
	{
		m_state = 0;
	}
}

void CBubbling::Precache( void )
{
	m_bubbleModel = PrecacheModel("sprites/bubble.vmt");			// Precache bubble sprite
}


void CBubbling::Toggle()
{
	if (!m_state)
	{
		TurnOn();
	}
	else
	{
		TurnOff();
	}
}


void CBubbling::TurnOn()
{
	m_state = 1;
	SetThink( &CBubbling::FizzThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CBubbling::TurnOff()
{
	m_state = 0;
	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBubbling::InputActivate( inputdata_t &inputdata )
{
	TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBubbling::InputDeactivate( inputdata_t &inputdata )
{
	TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBubbling::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBubbling::InputSetCurrent( inputdata_t &inputdata )
{
	m_flSpeed = (float)inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBubbling::InputSetDensity( inputdata_t &inputdata )
{
	m_density = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CBubbling::InputSetFrequency( inputdata_t &inputdata )
{
	m_frequency = inputdata.value.Int();

	// Reset think time
	if ( m_state )
	{
		if ( m_frequency > 19 )
		{
			SetNextThink( gpGlobals->curtime + 0.5f );
		}
		else
		{
			SetNextThink( gpGlobals->curtime + 2.5 - (0.1 * m_frequency) );
		}
	}
}

void CBubbling::FizzThink( void )
{
	Vector center = WorldSpaceCenter();
	CPASFilter filter( center );
	te->Fizz( filter, 0.0, this, m_bubbleModel, m_density, (int)m_flSpeed );

	if ( m_frequency > 19 )
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 2.5 - (0.1 * m_frequency) );
	}
}


// ENV_TRACER
// Fakes a tracer
class CEnvTracer : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvTracer, CPointEntity );

	void Spawn( void );
	void TracerThink( void );
	void Activate( void );

	DECLARE_DATADESC();

	Vector m_vecEnd;
	float  m_flDelay;
};

LINK_ENTITY_TO_CLASS( env_tracer, CEnvTracer );

BEGIN_DATADESC( CEnvTracer )

	DEFINE_KEYFIELD( m_flDelay, FIELD_FLOAT, "delay" ),

	// Function Pointers
	DEFINE_FUNCTION( TracerThink ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called after keyvalues are parsed.
//-----------------------------------------------------------------------------
void CEnvTracer::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	if (!m_flDelay)
		m_flDelay = 1;
}


//-----------------------------------------------------------------------------
// Purpose: Called after all the entities have been loaded.
//-----------------------------------------------------------------------------
void CEnvTracer::Activate( void )
{
	BaseClass::Activate();

	CBaseEntity *pEnd = gEntList.FindEntityByName( NULL, m_target );
	if (pEnd != NULL)
	{
		m_vecEnd = pEnd->GetLocalOrigin();
		SetThink( &CEnvTracer::TracerThink );
		SetNextThink( gpGlobals->curtime + m_flDelay );
	}
	else
	{
		Msg( "env_tracer: unknown entity \"%s\"\n", STRING(m_target) );
	}
}

// Think
void CEnvTracer::TracerThink( void )
{
	UTIL_Tracer( GetAbsOrigin(), m_vecEnd );

	SetNextThink( gpGlobals->curtime + m_flDelay );
}

// Blood effects
class CBlood : public CPointEntity
{
public:
	DECLARE_CLASS( CBlood, CPointEntity );

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	inline	int		Color( void ) { return m_Color; }
	inline	float 	BloodAmount( void ) { return m_flAmount; }

	inline	void SetColor( int color ) { m_Color = color; }

	// Input handlers
	void InputEmitBlood( inputdata_t &inputdata );

	Vector	Direction( void );
	Vector	BloodPosition( CBaseEntity *pActivator );

	DECLARE_DATADESC();

	Vector m_vecSprayDir;
	float m_flAmount;
	int m_Color;

private:
};

LINK_ENTITY_TO_CLASS( env_blood, CBlood );

BEGIN_DATADESC( CBlood )

	DEFINE_KEYFIELD( m_vecSprayDir, FIELD_VECTOR, "spraydir" ),
	DEFINE_KEYFIELD( m_flAmount, FIELD_FLOAT, "amount" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EmitBlood", InputEmitBlood ),

END_DATADESC()


#define SF_BLOOD_RANDOM		0x0001
#define SF_BLOOD_STREAM		0x0002
#define SF_BLOOD_PLAYER		0x0004
#define SF_BLOOD_DECAL		0x0008
#define SF_BLOOD_CLOUD		0x0010
#define SF_BLOOD_DROPS		0x0020
#define SF_BLOOD_GORE		0x0040


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBlood::Spawn( void )
{
	// Convert spraydir from angles to a vector
	QAngle angSprayDir = QAngle( m_vecSprayDir.x, m_vecSprayDir.y, m_vecSprayDir.z );
	AngleVectors( angSprayDir, &m_vecSprayDir );

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetColor( BLOOD_COLOR_RED );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : szKeyName -
//			szValue -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBlood::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "color"))
	{
		int color = atoi(szValue);
		switch ( color )
		{
			case 1:
			{
				SetColor( BLOOD_COLOR_YELLOW );
				break;
			}
		}
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


Vector CBlood::Direction( void )
{
	if ( HasSpawnFlags( SF_BLOOD_RANDOM ) )
		return UTIL_RandomBloodVector();

	return m_vecSprayDir;
}


Vector CBlood::BloodPosition( CBaseEntity *pActivator )
{
	if ( HasSpawnFlags( SF_BLOOD_PLAYER ) )
	{
		CBasePlayer *player;

		if ( pActivator && pActivator->IsPlayer() )
		{
			player = ToBasePlayer( pActivator );
		}
		else
		{
		#ifdef BB2_AI
			player = UTIL_GetNearestVisiblePlayer(this); 
		#else
			player = UTIL_GetLocalPlayer();
		#endif //BB2_AI

		}

		if ( player )
		{
			return (player->EyePosition()) + Vector( random->RandomFloat(-10,10), random->RandomFloat(-10,10), random->RandomFloat(-10,10) );
		}
	}

	return GetLocalOrigin();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags )
{
	if( color == DONT_BLEED )
		return;

	CEffectData	data;

	data.m_vOrigin = pos;
	data.m_vNormal = dir;
	data.m_flScale = (float)amount;
	data.m_fFlags = flags;
	data.m_nColor = color;

	DispatchEffect( "bloodspray", data );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for triggering the blood effect.
//-----------------------------------------------------------------------------
void CBlood::InputEmitBlood( inputdata_t &inputdata )
{
	if ( HasSpawnFlags( SF_BLOOD_STREAM ) )
	{
		UTIL_BloodStream( BloodPosition(inputdata.pActivator), Direction(), Color(), BloodAmount() );
	}
	else
	{
		UTIL_BloodDrips( BloodPosition(inputdata.pActivator), Direction(), Color(), BloodAmount() );
	}

	if ( HasSpawnFlags( SF_BLOOD_DECAL ) )
	{
		Vector forward = Direction();
		Vector start = BloodPosition( inputdata.pActivator );
		trace_t tr;

		UTIL_TraceLine( start, start + forward * BloodAmount() * 2, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &tr, Color() );
		}
	}

	//
	// New-fangled blood effects.
	//
	if ( HasSpawnFlags( SF_BLOOD_CLOUD | SF_BLOOD_DROPS | SF_BLOOD_GORE ) )
	{
		int nFlags = 0;
		if (HasSpawnFlags(SF_BLOOD_CLOUD))
		{
			nFlags |= FX_BLOODSPRAY_CLOUD;
		}

		if (HasSpawnFlags(SF_BLOOD_DROPS))
		{
			nFlags |= FX_BLOODSPRAY_DROPS;
		}

		if (HasSpawnFlags(SF_BLOOD_GORE))
		{
			nFlags |= FX_BLOODSPRAY_GORE;
		}

		UTIL_BloodSpray(GetAbsOrigin(), Direction(), Color(), BloodAmount(), nFlags);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Console command for emitting the blood spray effect from an NPC.
//-----------------------------------------------------------------------------
void CC_BloodSpray( const CCommand &args )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityGeneric( pEnt, args[1] ) ) != NULL )
	{
		Vector forward;
		pEnt->GetVectors(&forward, NULL, NULL);
		UTIL_BloodSpray( (forward * 4 ) + ( pEnt->EyePosition() + pEnt->WorldSpaceCenter() ) * 0.5f, forward, BLOOD_COLOR_RED, 4, FX_BLOODSPRAY_ALL );
	}
}

static ConCommand bloodspray( "bloodspray", CC_BloodSpray, "blood", FCVAR_CHEAT );


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CEnvFunnel : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvFunnel, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int		m_iSprite;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( env_funnel, CEnvFunnel );

void CEnvFunnel::Precache ( void )
{
	m_iSprite = PrecacheModel ( "sprites/flare6.vmt" );
}

void CEnvFunnel::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBroadcastRecipientFilter filter;
	te->LargeFunnel( filter, 0.0,
		&GetAbsOrigin(), m_iSprite, HasSpawnFlags( SF_FUNNEL_REVERSE ) ? 1 : 0 );

	SetThink( &CEnvFunnel::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

void CEnvFunnel::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );
}

#ifndef _XBOX
//=========================================================
// func_precipitation - temporary snow solution for first HL2
// technology demo
//=========================================================

class CPrecipitation : public CBaseEntity
{
public:
	DECLARE_CLASS( CPrecipitation, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CPrecipitation();
	void	Spawn( void );

	CNetworkVar( PrecipitationType_t, m_nPrecipType );
};

LINK_ENTITY_TO_CLASS( func_precipitation, CPrecipitation );

BEGIN_DATADESC( CPrecipitation )
	DEFINE_KEYFIELD( m_nPrecipType, FIELD_INTEGER, "preciptype" ),
END_DATADESC()

// Just send the normal entity crap
IMPLEMENT_SERVERCLASS_ST( CPrecipitation, DT_Precipitation)
	SendPropInt( SENDINFO( m_nPrecipType ), Q_log2( NUM_PRECIPITATION_TYPES ) + 1, SPROP_UNSIGNED )
END_SEND_TABLE()


CPrecipitation::CPrecipitation()
{
	m_nPrecipType = PRECIPITATION_TYPE_RAIN; // default to rain.
}

void CPrecipitation::Spawn( void )
{
	PrecacheMaterial( "effects/fleck_ash1" );
	PrecacheMaterial( "effects/fleck_ash2" );
	PrecacheMaterial( "effects/fleck_ash3" );
	PrecacheMaterial( "effects/ember_swirling001" );

	Precache();
	SetSolid( SOLID_NONE );							// Remove model & collisions
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );		// Set size

	// Default to rain.
	if ( m_nPrecipType < 0 || m_nPrecipType > NUM_PRECIPITATION_TYPES )
		m_nPrecipType = PRECIPITATION_TYPE_RAIN;

	m_nRenderMode = kRenderEnvironmental;
}
#endif

//-----------------------------------------------------------------------------
// EnvWind - global wind info
//-----------------------------------------------------------------------------
class CEnvWind : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvWind, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );
	void	WindThink( void );
	int		UpdateTransmitState( void );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

private:
#ifdef POSIX
	CEnvWindShared m_EnvWindShared; // FIXME - fails to compile as networked var due to operator= problem
#else
	CNetworkVarEmbedded( CEnvWindShared, m_EnvWindShared );
#endif
};

LINK_ENTITY_TO_CLASS( env_wind, CEnvWind );

BEGIN_DATADESC( CEnvWind )

	DEFINE_KEYFIELD( m_EnvWindShared.m_iMinWind, FIELD_INTEGER, "minwind" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_iMaxWind, FIELD_INTEGER, "maxwind" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_iMinGust, FIELD_INTEGER, "mingust" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_iMaxGust, FIELD_INTEGER, "maxgust" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_flMinGustDelay, FIELD_FLOAT, "mingustdelay" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_flMaxGustDelay, FIELD_FLOAT, "maxgustdelay" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_iGustDirChange, FIELD_INTEGER, "gustdirchange" ),
	DEFINE_KEYFIELD( m_EnvWindShared.m_flGustDuration, FIELD_FLOAT, "gustduration" ),
//	DEFINE_KEYFIELD( m_EnvWindShared.m_iszGustSound, FIELD_STRING, "gustsound" ),

	DEFINE_OUTPUT( m_EnvWindShared.m_OnGustStart, "OnGustStart" ),
	DEFINE_OUTPUT( m_EnvWindShared.m_OnGustEnd,	"OnGustEnd" ),

	// Function Pointers
	DEFINE_FUNCTION( WindThink ),

END_DATADESC()


BEGIN_SEND_TABLE_NOBASE(CEnvWindShared, DT_EnvWindShared)
	// These are parameters that are used to generate the entire motion
	SendPropInt		(SENDINFO(m_iMinWind),		10, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxWind),		10, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMinGust),		10, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iMaxGust),		10, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_flMinGustDelay), 0, SPROP_NOSCALE),		// NOTE: Have to do this, so it's *exactly* the same on client
	SendPropFloat	(SENDINFO(m_flMaxGustDelay), 0, SPROP_NOSCALE),
	SendPropInt		(SENDINFO(m_iGustDirChange), 9, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_iWindSeed),		32, SPROP_UNSIGNED ),

	// These are related to initial state
	SendPropInt		(SENDINFO(m_iInitialWindDir),9, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_flInitialWindSpeed),0, SPROP_NOSCALE ),
	SendPropFloat	(SENDINFO(m_flStartTime),	 0, SPROP_NOSCALE ),

	SendPropFloat	(SENDINFO(m_flGustDuration), 0, SPROP_NOSCALE),
	// Sound related
//	SendPropInt		(SENDINFO(m_iszGustSound),	10, SPROP_UNSIGNED ),
END_SEND_TABLE()

// This table encodes the CBaseEntity data.
IMPLEMENT_SERVERCLASS_ST_NOBASE(CEnvWind, DT_EnvWind)
	SendPropDataTable(SENDINFO_DT(m_EnvWindShared), &REFERENCE_SEND_TABLE(DT_EnvWindShared)),
END_SEND_TABLE()

void CEnvWind::Precache ( void )
{
//	if (m_iszGustSound)
//	{
//		PrecacheScriptSound( STRING( m_iszGustSound ) );
//	}
}

void CEnvWind::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );

	m_EnvWindShared.Init( entindex(), 0, gpGlobals->frametime, GetLocalAngles().y, 0 );

	SetThink( &CEnvWind::WindThink );
	SetNextThink( gpGlobals->curtime );
}

int CEnvWind::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CEnvWind::WindThink( void )
{
	SetNextThink( m_EnvWindShared.WindThink( gpGlobals->curtime ) );
}



//==================================================
// CEmbers
//==================================================

#define	bitsSF_EMBERS_START_ON	0x00000001
#define	bitsSF_EMBERS_TOGGLE	0x00000002

// UNDONE: This is a brush effect-in-volume entity, move client side.
class CEmbers : public CBaseEntity
{
public:
	DECLARE_CLASS( CEmbers, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );

	void	EmberUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CNetworkVar( int, m_nDensity );
	CNetworkVar( int, m_nLifetime );
	CNetworkVar( int, m_nSpeed );

	CNetworkVar( bool, m_bEmit );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( env_embers, CEmbers );

//Data description
BEGIN_DATADESC( CEmbers )

	DEFINE_KEYFIELD( m_nDensity,	FIELD_INTEGER, "density" ),
	DEFINE_KEYFIELD( m_nLifetime,	FIELD_INTEGER, "lifetime" ),
	DEFINE_KEYFIELD( m_nSpeed,		FIELD_INTEGER, "speed" ),

	//Function pointers
	DEFINE_FUNCTION( EmberUse ),

END_DATADESC()


//Data table
IMPLEMENT_SERVERCLASS_ST( CEmbers, DT_Embers )
	SendPropInt(	SENDINFO( m_nDensity ),		32,	SPROP_UNSIGNED ),
	SendPropInt(	SENDINFO( m_nLifetime ),	32,	SPROP_UNSIGNED ),
	SendPropInt(	SENDINFO( m_nSpeed ),		32,	SPROP_UNSIGNED ),
	SendPropInt(	SENDINFO( m_bEmit ),		2,	SPROP_UNSIGNED ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEmbers::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	SetSolid( SOLID_NONE );
	SetRenderColorA( 0 );
	m_nRenderMode	= kRenderTransTexture;

	SetUse( &CEmbers::EmberUse );

	//Start off if we're targetted (unless flagged)
	m_bEmit = ( HasSpawnFlags( bitsSF_EMBERS_START_ON ) || ( !GetEntityName() ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEmbers::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pActivator -
//			*pCaller -
//			useType -
//			value -
//-----------------------------------------------------------------------------
void CEmbers::EmberUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//If we're not toggable, only allow one use
	if ( !HasSpawnFlags( bitsSF_EMBERS_TOGGLE ) )
	{
		SetUse( NULL );
	}

	//Handle it
	switch ( useType )
	{
	case USE_OFF:
		m_bEmit = false;
		break;

	case USE_ON:
		m_bEmit = true;
		break;

	case USE_SET:
		m_bEmit = !!(int)value;
		break;

	default:
	case USE_TOGGLE:
		m_bEmit = !m_bEmit;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CPhysicsWire : public CBaseEntity
{
public:
	DECLARE_CLASS( CPhysicsWire, CBaseEntity );

	void	Spawn( void );
	void	Precache( void );

	DECLARE_DATADESC();

protected:

	bool SetupPhysics( void );

	int		m_nDensity;
};

LINK_ENTITY_TO_CLASS( env_physwire, CPhysicsWire );

BEGIN_DATADESC( CPhysicsWire )

	DEFINE_KEYFIELD( m_nDensity,	FIELD_INTEGER, "Density" ),
//	DEFINE_KEYFIELD( m_frequency, FIELD_INTEGER, "frequency" ),

	// Function Pointers
//	DEFINE_FUNCTION( WireThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysicsWire::Spawn( void )
{
	BaseClass::Spawn();

	Precache();

//	if ( SetupPhysics() == false )
//		return;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPhysicsWire::Precache( void )
{
	BaseClass::Precache();
}

class CPhysBallSocket;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CPhysicsWire::SetupPhysics( void )
{
	return true;
}

//
// Muzzle flash
//

class CEnvMuzzleFlash : public CPointEntity
{
	DECLARE_CLASS( CEnvMuzzleFlash, CPointEntity );

public:
	virtual void Spawn();

	// Input handlers
	void	InputFire( inputdata_t &inputdata );

	DECLARE_DATADESC();

	float	m_flScale;
	string_t m_iszParentAttachment;
};

BEGIN_DATADESC( CEnvMuzzleFlash )

	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( m_iszParentAttachment, FIELD_STRING, "parentattachment" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Fire", InputFire ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( env_muzzleflash, CEnvMuzzleFlash );


//-----------------------------------------------------------------------------
// Spawn!
//-----------------------------------------------------------------------------
void CEnvMuzzleFlash::Spawn()
{
	if ( (m_iszParentAttachment != NULL_STRING) && GetParent() && GetParent()->GetBaseAnimating() )
	{
		CBaseAnimating *pAnim = GetParent()->GetBaseAnimating();
		int nParentAttachment = pAnim->LookupAttachment( STRING(m_iszParentAttachment) );
		if ( nParentAttachment > 0 )
		{
			SetParent( GetParent(), nParentAttachment );
			SetLocalOrigin( vec3_origin );
			SetLocalAngles( vec3_angle );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CEnvMuzzleFlash::InputFire( inputdata_t &inputdata )
{
	g_pEffects->MuzzleFlash( GetAbsOrigin(), GetAbsAngles(), m_flScale, MUZZLEFLASH_TYPE_DEFAULT );
}


//=========================================================
// Splash!
//=========================================================
#define SF_ENVSPLASH_FINDWATERSURFACE	0x00000001
#define SF_ENVSPLASH_DIMINISH			0x00000002
class CEnvSplash : public CPointEntity
{
	DECLARE_CLASS( CEnvSplash, CPointEntity );

public:
	// Input handlers
	void	InputSplash( inputdata_t &inputdata );

protected:

	float	m_flScale;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CEnvSplash )
	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Splash", InputSplash ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_splash, CEnvSplash );

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
#define SPLASH_MAX_DEPTH	120.0f
void CEnvSplash::InputSplash( inputdata_t &inputdata )
{
	CEffectData	data;

	data.m_fFlags = 0;

	float scale = m_flScale;

	if( HasSpawnFlags( SF_ENVSPLASH_FINDWATERSURFACE ) )
	{
		if( UTIL_PointContents(GetAbsOrigin()) & MASK_WATER )
		{
			// No splash if I'm supposed to find the surface of the water, but I'm underwater.
			return;
		}

		// Trace down and find the water's surface. This is designed for making
		// splashes on the surface of water that can change water level.
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 4096 ), (MASK_WATER|MASK_SOLID_BRUSHONLY), this, COLLISION_GROUP_NONE, &tr );
		data.m_vOrigin = tr.endpos;

		if ( tr.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}
	}
	else
	{
		data.m_vOrigin = GetAbsOrigin();
	}

	if( HasSpawnFlags( SF_ENVSPLASH_DIMINISH ) )
	{
		// Get smaller if I'm in deeper water.
		float depth = 0.0f;

		trace_t tr;
		UTIL_TraceLine( data.m_vOrigin, data.m_vOrigin - Vector( 0, 0, 4096 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		depth = fabs( tr.startpos.z - tr.endpos.z );

		float factor = 1.0f - (depth / SPLASH_MAX_DEPTH);

		if( factor < 0.1 )
		{
			// Don't bother making one this small.
			return;
		}

		scale *= factor;
	}

	data.m_vNormal = Vector( 0, 0, 1 );
	data.m_flScale = scale;

	DispatchEffect( "watersplash", data );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CEnvGunfire : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvGunfire, CPointEntity );

	CEnvGunfire()
	{
		// !!!HACKHACK
		// These fields came along kind of late, so they get
		// initialized in the constructor for now. (sjb)
		m_flBias = 1.0f;
		m_bCollide = false;
	}

	void Precache();
	void Spawn();
	void Activate();
	void StartShooting();
	void StopShooting();
	void ShootThink();
	void UpdateTarget();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	int	m_iMinBurstSize;
	int m_iMaxBurstSize;

	float m_flMinBurstDelay;
	float m_flMaxBurstDelay;

	float m_flRateOfFire;

	string_t	m_iszShootSound;
	string_t	m_iszTracerType;

	bool m_bDisabled;

	int	m_iShotsRemaining;

	int		m_iSpread;
	Vector	m_vecSpread;
	Vector	m_vecTargetPosition;
	float	m_flTargetDist;

	float	m_flBias;
	bool	m_bCollide;

	EHANDLE m_hTarget;


	DECLARE_DATADESC();
};

BEGIN_DATADESC( CEnvGunfire )
	DEFINE_KEYFIELD( m_iMinBurstSize, FIELD_INTEGER, "minburstsize" ),
	DEFINE_KEYFIELD( m_iMaxBurstSize, FIELD_INTEGER, "maxburstsize" ),
	DEFINE_KEYFIELD( m_flMinBurstDelay, FIELD_TIME, "minburstdelay" ),
	DEFINE_KEYFIELD( m_flMaxBurstDelay, FIELD_TIME, "maxburstdelay" ),
	DEFINE_KEYFIELD( m_flRateOfFire, FIELD_FLOAT, "rateoffire" ),
	DEFINE_KEYFIELD( m_iszShootSound, FIELD_STRING, "shootsound" ),
	DEFINE_KEYFIELD( m_iszTracerType, FIELD_STRING, "tracertype" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "startdisabled" ),
	DEFINE_KEYFIELD( m_iSpread, FIELD_INTEGER, "spread" ),
	DEFINE_KEYFIELD( m_flBias, FIELD_FLOAT, "bias" ),
	DEFINE_KEYFIELD( m_bCollide, FIELD_BOOLEAN, "collisions" ),

	DEFINE_THINKFUNC( ShootThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_gunfire, CEnvGunfire );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::Precache()
{
	PrecacheScriptSound( STRING( m_iszShootSound ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::Spawn()
{
	Precache();

	m_iShotsRemaining = 0;
	m_flRateOfFire = 1.0f / m_flRateOfFire;

	switch( m_iSpread )
	{
	case 1:
		m_vecSpread = VECTOR_CONE_1DEGREES;
		break;
	case 5:
		m_vecSpread = VECTOR_CONE_5DEGREES;
		break;
	case 10:
		m_vecSpread = VECTOR_CONE_10DEGREES;
		break;
	case 15:
		m_vecSpread = VECTOR_CONE_15DEGREES;
		break;

	default:
		m_vecSpread = vec3_origin;
		break;
	}

	if( !m_bDisabled )
	{
		StartShooting();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::Activate( void )
{
	// Find my target
	if (m_target != NULL_STRING)
	{
		m_hTarget = gEntList.FindEntityByName( NULL, m_target );
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::StartShooting()
{
	m_iShotsRemaining = random->RandomInt( m_iMinBurstSize, m_iMaxBurstSize );

	SetThink( &CEnvGunfire::ShootThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::UpdateTarget()
{
	if( m_hTarget )
	{
		if( m_hTarget->WorldSpaceCenter() != m_vecTargetPosition )
		{
			// Target has moved.
			// Locate my target and cache the position and distance.
			m_vecTargetPosition = m_hTarget->WorldSpaceCenter();
			m_flTargetDist = (GetAbsOrigin() - m_vecTargetPosition).Length();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::StopShooting()
{
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::ShootThink()
{
	if( !m_hTarget )
	{
		StopShooting();
	}

	SetNextThink( gpGlobals->curtime + m_flRateOfFire );

	UpdateTarget();

	Vector vecDir = m_vecTargetPosition - GetAbsOrigin();
	VectorNormalize( vecDir );

	CShotManipulator manipulator( vecDir );

	vecDir = manipulator.ApplySpread( m_vecSpread, m_flBias );

	Vector vecEnd;

	if( m_bCollide )
	{
		trace_t tr;

		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecDir * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		if( tr.fraction != 1.0 )
		{
			DoImpactEffect( tr, DMG_BULLET );
		}

		vecEnd = tr.endpos;
	}
	else
	{
		vecEnd = GetAbsOrigin() + vecDir * m_flTargetDist;
	}

	if( m_iszTracerType != NULL_STRING )
	{
		UTIL_Tracer( GetAbsOrigin(), vecEnd, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true, STRING(m_iszTracerType) );
	}
	else
	{
		UTIL_Tracer( GetAbsOrigin(), vecEnd, 0, TRACER_DONT_USE_ATTACHMENT, 5000, true );
	}

	EmitSound( STRING(m_iszShootSound) );

	m_iShotsRemaining--;

	if( m_iShotsRemaining == 0 )
	{
		StartShooting();
		SetNextThink( gpGlobals->curtime + random->RandomFloat( m_flMinBurstDelay, m_flMaxBurstDelay ) );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
	StartShooting();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvGunfire::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Quadratic spline beam effect
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( env_quadraticbeam, CEnvQuadraticBeam );

IMPLEMENT_SERVERCLASS_ST( CEnvQuadraticBeam, DT_QuadraticBeam )
	SendPropVector(SENDINFO(m_targetPosition), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_controlPosition), -1, SPROP_COORD),
	SendPropFloat(SENDINFO(m_scrollRate), 8, 0, -4, 4),
	SendPropFloat(SENDINFO(m_flWidth), -1, SPROP_NOSCALE),
END_SEND_TABLE()

void CEnvQuadraticBeam::Spawn()
{
	BaseClass::Spawn();
	m_nRenderMode = kRenderTransAdd;
	SetRenderColor( 255, 255, 255 );
}

CEnvQuadraticBeam *CreateQuadraticBeam( const char *pSpriteName, const Vector &start, const Vector &control, const Vector &end, float width, CBaseEntity *pOwner )
{
	CEnvQuadraticBeam *pBeam = (CEnvQuadraticBeam *)CBaseEntity::Create( "env_quadraticbeam", start, vec3_angle, pOwner );
	UTIL_SetModel( pBeam, pSpriteName );
	pBeam->SetSpline( control, end );
	pBeam->SetScrollRate( 0.0 );
	pBeam->SetWidth(width);
	return pBeam;
}

void EffectsPrecache( void *pUser )
{
	CBaseEntity::PrecacheScriptSound( "Underwater.BulletImpact" );

	CBaseEntity::PrecacheScriptSound( "FX_RicochetSound.Ricochet" );

	CBaseEntity::PrecacheScriptSound( "Physics.WaterSplash" );
	CBaseEntity::PrecacheScriptSound( "BaseExplosionEffect.Sound" );
	CBaseEntity::PrecacheScriptSound( "Splash.SplashSound" );

	if ( gpGlobals->maxClients > 1 )
	{
		CBaseEntity::PrecacheScriptSound( "HudChat.Message" );
	}
}

PRECACHE_REGISTER_FN( EffectsPrecache );


class CEnvViewPunch : public CPointEntity
{
public:

	DECLARE_CLASS( CEnvViewPunch, CPointEntity );

	virtual void Spawn();

	// Input handlers
	void InputViewPunch( inputdata_t &inputdata );

private:

	float m_flRadius;
	QAngle m_angViewPunch;

	void DoViewPunch();

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( env_viewpunch, CEnvViewPunch );

BEGIN_DATADESC( CEnvViewPunch )

	DEFINE_KEYFIELD( m_angViewPunch, FIELD_VECTOR, "punchangle" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ViewPunch", InputViewPunch ),

END_DATADESC()

#define SF_PUNCH_EVERYONE	0x0001		// Don't check radius
#define SF_PUNCH_IN_AIR		0x0002		// Punch players in air


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvViewPunch::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	if ( GetSpawnFlags() & SF_PUNCH_EVERYONE )
	{
		m_flRadius = 0;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvViewPunch::DoViewPunch()
{
	bool bAir = (GetSpawnFlags() & SF_PUNCH_IN_AIR) ? true : false;
	UTIL_ViewPunch( GetAbsOrigin(), m_angViewPunch, m_flRadius, bAir );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvViewPunch::InputViewPunch( inputdata_t &inputdata )
{
	DoViewPunch();
}

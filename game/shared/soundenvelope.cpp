//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "sharedInterface.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "gamestringpool.h"
#include "igamesystem.h"
#include "utlpriorityqueue.h"
#include "mempool.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "tier0/vprof.h"
#include "gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar soundpatch_captionlength( "soundpatch_captionlength", "2.0", FCVAR_REPLICATED, "How long looping soundpatch captions should display for." );

// Envelope
// This is a class that controls a ramp for a sound (pitch / volume / etc)
class CSoundEnvelope
{
public:
	CSoundEnvelope()
	{
		m_current = 0.0f;
		m_target = 0.0f;
		m_rate = 0.0f;
		m_forceupdate = false;
	}

	void SetTarget( float target, float deltaTime );
	void SetValue( float value );
	bool ShouldUpdate( void );
	void Update( float time );
	inline float	Value( void ) { return m_current; }

private:
	float	m_current;
	float	m_target;
	float	m_rate;
	bool	m_forceupdate;
};

//-----------------------------------------------------------------------------
// Purpose: Set the new target value for this ramp.  Reach this target in deltaTime
//			seconds from now
// Input  : target - new target value
//			deltaTime - time to reach target
//-----------------------------------------------------------------------------
void CSoundEnvelope::SetTarget( float target, float deltaTime )
{
	float deltaValue = target - m_current;

	if ( deltaValue && deltaTime > 0 )
	{
		m_target = target;
		m_rate = MAX( 0.1, fabs(deltaValue / deltaTime) );
	}
	else
	{
		if ( target != m_current )
		{
			m_forceupdate = true;
		}

		SetValue( target );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Instantaneously set the value of this ramp
// Input  : value - new value
//-----------------------------------------------------------------------------
void CSoundEnvelope::SetValue( float value )
{
	if ( m_target != value )
	{
		m_forceupdate = true;
	}

	m_current = m_target = value;
	m_rate = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Check to see if I need to update this envelope
// Output : Returns true if this envelope is changing
//-----------------------------------------------------------------------------
bool CSoundEnvelope::ShouldUpdate( void )
{
	if ( m_forceupdate )
	{
		m_forceupdate = false;
		return true;
	}

	if ( m_current != m_target )
	{
		return true;
	}
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Update the envelope for the current frame time
// Input  : time - amount of time that has passed
//-----------------------------------------------------------------------------
void CSoundEnvelope::Update( float deltaTime )
{
	m_current = Approach( m_target, m_current, m_rate * deltaTime );
}

class CCopyRecipientFilter : public IRecipientFilter
{
public:
	CCopyRecipientFilter() : m_Flags(0) {}

	void Init( IRecipientFilter *pSrc )
	{
		m_Flags = FLAG_ACTIVE;
		if ( pSrc->IsReliable() )
		{
			m_Flags |= FLAG_RELIABLE;
		}

		if ( pSrc->IsInitMessage() )
		{
			m_Flags |= FLAG_INIT_MESSAGE;
		}

		for ( int i = 0; i < pSrc->GetRecipientCount(); i++ )
		{
			int index = pSrc->GetRecipientIndex( i );
			
			if ( index >= 0 )
				m_Recipients.AddToTail( index );
		}
	}

	bool IsActive() const 
	{
		return (m_Flags & FLAG_ACTIVE) != 0;
	}

	virtual bool IsReliable( void ) const
	{
		return (m_Flags & FLAG_RELIABLE) != 0;
	}

	virtual int	GetRecipientCount( void ) const
	{
		return m_Recipients.Count();
	}

	virtual int	GetRecipientIndex( int slot ) const
	{
		return m_Recipients[ slot ];
	}

	virtual bool IsInitMessage( void ) const
	{
		return (m_Flags & FLAG_INIT_MESSAGE) != 0;
	}

	virtual bool AddRecipient( CBasePlayer *player )
	{
		Assert( player );

		int index = player->entindex();

		if ( index < 0 )
			return false;

		// Already in list
		if ( m_Recipients.Find( index ) != m_Recipients.InvalidIndex() )
			return false;

		m_Recipients.AddToTail( index );
		return true;
	}

private:
	enum
	{
		FLAG_ACTIVE = 0x1,
		FLAG_RELIABLE = 0x2,
		FLAG_INIT_MESSAGE = 0x4,
	};

	int m_Flags;
	CUtlVector< int > m_Recipients;
};

#include "tier0/memdbgoff.h"
// This is the a basic sound controller, a "patch"
// It has envelopes for pitch and volume and can manage state changes to those
class CSoundPatch
{
public:
	static int g_SoundPatchCount;
	CSoundPatch()
	{
		g_SoundPatchCount++;
		m_iszSoundName = NULL_STRING;
		m_iszSoundScriptName = NULL_STRING;
		m_flCloseCaptionDuration = soundpatch_captionlength.GetFloat();
	}
	~CSoundPatch()
	{
		g_SoundPatchCount--;
	}

	void	Init( IRecipientFilter *pFilter, CBaseEntity *pEnt, int channel, const char *pSoundName, 
				soundlevel_t iSoundLevel );
	void	ChangePitch( float pitchTarget, float deltaTime );
	void	ChangeVolume( float volumeTarget, float deltaTime );
	void	FadeOut( float deltaTime, bool destroyOnFadeout );
	float	GetPitch( void );
	float	GetVolume( void );
	string_t GetName() { return m_iszSoundName; };
	string_t GetScriptName() { return m_iszSoundScriptName; }
	// UNDONE: Don't call this, use the controller to shut down
	void	Shutdown( void );
	bool	Update( float time, float deltaTime );
	void	Reset( void );
	void	StartSound( float flStartTime = 0 );
	void	ResumeSound( void );
	int		IsPlaying( void ) { return m_isPlaying; }
	void	AddPlayerPost( CBasePlayer *pPlayer );
	void	SetCloseCaptionDuration( float flDuration ) { m_flCloseCaptionDuration = flDuration; }

	void	SetBaseFlags( int iFlags ) { m_baseFlags = iFlags; }
	
	// Returns the ent index
	int		EntIndex() const;

private:
	// SoundPatches take volumes between 0 & 1, and use that to multiply the sounds.txt specified volume.
	// This function is an internal method of accessing the real volume passed into the engine (i.e. post multiply)
	float	GetVolumeForEngine( void );

private:
	CSoundEnvelope	m_pitch;
	CSoundEnvelope	m_volume;

	soundlevel_t	m_soundlevel;
	float			m_shutdownTime;
	float			m_flLastTime;
	string_t		m_iszSoundName;
	string_t		m_iszSoundScriptName;
	EHANDLE			m_hEnt;
	int				m_entityChannel;
	int				m_flags;
	int				m_baseFlags;
	int				m_isPlaying;
	float			m_flScriptVolume;	// Volume for this sound in sounds.txt
	CCopyRecipientFilter m_Filter;

	float			m_flCloseCaptionDuration;

#ifdef _DEBUG
	// Used to get the classname of the entity associated with the sound
	string_t		m_iszClassName;
#endif

	DECLARE_FIXEDSIZE_ALLOCATOR(CSoundPatch);
};
#include "tier0/memdbgon.h"

int CSoundPatch::g_SoundPatchCount = 0;

CON_COMMAND( report_soundpatch, "reports sound patch count" )
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	Msg("Current sound patches: %d\n", CSoundPatch::g_SoundPatchCount );
}
DEFINE_FIXEDSIZE_ALLOCATOR( CSoundPatch, 64, CUtlMemoryPool::GROW_FAST );

//-----------------------------------------------------------------------------
// Purpose: Setup the patch
// Input  : nEntIndex - index of the edict that owns the sound channel
//			channel - This is a sound channel (CHAN_ITEM, CHAN_STATIC)
//			*pSoundName - sound script string name
//			attenuation - attenuation of this sound (not animated)
//-----------------------------------------------------------------------------
void CSoundPatch::Init( IRecipientFilter *pFilter, CBaseEntity *pEnt, int channel, const char *pSoundName, 
			soundlevel_t soundlevel )
{
	m_hEnt = pEnt;
	m_entityChannel = channel;
	// Get the volume from the script
	CSoundParameters params;
	if ( !Q_stristr( pSoundName, ".wav" ) && !Q_stristr( pSoundName, ".mp3" ) &&
		CBaseEntity::GetParametersForSound( pSoundName, params, NULL ) )
	{
		m_flScriptVolume = params.volume;
		// This has to be the actual .wav because rndwave would cause a bunch of new .wavs to play... bad...
		//  e.g., when you pitch shift it would start a different wav instead.

		m_iszSoundScriptName = AllocPooledString( pSoundName );

		pSoundName = params.soundname;
		m_soundlevel = params.soundlevel;

		m_entityChannel = params.channel;
	}
	else
	{

		m_iszSoundScriptName = AllocPooledString( pSoundName );

		m_flScriptVolume = 1.0;
		m_soundlevel = soundlevel;
	}

	m_iszSoundName = AllocPooledString( pSoundName );
	m_volume.SetValue( 0 );
	m_pitch.SetValue( 0 );
	m_isPlaying = false;
	m_shutdownTime = 0;
	m_flLastTime = 0;
	m_Filter.Init( pFilter );
	m_baseFlags = 0;

#ifdef _DEBUG
	if ( pEnt )
	{
		m_iszClassName = AllocPooledString( pEnt->GetClassname() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Ramps the pitch to a new value
// Input  : pitchTarget - new value
//			deltaTime - seconds to reach the value
//-----------------------------------------------------------------------------
void CSoundPatch::ChangePitch( float pitchTarget, float deltaTime )
{
	m_flags |= SND_CHANGE_PITCH;
	m_pitch.SetTarget( pitchTarget, deltaTime );
}


//-----------------------------------------------------------------------------
// Purpose: Ramps the volume to a new value
// Input  : volumeTarget - new volume
//			deltaTime - seconds to reach the new volume 
//-----------------------------------------------------------------------------
void CSoundPatch::ChangeVolume( float volumeTarget, float deltaTime )
{
	m_flags |= SND_CHANGE_VOL;
	if ( volumeTarget > 1.0 )
		volumeTarget = 1.0;
	m_volume.SetTarget( volumeTarget, deltaTime );
}


//-----------------------------------------------------------------------------
// Purpose: Fade volume to zero AND SHUT DOWN THIS SOUND
// Input  : deltaTime - seconds before done/shutdown
//-----------------------------------------------------------------------------
void CSoundPatch::FadeOut( float deltaTime, bool destroyOnFadeout )
{
	ChangeVolume( 0, deltaTime );
	if ( !destroyOnFadeout )
	{
		m_shutdownTime = g_pEffects->Time() + deltaTime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the sound's current pitch
//-----------------------------------------------------------------------------
float CSoundPatch::GetPitch( void )
{
	return m_pitch.Value();
}

//-----------------------------------------------------------------------------
// Purpose: Get the sound's current volume
//-----------------------------------------------------------------------------
float CSoundPatch::GetVolume( void )
{
	return m_volume.Value();
}

//-----------------------------------------------------------------------------
// Returns the ent index
//-----------------------------------------------------------------------------
inline int CSoundPatch::EntIndex() const
{
	Assert( !m_hEnt.IsValid() || m_hEnt.Get() );
	return m_hEnt.Get() ? m_hEnt->entindex() : -1;
}

//-----------------------------------------------------------------------------
// Purpose: SoundPatches take volumes between 0 & 1, and use that to multiply the sounds.txt specified volume.
// This function is an internal method of accessing the real volume passed into the engine (i.e. post multiply)
// Output : float
//-----------------------------------------------------------------------------
float CSoundPatch::GetVolumeForEngine( void )
{
	return ( m_flScriptVolume * m_volume.Value() );
}

//-----------------------------------------------------------------------------
// Purpose: Stop the sound
//-----------------------------------------------------------------------------
void CSoundPatch::Shutdown( void )
{
//	Msg( "Removing sound %s\n", m_pszSoundName );
	if ( m_isPlaying )
	{
		int entIndex = EntIndex();
		Assert( entIndex >= 0 );
		// BUGBUG: Don't crash in release mode
		if ( entIndex >= 0 )
		{
			CBaseEntity::StopSound( entIndex, m_entityChannel, STRING( m_iszSoundName ) );
		}
		m_isPlaying = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Update all envelopes and send appropriate data to the client
// Input  : time - new global clock
//			deltaTime - amount of time that has passed
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSoundPatch::Update( float time, float deltaTime )
{
	VPROF( "CSoundPatch::Update" );
	if ( m_shutdownTime && time > m_shutdownTime )
	{
		Shutdown();
		return false;
	}

	if ( EntIndex() < 0 )
	{
		// FIXME:  The pointer to this soundpatch is probably leaked since no entity is around to clean it up (ywb)
		DevWarning( "CSoundPatch::Update:  Removing CSoundPatch (%s) with NULL EHandle\n", STRING(m_iszSoundName) );
		return false;
	}

	if ( m_pitch.ShouldUpdate() )
	{
		m_pitch.Update( deltaTime );
		m_flags |= SND_CHANGE_PITCH;
	}
	else 
	{
		m_flags &= ~SND_CHANGE_PITCH;
	}

	if ( m_volume.ShouldUpdate() )
	{
		m_volume.Update( deltaTime );
		m_flags |= SND_CHANGE_VOL;
	}
	else 
	{
		m_flags &= ~SND_CHANGE_VOL;
	}

	if ( m_flags && m_Filter.IsActive() )
	{
		// SoundPatches take volumes between 0 & 1, and use that to multiply the sounds.txt specified volume.
		// Because of this, we need to always set the SND_CHANGE_VOL flag when we emit sound, or it'll use the scriptfile's instead.
		m_flags |= SND_CHANGE_VOL;

		EmitSound_t ep;
		ep.m_nChannel = m_entityChannel;
		ep.m_pSoundName = STRING(m_iszSoundName);
		ep.m_flVolume = GetVolumeForEngine();
		ep.m_SoundLevel = m_soundlevel;
		ep.m_nFlags = m_flags;
		ep.m_nPitch = (int)m_pitch.Value();

		CBaseEntity::EmitSound( m_Filter, EntIndex(), ep );

		m_flags = 0;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sound is going to start playing again, clear any shutdown time
//-----------------------------------------------------------------------------
void CSoundPatch::Reset( void )
{
	m_shutdownTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Start playing the sound - send updates to the client
//-----------------------------------------------------------------------------
void CSoundPatch::StartSound( float flStartTime )
{
//	Msg( "Start sound %s\n", m_pszSoundName );
	m_flags = 0;
	if ( m_Filter.IsActive() )
	{
		EmitSound_t ep;
		ep.m_nChannel = m_entityChannel;
		ep.m_pSoundName = STRING(m_iszSoundName);
		ep.m_flVolume = GetVolumeForEngine();
		ep.m_SoundLevel = m_soundlevel;
		ep.m_nFlags = (SND_CHANGE_VOL | m_baseFlags);
		ep.m_nPitch = (int)m_pitch.Value();
		ep.m_bEmitCloseCaption = false;

		if ( flStartTime )
		{
			ep.m_flSoundTime = flStartTime;
		}

		CBaseEntity::EmitSound( m_Filter, EntIndex(), ep );
		CBaseEntity::EmitCloseCaption( m_Filter, EntIndex(), STRING( m_iszSoundScriptName ), ep.m_UtlVecSoundOrigin, m_flCloseCaptionDuration, true );
	}
	m_isPlaying = true;
}


//-----------------------------------------------------------------------------
// Purpose: resumes playing the sound on restore
//-----------------------------------------------------------------------------
void CSoundPatch::ResumeSound( void )
{
	if ( IsPlaying() && m_Filter.IsActive() )
	{
		if ( EntIndex() >= 0 )
		{
			EmitSound_t ep;
			ep.m_nChannel = m_entityChannel;
			ep.m_pSoundName = STRING(m_iszSoundName);
			ep.m_flVolume = GetVolumeForEngine();
			ep.m_SoundLevel = m_soundlevel;
			ep.m_nFlags = (SND_CHANGE_VOL | SND_CHANGE_PITCH | m_baseFlags);
			ep.m_nPitch = (int)m_pitch.Value();

			CBaseEntity::EmitSound( m_Filter, EntIndex(), ep );
		}
		else
		{
			// FIXME: Lost the entity on restore. It might have been suppressed by the save/restore system.
			// This will probably leak the sound patch since there's no one to delete it, but the next
			// call to CSoundPatch::Update should at least remove it from the list of sound patches.
			DevWarning( "CSoundPatch::ResumeSound: Lost EHAndle on restore - destroy the sound patch in your entity's StopLoopingSounds! (%s)\n", STRING( m_iszSoundName ) );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: A new player's entered the game. See if we need to restart our sound.
//-----------------------------------------------------------------------------
void CSoundPatch::AddPlayerPost( CBasePlayer *pPlayer )
{
	if ( m_Filter.IsActive() && m_Filter.AddRecipient(pPlayer) )
	{
		// Alrighty, he's new. We need to restart our sound just to him.
		// Create a new filter just to him.
		CSingleUserRecipientFilter filter( pPlayer );

		EmitSound_t ep;
		ep.m_nChannel = m_entityChannel;
		ep.m_pSoundName = STRING(m_iszSoundName);
		ep.m_flVolume = GetVolumeForEngine();
		ep.m_SoundLevel = m_soundlevel;
		ep.m_nFlags = (SND_CHANGE_VOL | m_baseFlags);
		ep.m_nPitch = (int)m_pitch.Value();

		CBaseEntity::EmitSound( filter, EntIndex(), ep );
	}
}

// This is an entry in the command queue.  It's used to queue up various pitch and volume changes
// so you can define an envelope without writing timing code in an entity.  Existing queued commands
// can be deleted later if the envelope changes dynamically.
#include "tier0/memdbgoff.h"
struct SoundCommand_t
{
	SoundCommand_t( void ) { memset( this, 0, sizeof(*this) ); }
	SoundCommand_t( CSoundPatch *pSound, float executeTime, soundcommands_t command, float deltaTime, float value ) : m_pPatch(pSound), m_time(executeTime), m_deltaTime(deltaTime), m_command(command), m_value(value) {}

	CSoundPatch		*m_pPatch;
	float			m_time;
	float			m_deltaTime;
	soundcommands_t	m_command;
	float			m_value;
	
	SoundCommand_t	*m_pNext;

	DECLARE_FIXEDSIZE_ALLOCATOR(SoundCommand_t);
};
#include "tier0/memdbgon.h"

DEFINE_FIXEDSIZE_ALLOCATOR( SoundCommand_t, 32, CUtlMemoryPool::GROW_FAST );

typedef SoundCommand_t *SOUNDCOMMANDPTR;

bool SoundCommandLessFunc( const SOUNDCOMMANDPTR &lhs, const SOUNDCOMMANDPTR &rhs )	
{ 
	// NOTE: A greater time means "less" priority
	return ( lhs->m_time > rhs->m_time );	
}


// This implements the sound controller
class CSoundControllerImp : public CSoundEnvelopeController, public CAutoGameSystemPerFrame
{
	//-----------------------------------------------------------------------------
	// internal functions, private to this file
	//-----------------------------------------------------------------------------
public:
	CSoundControllerImp( void ) : CAutoGameSystemPerFrame( "CSoundControllerImp" )
	{
		m_commandList.SetLessFunc( SoundCommandLessFunc );
	}

	void ProcessCommand( SoundCommand_t *pCmd );
	void RemoveFromList( CSoundPatch *pSound );

	//-----------------------------------------------------------------------------
	// external interface functions (from CSoundEnvelopeController)
	//-----------------------------------------------------------------------------
public:

	// Start this sound playing, or reset if already playing with new volume/pitch
	void			Play( CSoundPatch *pSound, float volume, float pitch, float flStartTime = 0 );
	void			CommandAdd( CSoundPatch *pSound, float executeDeltaTime, soundcommands_t command, float commandTime, float commandValue );
	
	void			SystemReset( void );
	void			SystemUpdate( void );
	void			CommandClear( CSoundPatch *pSound );
	void			Shutdown( CSoundPatch *pSound );

	CSoundPatch		*SoundCreate( IRecipientFilter& filter, int nEntIndex, const char *pSoundName );
	CSoundPatch		*SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
						float attenuation );
	CSoundPatch		*SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
						soundlevel_t soundlevel );
	CSoundPatch		*SoundCreate( IRecipientFilter& filter, int nEntIndex, const EmitSound_t &es );
	void			SoundDestroy( CSoundPatch *pSound );
	void			SoundChangePitch( CSoundPatch *pSound, float pitchTarget, float deltaTime );
	void			SoundChangeVolume( CSoundPatch *pSound, float volumeTarget, float deltaTime );
	void			SoundFadeOut( CSoundPatch *pSound, float deltaTime, bool destroyOnFadeout );
	float			SoundGetPitch( CSoundPatch *pSound );
	float			SoundGetVolume( CSoundPatch *pSound );
	string_t		SoundGetName( CSoundPatch *pSound ) { return pSound->GetName(); }
	void			SoundSetCloseCaptionDuration( CSoundPatch *pSound, float flDuration ) { pSound->SetCloseCaptionDuration(flDuration); }

	float			SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopePoint_t *points, int numPoints );
	float			SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopeDescription_t *envelope );

	void			CheckLoopingSoundsForPlayer( CBasePlayer *pPlayer );

	// Inserts the command into the list, sorted by time
	void			CommandInsert( SoundCommand_t *pCommand );

#ifdef CLIENT_DLL
	// CAutoClientSystem
	virtual void Update( float frametime ) 
	{
		SystemUpdate();
	}
#else
	virtual void PreClientUpdate()
	{
		SystemUpdate();
	}
#endif

	virtual void LevelShutdownPreEntity() 
	{
		SystemReset();
	}
	
private:
	CUtlVector<CSoundPatch *>			m_soundList;
	CUtlPriorityQueue<SoundCommand_t *>	m_commandList;
	float				m_flLastTime;
};

// Execute a command from the list
// currently only 3 commands 
// UNDONE: Add start command?
void CSoundControllerImp::ProcessCommand( SoundCommand_t *pCmd )
{
	switch( pCmd->m_command )
	{
	case SOUNDCTRL_CHANGE_VOLUME:
		pCmd->m_pPatch->ChangeVolume( pCmd->m_value, pCmd->m_deltaTime );
		break;

	case SOUNDCTRL_CHANGE_PITCH:
		pCmd->m_pPatch->ChangePitch( pCmd->m_value, pCmd->m_deltaTime );
		break;

	case SOUNDCTRL_STOP:
		pCmd->m_pPatch->Shutdown();
		break;

	case SOUNDCTRL_DESTROY:
		RemoveFromList( pCmd->m_pPatch );
		delete pCmd->m_pPatch;
		pCmd->m_pPatch = NULL;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Remove this sound from the sound list & shutdown (not in external interface)
// Input  : *pSound - patch to remove
//-----------------------------------------------------------------------------
void CSoundControllerImp::RemoveFromList( CSoundPatch *pSound )
{
	m_soundList.FindAndRemove( pSound );
	pSound->Shutdown();
}


//-----------------------------------------------------------------------------
// Start this sound playing, or reset if already playing with new volume/pitch
//-----------------------------------------------------------------------------
void CSoundControllerImp::Play( CSoundPatch *pSound, float volume, float pitch, float flStartTime )
{
	// reset the vars
	pSound->Reset();

	pSound->ChangeVolume( volume, 0 );
	pSound->ChangePitch( pitch, 0 );

	if ( pSound->IsPlaying() )
	{
		// remove any previous commands in the queue
		CommandClear( pSound );
	}
	else
	{
		m_soundList.AddToTail( pSound );
		pSound->StartSound( flStartTime );
	}
}


//-----------------------------------------------------------------------------
// Inserts the command into the list, sorted by time
//-----------------------------------------------------------------------------
void CSoundControllerImp::CommandInsert( SoundCommand_t *pCommand )
{
	m_commandList.Insert( pCommand );
}


//-----------------------------------------------------------------------------
// Purpose: puts a command into the queue
// Input  : *pSound - patch this command affects
//			executeDeltaTime - relative time to execute this command
//			command - command to execute (SOUNDCTRL_*)
//			commandTime - commands have 2 parameters, a time and a value
//			value - 
// Output : 	void
//-----------------------------------------------------------------------------
void CSoundControllerImp::CommandAdd( CSoundPatch *pSound, float executeDeltaTime, soundcommands_t command, float commandTime, float commandValue )
{
	SoundCommand_t *pCommand = new SoundCommand_t( pSound, g_pEffects->Time() + executeDeltaTime, command, commandTime, commandValue );
	CommandInsert( pCommand );
}

// Reset the whole system (level change, etc.)
void CSoundControllerImp::SystemReset( void )
{
	for ( int i = m_soundList.Count()-1; i >=0; i-- )
	{
		CSoundPatch *pNode = m_soundList[i];
	
		// shutdown all active sounds
		pNode->Shutdown();
	}

	// clear the list
	m_soundList.Purge();

	// clear the command queue
	m_commandList.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Update the active sounds, dequeue any events and move the ramps
//-----------------------------------------------------------------------------
void CSoundControllerImp::SystemUpdate( void )
{
	VPROF( "CSoundControllerImp::SystemUpdate" );
	float time = g_pEffects->Time();
	float deltaTime = time - m_flLastTime;
	
	// handle clock resets
	if ( deltaTime < 0 )
		deltaTime = 0;

	m_flLastTime = time;

	{
		VPROF( "CSoundControllerImp::SystemUpdate:processcommandlist" );
		while ( m_commandList.Count() )
		{
			SoundCommand_t *pCmd = m_commandList.ElementAtHead();
			// Commands are sorted by time.
			// process any that should occur by the current time
			if ( time >= pCmd->m_time )
			{
				m_commandList.RemoveAtHead();
				ProcessCommand( pCmd );
				delete pCmd;
			}
			else
			{
				break;
			}
		}
	}

	// NOTE: Because this loop goes from the end to the beginning
	// we can fast remove inside it without breaking the indexing
	{
		VPROF( "CSoundControllerImp::SystemUpdate:removesounds" );
		for ( int i = m_soundList.Count()-1; i >=0; i-- )
		{
			CSoundPatch *pNode = m_soundList[i];
			if ( !pNode->Update( time, deltaTime ) )
			{
				pNode->Reset();
				m_soundList.FastRemove( i );
			}
		}
	}
}

// Remove any envelope commands from the list (dynamically changing envelope)
void CSoundControllerImp::CommandClear( CSoundPatch *pSound )
{
	for ( int i = m_commandList.Count()-1; i >= 0; i-- )
	{
		SoundCommand_t *pCmd = m_commandList.Element( i );
		if ( pCmd->m_pPatch == pSound )
		{
			m_commandList.RemoveAt(i);
			delete pCmd;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: immediately stop playing this sound 
// Input  : *pSound - Patch to shut down
//-----------------------------------------------------------------------------
void CSoundControllerImp::Shutdown( CSoundPatch *pSound )
{
	if ( !pSound )
		return;

	pSound->Shutdown();
	CommandClear( pSound );
	RemoveFromList( pSound );
}

CSoundPatch *CSoundControllerImp::SoundCreate( IRecipientFilter& filter, int nEntIndex, const char *pSoundName )
{
#ifdef CLIENT_DLL
	if ( GameRules() )
	{
		pSoundName = GameRules()->TranslateEffectForVisionFilter( "sounds", pSoundName );
	}
#endif

	CSoundPatch *pSound = new CSoundPatch;

	// FIXME: This is done so we don't have to futz with the public interface
	EHANDLE hEnt = (nEntIndex != -1) ? g_pEntityList->GetNetworkableHandle( nEntIndex ) : NULL;
	pSound->Init( &filter, hEnt.Get(), CHAN_AUTO, pSoundName, SNDLVL_NORM );

	return pSound;
}

CSoundPatch *CSoundControllerImp::SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, 
			const char *pSoundName, float attenuation )
{
#ifdef CLIENT_DLL
	if ( GameRules() )
	{
		pSoundName = GameRules()->TranslateEffectForVisionFilter( "sounds", pSoundName );
	}
#endif

	CSoundPatch *pSound = new CSoundPatch;
	EHANDLE hEnt = (nEntIndex != -1) ? g_pEntityList->GetNetworkableHandle( nEntIndex ) : NULL;
	pSound->Init( &filter, hEnt.Get(), channel, pSoundName, ATTN_TO_SNDLVL( attenuation ) );

	return pSound;
}

CSoundPatch *CSoundControllerImp::SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, 
			const char *pSoundName, soundlevel_t soundlevel )
{
#ifdef CLIENT_DLL
	if ( GameRules() )
	{
		pSoundName = GameRules()->TranslateEffectForVisionFilter( "sounds", pSoundName );
	}
#endif

	CSoundPatch *pSound = new CSoundPatch;
	EHANDLE hEnt = (nEntIndex != -1) ? g_pEntityList->GetNetworkableHandle( nEntIndex ) : NULL;
	pSound->Init( &filter, hEnt.Get(), channel, pSoundName, soundlevel );

	return pSound;
}

CSoundPatch *CSoundControllerImp::SoundCreate( IRecipientFilter& filter, int nEntIndex, const EmitSound_t &es )
{
	CSoundPatch *pSound = new CSoundPatch;

	// FIXME: This is done so we don't have to futz with the public interface
	EHANDLE hEnt = (nEntIndex != -1) ? g_pEntityList->GetNetworkableHandle( nEntIndex ) : NULL;
	pSound->Init( &filter, hEnt.Get(), es.m_nChannel, es.m_pSoundName, es.m_SoundLevel );
	pSound->ChangeVolume( es.m_flVolume, 0 );
	pSound->ChangePitch( es.m_nPitch, 0 );

	if ( es.m_nFlags & SND_SHOULDPAUSE )
	{
		pSound->SetBaseFlags( SND_SHOULDPAUSE );
	}

	return pSound;
}

void CSoundControllerImp::SoundDestroy( CSoundPatch	*pSound )
{
	if ( !pSound )
		return;

	Shutdown( pSound );
	delete pSound;
}

void CSoundControllerImp::SoundChangePitch( CSoundPatch *pSound, float pitchTarget, float deltaTime )
{
	pSound->ChangePitch( pitchTarget, deltaTime );
}


void CSoundControllerImp::SoundChangeVolume( CSoundPatch *pSound, float volumeTarget, float deltaTime )
{
	pSound->ChangeVolume( volumeTarget, deltaTime );
}

float CSoundControllerImp::SoundGetPitch( CSoundPatch *pSound )
{
	return pSound->GetPitch();
}

float CSoundControllerImp::SoundGetVolume( CSoundPatch *pSound )
{
	return pSound->GetVolume();
}

void CSoundControllerImp::SoundFadeOut( CSoundPatch *pSound, float deltaTime, bool destroyOnFadeout )
{
	if ( destroyOnFadeout && (deltaTime == 0.0f) )
	{
		SoundDestroy( pSound );
		return;
	}

	pSound->FadeOut( deltaTime, destroyOnFadeout );
	if ( destroyOnFadeout )
	{
		CommandAdd( pSound, deltaTime, SOUNDCTRL_DESTROY, 0.0f, 0.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Queue a list of envelope points into a sound patch's event list
// Input  : *pSound - The sound patch to be operated on
//			soundCommand - Type of operation the envelope describes
//			*points - List of enevelope points
//			numPoints - Number of points provided
// Output : float - Returns the total duration of the envelope
//-----------------------------------------------------------------------------
float CSoundControllerImp::SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopePoint_t *points, int numPoints )
{
	float	amplitude	= 0.0f;
	float	duration	= 0.0f;
	float	totalDuration	= 0.0f;

	Assert( points );

	// Clear out all previously acting commands
	CommandClear( pSound );

	// Evaluate and queue all points
	for ( int i = 0; i < numPoints; i++ )
	{
		// See if we're keeping our last amplitude for this new point
		if ( ( points[i].amplitudeMin != -1.0f ) || ( points[i].amplitudeMax != -1.0f ) )
		{
			amplitude = random->RandomFloat( points[i].amplitudeMin, points[i].amplitudeMax );
		}
		else if ( i == 0 )
		{
			// Can't do this on the first entry
			Msg( "Invalid starting amplitude value in envelope!  (Cannot be -1)\n" );
		}

		// See if we're keeping our last duration for this new point
		if ( ( points[i].durationMin != -1.0f ) || ( points[i].durationMax != -1.0f ) )
		{
			duration = random->RandomFloat( points[i].durationMin, points[i].durationMax );	
			//duration = points[i].durationMin;
		}
		else if ( i == 0 )
		{
			// Can't do this on the first entry
			Msg( "Invalid starting duration value in envelope! (Cannot be -1)\n" );
		}

		// Queue the command
		CommandAdd( pSound, totalDuration, soundCommand, duration, amplitude );

		// Tack this command's duration onto the running duration
		totalDuration += duration;
	}

	return totalDuration;
}

//-----------------------------------------------------------------------------
// Purpose: Queue a list of envelope points into a sound patch's event list
// Input  : *pSound - The sound patch to be operated on
//			soundCommand - Type of operation the envelope describes
//			*envelope - The envelope description to be queued
// Output : float - Returns the total duration of the envelope
//-----------------------------------------------------------------------------
float CSoundControllerImp::SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopeDescription_t *envelope )
{
	return SoundPlayEnvelope( pSound, soundCommand, envelope->pPoints, envelope->nNumPoints );
}

//-----------------------------------------------------------------------------
// Purpose: Looping sounds are often started in entity spawn/activate functions. 
//			In singleplayer, the player's not ready to receive sounds then, so restart 
//			and SoundPatches that are active and have no receivers.
//-----------------------------------------------------------------------------
void CSoundControllerImp::CheckLoopingSoundsForPlayer( CBasePlayer *pPlayer )
{
	for ( int i = m_soundList.Count()-1; i >=0; i-- )
	{
		CSoundPatch *pNode = m_soundList[i];
		pNode->AddPlayerPost( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Singleton accessors
//-----------------------------------------------------------------------------
static CSoundControllerImp g_Controller;
CSoundEnvelopeController &CSoundEnvelopeController::GetController( void )
{
	return g_Controller;
}

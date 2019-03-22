//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Normal HUD mode
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "clientmode_shared.h"
#include "iinput.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "hud_basechat.h"
#include "weapon_selection.h"
#include <vgui/IVGui.h>
#include <vgui/Cursor.h>
#include <vgui/IPanel.h>
#include <vgui/IInput.h>
#include "engine/IEngineSound.h"
#include <KeyValues.h>
#include <vgui_controls/AnimationController.h>
#include "vgui_int.h"
#include "hud_macros.h"
#include "hltvcamera.h"
#include "particlemgr.h"
#include "c_vguiscreen.h"
#include "c_team.h"
#include "c_rumble.h"
#include "fmtstr.h"
#include "achievementmgr.h"
#include "c_playerresource.h"
#include "cam_thirdperson.h"
#include <vgui/ILocalize.h>
#include "ienginevgui.h"
#include "sourcevr/isourcevirtualreality.h"

// BB2
#include "GameBase_Client.h"
#include "c_hl2mp_player.h"
#include "GameBase_Shared.h"
#include "c_client_gib.h"
#include "c_bb2_player_shared.h"
#include "c_ai_basenpc.h"
#include "hud_npc_health_bar.h"

#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif

#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/ireplaysystem.h"
#include "replay/iclientreplaycontext.h"
#include "replay/ireplaymanager.h"
#include "replay/replay.h"
#include "replay/ienginereplay.h"
#include "replay/vgui/replayreminderpanel.h"
#include "replay/vgui/replaymessagepanel.h"
#include "econ/econ_controls.h"
#include "econ/confirm_dialog.h"
extern IClientReplayContext *g_pClientReplayContext;
extern ConVar replay_rendersetting_renderglow;
#endif

#if defined USES_ECON_ITEMS
#include "econ_item_view.h"
#endif

#ifdef BB2_GLOWS
#include "clienteffectprecachesystem.h"
#endif //BB2_GLOWS

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ACHIEVEMENT_ANNOUNCEMENT_MIN_TIME 10

#ifdef BB2_GLOWS
CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffectsGlow )
	CLIENTEFFECT_MATERIAL( "dev/glow_color" )
	CLIENTEFFECT_MATERIAL( "dev/halo_add_to_screen" )
CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90 )
#endif //BB2_GLOWS

class CHudWeaponSelection;
class CHudChat;
class CHudVote;

static vgui::HContext s_hVGuiContext = DEFAULT_VGUI_CONTEXT;

ConVar cl_drawhud( "cl_drawhud", "1", FCVAR_CHEAT, "Enable the rendering of the hud" );
ConVar hud_takesshots( "hud_takesshots", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Auto-save a scoreboard screenshot at the end of a map." );
ConVar hud_freezecamhide( "hud_freezecamhide", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Hide the HUD during freeze-cam" );
ConVar cl_show_num_particle_systems( "cl_show_num_particle_systems", "0", FCVAR_CLIENTDLL, "Display the number of active particle systems." );

// BB2 Vars:
ConVar bb2_extreme_gore( "bb2_extreme_gore", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Enable old stylish BrainBread 1 blood amounts!" );
ConVar bb2_enable_healthbar_for_all("bb2_enable_healthbar_for_all", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Display an npc health bar for every npc? Bosses will always show an health bar!", true, 0.0f, true, 1.0f);

extern ConVar v_viewmodel_fov;
extern ConVar voice_modenable;

extern bool IsInCommentaryMode( void );
extern const char* GetWearLocalizationString( float flWear );

CON_COMMAND( cl_reload_localization_files, "Reloads all localization files" )
{
	g_pVGuiLocalize->ReloadLocalizationFiles();
}

#ifdef VOICE_VOX_ENABLE
void VoxCallback( IConVar *var, const char *oldString, float oldFloat )
{
	if ( engine && engine->IsConnected() )
	{
		ConVarRef voice_vox( var->GetName() );
		if ( voice_vox.GetBool() && voice_modenable.GetBool() )
		{
			engine->ClientCmd_Unrestricted( "voicerecord_toggle on\n" );
		}
		else
		{
			engine->ClientCmd_Unrestricted( "voicerecord_toggle off\n" );
		}
	}
}
ConVar voice_vox( "voice_vox", "0", FCVAR_ARCHIVE, "Voice chat uses a vox-style always on", true, 0, true, 1, VoxCallback );

// --------------------------------------------------------------------------------- //
// CVoxManager.
// --------------------------------------------------------------------------------- //
class CVoxManager : public CAutoGameSystem
{
public:
	CVoxManager() : CAutoGameSystem( "VoxManager" )
	{
	}

	virtual void LevelInitPostEntity( void )
	{
		if ( voice_vox.GetBool() && voice_modenable.GetBool() )
		{
			engine->ClientCmd_Unrestricted( "voicerecord_toggle on\n" );
		}
	}

	virtual void LevelShutdownPreEntity( void )
	{
		if ( voice_vox.GetBool() )
		{
			engine->ClientCmd_Unrestricted( "voicerecord_toggle off\n" );
		}
	}
};

static CVoxManager s_VoxManager;
// --------------------------------------------------------------------------------- //
#endif // VOICE_VOX_ENABLE

CON_COMMAND( hud_reloadscheme, "Reloads hud layout and animation scripts." )
{
	ClientModeShared *mode = ( ClientModeShared * )GetClientModeNormal();
	if ( !mode )
		return;

	mode->ReloadScheme(true);
}

#ifdef _DEBUG
CON_COMMAND_F( crash, "Crash the client. Optional parameter -- type of crash:\n 0: read from NULL\n 1: write to NULL\n 2: DmCrashDump() (xbox360 only)", FCVAR_CHEAT )
{
	int crashtype = 0;
	int dummy;
	if ( args.ArgC() > 1 )
	{
		 crashtype = Q_atoi( args[1] );
	}
	switch (crashtype)
	{
		case 0:
			dummy = *((int *) NULL);
			Msg("Crashed! %d\n", dummy); // keeps dummy from optimizing out
			break;
		case 1:
			*((int *)NULL) = 42;
			break;
#if defined( _X360 )
		case 2:
			XBX_CrashDump(false);
			break;
#endif
		default:
			Msg("Unknown variety of crash. You have now failed to crash. I hope you're happy.\n");
			break;
	}
}
#endif // _DEBUG

static void __MsgFunc_Rumble( bf_read &msg )
{
	unsigned char waveformIndex;
	unsigned char rumbleData;
	unsigned char rumbleFlags;

	waveformIndex = msg.ReadByte();
	rumbleData = msg.ReadByte();
	rumbleFlags = msg.ReadByte();

	RumbleEffect( waveformIndex, rumbleData, rumbleFlags );
}

static void __MsgFunc_VGUIMenu( bf_read &msg )
{
	char panelname[2048]; 
	
	msg.ReadString( panelname, sizeof(panelname) );

	bool  bShow = msg.ReadByte()!=0;
	
	IViewPortPanel *viewport = gViewPortInterface->FindPanelByName( panelname );

	if ( !viewport )
	{
		// DevMsg("VGUIMenu: couldn't find panel '%s'.\n", panelname );
		return;
	}

	int count = msg.ReadByte();

	if ( count > 0 )
	{
		KeyValues *keys = new KeyValues("data");

		for ( int i=0; i<count; i++)
		{
			char name[255];
			char data[255];

			msg.ReadString( name, sizeof(name) );
			msg.ReadString( data, sizeof(data) );

			keys->SetString( name, data );
		}

		// !KLUDGE! Whitelist of URL protocols formats for MOTD
		if (
			!V_stricmp( panelname, PANEL_INFO ) // MOTD
			&& keys->GetInt( "type", 0 ) == 2 // URL message type
		) {
			const char *pszURL = keys->GetString( "msg", "" );
			if ( Q_strncmp( pszURL, "http://", 7 ) != 0 && Q_strncmp( pszURL, "https://", 8 ) != 0 && Q_stricmp( pszURL, "about:blank" ) != 0 )
			{
				Warning( "Blocking MOTD URL '%s'; must begin with 'http://' or 'https://' or be about:blank\n", pszURL );
				keys->deleteThis();
				return;
			}
		}		
		
		viewport->SetData( keys );

		keys->deleteThis();
	}

	// is the server telling us to show the scoreboard (at the end of a map)?
	if ( Q_stricmp( panelname, "scores" ) == 0 )
	{
		if ( hud_takesshots.GetBool() == true )
		{
			gHUD.SetScreenShotTime( gpGlobals->curtime + 1.0 ); // take a screenshot in 1 second
		}
	}

	// is the server trying to show an MOTD panel? Check that it's allowed right now.
	ClientModeShared *mode = ( ClientModeShared * )GetClientModeNormal();
	if ( Q_stricmp( panelname, PANEL_INFO ) == 0 && mode )
	{
		if ( !mode->IsInfoPanelAllowed() )
		{
			return;
		}
		else
		{
			mode->InfoPanelDisplayed();
		}
	}

	gViewPortInterface->ShowPanel( viewport, bShow );
}

// Play a Sound depending on failure or success.
static void __MsgFunc_RunBuyCommand( bf_read &msg )
{
	int iValue = msg.ReadByte();

	if ( iValue >= 1 )
		vgui::surface()->PlaySound( "common/wpn_hudoff.wav" );
	else
		vgui::surface()->PlaySound( "common/wpn_moveselect.wav" );
}

// Read note stuff, send to bb2 menu.
static void __MsgFunc_ShowNote( bf_read &msg )
{
	char szHeader[128];
	msg.ReadString( szHeader, 128 );

	char szFile[128];
	msg.ReadString( szFile, 128 );

	GameBaseClient->ShowNote( szHeader, szFile );
}

static void __MsgFunc_ClientEffect(bf_read &msg)
{
	int iType = msg.ReadShort();
	int iState = msg.ReadShort();
	GameBaseClient->RunClientEffect(iType, iState);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeShared::ClientModeShared()
{
	m_pViewport = NULL;
	m_pChatElement = NULL;
	m_pWeaponSelection = NULL;
	m_nRootSize[ 0 ] = m_nRootSize[ 1 ] = -1;

#if defined( REPLAY_ENABLED )
	m_pReplayReminderPanel = NULL;
	m_flReplayStartRecordTime = 0.0f;
	m_flReplayStopRecordTime = 0.0f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeShared::~ClientModeShared()
{
	delete m_pViewport; 
}

void ClientModeShared::ReloadScheme( bool flushLowLevel )
{
	// Invalidate the global cache first.
	if (flushLowLevel)
	{
		KeyValuesSystem()->InvalidateCache();
	}

	m_pViewport->ReloadScheme( "resource/ClientScheme.res" );
	ClearKeyValuesCache();
}

//----------------------------------------------------------------------------
// Purpose: Let the client mode set some vgui conditions
//-----------------------------------------------------------------------------
void	ClientModeShared::ComputeVguiResConditions( KeyValues *pkvConditions ) 
{
	if ( UseVR() )
	{
		pkvConditions->FindKey( "if_vr", true );
	}
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::Init()
{
	m_pChatElement = ( CBaseHudChat * )GET_HUDELEMENT( CHudChat );
	Assert( m_pChatElement );

	m_pWeaponSelection = ( CBaseHudWeaponSelection * )GET_HUDELEMENT( CHudWeaponSelection );
	Assert( m_pWeaponSelection );

	KeyValuesAD pConditions( "conditions" );
	ComputeVguiResConditions( pConditions );

	// Derived ClientMode class must make sure m_Viewport is instantiated
	Assert( m_pViewport );
	m_pViewport->LoadControlSettings( "scripts/HudLayout.res", NULL, NULL, pConditions );

#if defined( REPLAY_ENABLED )
 	m_pReplayReminderPanel = GET_HUDELEMENT( CReplayReminderPanel );
 	Assert( m_pReplayReminderPanel );
#endif

	ListenForGameEvent("player_connect_client");
	ListenForGameEvent("player_disconnect");
	ListenForGameEvent("player_team");
	ListenForGameEvent("server_cvar");
	ListenForGameEvent("player_changename");
	ListenForGameEvent("teamplay_broadcast_audio");
	ListenForGameEvent("achievement_earned");
	ListenForGameEvent("client_sound_transmit");
	ListenForGameEvent("round_start");
	ListenForGameEvent("changelevel");
	ListenForGameEvent("game_achievement");
	ListenForGameEvent("player_connection");

#if defined( REPLAY_ENABLED )
	ListenForGameEvent( "replay_startrecord" );
	ListenForGameEvent( "replay_endrecord" );
	ListenForGameEvent( "replay_replaysavailable" );
	ListenForGameEvent( "replay_servererror" );
	ListenForGameEvent( "game_newmap" );
#endif

#ifndef _XBOX
	HLTVCamera()->Init();
#if defined( REPLAY_ENABLED )
	ReplayCamera()->Init();
#endif
#endif

	m_CursorNone = vgui::dc_none;

	HOOK_MESSAGE(VGUIMenu);
	HOOK_MESSAGE(Rumble);
	HOOK_MESSAGE(RunBuyCommand);
	HOOK_MESSAGE(ShowNote);
	HOOK_MESSAGE(ClientEffect);
}


void ClientModeShared::InitViewport()
{
}


void ClientModeShared::VGui_Shutdown()
{
	delete m_pViewport;
	m_pViewport = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::Shutdown()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : frametime - 
//			*cmd - 
//-----------------------------------------------------------------------------
bool ClientModeShared::CreateMove(float flInputSampleTime, CUserCmd *cmd, bool bFakeInput)
{
	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return true;

	// Let the player at it
	return pPlayer->CreateMove(flInputSampleTime, cmd, bFakeInput);
}

bool ClientModeShared::CreateMove(float flInputSampleTime, CUserCmd *cmd)
{
	return CreateMove(flInputSampleTime, cmd, false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSetup - 
//-----------------------------------------------------------------------------
void ClientModeShared::OverrideView( CViewSetup *pSetup )
{
	QAngle camAngles;

	// Let the player override the view.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if(!pPlayer)
		return;

	pPlayer->OverrideView( pSetup );

	if( ::input->CAM_IsThirdPerson() )
	{
		const Vector& cam_ofs = g_ThirdPersonManager.GetCameraOffsetAngles();
		Vector cam_ofs_distance = g_ThirdPersonManager.GetFinalCameraOffset();

		cam_ofs_distance *= g_ThirdPersonManager.GetDistanceFraction();

		camAngles[ PITCH ] = cam_ofs[ PITCH ];
		camAngles[ YAW ] = cam_ofs[ YAW ];
		camAngles[ ROLL ] = 0;

		Vector camForward, camRight, camUp;
		

		if ( g_ThirdPersonManager.IsOverridingThirdPerson() == false )
		{
			engine->GetViewAngles( camAngles );
		}
			
		// get the forward vector
		AngleVectors( camAngles, &camForward, &camRight, &camUp );
	
		VectorMA( pSetup->origin, -cam_ofs_distance[0], camForward, pSetup->origin );
		VectorMA( pSetup->origin, cam_ofs_distance[1], camRight, pSetup->origin );
		VectorMA( pSetup->origin, cam_ofs_distance[2], camUp, pSetup->origin );

		// Override angles from third person camera
		VectorCopy( camAngles, pSetup->angles );
	}
	else if (::input->CAM_IsOrthographic())
	{
		pSetup->m_bOrtho = true;
		float w, h;
		::input->CAM_OrthographicSize( w, h );
		w *= 0.5f;
		h *= 0.5f;
		pSetup->m_OrthoLeft   = -w;
		pSetup->m_OrthoTop    = -h;
		pSetup->m_OrthoRight  = w;
		pSetup->m_OrthoBottom = h;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeShared::ShouldDrawEntity(C_BaseEntity *pEnt)
{
	return true;
}

bool ClientModeShared::ShouldDrawParticles( )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Allow weapons to override mouse input (for binoculars)
//-----------------------------------------------------------------------------
void ClientModeShared::OverrideMouseInput( float *x, float *y )
{
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->OverrideMouseInput( x, y );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeShared::ShouldDrawViewModel()
{
	return true;
}

bool ClientModeShared::ShouldDrawDetailObjects( )
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if VR mode should black out everything outside the HUD.
//			This is used for things like sniper scopes and full screen UI
//-----------------------------------------------------------------------------
bool ClientModeShared::ShouldBlackoutAroundHUD()
{
	return enginevgui->IsGameUIVisible();
}


//-----------------------------------------------------------------------------
// Purpose: Allows the client mode to override mouse control stuff in sourcevr
//-----------------------------------------------------------------------------
HeadtrackMovementMode_t ClientModeShared::ShouldOverrideHeadtrackControl() 
{
	return HMM_NOOVERRIDE;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ClientModeShared::ShouldDrawCrosshair( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw the current view entity if we are using the fake viewmodel instead
//-----------------------------------------------------------------------------
bool ClientModeShared::ShouldDrawLocalPlayer( C_BasePlayer *pPlayer )
{
	if ( ( pPlayer->index == render->GetViewEntity() ) && !C_BasePlayer::ShouldDrawLocalPlayer() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: The mode can choose to not draw fog
//-----------------------------------------------------------------------------
bool ClientModeShared::ShouldDrawFog( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::PreRender( CViewSetup *pSetup )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::PostRender()
{
	// Let the particle manager simulate things that haven't been simulated.
	ParticleMgr()->PostRender();
}

void ClientModeShared::PostRenderVGui()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::Update()
{
#if defined( REPLAY_ENABLED )
	UpdateReplayMessages();
#endif

	if ( m_pViewport->IsVisible() != cl_drawhud.GetBool() )
	{
		m_pViewport->SetVisible( cl_drawhud.GetBool() );
	}

	UpdateRumbleEffects();

	if ( cl_show_num_particle_systems.GetBool() )
	{
		int nCount = 0;

		for ( int i = 0; i < g_pParticleSystemMgr->GetParticleSystemCount(); i++ )
		{
			const char *pParticleSystemName = g_pParticleSystemMgr->GetParticleSystemNameFromIndex(i);
			CParticleSystemDefinition *pParticleSystem = g_pParticleSystemMgr->FindParticleSystem( pParticleSystemName );
			if ( !pParticleSystem )
				continue;

			for ( CParticleCollection *pCurCollection = pParticleSystem->FirstCollection();
				  pCurCollection != NULL;
				  pCurCollection = pCurCollection->GetNextCollectionUsingSameDef() )
			{
				++nCount;
			}
		}

		engine->Con_NPrintf( 0, "# Active particle systems: %i", nCount );
	}
}

//-----------------------------------------------------------------------------
// This processes all input before SV Move messages are sent
//-----------------------------------------------------------------------------

void ClientModeShared::ProcessInput(bool bActive)
{
	gHUD.ProcessInput( bActive );
}

//-----------------------------------------------------------------------------
// Purpose: We've received a keypress from the engine. Return 1 if the engine is allowed to handle it.
//-----------------------------------------------------------------------------
int	ClientModeShared::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( engine->Con_IsVisible() )
		return 1;
	
	if (HL2MPRules() && (HL2MPRules()->m_iCurrentVoteType != 0) && !BB2PlayerGlobals->GetPlayerVoteResponse() && BB2PlayerGlobals->IsVotePanelActive())
	{
		if (keynum == KEY_F1)
		{
			BB2PlayerGlobals->SetPlayerVoteResponse(1);
			engine->ClientCmd_Unrestricted("player_vote_yes\n");
			return 0;
		}
		else if (keynum == KEY_F2)
		{
			BB2PlayerGlobals->SetPlayerVoteResponse(2);
			engine->ClientCmd_Unrestricted("player_vote_no\n");
			return 0;
		}
	}

	// Should we start typing a message?
	if ( pszCurrentBinding &&
		( Q_strcmp( pszCurrentBinding, "messagemode" ) == 0 ||
		  Q_strcmp( pszCurrentBinding, "say" ) == 0 ) )
	{
		if ( down )
		{
			StartMessageMode( MM_SAY );
		}
		return 0;
	}
	else if ( pszCurrentBinding &&
				( Q_strcmp( pszCurrentBinding, "messagemode2" ) == 0 ||
				  Q_strcmp( pszCurrentBinding, "say_team" ) == 0 ) )
	{
		if ( down )
		{
			StartMessageMode( MM_SAY_TEAM );
		}
		return 0;
	}
	
	// If we're voting...
#ifdef VOTING_ENABLED
	CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
	if ( pHudVote && pHudVote->IsVisible() )
	{
		if ( !pHudVote->KeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}
#endif

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// if ingame spectator mode, let spectator input intercept key event here
	if( pPlayer &&
		( pPlayer->GetObserverMode() > OBS_MODE_DEATHCAM ) &&
		!HandleSpectatorKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	// Let game-specific hud elements get a crack at the key input
	if ( !HudElementKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		return pWeapon->KeyInput( down, keynum, pszCurrentBinding );
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: See if spectator input occurred. Return 0 if the key is swallowed.
//-----------------------------------------------------------------------------
int ClientModeShared::HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	// we are in spectator mode, open spectator menu
	if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+duck" ) == 0 )
	{
		//m_pViewport->ShowPanel( PANEL_SPECMENU, true ); Disable this one for BB2... Maybe replace it later?
		return 0; // we handled it, don't handle twice or send to server
	}
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+attack" ) == 0 )
	{
		engine->ClientCmd( "spec_next" );
		return 0;
	}
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+attack2" ) == 0 )
	{
		engine->ClientCmd( "spec_prev" );
		return 0;
	}
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+jump" ) == 0 )
	{
		engine->ClientCmd( "spec_mode" );
		return 0;
	}
	else if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+strafe" ) == 0 )
	{
		HLTVCamera()->SetAutoDirector( true );
#if defined( REPLAY_ENABLED )
		ReplayCamera()->SetAutoDirector( true );
#endif
		return 0;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: See if hud elements want key input. Return 0 if the key is swallowed
//-----------------------------------------------------------------------------
int ClientModeShared::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( m_pWeaponSelection )
	{
		if ( !m_pWeaponSelection->KeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

#if defined( REPLAY_ENABLED )
	if ( m_pReplayReminderPanel )
	{
		if ( m_pReplayReminderPanel->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}
#endif

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeShared::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
#if defined( REPLAY_ENABLED )
	if ( engine->IsPlayingDemo() )
	{
		if ( !replay_rendersetting_renderglow.GetBool() )
			return false;
	}
#endif 

#ifdef BB2_GLOWS
	g_GlowObjectManager.RenderGlowEffects( pSetup );
#endif //BB2_GLOWS

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *ClientModeShared::GetMessagePanel()
{
	if ( m_pChatElement && m_pChatElement->GetInputPanel() && m_pChatElement->GetInputPanel()->IsVisible() )
		return m_pChatElement->GetInputPanel();

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: The player has started to type a message
//-----------------------------------------------------------------------------
void ClientModeShared::StartMessageMode( int iMessageModeType )
{
	// Can only show chat UI in multiplayer!!!
	if ( gpGlobals->maxClients == 1 )
	{
		return;
	}
	if ( m_pChatElement )
	{
		m_pChatElement->StartMessageMode( iMessageModeType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void ClientModeShared::LevelInit( const char *newmap )
{
	engine->ClientCmd_Unrestricted("progress_enable\n");

	GameBaseShared()->GetGameInventory().Purge();
	GameBaseClient->CloseGamePanels();

	if (GetHealthBarHUD())
		GetHealthBarHUD()->Cleanup();

	m_pViewport->GetAnimationController()->StartAnimationSequence("LevelInit");

	// Tell the Chat Interface
	if ( m_pChatElement )
	{
		m_pChatElement->LevelInit( newmap );
	}

	// we have to fake this event clientside, because clients connect after that
	IGameEvent *event = gameeventmanager->CreateEvent( "game_newmap" );
	if ( event )
	{
		event->SetString("mapname", newmap );
		gameeventmanager->FireEventClientSide( event );
	}

	// Create a vgui context for all of the in-game vgui panels...
	if ( s_hVGuiContext == DEFAULT_VGUI_CONTEXT )
	{
		s_hVGuiContext = vgui::ivgui()->CreateContext();
	}

	// Reset any player explosion/shock effects
	CLocalPlayerFilter filter;
	enginesound->SetPlayerDSP( filter, 0, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeShared::LevelShutdown( void )
{
	engine->ClientCmd_Unrestricted("progress_enable\n");

	GameBaseShared()->GetGameInventory().Purge();
	GameBaseClient->CloseGamePanels();

	if (GetHealthBarHUD())
		GetHealthBarHUD()->Cleanup();

	// Reset the third person camera so we don't crash
	g_ThirdPersonManager.Init();

	if ( m_pChatElement )
	{
		m_pChatElement->LevelShutdown();
	}
	if ( s_hVGuiContext != DEFAULT_VGUI_CONTEXT )
	{
		vgui::ivgui()->DestroyContext( s_hVGuiContext );
 		s_hVGuiContext = DEFAULT_VGUI_CONTEXT;
	}

	// Reset any player explosion/shock effects
	CLocalPlayerFilter filter;
	enginesound->SetPlayerDSP( filter, 0, true );
}


void ClientModeShared::Enable()
{
	vgui::VPANEL pRoot = VGui_GetClientDLLRootPanel();

	// Add our viewport to the root panel.
	if( pRoot != 0 )
	{
		m_pViewport->SetParent( pRoot );
	}

	// All hud elements should be proportional
	// This sets that flag on the viewport and all child panels
	m_pViewport->SetProportional( true );

	m_pViewport->SetCursor( m_CursorNone );
	vgui::surface()->SetCursor( m_CursorNone );

	m_pViewport->SetVisible( true );
	if ( m_pViewport->IsKeyBoardInputEnabled() )
	{
		m_pViewport->RequestFocus();
	}

	Layout();
}


void ClientModeShared::Disable()
{
	vgui::VPANEL pRoot = VGui_GetClientDLLRootPanel();

	// Remove our viewport from the root panel.
	if( pRoot != 0 )
	{
		m_pViewport->SetParent( (vgui::VPANEL)NULL );
	}

	m_pViewport->SetVisible( false );
}


void ClientModeShared::Layout()
{
	vgui::VPANEL pRoot = VGui_GetClientDLLRootPanel();
	int wide, tall;

	// Make the viewport fill the root panel.
	if( pRoot != 0 )
	{
		vgui::ipanel()->GetSize(pRoot, wide, tall);

		bool changed = wide != m_nRootSize[ 0 ] || tall != m_nRootSize[ 1 ];
		m_nRootSize[ 0 ] = wide;
		m_nRootSize[ 1 ] = tall;

		m_pViewport->SetBounds(0, 0, wide, tall);
		if ( changed )
		{
			ReloadScheme(false);
		}
	}
}

float ClientModeShared::GetViewModelFOV( void )
{
	return v_viewmodel_fov.GetFloat();
}

class CHudChat;

bool PlayerNameNotSetYet( const char *pszName )
{
	if ( pszName && pszName[0] )
	{
		// Don't show "unconnected" if we haven't got the players name yet
		if ( Q_strnicmp(pszName,"unconnected",11) == 0 )
			return true;
		if ( Q_strnicmp(pszName,"NULLNAME",11) == 0 )
			return true;
	}

	return false;
}

void ClientModeShared::FireGameEvent( IGameEvent *event )
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();

	const char *eventname = event->GetName();

	if (Q_strcmp("changelevel", eventname) == 0)
	{
		const char *szMap = event->GetString("map");
		GameBaseClient->Changelevel(szMap);
	}
	else if ((Q_strcmp("player_connection", eventname) == 0))
	{
		bool bState = event->GetBool("state");

		if (!g_PR)
			return;

		if (HL2MPRules() && ((HL2MPRules()->GetCurrentGamemode() != MODE_OBJECTIVE) && (HL2MPRules()->GetCurrentGamemode() != MODE_ARENA)))
			return;

		char szLocalized[100];
	
		if (bState)
		{
			g_pVGuiLocalize->ConvertUnicodeToANSI(g_pVGuiLocalize->Find("#NOTIFICATION_LEAVE"), szLocalized, sizeof(szLocalized));
			m_pChatElement->Printf(CHAT_FILTER_ACHIEVEMENT, "%c%s", COLOR_CREEPY, szLocalized);
		}
		else
		{
			g_pVGuiLocalize->ConvertUnicodeToANSI(g_pVGuiLocalize->Find("#NOTIFICATION_JOIN"), szLocalized, sizeof(szLocalized));
			m_pChatElement->Printf(CHAT_FILTER_ACHIEVEMENT, "%c%s", COLOR_CREEPY, szLocalized);
		}
	}
	else if (Q_strcmp("game_achievement", eventname) == 0)
	{
		const char *szAchievement = event->GetString("ach_str");
		int m_iIndex = event->GetInt("index");
		int m_iType = event->GetInt("type");

		if (g_PR)
		{
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode(g_PR->GetPlayerName(m_iIndex), wszPlayerName, sizeof(wszPlayerName));

			wchar_t wszAchievementName[128];
			g_pVGuiLocalize->ConvertANSIToUnicode(steamapicontext->SteamUserStats()->GetAchievementDisplayAttribute(szAchievement, "name"), wszAchievementName, sizeof(wszAchievementName));

			wchar_t wszLocalized[256];

			if (m_iType != ACHIEVEMENT_TYPE_REWARD)
				g_pVGuiLocalize->ConstructString(wszLocalized, sizeof(wszLocalized), g_pVGuiLocalize->Find("#NOTIFICATION_ACHIEVEMENT"), 2, wszPlayerName, wszAchievementName);
			else
				g_pVGuiLocalize->ConstructString(wszLocalized, sizeof(wszLocalized), g_pVGuiLocalize->Find("#NOTIFICATION_MASTERY"), 2, wszPlayerName, wszAchievementName);

			char szLocalized[256];
			g_pVGuiLocalize->ConvertUnicodeToANSI(wszLocalized, szLocalized, sizeof(szLocalized));

			m_pChatElement->Printf(CHAT_FILTER_ACHIEVEMENT, "%c%s", COLOR_ACHIEVEMENT, szLocalized);
		}		

		if (pClient && (pClient->entindex() == m_iIndex))
		{
			// Play Congratulation Sound...
			// If this is some mastery we'll reload the inventory.
			//if (m_iType >= ACHIEVEMENT_TYPE_REWARD)
			//
		}
	}
	else if (Q_strcmp("round_start", eventname) == 0)
	{
		RemoveAllClientGibs();

		if (pClient && (pClient->GetTeamNumber() >= TEAM_HUMANS))
			GameBaseClient->CloseGamePanels(true);
	}
	else if ( Q_strcmp( "client_sound_transmit", eventname ) == 0 )
	{
		int iEntIndex = event->GetInt("entity");
		int iType = event->GetInt("type");
		int iPlayerIndexForce = event->GetInt("playerindex");
		bool bGender = event->GetBool("gender");
		const char *szOriginal = event->GetString("original");
		const char *szGender = bGender ? "Male" : "Female";
		const char *szSurvivorLink = event->GetString("survivorlink");
		const char *szSoundsetPrefix = event->GetString("survivorprefix");
		char szSoundToEmit[256];
		szSoundToEmit[0] = 0;
		bool bIsDeathmatchAnnouncer = ((iEntIndex == -1) && (iType == BB2_SoundTypes::TYPE_ANNOUNCER));
		bool bIsPlayerSound = ((iType == BB2_SoundTypes::TYPE_PLAYER) || (iType == BB2_SoundTypes::TYPE_DECEASED));
		if (bIsPlayerSound)
		{
			const DataPlayerItem_Survivor_Shared_t *data = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(szSurvivorLink, true);
			if (data != NULL) // Override gender check:			
				szGender = data->bGender ? "Male" : "Female";
		}

		if (pClient && g_PR)
		{
			// Filter:
			if ((iPlayerIndexForce != 0) && (pClient->entindex() != iPlayerIndexForce))
				return;

			const char *entName = GameBaseShared()->GetSharedGameDetails()->GetEntityNameFromEntitySoundType(iType);
			bool bEmitSound = false;
			int soundSetIndex = GameBaseShared()->GetSharedGameDetails()->GetConVarValueForEntitySoundType(iType);

			if (iType == BB2_SoundTypes::TYPE_CUSTOM)
			{
				C_BaseEntity *pEntity = ClientEntityList().GetEnt(iEntIndex);
				if (pEntity && pEntity->IsNPC() && pEntity->MyNPCPointer())
				{
					bEmitSound = true;
					Q_snprintf(szSoundToEmit, 256, "Custom.%s.%s.%s", pEntity->MyNPCPointer()->GetNPCName(), szOriginal, szGender);
				}
			}
			else if (bIsPlayerSound)
			{
				bEmitSound = true;
				Q_snprintf(szSoundToEmit, 256, "%s_%s_%s.%s", entName, GameBaseShared()->GetSharedGameDetails()->GetPlayerSoundsetPrefix(iType, szSurvivorLink, szSoundsetPrefix), szGender, szOriginal);
			}
			else if (soundSetIndex != -1)
			{
				if (bIsDeathmatchAnnouncer)
				{
					Q_snprintf(szSoundToEmit, 256, "%s_%s.%s", entName, GameBaseShared()->GetSharedGameDetails()->GetSoundPrefix(iType, soundSetIndex), szOriginal);
					if (szSoundToEmit[0] && (strlen(szSoundToEmit) > 0))
					{
						// If this sound doesn't exist / not parsed then fallback to anything available:
						CSoundParameters params;
						if (pClient->GetParametersForSound(szSoundToEmit, params, NULL) == false)
							Q_snprintf(szSoundToEmit, 256, "%s_%s.%s", entName, GameBaseShared()->GetSharedGameDetails()->GetSoundPrefix(iType, 0), szOriginal);

						if (!enginesound->IsSoundPrecached(szSoundToEmit))
							pClient->PrecacheScriptSound(szSoundToEmit);

						CLocalPlayerFilter filter;
						pClient->EmitSound(filter, pClient->entindex(), szSoundToEmit, &pClient->GetLocalOrigin());
					}
				}
				else
				{
					bEmitSound = true;
					Q_snprintf(szSoundToEmit, 256, "%s_%s_%s.%s", entName, GameBaseShared()->GetSharedGameDetails()->GetSoundPrefix(iType, soundSetIndex, szSurvivorLink), szGender, szOriginal);
				}
			}

			if (bEmitSound)
			{
				C_BaseEntity *pEntity = ClientEntityList().GetEnt(iEntIndex);
				if (szSoundToEmit[0] && pEntity && (strlen(szSoundToEmit) > 0))
				{
					// If this sound doesn't exist / not parsed then fallback to anything available:
					CSoundParameters params;
					if (pClient->GetParametersForSound(szSoundToEmit, params, NULL) == false)
					{
						if ((iType == BB2_SoundTypes::TYPE_CUSTOM) || (iType == BB2_SoundTypes::TYPE_UNKNOWN))
							return;

						if (pEntity->IsPlayer()) // For players we simplify it a bit.
							Q_snprintf(szSoundToEmit, 256, "%s_%s.%s", entName, szGender, szOriginal);
						else // For NPC we use the default (first) soundset.
							Q_snprintf(szSoundToEmit, 256, "%s_%s_%s.%s", entName, GameBaseShared()->GetSharedGameDetails()->GetSoundPrefix(iType, 0), szGender, szOriginal);
					}

					if (!enginesound->IsSoundPrecached(szSoundToEmit))
						pClient->PrecacheScriptSound(szSoundToEmit);

					CLocalPlayerFilter filter;
					pEntity->EmitSound(filter, pEntity->entindex(), szSoundToEmit, &pEntity->GetLocalOrigin());
				}
			}
		}
	}
	else if ( Q_strcmp( "player_connect_client", eventname ) == 0 )
	{
		if (!m_pChatElement)
			return;

		if ( PlayerNameNotSetYet(event->GetString("name")) )
			return;

		if ( !IsInCommentaryMode() )
		{
			wchar_t wszLocalized[100];
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString("name"), wszPlayerName, sizeof(wszPlayerName) );
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_joined_game" ), 1, wszPlayerName );

			char szLocalized[100];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

			m_pChatElement->Printf(CHAT_FILTER_JOINLEAVE, "%s", szLocalized);
		}
	}
	else if ( Q_strcmp( "player_disconnect", eventname ) == 0 )
	{
		C_BasePlayer *pPlayer = USERID2PLAYER( event->GetInt("userid") );

		if (!m_pChatElement || !pPlayer)
			return;
		if ( PlayerNameNotSetYet(event->GetString("name")) )
			return;

		if ( !IsInCommentaryMode() )
		{
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName) );

			wchar_t wszReason[64];
			const char *pszReason = event->GetString( "reason" );
			if ( pszReason && ( pszReason[0] == '#' ) && g_pVGuiLocalize->Find( pszReason ) )
			{
				V_wcsncpy( wszReason, g_pVGuiLocalize->Find( pszReason ), sizeof( wszReason ) );
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode( pszReason, wszReason, sizeof(wszReason) );
			}

			wchar_t wszLocalized[100];
			if (IsPC())
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_left_game" ), 2, wszPlayerName, wszReason );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_left_game" ), 1, wszPlayerName );
			}

			char szLocalized[100];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

			m_pChatElement->Printf(CHAT_FILTER_JOINLEAVE, "%s", szLocalized);
		}
	}
	else if ( Q_strcmp( "player_team", eventname ) == 0 )
	{
		C_BasePlayer *pPlayer = USERID2PLAYER(event->GetInt("userid"));
		if (!m_pChatElement)
			return;

		bool bDisconnected = event->GetBool("disconnect");
		if (bDisconnected)
			return;

		int team = event->GetInt("team");
		bool bAutoTeamed = event->GetInt("autoteam", false);
		bool bSilent = event->GetInt("silent", false);

		const char *pszName = event->GetString("name");
		if (PlayerNameNotSetYet(pszName))
			return;

		if ((HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION))
			bSilent = true;
		else
		{
			bSilent = true;
			if (pPlayer && g_PR)
			{
				int selectedTeam = g_PR->GetSelectedTeam(pPlayer->entindex());
				if (selectedTeam <= 0 || (selectedTeam != team && (team > TEAM_SPECTATOR)))
				{
					bSilent = false;
				}
			}
		}

		if (!bSilent)
		{
			wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
			g_pVGuiLocalize->ConvertANSIToUnicode(pszName, wszPlayerName, sizeof(wszPlayerName));

			wchar_t wszTeam[64];
			C_Team *pTeam = GetGlobalTeam(team);
			if (pTeam)
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(pTeam->Get_Name(), wszTeam, sizeof(wszTeam));
			}
			else
			{
				_snwprintf(wszTeam, sizeof(wszTeam) / sizeof(wchar_t), L"%d", team);
			}

			if (!IsInCommentaryMode())
			{
				wchar_t wszLocalized[100];
				if (bAutoTeamed)
				{
					g_pVGuiLocalize->ConstructString(wszLocalized, sizeof(wszLocalized), g_pVGuiLocalize->Find("#game_player_joined_autoteam"), 2, wszPlayerName, wszTeam);
				}
				else
				{
					g_pVGuiLocalize->ConstructString(wszLocalized, sizeof(wszLocalized), g_pVGuiLocalize->Find("#game_player_joined_team"), 2, wszPlayerName, wszTeam);
				}

				char szLocalized[100];
				g_pVGuiLocalize->ConvertUnicodeToANSI(wszLocalized, szLocalized, sizeof(szLocalized));

				m_pChatElement->Printf(CHAT_FILTER_TEAMCHANGE, "%s", szLocalized);
			}
		}

		if (pPlayer && pPlayer->IsLocalPlayer())
		{
			// that's me
			pPlayer->TeamChange(team);
		}
	}
	else if ( Q_strcmp( "player_changename", eventname ) == 0 )
	{
		if (!m_pChatElement)
			return;

		const char *pszOldName = event->GetString("oldname");
		if ( PlayerNameNotSetYet(pszOldName) )
			return;

		wchar_t wszOldName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( pszOldName, wszOldName, sizeof(wszOldName) );

		wchar_t wszNewName[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString( "newname" ), wszNewName, sizeof(wszNewName) );

		wchar_t wszLocalized[100];
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_player_changed_name" ), 2, wszOldName, wszNewName );

		char szLocalized[100];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

		m_pChatElement->Printf(CHAT_FILTER_NAMECHANGE, "%s", szLocalized);
	}
	else if (Q_strcmp( "teamplay_broadcast_audio", eventname ) == 0 )
	{
		int team = event->GetInt( "team" );

		bool bValidTeam = false;

		if ( (GetLocalTeam() && GetLocalTeam()->GetTeamNumber() == team) )
		{
			bValidTeam = true;
		}

		//If we're in the spectator team then we should be getting whatever messages the person I'm spectating gets.
		if ( bValidTeam == false )
		{
			CBasePlayer *pSpectatorTarget = UTIL_PlayerByIndex( GetSpectatorTarget() );

			if ( pSpectatorTarget && (GetSpectatorMode() == OBS_MODE_IN_EYE || GetSpectatorMode() == OBS_MODE_CHASE) )
			{
				if ( pSpectatorTarget->GetTeamNumber() == team )
				{
					bValidTeam = true;
				}
			}
		}

		if ( team == 0 && GetLocalTeam() > 0 )
		{
			bValidTeam = false;
		}

		if ( team == 255 )
		{
			bValidTeam = true;
		}

		if ( bValidTeam == true )
		{
			EmitSound_t et;
			et.m_pSoundName = event->GetString("sound");
			et.m_nFlags = event->GetInt("additional_flags");

			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, et );
		}
	}
	else if ( Q_strcmp( "server_cvar", eventname ) == 0 )
	{
		const char *cvar = event->GetString("cvarname");
		bool bIsTags = false;
		if (cvar && (strlen(cvar) > 0))
			bIsTags = !strcmp(cvar, "sv_tags");

		if (!IsInCommentaryMode() && !bIsTags)
		{
			wchar_t wszCvarName[64];
			g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString("cvarname"), wszCvarName, sizeof(wszCvarName) );

			wchar_t wszCvarValue[64];
			g_pVGuiLocalize->ConvertANSIToUnicode( event->GetString("cvarvalue"), wszCvarValue, sizeof(wszCvarValue) );

			wchar_t wszLocalized[256];
			g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#game_server_cvar_changed" ), 2, wszCvarName, wszCvarValue );

			char szLocalized[256];
			g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof(szLocalized) );

			m_pChatElement->Printf(CHAT_FILTER_SERVERMSG, "%s", szLocalized);
		}
	}
	else if ( Q_strcmp( "achievement_earned", eventname ) == 0 )
	{
		int iPlayerIndex = event->GetInt( "player" );
		C_BasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
		int iAchievement = event->GetInt( "achievement" );

		if (!m_pChatElement || !pPlayer)
			return;

		if ( !IsInCommentaryMode() )
		{
			CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
			if ( !pAchievementMgr )
				return;

			IAchievement *pAchievement = pAchievementMgr->GetAchievementByID( iAchievement );
			if ( pAchievement )
			{
				if ( !pPlayer->IsDormant() && pPlayer->ShouldAnnounceAchievement() )
				{
					pPlayer->SetNextAchievementAnnounceTime( gpGlobals->curtime + ACHIEVEMENT_ANNOUNCEMENT_MIN_TIME );

					// no particle effect if the local player is the one with the achievement or the player is dead
					if ( !pPlayer->IsLocalPlayer() && pPlayer->IsAlive() ) 
					{
						//tagES using the "head" attachment won't work for CS and DoD
						pPlayer->ParticleProp()->Create( "achieved", PATTACH_POINT_FOLLOW, "head" );
					}

					pPlayer->OnAchievementAchieved( iAchievement );
				}

				if ( g_PR )
				{
					wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
					g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iPlayerIndex ), wszPlayerName, sizeof( wszPlayerName ) );

					const wchar_t *pchLocalizedAchievement = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pAchievement->GetName() );
					if ( pchLocalizedAchievement )
					{
						wchar_t wszLocalizedString[128];
						g_pVGuiLocalize->ConstructString( wszLocalizedString, sizeof( wszLocalizedString ), g_pVGuiLocalize->Find( "#Achievement_Earned" ), 2, wszPlayerName, pchLocalizedAchievement );

						char szLocalized[128];
						g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedString, szLocalized, sizeof( szLocalized ) );

						m_pChatElement->ChatPrintf(iPlayerIndex, CHAT_FILTER_SERVERMSG, "%s", szLocalized);
					}
				}
			}
		}
	}
#if defined( REPLAY_ENABLED )
	else if ( !V_strcmp( "replay_servererror", eventname ) )
	{
		DisplayReplayMessage( event->GetString( "error", "#Replay_DefaultServerError" ), replay_msgduration_error.GetFloat(), true, NULL, false );
	}
	else if ( !V_strcmp( "replay_startrecord", eventname ) )
	{
		m_flReplayStartRecordTime = gpGlobals->curtime;
	}
	else if ( !V_strcmp( "replay_endrecord", eventname ) )
	{
		m_flReplayStopRecordTime = gpGlobals->curtime;
	}
	else if ( !V_strcmp( "replay_replaysavailable", eventname ) )
	{
		DisplayReplayMessage( "#Replay_ReplaysAvailable", replay_msgduration_replaysavailable.GetFloat(), false, NULL, false );
	}

	else if ( !V_strcmp( "game_newmap", eventname ) )
	{
		// Make sure the instance count is reset to 0.  Sometimes the count stay in sync and we get replay messages displaying lower than they should.
		CReplayMessagePanel::RemoveAll();
	}
#endif

	else
	{
		DevMsg( 2, "Unhandled GameEvent in ClientModeShared::FireGameEvent - %s\n", event->GetName()  );
	}
}

void ClientModeShared::UpdateReplayMessages()
{
#if defined( REPLAY_ENABLED )
	// Received a replay_startrecord event?
	if ( m_flReplayStartRecordTime != 0.0f )
	{
		DisplayReplayMessage( "#Replay_StartRecord", replay_msgduration_startrecord.GetFloat(), true, "replay\\startrecord.mp3", false );

		m_flReplayStartRecordTime = 0.0f;
		m_flReplayStopRecordTime = 0.0f;
	}

	// Received a replay_endrecord event?
	if ( m_flReplayStopRecordTime != 0.0f )
	{
		DisplayReplayMessage( "#Replay_EndRecord", replay_msgduration_stoprecord.GetFloat(), true, "replay\\stoprecord.wav", false );

		// Hide the replay reminder
		if ( m_pReplayReminderPanel )
		{
			m_pReplayReminderPanel->Hide();
		}

		m_flReplayStopRecordTime = 0.0f;
	}

	if ( !engine->IsConnected() )
	{
		ClearReplayMessageList();
	}
#endif
}

void ClientModeShared::ClearReplayMessageList()
{
#if defined( REPLAY_ENABLED )
	CReplayMessagePanel::RemoveAll();
#endif
}

void ClientModeShared::DisplayReplayMessage( const char *pLocalizeName, float flDuration, bool bUrgent,
											 const char *pSound, bool bDlg )
{
#if defined( REPLAY_ENABLED )
	// Don't display during replay playback, and don't allow more than 4 at a time
	const bool bInReplay = g_pEngineClientReplay->IsPlayingReplayDemo();
	if ( bInReplay || ( !bDlg && CReplayMessagePanel::InstanceCount() >= 4 ) )
		return;

	// Use default duration?
	if ( flDuration == -1.0f )
	{
		flDuration = replay_msgduration_misc.GetFloat();
	}

	// Display a replay message
	if ( bDlg )
	{
		if ( engine->IsInGame() )
		{
			Panel *pPanel = new CReplayMessageDlg( pLocalizeName );
			pPanel->SetVisible( true );
			pPanel->MakePopup();
			pPanel->MoveToFront();
			pPanel->SetKeyBoardInputEnabled( true );
			pPanel->SetMouseInputEnabled( true );
		}
		else
		{
			ShowMessageBox( "#Replay_GenericMsgTitle", pLocalizeName, "#GameUI_OK" );
		}
	}
	else
	{
		CReplayMessagePanel *pMsgPanel = new CReplayMessagePanel( pLocalizeName, flDuration, bUrgent );
		pMsgPanel->Show();
	}

	// Play a sound if appropriate
	if ( pSound )
	{
		surface()->PlaySound( pSound );
	}
#endif
}

void ClientModeShared::DisplayReplayReminder()
{
#if defined( REPLAY_ENABLED )
	if ( m_pReplayReminderPanel && g_pReplay->IsRecording() )
	{
		// Only display the panel if we haven't already requested a replay for the given life
		CReplay *pCurLifeReplay = static_cast< CReplay * >( g_pClientReplayContext->GetReplayManager()->GetReplayForCurrentLife() );
		if ( pCurLifeReplay && !pCurLifeReplay->m_bRequestedByUser && !pCurLifeReplay->m_bSaved )
		{
			m_pReplayReminderPanel->Show();
		}
	}
#endif
}


//-----------------------------------------------------------------------------
// In-game VGUI context 
//-----------------------------------------------------------------------------
void ClientModeShared::ActivateInGameVGuiContext( vgui::Panel *pPanel )
{
	vgui::ivgui()->AssociatePanelWithContext( s_hVGuiContext, pPanel->GetVPanel() );
	vgui::ivgui()->ActivateContext( s_hVGuiContext );
}

void ClientModeShared::DeactivateInGameVGuiContext()
{
	vgui::ivgui()->ActivateContext( DEFAULT_VGUI_CONTEXT );
}


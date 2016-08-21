//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the implementation of game rules for multiplayer.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "cdll_int.h"
#include "multiplay_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "mp_shareddefs.h"
#include "utlbuffer.h"
#include "GameBase_Shared.h"
#include "movevars_shared.h"

#ifdef CLIENT_DLL
    #include "c_hl2mp_player.h"
#else
    #include "hl2mp_player.h"
	#include "eventqueue.h"
	#include "player.h"
	#include "basecombatweapon.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "in_buttons.h" 
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "hltvdirector.h"
	#include "AI_Criteria.h"
	#include "sceneentity.h"
	#include "basemultiplayerplayer.h"
	#include "team.h"
	#include "usermessages.h"
	#include "tier0/icommandline.h"
    #include "GameBase_Server.h"

#ifdef NEXT_BOT
	#include "NextBotManager.h"
#endif

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CMultiplayRules );

ConVar mp_chattime(
		"mp_chattime", 
		"10", 
		FCVAR_REPLICATED,
		"Amount of time players can chat after the game or round is over",
		true, 4.0f,
		true, 120.0f );

// BB2
ConVar mp_timelimit("mp_timelimit", "30", FCVAR_REPLICATED, "Set the timelimit in minutes.", true, 10.0f, false, 0.0f);
ConVar mp_limitteams("mp_limitteams", "1", FCVAR_REPLICATED, "If the player count difference between two teams are bigger than this value it will auto-balance the teams.", true, 1.0f, false, 0.0f);

ConVar bb2_elimination_fraglimit("bb2_elimination_fraglimit", "200", FCVAR_REPLICATED, "Set the max amount of frags a team can obtain before that team will win the game.", true, 50.0f, false, 500.0f);
ConVar bb2_elimination_respawn_time("bb2_elimination_respawn_time", "1.5", FCVAR_REPLICATED, "Set the default respawn time in seconds.", true, 1.0f, true, 30.0f);
ConVar bb2_elimination_respawn_time_scale("bb2_elimination_respawn_time_scale", "0.5", FCVAR_REPLICATED, "Increase the respawn time by this many seconds for the teams.", true, 0.0f, true, 10.0f);
ConVar bb2_elimination_score_from_extermination("bb2_elimination_score_from_extermination", "10", FCVAR_REPLICATED, "How much score should a team be awarded for exterminating the other team? (kill everyone and allow no one to respawn)", true, 0.0f, true, 50.0f);
ConVar bb2_elimination_teamperk_kills_required("bb2_elimination_teamperk_kills_required", "10", FCVAR_REPLICATED, "How many kills does a team need to get in order to activate a team perk?", true, 10.0f, true, 100.0f);
ConVar bb2_elimination_teamperk_duration("bb2_elimination_teamperk_duration", "12", FCVAR_REPLICATED, "How many seconds does a team perk last?", true, 10.0f, true, 30.0f);
ConVar bb2_elimination_score_zombies("bb2_elimination_score_zombies", "4", FCVAR_REPLICATED, "Whenever the zombie team gets a kill this is the value to add.", true, 1.0f, true, 10.0f);
ConVar bb2_elimination_score_humans("bb2_elimination_score_humans", "1", FCVAR_REPLICATED, "Whenever the human team gets a zombie player kill this is the value to add.", true, 1.0f, true, 10.0f);
ConVar bb2_elimination_teammate_distance("bb2_elimination_teammate_distance", "1000", FCVAR_REPLICATED, "When a teammate is this much away from me then I will glow him/her.", true, 500.0f, true, 4000.0f);

ConVar bb2_arena_respawn_time("bb2_arena_respawn_time", "40", FCVAR_REPLICATED, "Server respawn interval, respawns waiting reinforcements every X seconds! Minimum 20 sec.", true, 20.0f, false, 0.0f);
ConVar bb2_arena_reinforcement_count("bb2_arena_reinforcement_count", "14", FCVAR_REPLICATED, "How many respawns will you allow?", true, 0.0f, true, 100.0f);

ConVar bb2_deathmatch_respawn_time("bb2_deathmatch_respawn_time", "3", FCVAR_REPLICATED, "Set the default respawn time in seconds.", true, 1.0f, true, 30.0f);
ConVar bb2_deathmatch_fraglimit("bb2_deathmatch_fraglimit", "60", FCVAR_REPLICATED, "Set the max amount of frags a person can get before the game ends.", true, 20.0f, false, 1000.0f);

ConVar bb2_story_respawn_time("bb2_story_respawn_time", "10", FCVAR_REPLICATED, "Set the default respawn time in seconds.", true, 4.0f, true, 30.0f);
ConVar bb2_classic_mode_enabled("bb2_classic_mode_enabled", "0", FCVAR_REPLICATED, "This will enable BrainBread Classic Mode for Objective Mode. Enabling this will prevent non Walker and Military NPCs from spawning.", true, 0.0f, true, 1.0f);
ConVar bb2_classic_zombie_noteamchange("bb2_classic_zombie_noteamchange", "1", FCVAR_REPLICATED, "When a player zombie kills a human player the human will not become a zombie on respawn.", true, 0.0f, true, 1.0f);

ConVar bb2_roundstart_freezetime("bb2_roundstart_freezetime", "8", FCVAR_REPLICATED, "How many sec until the round starts?", true, 1.0f, true, 14.0f);

ConVar bb2_allow_profile_system( "bb2_allow_profile_system", "1", FCVAR_REPLICATED, "Allow players to load Global stats or local stats defined on the server. 1 = Global, 2 = Local!", true, 0, true, 2 );
ConVar bb2_profile_system_status("bb2_profile_system_status", "0", FCVAR_REPLICATED | FCVAR_HIDDEN, "Is stats, achievements and level stuff enabled?", true, 0, true, 2);

ConVar bb2_enable_scaling("bb2_enable_scaling", "1", FCVAR_REPLICATED, "Should NPCs scale to the amount of players in the game? Disabling this will enable the 'Noob Mode'. Scaling will increase the npcs health and damage for every player in the game.", true, 0, true, 1);

ConVar bb2_spawn_protection("bb2_spawn_protection", "5", FCVAR_REPLICATED, "Player Spawn Protection Time(sec) On Spawn!", true, 0.0f, true, 10.0f);

ConVar bb2_zombie_lifespan_max("bb2_zombie_lifespan_max", "1.5", FCVAR_REPLICATED, "The max amount of minutes a regular zombie will last before automatically dying.", true, 0.5f, true, 4.0f);
ConVar bb2_zombie_lifespan_min("bb2_zombie_lifespan_min", "0.5", FCVAR_REPLICATED, "The min amount of minutes a regular zombie will last before automatically dying.", true, 0.5f, true, 4.0f);

ConVar bb2_zombie_max("bb2_zombie_max", "50", FCVAR_REPLICATED, "Maximum amount of zombies allowed in the game.", true, 1.0f, true, 128.0f);

ConVar bb2_npc_scaling("bb2_npc_scaling", "1", FCVAR_REPLICATED, "Set the start scale value to add per player in the game. If the value is 5 then every player in the game * 5 is the scale of the npc which means you scale the npc's health and damage by that amount.", true, 1.0f, true, 100.0f);

ConVar bb2_allow_latejoin( "bb2_allow_latejoin", "1", FCVAR_REPLICATED, "Allow players who join late to spawn as a human.", true, 0.0f, true, 1.0f );

ConVar bb2_vote_disable_kick("bb2_vote_disable_kick", "0", FCVAR_REPLICATED, "Disable vote kick.", true, 0.0f, true, 1.0f);
ConVar bb2_vote_disable_ban("bb2_vote_disable_ban", "0", FCVAR_REPLICATED, "Disable vote ban.", true, 0.0f, true, 1.0f);
ConVar bb2_vote_disable_map("bb2_vote_disable_map", "0", FCVAR_REPLICATED, "Disable vote map.", true, 0.0f, true, 1.0f);
ConVar bb2_vote_time("bb2_vote_time", "30", FCVAR_REPLICATED, "The time in seconds until the vote ends.", true, 10.0f, true, 120.0f);
ConVar bb2_vote_frequency_time("bb2_vote_frequency_time", "60", FCVAR_REPLICATED, "The time in seconds until players can vote again.", true, 10.0f, true, 120.0f);
ConVar bb2_ban_time("bb2_ban_time", "30", FCVAR_REPLICATED, "For how many minutes will a player be banned if a ban vote passes?", true, 0.0f, false, 0.0f);

ConVar bb2_allow_mercy( "bb2_allow_mercy", "0", FCVAR_REPLICATED, "If this command's value is higher than 0 and the death count of a zombie player exceed this value he will respawn as a human on death.", true, 0.0f, false, 0.0f );

ConVar bb2_spawn_frequency( "bb2_spawn_frequency", "1.0", FCVAR_REPLICATED, "When a wave has been spawned this is the time between each zombie to spawn. Shouldn't be too low due to lag.", true, 0.1f, false, 0.0f );

ConVar bb2_allow_npc_to_score("bb2_allow_npc_to_score", "1", FCVAR_REPLICATED, "Should friendly npcs be allowed to affect scoring? For example for quests.", true, 0.0f, true, 1.0f);

ConVar bb2_max_fall_height("bb2_max_fall_height", "4", FCVAR_REPLICATED, "You'll take damage from falling X amount of meters.");

ConVar mp_show_voice_icons( "mp_show_voice_icons", "1", FCVAR_REPLICATED, "Show overhead player voice icons when players are speaking.\n" );

ConVar bb2_enable_afk_kicker("bb2_enable_afk_kicker", "1", FCVAR_REPLICATED, "Kick players who're afk after a certain amount of time.", true, 0.0f, true, 1.0f);
ConVar bb2_afk_kick_time("bb2_afk_kick_time", "90", FCVAR_REPLICATED, "If a player is afk for longer than this amount of seconds then he'll be kicked.", true, 10.0f, true, 400.0f);

ConVar bb2_enable_high_ping_kicker("bb2_enable_high_ping_kicker", "1", FCVAR_REPLICATED, "When a player connects, check his average ping after 30 sec, if the average ping is higher than bb2_high_ping_limit he will be kicked.", true, 0.0f, true, 1.0f);
ConVar bb2_high_ping_limit("bb2_high_ping_limit", "200", FCVAR_REPLICATED, "If the players average ping exceeds this value then kick the player.", true, 100.0f, true, 600.0f);

#ifdef GAME_DLL
ConVar tv_delaymapchange("tv_delaymapchange", "0", FCVAR_NONE, "Delays map change until broadcast is complete");
ConVar tv_delaymapchange_protect("tv_delaymapchange_protect", "1", FCVAR_NONE, "Protect against doing a manual map change if HLTV is broadcasting and has not caught up with a major game event such as round_end");

ConVar mp_restartgame( "mp_restartgame", "0", FCVAR_GAMEDLL, "If non-zero, game will restart in the specified number of seconds" );

ConVar mp_mapcycle_empty_timeout_seconds( "mp_mapcycle_empty_timeout_seconds", "0", FCVAR_REPLICATED, "If nonzero, server will cycle to the next map if it has been empty on the current map for N seconds");

void cc_SkipNextMapInCycle()
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( MultiplayRules() )
	{
		MultiplayRules()->SkipNextMapInCycle();
	}
}

void cc_GotoNextMapInCycle()
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( MultiplayRules() )
	{
		MultiplayRules()->ChangeLevel();
	}
}

ConCommand skip_next_map( "skip_next_map", cc_SkipNextMapInCycle, "Skips the next map in the map rotation for the server." );
ConCommand changelevel_next( "changelevel_next", cc_GotoNextMapInCycle, "Immediately changes to the next map in the map rotation for the server." );

ConVar nextlevel( "nextlevel", 
				  "", 
				  FCVAR_GAMEDLL | FCVAR_NOTIFY,
				  "If set to a valid map name, will change to this map during the next changelevel" );
					  					  
#endif

#ifndef CLIENT_DLL
int CMultiplayRules::m_nMapCycleTimeStamp = 0;
int CMultiplayRules::m_nMapCycleindex = 0;
CUtlVector<char*> CMultiplayRules::m_MapList;
#endif

//=========================================================
//=========================================================
bool CMultiplayRules::IsMultiplayer( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMultiplayRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CMultiplayRules::Damage_GetShouldGibCorpse( void )
{
	int iDamage = ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CMultiplayRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CMultiplayRules::Damage_GetNoPhysicsForce( void )
{
	int iTimeBasedDamage = Damage_GetTimeBased();
	int iDamage = ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CMultiplayRules::Damage_GetShouldNotBleed( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiplayRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiplayRules::Damage_ShouldGibCorpse( int iDmgType )
{
	// Damage types that gib the corpse.
	return ( ( iDmgType & ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiplayRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiplayRules::Damage_NoPhysicsForce( int iDmgType )
{
	// Damage types that don't have to supply a physics force & position.
	int iTimeBasedDamage = Damage_GetTimeBased();
	return ( ( iDmgType & ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMultiplayRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Damage types that don't make the player bleed.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID ) ) != 0 );
}

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************
CMultiplayRules::CMultiplayRules()
{
#ifndef CLIENT_DLL
	m_flTimeLastMapChangeOrPlayerWasConnected = 0.0f;

	RefreshSkillData( true );

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if ( engine->IsDedicatedServer() )
	{
		// dedicated server
		const char *cfgfile = servercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[MAX_PATH];

			Log( "Executing dedicated server config file %s\n", cfgfile );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}
	else
	{
		// listen server
		const char *cfgfile = lservercfgfile.GetString();

		if ( cfgfile && cfgfile[0] )
		{
			char szCommand[MAX_PATH];

			Log( "Executing listen server config file %s\n", cfgfile );
			Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
			engine->ServerCommand( szCommand );
		}
	}

	nextlevel.SetValue( "" );
	LoadMapCycleFile();

#endif

	LoadVoiceCommandScript();
}

bool CMultiplayRules::Init()
{
#ifdef GAME_DLL

	// Initialize the custom response rule dictionaries.
	InitCustomResponseRulesDicts();

#endif

	return BaseClass::Init();
}


#ifdef CLIENT_DLL


#else 

	extern bool			g_fGameOver;

	#define ITEM_RESPAWN_TIME	30
	#define WEAPON_RESPAWN_TIME	20
	#define AMMO_RESPAWN_TIME	20

	//=========================================================
	//=========================================================
	void CMultiplayRules::RefreshSkillData(bool forceUpdate)
	{
		// load all default values
		BaseClass::RefreshSkillData(forceUpdate);
	}


	//=========================================================
	//=========================================================
	void CMultiplayRules::Think ( void )
	{
		BaseClass::Think();
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::FrameUpdatePostEntityThink()
	{
		BaseClass::FrameUpdatePostEntityThink();

		float flNow = Plat_FloatTime();

		// Update time when client was last connected
		if ( m_flTimeLastMapChangeOrPlayerWasConnected <= 0.0f )
		{
			m_flTimeLastMapChangeOrPlayerWasConnected = flNow;
		}
		else
		{
			for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
			{
				player_info_t pi;
				if ( !engine->GetPlayerInfo( iPlayerIndex, &pi ) )
					continue;
#if defined( REPLAY_ENABLED )
				if ( pi.ishltv || pi.isreplay || pi.fakeplayer )
#else
				if ( pi.ishltv || pi.fakeplayer )
#endif
					continue;

				m_flTimeLastMapChangeOrPlayerWasConnected = flNow;
				break;
			}
		}

		// Check if we should cycle the map because we've been empty
		// for long enough
		if ( mp_mapcycle_empty_timeout_seconds.GetInt() > 0 )
		{
			int iIdleSeconds = (int)( flNow - m_flTimeLastMapChangeOrPlayerWasConnected );
			if ( iIdleSeconds >= mp_mapcycle_empty_timeout_seconds.GetInt() )
			{

				Log( "Server has been empty for %d seconds on this map, cycling map as per mp_mapcycle_empty_timeout_seconds\n", iIdleSeconds );
				ChangeLevel();
			}
		}
	}


	//=========================================================
	//=========================================================
	bool CMultiplayRules::IsDeathmatch( void )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::IsCoOp( void )
	{
		return false;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{
		//if ( !pPlayer->Weapon_CanSwitchTo( pWeapon ) )
		//{
		//	// Can't switch weapons for some reason.
		//	return false;
		//}

		if ( !pPlayer->GetActiveWeapon() )
		{
			// Player doesn't have an active item, might as well switch.
			return true;
		}

		if ( !pWeapon->AllowsAutoSwitchTo() )
		{
			// The given weapon should not be auto switched to from another weapon.
			return false;
		}

		if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
		{
			// The active weapon does not allow autoswitching away from it.
			return false;
		}

		if ( pWeapon->GetWeight() > pPlayer->GetActiveWeapon()->GetWeight() )
		{
			return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Returns the weapon in the player's inventory that would be better than
	//			the given weapon.
	//-----------------------------------------------------------------------------
	CBaseCombatWeapon *CMultiplayRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
	{
		CBaseCombatWeapon *pCheck;
		CBaseCombatWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.

		int iCurrentWeight = -1;
		int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
		pBest = NULL;

		// If I have a weapon, make sure I'm allowed to holster it
		if ( pCurrentWeapon )
		{
			if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
			{
				// Either this weapon doesn't allow autoswitching away from it or I
				// can't put this weapon away right now, so I can't switch.
				return NULL;
			}

			iCurrentWeight = pCurrentWeapon->GetWeight();
		}

		for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
		{
			pCheck = pPlayer->GetWeapon( i );
			if ( !pCheck )
				continue;

			// If we have an active weapon and this weapon doesn't allow autoswitching away
			// from another weapon, skip it.
			if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
				continue;

			if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
			{
				return pCheck;
			}
			else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				// if this weapon is useable, flag it as the best
				iBestWeight = pCheck->GetWeight();
				pBest = pCheck;
			}
		}

		// if we make it here, we've checked all the weapons and found no useable 
		// weapon in the same catagory as the current weapon. 
		
		// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
		// at least get the crowbar, but ya never know.
		return pBest;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CMultiplayRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
	{
		CBaseCombatWeapon *pWeapon = GetNextBestWeapon( pPlayer, pCurrentWeapon );

		if ( pWeapon != NULL )
			return pPlayer->Weapon_Switch( pWeapon );
		
		return false;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
	{
		GetVoiceGameMgr()->ClientConnected( pEntity );
		return true;
	}

	void CMultiplayRules::InitHUD( CBasePlayer *pl )
	{
	} 

	//=========================================================
	//=========================================================
	void CMultiplayRules::ClientDisconnected( edict_t *pClient )
	{
		if ( pClient )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

			if ( pPlayer )
			{
				FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

				pPlayer->RemoveAllItems();// destroy all of the players weapons and items

				// Kill off view model entities
				pPlayer->DestroyViewModels();

				pPlayer->SetConnected( PlayerDisconnected );

				GameBaseShared()->NewPlayerConnection(true);
			}
		}
	}

	//=========================================================
	//=========================================================
	float CMultiplayRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
	{
		int iFallDamage = (int)falldamage.GetFloat();

		// We calculate the safe fall limit into feet.	
		float flMaxSafeFallSpeed = sqrt(2 * sv_gravity.GetFloat() * (bb2_max_fall_height.GetFloat() / 0.3048f) * 12);

		CHL2MP_Player *pClient = ToHL2MPPlayer(pPlayer);
		if (pClient && pClient->IsHuman() && (pClient->GetSkillValue(PLAYER_SKILL_HUMAN_WEIGHTLESS) > 0))
			flMaxSafeFallSpeed -= ((flMaxSafeFallSpeed / 100.0f) * pClient->GetSkillValue(PLAYER_SKILL_HUMAN_WEIGHTLESS, TEAM_HUMANS));

		switch ( iFallDamage )
		{
		case 1://progressive
			pPlayer->m_Local.m_flFallVelocity -= flMaxSafeFallSpeed; // OLD PLAYER_MAX_SAFE_FALL_SPEED
			return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
			break;
		default:
		case 0:// fixed
			return 10;
			break;
		}
	} 

	//=========================================================
	//=========================================================
	bool CMultiplayRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerThink( CBasePlayer *pPlayer )
	{
		if ( g_fGameOver )
		{
			// clear attack/use commands from player
			pPlayer->m_afButtonPressed = 0;
			pPlayer->m_nButtons = 0;
			pPlayer->m_afButtonReleased = 0;
		}
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerSpawn( CBasePlayer *pPlayer )
	{
		bool		addDefault;
		CBaseEntity	*pWeaponEntity = NULL;
		
		addDefault = true;

		while ( (pWeaponEntity = gEntList.FindEntityByClassname( pWeaponEntity, "game_player_equip" )) != NULL)
		{
			pWeaponEntity->Touch( pPlayer );
			addDefault = false;
		}
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	float CMultiplayRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
	{
		return gpGlobals->curtime;//now!
	}

	bool CMultiplayRules::AllowAutoTargetCrosshair( void )
	{
		return ( aimcrosshair.GetInt() != 0 );
	}

	//=========================================================
	// IPointsForKill - how many points awarded to anyone
	// that kills this player?
	//=========================================================
	int CMultiplayRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
	{
		return 1;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CBasePlayer *CMultiplayRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor )
	{
		if ( pKiller)
		{
			if ( pKiller->IsZombie() || pKiller->IsHuman() )
				return (CBasePlayer*)pKiller;

			// Killing entity might be specifying a scorer player
			IScorer *pScorerInterface = dynamic_cast<IScorer*>( pKiller );
			if ( pScorerInterface )
			{
				CBasePlayer *pPlayer = pScorerInterface->GetScorer();
				if ( pPlayer )
					return pPlayer;
			}

			// Inflicting entity might be specifying a scoring player
			pScorerInterface = dynamic_cast<IScorer*>( pInflictor );
			if ( pScorerInterface )
			{
				CBasePlayer *pPlayer = pScorerInterface->GetScorer();
				if ( pPlayer )
					return pPlayer;
			}
		}

		return NULL;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Returns player who should receive credit for kill
	//-----------------------------------------------------------------------------
	CBasePlayer *CMultiplayRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
	{
		// if this method not overridden by subclass, just call our default implementation
		return GetDeathScorer( pKiller, pInflictor );
	}

	//=========================================================
	// PlayerKilled - someone/something killed this player
	//=========================================================
	void CMultiplayRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
	{
		DeathNotice( pVictim, info );

		// Find the killer & the scorer
		CBaseEntity *pInflictor = info.GetInflictor();
		CBaseEntity *pKiller = info.GetAttacker();
		CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );
		
		pVictim->IncrementDeathCount( 1 );

		// dvsents2: uncomment when removing all FireTargets
		// variant_t value;
		// g_EventQueue.AddEvent( "game_playerdie", "Use", value, 0, pVictim, pVictim );
		FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

		// Did the player kill himself?
		if ( pVictim == pScorer )  
		{			
				// Players lose a frag for killing themselves
				pVictim->IncrementFragCount( -1 );
		
		}
		else if ( pScorer )
		{
			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			pScorer->IncrementFragCount( IPointsForKill( pScorer, pVictim ) );
			
			// Allow the scorer to immediately paint a decal
			pScorer->AllowImmediateDecalPainting();

			// dvsents2: uncomment when removing all FireTargets
			//variant_t value;
			//g_EventQueue.AddEvent( "game_playerkill", "Use", value, 0, pScorer, pScorer );
			FireTargets( "game_playerkill", pScorer, pScorer, USE_TOGGLE, 0 );
		}
		else
		{  

			// NO loss on death!!! BB2
		}
	}

	//=========================================================
	// Deathnotice. 
	//=========================================================
	void CMultiplayRules::DeathNotice(CBaseEntity *pVictim, const CTakeDamageInfo &info)
	{
	}

	//=========================================================
	// FlWeaponRespawnTime - what is the time in the future
	// at which this weapon may spawn?
	//=========================================================
	float CMultiplayRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
	{
		if ( weaponstay.GetInt() > 0 )
		{
			// make sure it's only certain weapons
			if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
			{
				return gpGlobals->curtime + 0;		// weapon respawns almost instantly
			}
		}

		return gpGlobals->curtime + WEAPON_RESPAWN_TIME;
	}

	// when we are within this close to running out of entities,  items 
	// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
	#define ENTITY_INTOLERANCE	100

	//=========================================================
	// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
	// now,  otherwise it returns the time at which it can try
	// to spawn again.
	//=========================================================
	float CMultiplayRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
	{
		if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
				return 0;

			// we're past the entity tolerance level,  so delay the respawn
			return FlWeaponRespawnTime( pWeapon );
		}

		return 0;
	}

	//=========================================================
	// VecWeaponRespawnSpot - where should this weapon spawn?
	// Some game variations may choose to randomize spawn locations
	//=========================================================
	Vector CMultiplayRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
	{
		return pWeapon->GetAbsOrigin();
	}

	//=========================================================
	// WeaponShouldRespawn - any conditions inhibiting the
	// respawning of this weapon?
	//=========================================================
	int CMultiplayRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
	{
		if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
		{
			return GR_WEAPON_RESPAWN_NO;
		}

		return GR_WEAPON_RESPAWN_YES;
	}

	//=========================================================
	// CanHaveWeapon - returns false if the player is not allowed
	// to pick up this weapon
	//=========================================================
	bool CMultiplayRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
	{
		if ( weaponstay.GetInt() > 0 )
		{
			if ( pItem->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD )
				return BaseClass::CanHavePlayerItem( pPlayer, pItem );

			// check if the player already has this weapon
			for ( int i = 0 ; i < pPlayer->WeaponCount() ; i++ )
			{
				if ( pPlayer->GetWeapon(i) == pItem )
				{
					return false;
				}
			}
		}

		return BaseClass::CanHavePlayerItem( pPlayer, pItem );
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
	{
	}

	//=========================================================
	//=========================================================
	int CMultiplayRules::ItemShouldRespawn( CItem *pItem )
	{
		if ( pItem->HasSpawnFlags( SF_NORESPAWN ) )
		{
			return GR_ITEM_RESPAWN_NO;
		}

		return GR_ITEM_RESPAWN_YES;
	}


	//=========================================================
	// At what time in the future may this Item respawn?
	//=========================================================
	float CMultiplayRules::FlItemRespawnTime( CItem *pItem )
	{
		return gpGlobals->curtime + ITEM_RESPAWN_TIME;
	}

	//=========================================================
	// Where should this item respawn?
	// Some game variations may choose to randomize spawn locations
	//=========================================================
	Vector CMultiplayRules::VecItemRespawnSpot( CItem *pItem )
	{
		return pItem->GetAbsOrigin();
	}

	//=========================================================
	// What angles should this item use to respawn?
	//=========================================================
	QAngle CMultiplayRules::VecItemRespawnAngles( CItem *pItem )
	{
		return pItem->GetAbsAngles();
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount )
	{
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::IsAllowedToSpawn( CBaseEntity *pEntity )
	{
	//	if ( pEntity->GetFlags() & FL_NPC )
	//		return false;

		return true;
	}


	//=========================================================
	//=========================================================
	float CMultiplayRules::FlHealthChargerRechargeTime( void )
	{
		return 60;
	}


	float CMultiplayRules::FlHEVChargerRechargeTime( void )
	{
		return 30;
	}

	//=========================================================
	//=========================================================
	int CMultiplayRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
	{
		return GR_PLR_DROP_GUN_ACTIVE;
	}

	//=========================================================
	//=========================================================
	int CMultiplayRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
	{
		return GR_PLR_DROP_AMMO_ACTIVE;
	}

	CBaseEntity *CMultiplayRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
	{
		CBaseEntity *pentSpawnSpot = BaseClass::GetPlayerSpawnSpot( pPlayer );	

	//!! replace this with an Event
	/*
		if ( IsMultiplayer() && pentSpawnSpot->m_target )
		{
			FireTargets( STRING(pentSpawnSpot->m_target), pPlayer, pPlayer, USE_TOGGLE, 0 ); // dvsents2: what is this code supposed to do?
		}
	*/

		return pentSpawnSpot;
	}


	//=========================================================
	//=========================================================
	bool CMultiplayRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
	{
		return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
	}

	int CMultiplayRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
	{
		// half life deathmatch has only enemies
		return GR_NOTTEAMMATE;
	}

	bool CMultiplayRules::PlayFootstepSounds( CBasePlayer *pl )
	{
		if ( footsteps.GetInt() == 0 )
			return false;

		if ( pl->IsOnLadder() || pl->GetAbsVelocity().Length2D() > 220 )
			return true;  // only make step sounds in multiplayer if the player is moving fast enough

		return false;
	}

	bool CMultiplayRules::FAllowFlashlight( void ) 
	{ 
		return true; 
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FAllowNPCs( void )
	{
		return true;
	}

	//=========================================================
	//======== CMultiplayRules private functions ===========

	void CMultiplayRules::GoToIntermission( void )
	{
		if ( g_fGameOver )
			return;

		g_fGameOver = true;

		float flWaitTime = mp_chattime.GetInt();

		if (tv_delaymapchange.GetBool())
		{
			if (HLTVDirector()->IsActive())
				flWaitTime = MAX(flWaitTime, HLTVDirector()->GetDelay());
		}
				
		m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;

		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		}
	}

	// Strip ' ' and '\n' characters from string.
	static void StripWhitespaceChars( char *szBuffer )
	{
		char *szOut = szBuffer;

		for ( char *szIn = szOut; *szIn; szIn++ )
		{
			if ( *szIn != ' ' && *szIn != '\r' )
				*szOut++ = *szIn;
		}
		*szOut = '\0';
	}
	
	void CMultiplayRules::GetNextLevelName( char *pszNextMap, int bufsize, bool bRandom /* = false */ )
	{
		char mapcfile[MAX_PATH];
		DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), false );

		// Check the time of the mapcycle file and re-populate the list of level names if the file has been modified
		const int nMapCycleTimeStamp = filesystem->GetPathTime( mapcfile, "GAME" );

		if ( 0 == nMapCycleTimeStamp )
		{
			// Map cycle file does not exist, make a list containing only the current map
			char *szCurrentMapName = new char[MAX_MAP_NAME];
			Q_strncpy( szCurrentMapName, STRING(gpGlobals->mapname), MAX_MAP_NAME );
			m_MapList.AddToTail( szCurrentMapName );
		}
		else
		{
			// If map cycle file has changed or this is the first time through ...
			if ( m_nMapCycleTimeStamp != nMapCycleTimeStamp )
			{
				// Reload
				LoadMapCycleFile();
			}
		}

		// If somehow we have no maps in the list then add the current one
		if ( 0 == m_MapList.Count() )
		{
			char *szDefaultMapName = new char[MAX_MAP_NAME];
			Q_strncpy( szDefaultMapName, STRING(gpGlobals->mapname), MAX_MAP_NAME );
			m_MapList.AddToTail( szDefaultMapName );
		}

		if ( bRandom )
		{
			m_nMapCycleindex = RandomInt( 0, m_MapList.Count() - 1 );
		}

		// Here's the return value
		Q_strncpy( pszNextMap, m_MapList[m_nMapCycleindex], bufsize);
	}

	void CMultiplayRules::DetermineMapCycleFilename( char *pszResult, int nSizeResult, bool bForceSpew )
	{
		static char szLastResult[MAX_PATH];

		const char *pszVar = mapcyclefile.GetString();
		if ( *pszVar == '\0' )
		{
			if ( bForceSpew || V_stricmp( szLastResult, "__novar") )
			{
				Msg( "mapcyclefile convar not set.\n" );
				V_strcpy_safe( szLastResult, "__novar" );
			}
			*pszResult = '\0';
			return;
		}

		char szRecommendedName[ MAX_PATH ];
		V_sprintf_safe( szRecommendedName, "cfg/%s", pszVar );

		// First, look for a mapcycle file in the cfg directory, which is preferred
		V_strncpy( pszResult, szRecommendedName, nSizeResult );
		if ( filesystem->FileExists( pszResult, "GAME" ) )
		{
			if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
			{
				Msg( "Using map cycle file '%s'.\n", pszResult );
				V_strcpy_safe( szLastResult, pszResult );
			}
			return;
		}

		// Nope?  Try the root.
		V_strncpy( pszResult, pszVar, nSizeResult );
		if ( filesystem->FileExists( pszResult, "GAME" ) )
		{
			if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
			{
				Msg( "Using map cycle file '%s'.  ('%s' was not found.)\n", pszResult, szRecommendedName );
				V_strcpy_safe( szLastResult, pszResult );
			}
			return;
		}

		// Nope?  Use the default.
		if ( !V_stricmp( pszVar, "mapcycle.txt" ) )
		{
			V_strncpy( pszResult, "cfg/mapcycle_default.txt", nSizeResult );
			if ( filesystem->FileExists( pszResult, "GAME" ) )
			{
				if ( bForceSpew || V_stricmp( szLastResult, pszResult) )
				{
					Msg( "Using map cycle file '%s'.  ('%s' was not found.)\n", pszResult, szRecommendedName );
					V_strcpy_safe( szLastResult, pszResult );
				}
				return;
			}
		}

		// Failed
		*pszResult = '\0';
		if ( bForceSpew || V_stricmp( szLastResult, "__notfound") )
		{
			Msg( "Map cycle file '%s' was not found.\n", szRecommendedName );
			V_strcpy_safe( szLastResult, "__notfound" );
		}
	}

	void CMultiplayRules::LoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
	{
		CMultiplayRules::RawLoadMapCycleFileIntoVector( pszMapCycleFile, mapList );
	}

	void CMultiplayRules::RawLoadMapCycleFileIntoVector( const char *pszMapCycleFile, CUtlVector<char *> &mapList )
	{
		CUtlBuffer buf;
		if ( !filesystem->ReadFile( pszMapCycleFile, "GAME", buf ) )
			return;
		buf.PutChar( 0 );
		V_SplitString( (char*)buf.Base(), "\n", mapList );

		for ( int i = 0; i < mapList.Count(); i++ )
		{
			bool bIgnore = false;

			// Strip out ' ' and '\r' chars.
			StripWhitespaceChars( mapList[i] );

			if ( !Q_strncmp( mapList[i], "//", 2 ) || mapList[i][0] == '\0' )
			{
				bIgnore = true;
			}

			if ( bIgnore )
			{
				delete [] mapList[i];
				mapList.Remove( i );
				--i;
			}
		}
	}

	void CMultiplayRules::FreeMapCycleFileVector( CUtlVector<char *> &mapList )
	{
		// Clear out existing map list. Not using Purge() or PurgeAndDeleteAll() because they won't delete [] each element.
		for ( int i = 0; i < mapList.Count(); i++ )
		{
			delete [] mapList[i];
		}

		mapList.RemoveAll();
	}

	bool CMultiplayRules::IsManualMapChangeOkay( const char **pszReason )
	{
		if ( HLTVDirector()->IsActive() && ( HLTVDirector()->GetDelay() >= HLTV_MIN_DIRECTOR_DELAY ) )
		{
			if ( tv_delaymapchange.GetBool() && tv_delaymapchange_protect.GetBool() )
			{
				float flLastEvent = GetLastMajorEventTime();
				if ( flLastEvent > -1 )
				{
					if ( flLastEvent > ( gpGlobals->curtime - ( HLTVDirector()->GetDelay() + 3 ) ) ) // +3 second delay to prevent instant change after a major event
					{
						*pszReason = "\n***WARNING*** Map change blocked. HLTV is broadcasting and has not caught up to the last major game event yet.\nYou can disable this check by setting the value of the server convar \"tv_delaymapchange_protect\" to 0.\n";
						return false;
					}
				}
			}
		}

		return true;
	}	
	
	bool CMultiplayRules::IsMapInMapCycle( const char *pszName )
	{
		for ( int i = 0; i < m_MapList.Count(); i++ )
		{
			if ( V_stricmp( pszName, m_MapList[i] ) == 0 )
			{
				return true;
			}
		}	

		return false;
	}

	void CMultiplayRules::ChangeLevel( void )
	{
		char szNextMap[MAX_MAP_NAME];

		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
		}
		else
		{
			GetNextLevelName( szNextMap, sizeof(szNextMap) );
			IncrementMapCycleIndex();
		}

		ChangeLevelToMap( szNextMap );
	}

	void CMultiplayRules::LoadMapCycleFile( void )
	{
		int nOldCycleIndex = m_nMapCycleindex;
		m_nMapCycleindex = 0;

		char mapcfile[MAX_PATH];
		DetermineMapCycleFilename( mapcfile, sizeof(mapcfile), false );

		FreeMapCycleFileVector( m_MapList );

		const int nMapCycleTimeStamp = filesystem->GetPathTime( mapcfile, "GAME" );
		m_nMapCycleTimeStamp = nMapCycleTimeStamp;

		// Repopulate map list from mapcycle file
		LoadMapCycleFileIntoVector( mapcfile, m_MapList );

		// Load server's mapcycle into network string table for client-side voting
		if ( g_pStringTableServerMapCycle )
		{
			CUtlString sFileList;
			for ( int i = 0; i < m_MapList.Count(); i++ )
			{
				sFileList += m_MapList[i];
				sFileList += '\n';
			}

			g_pStringTableServerMapCycle->AddString( CBaseEntity::IsServer(), "ServerMapCycle", sFileList.Length() + 1, sFileList.String() );
		}

		// If the current map is in the same location in the new map cycle, keep that index. This gives better behavior
		// when reloading a map cycle that has the current map in it multiple times.
		int nOldPreviousMap = ( nOldCycleIndex == 0 ) ? ( m_MapList.Count() - 1 ) : ( nOldCycleIndex - 1 );
		if ( nOldCycleIndex >= 0 && nOldCycleIndex < m_MapList.Count() &&
		     nOldPreviousMap >= 0 && nOldPreviousMap < m_MapList.Count() &&
		     V_strcmp( STRING( gpGlobals->mapname ), m_MapList[ nOldPreviousMap ] ) == 0 )
		{
			// The old index is still valid, and falls after our current map in the new cycle, use it
			m_nMapCycleindex = nOldCycleIndex;
		}
		else
		{
			// Otherwise, if the current map selection is in the list, set m_nMapCycleindex to the map that follows it.
			for ( int i = 0; i < m_MapList.Count(); i++ )
			{
				if ( V_strcmp( STRING( gpGlobals->mapname ), m_MapList[i] ) == 0 )
				{
					m_nMapCycleindex = i;
					IncrementMapCycleIndex();
					break;
				}
			}
		}
	}

	void CMultiplayRules::ChangeLevelToMap( const char *pszMap )
	{
		g_fGameOver = true;
		m_flTimeLastMapChangeOrPlayerWasConnected = 0.0f;
		Msg( "CHANGE LEVEL: %s\n", pszMap );
		GameBaseServer()->DoMapChange(pszMap);
	}

#endif		

	//-----------------------------------------------------------------------------
	// Purpose: Shared script resource of voice menu commands and hud strings
	//-----------------------------------------------------------------------------
	void CMultiplayRules::LoadVoiceCommandScript( void )
	{
		KeyValues *pKV = new KeyValues( "VoiceCommands" );

		if ( pKV->LoadFromFile( filesystem, "scripts/voicecommands.txt", "GAME" ) )
		{
			for ( KeyValues *menu = pKV->GetFirstSubKey(); menu != NULL; menu = menu->GetNextKey() )
			{
				int iMenuIndex = m_VoiceCommandMenus.AddToTail();

				int iNumItems = 0;

				// for each subkey of this menu, add a menu item
				for ( KeyValues *menuitem = menu->GetFirstSubKey(); menuitem != NULL; menuitem = menuitem->GetNextKey() )
				{
					iNumItems++;

					if ( iNumItems > 9 )
					{
						Warning( "Trying to load more than 9 menu items in voicecommands.txt, extras ignored" );
						continue;
					}

					VoiceCommandMenuItem_t item;

#ifndef CLIENT_DLL
					int iConcept = GetMPConceptIndexFromString( menuitem->GetString( "concept", "" ) );
					if ( iConcept == MP_CONCEPT_NONE )
					{
						Warning( "Voicecommand script attempting to use unknown concept. Need to define new concepts in code. ( %s )\n", menuitem->GetString( "concept", "" ) );
					}
					item.m_iConcept = iConcept;

					item.m_bShowSubtitle = ( menuitem->GetInt( "show_subtitle", 0 ) > 0 );
					item.m_bDistanceBasedSubtitle = ( menuitem->GetInt( "distance_check_subtitle", 0 ) > 0 );

					Q_strncpy( item.m_szGestureActivity, menuitem->GetString( "activity", "" ), sizeof( item.m_szGestureActivity ) ); 
#else
					Q_strncpy( item.m_szSubtitle, menuitem->GetString( "subtitle", "" ), MAX_VOICE_COMMAND_SUBTITLE );
					Q_strncpy( item.m_szMenuLabel, menuitem->GetString( "menu_label", "" ), MAX_VOICE_COMMAND_SUBTITLE );

#endif
					m_VoiceCommandMenus.Element( iMenuIndex ).AddToTail( item );
				}
			}
		}

		pKV->deleteThis();
	}

#ifndef CLIENT_DLL

	void CMultiplayRules::SkipNextMapInCycle()
	{
		char szSkippedMap[MAX_MAP_NAME];
		char szNextMap[MAX_MAP_NAME];

		GetNextLevelName( szSkippedMap, sizeof( szSkippedMap ) );
		IncrementMapCycleIndex();
		GetNextLevelName( szNextMap, sizeof( szNextMap ) );

		Msg( "Skipping: %s\tNext map: %s\n", szSkippedMap, szNextMap );

		if ( nextlevel.GetString() && *nextlevel.GetString() )
		{
			Msg( "Warning! \"nextlevel\" is set to \"%s\" and will override the next map to be played.\n", nextlevel.GetString() );
		}
	}

	void CMultiplayRules::IncrementMapCycleIndex()
	{
		// Reset index if we've passed the end of the map list
		if ( ++m_nMapCycleindex >= m_MapList.Count() )
		{
			m_nMapCycleindex = 0;
		}
	}

	bool CMultiplayRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pEdict );

		const char *pcmd = args[0];
		if ( FStrEq( pcmd, "voicemenu" ) )
		{
			if ( args.ArgC() < 3 )
				return true;

			CBaseMultiplayerPlayer *pMultiPlayerPlayer = dynamic_cast< CBaseMultiplayerPlayer * >( pPlayer );

			if ( pMultiPlayerPlayer )
			{
				int iMenu = atoi( args[1] );
				int iItem = atoi( args[2] );

				VoiceCommand( pMultiPlayerPlayer, iMenu, iItem );
			}

			return true;
		}

		return BaseClass::ClientCommand( pEdict, args );
	}

	void CMultiplayRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
	{
		CBaseMultiplayerPlayer *pPlayer = dynamic_cast< CBaseMultiplayerPlayer * >( CBaseEntity::Instance( pEntity ) );

		if ( !pPlayer )
			return;

		char const *pszCommand = pKeyValues->GetName();
		if ( pszCommand && pszCommand[0] )
		{
			if ( FStrEq( pszCommand, "AchievementEarned" ) )
			{
				if ( pPlayer->ShouldAnnounceAchievement() )
				{
					int nAchievementID = pKeyValues->GetInt( "achievementID" );

					IGameEvent * event = gameeventmanager->CreateEvent( "achievement_earned" );
					if ( event )
					{
						event->SetInt( "player", pPlayer->entindex() );
						event->SetInt( "achievement", nAchievementID );
						gameeventmanager->FireEvent( event );
					}

					pPlayer->OnAchievementEarned( nAchievementID );
				}
			}
		}
	}

	VoiceCommandMenuItem_t *CMultiplayRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
	{
		// have the player speak the concept that is in a particular menu slot
		if ( !pPlayer )
			return NULL;

		if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
			return NULL;

		if ( iItem < 0 || iItem >= m_VoiceCommandMenus.Element( iMenu ).Count() )
			return NULL;

		VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( iItem );

		Assert( pItem );

		char szResponse[AI_Response::MAX_RESPONSE_NAME];

		if ( pPlayer->CanSpeakVoiceCommand() )
		{
			CMultiplayer_Expresser *pExpresser = pPlayer->GetMultiplayerExpresser();
			Assert( pExpresser );
			pExpresser->AllowMultipleScenes();

			if ( pPlayer->SpeakConceptIfAllowed( pItem->m_iConcept, NULL, szResponse, AI_Response::MAX_RESPONSE_NAME ) )
			{
				// show a subtitle if we need to
				if ( pItem->m_bShowSubtitle )
				{
					CRecipientFilter filter;

					if ( pItem->m_bDistanceBasedSubtitle )
					{
						filter.AddRecipientsByPAS( pPlayer->WorldSpaceCenter() );

						// further reduce the range to a certain radius
						int i;
						for ( i = filter.GetRecipientCount()-1; i >= 0; i-- )
						{
							int index = filter.GetRecipientIndex(i);

							CBasePlayer *pListener = UTIL_PlayerByIndex( index );

							if ( pListener && pListener != pPlayer )
							{
								float flDist = ( pListener->WorldSpaceCenter() - pPlayer->WorldSpaceCenter() ).Length2D();

								if ( flDist > VOICE_COMMAND_MAX_SUBTITLE_DIST )
									filter.RemoveRecipientByPlayerIndex( index );
							}
						}
					}
					else
					{
						filter.AddAllPlayers();
					}

					// if we aren't a disguised spy
					if ( !pPlayer->ShouldShowVoiceSubtitleToEnemy() )
					{
						// remove players on other teams
						filter.RemoveRecipientsNotOnTeam( pPlayer->GetTeam() );
					}

					// Register this event in the mod-specific usermessages .cpp file if you hit this assert
					Assert( usermessages->LookupUserMessage( "VoiceSubtitle" ) != -1 );

					// Send a subtitle to anyone in the PAS
					UserMessageBegin( filter, "VoiceSubtitle" );
						WRITE_BYTE( pPlayer->entindex() );
						WRITE_BYTE( iMenu );
						WRITE_BYTE( iItem );
					MessageEnd();
				}

				pPlayer->NoteSpokeVoiceCommand( szResponse );

#ifdef NEXT_BOT
				// let bots react to player's voice commands
				CUtlVector< INextBot * > botVector;
				TheNextBots().CollectAllBots( &botVector );

				for( int i=0; i<botVector.Count(); ++i )
				{
					botVector[i]->OnActorEmoted( pPlayer, pItem->m_iConcept );
				}
#endif
			}
			else
			{
				pItem = NULL;
			}

			pExpresser->DisallowMultipleScenes();
			return pItem;
		}

		return NULL;
	}

	bool CMultiplayRules::IsLoadingBugBaitReport()
	{
		return ( !engine->IsDedicatedServer()&& CommandLine()->CheckParm( "-bugbait" ) && sv_cheats->GetBool() );
	}

	void CMultiplayRules::HaveAllPlayersSpeakConceptIfAllowed( int iConcept, int iTeam /* = TEAM_UNASSIGNED */, const char *modifiers /* = NULL */ )
	{
		CBaseMultiplayerPlayer *pPlayer;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pPlayer )
				continue;

			if ( iTeam != TEAM_UNASSIGNED )
			{
				if ( pPlayer->GetTeamNumber() != iTeam )
					continue;
			}

			pPlayer->SpeakConceptIfAllowed( iConcept, modifiers );
		}
	}
	
	void CMultiplayRules::RandomPlayersSpeakConceptIfAllowed( int iConcept, int iNumRandomPlayer /*= 1*/, int iTeam /*= TEAM_UNASSIGNED*/, const char *modifiers /*= NULL*/ )
	{
		CUtlVector< CBaseMultiplayerPlayer* > speakCandidates;

		CBaseMultiplayerPlayer *pPlayer;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( i ) );

			if ( !pPlayer )
				continue;

			if ( iTeam != TEAM_UNASSIGNED )
			{
				if ( pPlayer->GetTeamNumber() != iTeam )
					continue;
			}

			speakCandidates.AddToTail( pPlayer );
		}

		int iSpeaker = iNumRandomPlayer;
		while ( iSpeaker > 0 && speakCandidates.Count() > 0 )
		{
			int iRandomSpeaker = RandomInt( 0, speakCandidates.Count() - 1 );
			speakCandidates[ iRandomSpeaker ]->SpeakConceptIfAllowed( iConcept, modifiers );
			speakCandidates.FastRemove( iRandomSpeaker );
			iSpeaker--;
		}
	}	

	void CMultiplayRules::ClientSettingsChanged( CBasePlayer *pPlayer )
	{
		// NVNT see if this user is still or has began using a haptic device
		const char *pszHH = engine->GetClientConVarValue( pPlayer->entindex(), "hap_HasDevice" );
		if( pszHH )
		{
			int iHH = atoi( pszHH );
			pPlayer->SetHaptics( iHH != 0 );
		}

	}
	void CMultiplayRules::GetTaggedConVarList( KeyValues *pCvarTagList )
	{
		BaseClass::GetTaggedConVarList( pCvarTagList );

		// sv_gravity
		KeyValues *pGravity = new KeyValues( "sv_gravity" );
		pGravity->SetString( "convar", "sv_gravity" );
		pGravity->SetString( "tag", "gravity" );

		pCvarTagList->AddSubKey( pGravity );

		// sv_alltalk
		KeyValues *pAllTalk = new KeyValues( "sv_alltalk" );
		pAllTalk->SetString( "convar", "sv_alltalk" );
		pAllTalk->SetString( "tag", "alltalk" );

		pCvarTagList->AddSubKey( pAllTalk );
	}

#else

	const char *CMultiplayRules::GetVoiceCommandSubtitle( int iMenu, int iItem )
	{
		Assert( iMenu >= 0 && iMenu < m_VoiceCommandMenus.Count() );
		if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
			return "";

		Assert( iItem >= 0 && iItem < m_VoiceCommandMenus.Element( iMenu ).Count() );
		if ( iItem < 0 || iItem >= m_VoiceCommandMenus.Element( iMenu ).Count() )
			return "";

		VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( iItem );

		Assert( pItem );

		return pItem->m_szSubtitle;
	}

	// Returns false if no such menu is declared or if it's an empty menu
	bool CMultiplayRules::GetVoiceMenuLabels( int iMenu, KeyValues *pKV )
	{
		Assert( iMenu >= 0 && iMenu < m_VoiceCommandMenus.Count() );
		if ( iMenu < 0 || iMenu >= m_VoiceCommandMenus.Count() )
			return false;

		int iNumItems = m_VoiceCommandMenus.Element( iMenu ).Count();

		for ( int i=0; i<iNumItems; i++ )
		{
			VoiceCommandMenuItem_t *pItem = &m_VoiceCommandMenus.Element( iMenu ).Element( i );

			KeyValues *pLabelKV = new KeyValues( pItem->m_szMenuLabel );

			pKV->AddSubKey( pLabelKV );
		}

		return iNumItems > 0;
	}

#endif

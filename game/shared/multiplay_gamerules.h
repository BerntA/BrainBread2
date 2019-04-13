//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MULTIPLAY_GAMERULES_H
#define MULTIPLAY_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"

#ifdef CLIENT_DLL

	#define CMultiplayRules C_MultiplayRules

#else

extern ConVar nextlevel;
extern INetworkStringTable *g_pStringTableServerMapCycle;

class CBaseMultiplayerPlayer;

#endif

extern ConVar mp_show_voice_icons;

extern ConVar mp_timelimit_objective;
extern ConVar mp_timelimit_arena;
extern ConVar mp_timelimit_deathmatch;
extern ConVar mp_timelimit_elimination;

extern ConVar mp_limitteams;

extern ConVar bb2_elimination_fraglimit;
extern ConVar bb2_elimination_respawn_time;
extern ConVar bb2_elimination_respawn_time_scale;
extern ConVar bb2_elimination_score_from_extermination;
extern ConVar bb2_elimination_score_zombies;
extern ConVar bb2_elimination_score_humans;
extern ConVar bb2_elimination_teamperk_kills_required;
extern ConVar bb2_elimination_teamperk_duration;
extern ConVar bb2_elimination_teammate_distance;

extern ConVar bb2_arena_respawn_time;
extern ConVar bb2_arena_reinforcement_count;
extern ConVar bb2_arena_hard_mode;

extern ConVar bb2_deathmatch_respawn_time;
extern ConVar bb2_deathmatch_fraglimit;

extern ConVar bb2_story_respawn_time;
extern ConVar bb2_story_dynamic_respawn;
extern ConVar bb2_classic_mode_enabled;
extern ConVar bb2_classic_zombie_noteamchange;

extern ConVar bb2_allow_profile_system;
extern ConVar bb2_profile_system_status;
extern ConVar bb2_enable_scaling;
extern ConVar bb2_zombie_lifespan_max;
extern ConVar bb2_zombie_lifespan_min;
extern ConVar bb2_zombie_max;
extern ConVar bb2_npc_scaling;
extern ConVar bb2_hard_scaling;
extern ConVar bb2_allow_latejoin;
extern ConVar bb2_allow_mercy;
extern ConVar bb2_zombie_kills_required;
extern ConVar bb2_spawn_frequency;
extern ConVar bb2_roundstart_freezetime;
extern ConVar bb2_vote_disable_kick;
extern ConVar bb2_vote_disable_ban;
extern ConVar bb2_vote_disable_map;
extern ConVar bb2_vote_required_percentage;
extern ConVar bb2_vote_required_level;
extern ConVar bb2_vote_time;
extern ConVar bb2_vote_time_endgame;
extern ConVar bb2_vote_frequency_time;
extern ConVar bb2_ban_time;
extern ConVar bb2_vote_roundstart_delay;
extern ConVar bb2_vote_kick_ban_time;
extern ConVar bb2_allow_npc_to_score;

extern ConVar bb2_enable_afk_kicker;
extern ConVar bb2_afk_kick_time;

extern ConVar bb2_enable_high_ping_kicker;
extern ConVar bb2_high_ping_limit;

extern ConVar bb2_active_workshop_item;

//=========================================================
// CMultiplayRules - rules for the basic half life multiplayer
// competition
//=========================================================
class CMultiplayRules : public CGameRules
{
public:
	DECLARE_CLASS( CMultiplayRules, CGameRules );

// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void );

	virtual	bool	Init();

	// Damage query implementations.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShouldGibCorpse( int iDmgType );		// Damage types that gib the corpse.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_NoPhysicsForce( int iDmgType );		// Damage types that don't have to supply a physics force & position.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP: These will go away once DamageTypes become enums.
	virtual int		Damage_GetTimeBased( void );
	virtual int		Damage_GetShouldGibCorpse( void );
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetNoPhysicsForce( void );
	virtual int		Damage_GetShouldNotBleed( void );

	CMultiplayRules();
	virtual ~CMultiplayRules() {}

	virtual bool ShouldDrawHeadLabels()
	{ 
		if ( mp_show_voice_icons.GetBool() == false )
			return false;

		return BaseClass::ShouldDrawHeadLabels();
	}

#ifndef CLIENT_DLL
	virtual void FrameUpdatePostEntityThink();

// GR_Think
	virtual void Think( void );
	virtual void RefreshSkillData( bool forceUpdate );
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual bool FAllowFlashlight( void );

	virtual CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );
	virtual bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );

// Functions to verify the single/multiplayer status of a game
	virtual bool IsDeathmatch( void );

// Client connection/disconnection
	// If ClientConnected returns FALSE, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual bool  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );
	virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor );									// old version of method - kept for backward compat
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );		// new version of method

// Weapon retrieval
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );// The player is touching an CBaseCombatWeapon, do I give it to him?

// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );

// Teamplay stuff	
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );

	virtual bool PlayTextureSounds( void ) { return FALSE; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl );

// NPCs
	virtual bool FAllowNPCs( void );
	
	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }

	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	virtual void ResetMapCycleTimeStamp( void ){ m_nMapCycleTimeStamp = 0; }

	virtual void HandleTimeLimitChange( void ){ return; }

	void IncrementMapCycleIndex();
	
	virtual void GetTaggedConVarList( KeyValues *pCvarTagList );

	void SkipNextMapInCycle();

	virtual void	ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );

public:

	// NVNT virtual to check for haptic device 
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual void GetNextLevelName( char *szNextMap, int bufsize, bool bRandom = false );

	static void DetermineMapCycleFilename( char *pszResult, int nSizeResult, bool bForceSpew );
	virtual void LoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList );
	static void FreeMapCycleFileVector ( CUtlVector<char *> &mapList );

	// LoadMapCycleFileIntoVector without the fixups inherited versions of gamerules may provide
	static void RawLoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList );
	
	bool IsMapInMapCycle( const char *pszName );
	
	virtual bool IsManualMapChangeOkay( const char **pszReason ) OVERRIDE;

protected:
	virtual float GetLastMajorEventTime( void ){ return -1.0f; }
	
public:
	virtual void ChangeLevel( void );

protected:
	virtual void GoToIntermission( void );
	virtual void LoadMapCycleFile( void );
	void ChangeLevelToMap( const char *pszMap );

	float m_flIntermissionEndTime;
	static int m_nMapCycleTimeStamp;
	static int m_nMapCycleindex;
	static CUtlVector<char*> m_MapList;

#else

#endif
};

inline CMultiplayRules* MultiplayRules()
{
	return static_cast<CMultiplayRules*>(g_pGameRules);
}

#endif // MULTIPLAY_GAMERULES_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2MP_GAMERULES_H
#define HL2MP_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "gamevars_shared.h"

#ifndef CLIENT_DLL
#include "hl2mp_player.h"
#endif

#define VEC_CROUCH_TRACE_MIN	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMax

#define VEC_SLIDE_TRACE_MIN HL2MPRules()->GetHL2MPViewVectors()->m_vSlideTraceMin
#define VEC_SLIDE_TRACE_MAX HL2MPRules()->GetHL2MPViewVectors()->m_vSlideTraceMax

enum
{
	TEAM_HUMANS = 2,
	TEAM_DECEASED,
};

enum BB2_VoteType
{
	VOTE_TYPE_NONE = 0,
	VOTE_TYPE_KICK,
	VOTE_TYPE_BAN,
	VOTE_TYPE_MAP,
};

enum BB2_GameMode
{
	MODE_OBJECTIVE = 1,
	MODE_ELIMINATION,
	MODE_ARENA,
	MODE_DEATHMATCH,
	MODE_TOTAL = 4,
};

enum BB2_SoundTypes // Current available types ( npcs )... Custom & Unknown will remain undefined for a while.
{
	TYPE_ZOMBIE = 0,
	TYPE_FRED,
	TYPE_SOLDIER,
	TYPE_BANDIT,
	TYPE_OMEN,
	TYPE_SECURITY,
	TYPE_RUNNER,
	TYPE_SPECIMEN,
	TYPE_PLAYER,
	TYPE_DECEASED,
	TYPE_ANNOUNCER,
	TYPE_UNKNOWN,
	TYPE_CUSTOM,
};

enum BB2_EndMapVoteTypes
{
	ENDMAP_VOTE_OBJECTIVE = 1,
	ENDMAP_VOTE_ARENA,
	ENDMAP_VOTE_ELIMINATION,
	ENDMAP_VOTE_DEATHMATCH,
	ENDMAP_VOTE_RETRY,
	ENDMAP_VOTE_REFRESH,
	ENDMAP_VOTE_TYPES_MAX = 6,
};

enum BB2_GamemodeFlags
{
	GM_FLAG_ARENA_HARDMODE = 0x01,
	GM_FLAG_EXTREME_SCALING = 0x02,
};

#ifdef CLIENT_DLL
	#define CHL2MPRules C_HL2MPRules
	#define CHL2MPGameRulesProxy C_HL2MPGameRulesProxy
#endif

class CHL2MPGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHL2MPGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class HL2MPViewVectors : public CViewVectors
{
public:
	HL2MPViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax,
		Vector vSlideHullMin,
		Vector vSlideHullMax,
		Vector vSlideView,
		Vector vSlideTraceMin,
		Vector vSlideTraceMax) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight,
				vSlideHullMin,
				vSlideHullMax,
				vSlideView
				)
	{
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;

		m_vSlideTraceMin = vSlideTraceMin;
		m_vSlideTraceMax = vSlideTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;

	Vector m_vSlideTraceMin;
	Vector m_vSlideTraceMax;
};

class CHL2MPRules : public CTeamplayRules
{
public:
	DECLARE_CLASS( CHL2MPRules, CTeamplayRules );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
	
	virtual bool ShouldBurningPropsEmitLight();
			
#endif
	
	CHL2MPRules();
	virtual ~CHL2MPRules();

	// Arena Specific:
	float GetReinforcementRespawnTime();
	int GetReinforcementsLeft() { return m_iNumReinforcements; }

	// Shared Stuff:
	float GetTimeLeft();
	float GetTimelimitValue();
	int GetRoundCountdownNum() { return m_iRoundCountdown; }
	int GetCurrentGamemode() { return m_iCurrentGamemode; }
	bool IsGamemodeFlagActive(int flag) { return ((m_iGamemodeFlags.Get() & flag) != 0); }

	CNetworkVar(bool, m_bRoundStarted);

	virtual void Precache( void );
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon );
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
	virtual void Think( void );

	virtual bool CanUseSkills(void);
	virtual bool IsFastPacedGameplay(void);
	virtual bool CanUseGameAnnouncer(void);
	virtual bool IsPowerupsAllowed(void);
	virtual bool CanPlayersRespawnIndividually(void);
	virtual bool ShouldHideHUDDuringRoundWait(void);
	virtual bool ShouldDrawHeadLabels();
	virtual bool IsGameoverOrScoresVisible(void) 
	{ 
#ifdef CLIENT_DLL
		return (IsIntermission() || m_bShouldShowScores);
#else
		return (IsIntermission() || m_bShouldShowScores || g_fGameOver);
#endif
	}

	virtual void CreateStandardEntities( void );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual void GoToIntermission(int iWinner = TEAM_HUMANS);
	virtual void DeathNotice(CBaseEntity *pVictim, const CTakeDamageInfo &info);
	virtual const char *GetGameDescription( void );
	virtual const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;

	int GetTeamSize(int team);
	int GetPlayersInGame(void);

	const char *GetNameForCombatCharacter(int index);

	// Vote System:
	CNetworkVar(int, m_iCurrentVoteType);
	CNetworkVar(int, m_iCurrentYesVotes);
	CNetworkVar(int, m_iCurrentNoVotes);
	CNetworkVar(float, m_flTimeUntilVoteEnds);
	CNetworkVar(float, m_flTimeVoteStarted);

	// Game End Map Vote System:
	CNetworkArray(int, m_iEndMapVotesForType, ENDMAP_VOTE_TYPES_MAX);
	
#ifndef CLIENT_DLL
	// User vote system:
	int m_iAmountOfVoters;
	float m_flNextVoteTime;
	char pchSteamIDKickBanTarget[MAX_NETWORKID_LENGTH];
	char pchMapToChangeTo[MAX_MAP_NAME];
	bool m_bPlayersAllowedToVote[MAX_PLAYERS];
	bool m_bPlayersVoted[MAX_PLAYERS];
	void VoteSystemThink(void);
	void ResetVote(bool bFullReset = false);
	bool CanCreateVote(CBasePlayer *pVoter);
	void CreateBanKickVote(CBasePlayer *pVoter, CBasePlayer *pTarget, bool bBan = false);
	void CreateMapVote(CBasePlayer *pVoter, const char *map);
	void SetupVote(int indexOfVoter);
	void PlayerVote(CBasePlayer *pPlayer, bool bYes);
	void DispatchVoteEvent(int indexOfVoter, int targetIndex, bool bVoteEnd = false);

	// End game map vote system:
	void GameEndVoteThink(void);
	void StartEndMapVote(bool bRefresh = false);
	void ResetEndMapVoting(void);
	void RecalculateEndMapVotes(void);
	int GetVoteTypeWithMostVotes(void);
	bool ShouldShowRandomMaps(void);
	const char *GetRandomMapForVoteSys(int mode, bool bRandom = false);
	char pchMapOptions[4][MAX_MAP_NAME];
	float m_flEndVoteTimeEnd;
	int m_iEndVotePlayerChoices[MAX_PLAYERS];
	bool m_bEndMapVotingEnabled;
	CUtlVector<int> m_listPrevMapOptions; // A list of idx from mapcycle list which imply that this map has already been checked, for refresh func to prevent the same map from appearing.

	void EmitSoundToClient(CBaseEntity *pAnnouncer, const char *szOriginalSound, int iType, bool bGenderMale, int playerIndex = 0);
	void DisplayScores(int iWinner = TEAM_HUMANS);
	void NewRoundInit(int iPlayersInGame);
	void EndRound(bool bRestart);
	void GameModeSharedThink(void);

	void CleanUpMap();
	void RestartGame();

	virtual bool DidClientDisconnectRecently(uint64 id) { return (m_uDisconnectedClients.Find(id) != -1); }

	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );
	virtual float	FlItemRespawnTime( CItem *pItem );

	const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

	// Elimination Mode
	int GetNewTeam(int wishTeam, bool bNewRound = false);
	float GetPlayerRespawnTime(CHL2MP_Player *pPlayer);

	// Shared
	int GetRewardFromRoundWin(CHL2MP_Player *pPlayer, int winnerTeam, bool gameOver);

	// Zombie Limiting
	bool CanSpawnZombie(void);

	void InitDefaultAIRelationships( void );

	char szCurrentMap[MAX_MAP_NAME];
	unsigned long long m_ulMapSize;
#endif
	virtual void ClientDisconnected( edict_t *pClient );

	virtual bool CheckGameOver( void );
	virtual bool IsIntermission( void );

	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );	
	virtual bool IsTeamplay(void);
	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );	
	
private:

	// Shared
	CNetworkVar(float, m_flServerStartTime);
	CNetworkVar(bool, m_bShouldShowScores);
	CNetworkVar(int, m_iRoundCountdown);
	CNetworkVar(int, m_iCurrentGamemode);
	CNetworkVar(int, m_iGamemodeFlags);

	// Arena
	CNetworkVar(float, m_flRespawnTime);
	CNetworkVar(int, m_iNumReinforcements);

#ifndef CLIENT_DLL
	CUtlVector<uint64> m_uDisconnectedClients;
	float m_flRoundStartTime;
	float m_flScoreBoardTime;
	float m_flTimeRoundStarted;
	bool m_bChangelevelDone;
#endif
};

inline CHL2MPRules* HL2MPRules()
{
	return static_cast<CHL2MPRules*>(g_pGameRules);
}

#endif //HL2MP_GAMERULES_H

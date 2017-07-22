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
	CNetworkVar(float, m_flRespawnTime);
	CNetworkVar(int, m_iNumReinforcements);
	float GetReinforcementRespawnTime();

	// Shared Stuff:
	float GetTimeLeft();
	float GetTimelimitValue();

	CNetworkVar(bool, m_bRoundStarted);
	CNetworkVar(bool, m_bShouldShowScores);
	CNetworkVar(int, m_iRoundCountdown);
	CNetworkVar(int, m_iCurrentGamemode);

	CNetworkVar(float, m_flServerStartTime);

	int GetCurrentGamemode() { return m_iCurrentGamemode; }

	virtual void Precache( void );
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon );
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
	int m_iUserIDToKickOrBan;
	float m_flNextVoteTime;
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

	void EmitSoundToClient(CBaseEntity *pAnnouncer, const char *szOriginalSound, int iType, bool bGenderMale, int playerIndex = 0);
	void DisplayScores(int iWinner = TEAM_HUMANS);
	void NewRoundInit(int iPlayersInGame);
	void EndRound(bool bRestart);
	void GameModeSharedThink(void);

	void CleanUpMap();
	void CheckRestartGame();
	void RestartGame();

	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );
	virtual float	FlItemRespawnTime( CItem *pItem );
	virtual bool	CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	void	AddLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity );
	bool    IsLevelDesignerPlacedObject(CBaseEntity *pEntity);
	void	ManageObjectRelocation( void );
	void    CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg );
	const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

	// Elimination Mode
	int GetNewTeam(int wishTeam, bool bNewRound = false);
	float GetPlayerRespawnTime(CHL2MP_Player *pPlayer);

	// Shared
	int GetRewardFromRoundWin(CHL2MP_Player *pPlayer, int winnerTeam, bool gameOver);

	// Zombie Limiting
	bool CanSpawnZombie(void);

#ifdef BB2_AI
	void InitDefaultAIRelationships( void );
#endif //BB2_AI

	void AddBreakableDoor(CBaseEntity *pEntity);
	void RemoveBreakableDoor(CBaseEntity *pEntity);
	CBaseEntity *GetNearbyBreakableDoorEntity(CBaseEntity *pChecker);

	char szCurrentMap[MAX_MAP_NAME];
	unsigned long long m_ulMapSize;
#endif
	virtual void ClientDisconnected( edict_t *pClient );

	bool CheckGameOver( void );
	bool IsIntermission( void );

	void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	
	bool IsTeamplay(void);

	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );
	
private:

#ifndef CLIENT_DLL
	CUtlVector<EHANDLE> m_hRespawnableItemsAndWeapons;
	CUtlVector<EHANDLE> m_hBreakableDoors;
	float m_tmNextPeriodicThink;
	float m_flRestartGameTime;
	float m_flRoundStartTime;
	float m_flScoreBoardTime;
	bool m_bCompleteReset;
	bool m_bChangelevelDone;
#endif
};

inline CHL2MPRules* HL2MPRules()
{
	return static_cast<CHL2MPRules*>(g_pGameRules);
}

#endif //HL2MP_GAMERULES_H

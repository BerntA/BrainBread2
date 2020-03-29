//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "ammodef.h"
#include "GameBase_Shared.h"
#include "random_extended.h"

#ifdef BB2_AI
#include "hl2_shareddefs.h"
#endif //BB2_AI

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "fmod_manager.h"
#include "c_ai_basenpc.h"
#include "c_playerresource.h"
#else
#include "BasePropDoor.h"
#include "world.h"
#include "eventqueue.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "items.h"
#include "entitylist.h"
#include "mapentities.h"
#include "in_buttons.h"
#include <ctype.h>
#include "voice_gamemgr.h"
#include "iscorer.h"
#include "hl2mp_player.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "team.h"
#include "voice_gamemgr.h"
#include "hl2mp_gameinterface.h"
#include "npc_base_soldier.h"
#include "npc_base_soldier_static.h"
#include "npc_BaseZombie.h"
#include "objective_icon.h"
#include "GameBase_Server.h"
#include "filesystem.h"
#include "hl2mp_bot_temp.h"
#include "game_announcer.h"

// Bernt - This needs to be moved into CAIBaseNPC at some point...
int GetExperienceReward(CBaseEntity *pEntity)
{
	if (!pEntity)
		return 0;

	int reward = 1;

	CNPC_BaseZombie *pGetZombieInstance = dynamic_cast<CNPC_BaseZombie*> (pEntity);
	if (pGetZombieInstance)
		reward = pGetZombieInstance->GetXP();

	CNPC_BaseSoldier *pSoldierHumanoid = dynamic_cast<CNPC_BaseSoldier*> (pEntity);
	if (pSoldierHumanoid)
		reward = pSoldierHumanoid->GetXP();

	CNPCBaseSoldierStatic *pStaticHumanoid = dynamic_cast<CNPCBaseSoldierStatic*> (pEntity);
	if (pStaticHumanoid)
		reward = pStaticHumanoid->GetXP();

	if (pEntity->IsPlayer())
		reward = 3;

	return reward;
}

int GetZombieCredits(CBaseEntity *pEntity)
{
	if (!pEntity)
		return 0;

	int reward = 1;

	if (pEntity->IsPlayer())
		reward = 2;

	return reward;
}

CBaseCombatWeapon *GetActiveWeaponFromEntity(CBaseEntity *pEntity)
{
	if (!pEntity)
		return NULL;

	if (pEntity->MyCombatCharacterPointer())
		return pEntity->MyCombatCharacterPointer()->GetActiveWeapon();

	return NULL;
}

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

bool FindInList(const char **pStrings, const char *pToFind)
{
	int i = 0;
	while (pStrings[i][0] != 0)
	{
		if (Q_stricmp(pStrings[i], pToFind) == 0)
			return true;
		i++;
	}

	return false;
}

ConVar sv_report_client_settings("sv_report_client_settings", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );

extern ConVar mp_chattime;

#endif

REGISTER_GAMERULES_CLASS( CHL2MPRules );

BEGIN_NETWORK_TABLE_NOBASE( CHL2MPRules, DT_HL2MPRules )

#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iCurrentGamemode ) ),
	RecvPropInt( RECVINFO( m_iGamemodeFlags ) ),
	RecvPropBool( RECVINFO( m_bRoundStarted ) ),
	RecvPropBool( RECVINFO(m_bShouldShowScores)),
	RecvPropInt( RECVINFO( m_iRoundCountdown ) ),
	RecvPropFloat( RECVINFO( m_flServerStartTime ) ),
	RecvPropFloat( RECVINFO( m_flRespawnTime ) ),
	RecvPropInt(RECVINFO(m_iNumReinforcements)),

	// Vote Sys.
	RecvPropInt(RECVINFO(m_iCurrentVoteType)),
	RecvPropInt(RECVINFO(m_iCurrentYesVotes)),
	RecvPropInt(RECVINFO(m_iCurrentNoVotes)),
	RecvPropFloat( RECVINFO( m_flTimeUntilVoteEnds ) ),
	RecvPropFloat(RECVINFO(m_flTimeVoteStarted)),

	RecvPropArray3(RECVINFO_ARRAY(m_iEndMapVotesForType), RecvPropInt(RECVINFO(m_iEndMapVotesForType[0]))),
#else
	SendPropInt(SENDINFO(m_iCurrentGamemode), 3, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_iGamemodeFlags), 2, SPROP_UNSIGNED),
	SendPropBool(SENDINFO(m_bRoundStarted)),
	SendPropBool(SENDINFO(m_bShouldShowScores)),
	SendPropInt(SENDINFO(m_iRoundCountdown), 6, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flServerStartTime), -1, SPROP_CHANGES_OFTEN, 0.0f, 2048.0f),
	SendPropFloat(SENDINFO(m_flRespawnTime), -1, SPROP_CHANGES_OFTEN, 0.0f, 2048.0f),
	SendPropInt(SENDINFO(m_iNumReinforcements), 8, SPROP_UNSIGNED),

	// Vote Sys.
	SendPropInt(SENDINFO(m_iCurrentVoteType), 3, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_iCurrentYesVotes), 5, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_iCurrentNoVotes), 5, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flTimeUntilVoteEnds)),
	SendPropFloat(SENDINFO(m_flTimeVoteStarted)),

	SendPropArray3(SENDINFO_ARRAY3(m_iEndMapVotesForType), SendPropInt(SENDINFO_ARRAY(m_iEndMapVotesForType), 4, SPROP_UNSIGNED)),
#endif

	END_NETWORK_TABLE()


	LINK_ENTITY_TO_CLASS( hl2mp_gamerules, CHL2MPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )

	static HL2MPViewVectors g_HL2MPViewVectors(
	Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 

	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)

	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  //VEC_DUCK_VIEW		(m_vDuckView)

	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)

	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 ),	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)

	Vector(-16, -16, 0),	  //VEC_SLIDE_HULL_MIN (m_vSlideHullMin)
	Vector(16, 16, 36),	  //VEC_SLIDE_HULL_MAX	(m_vSlideHullMax)
	Vector(0, 0, 28),		  //VEC_SLIDE_VIEW		(m_vSlideView)

	Vector(-16, -16, 0),	  //VEC_SLIDE_TRACE_MIN (m_vSlideTraceMin)
	Vector(16, 16, 60)	  //VEC_SLIDE_TRACE_MAX (m_vSlideTraceMax)
	);

static const char *s_PreserveEnts[] =
{
	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_illusionary",
	"func_reflective_glass",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_start",
	"info_start_camera",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"game_manager",
	"trigger_player_block",
	"trigger_changelevel",
	"func_npc_obstacle",
	"func_precipitation",
	"", // END Marker
};

#ifdef CLIENT_DLL
void RecvProxy_HL2MPRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CHL2MPRules *pRules = HL2MPRules();
	Assert( pRules );
	*pOut = pRules;
}

BEGIN_RECV_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
	RecvPropDataTable( "hl2mp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPRules ), RecvProxy_HL2MPRules )
	END_RECV_TABLE()
#else
void* SendProxy_HL2MPRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CHL2MPRules *pRules = HL2MPRules();
	Assert( pRules );
	return pRules;
}

BEGIN_SEND_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
	SendPropDataTable( "hl2mp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2MPRules ), SendProxy_HL2MPRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		return ( pListener->GetTeamNumber() == pTalker->GetTeamNumber() );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif

// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
static char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Humans",
	"Deceased",
};

CHL2MPRules::CHL2MPRules()
{
#ifndef CLIENT_DLL
	m_uDisconnectedClients.Purge();

	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));
		pTeam->Init( sTeamNames[i], i );
		g_Teams.AddToTail( pTeam );
	}	

#ifdef BB2_AI
	InitDefaultAIRelationships();
#endif //BB2_AI

	Q_strncpy(szCurrentMap, STRING(gpGlobals->mapname), MAX_MAP_NAME);

	if ( !Q_strnicmp( szCurrentMap, "bbe_", 4 ) ) // Elimination
		m_iCurrentGamemode = MODE_ELIMINATION;
	else if (!Q_strnicmp(szCurrentMap, "bba_", 4)) // Arena (Team Survival)
		m_iCurrentGamemode = MODE_ARENA;
	else if (!Q_strnicmp(szCurrentMap, "bbd_", 4)) // Deathmatch (Humans - Free For All!)
		m_iCurrentGamemode = MODE_DEATHMATCH; 
	else // Classic aka Story Mode... (Objective Mode too if you will...)
		m_iCurrentGamemode = MODE_OBJECTIVE;

	m_flIntermissionEndTime = 0.0f;
	m_flScoreBoardTime = 0;
	m_bShouldShowScores = false;

	m_hBreakableDoors.RemoveAll();
	m_bChangelevelDone = false;

	// BB2
	m_flRespawnTime = 0.0f;
	m_bRoundStarted = false;
	m_iRoundCountdown = 0;
	m_ulMapSize = 0;
	g_pZombiesInWorld = 0;
	m_flRoundStartTime = 0;

	// Vote System:
	m_iCurrentYesVotes = 0;
	m_iCurrentNoVotes = 0;
	m_flTimeUntilVoteEnds = 0.0f;
	m_flTimeVoteStarted = 0.0f;
	m_flNextVoteTime = 0.0f;
	m_iAmountOfVoters = 0;
	m_iCurrentVoteType = 0;
	m_ullSteamIDToKickOrBan = 0;
	pchMapToChangeTo[0] = 0;
	m_flTimeRoundStarted = 0.0f;

	ResetEndMapVoting();

	char pszFullPath[80];
	Q_snprintf(pszFullPath, 80, "maps/%s.bsp", szCurrentMap);
	if (filesystem->FileExists(pszFullPath, "MOD"))
	{
		FileHandle_t f = filesystem->Open(pszFullPath, "rb", "MOD");
		if (f)
		{
			m_ulMapSize = (unsigned long long)filesystem->Size(f);
			filesystem->Close(f);
		}
	}

	PrecacheFootStepSounds();

	m_flServerStartTime = gpGlobals->curtime;

	// Execute linked .cfg for this map, if any:
	engine->ServerCommand(UTIL_VarArgs("exec maps/%s.cfg\n", szCurrentMap));

	int gamemodeFlags = 0;

	if ((GetCurrentGamemode() == MODE_ARENA) && bb2_arena_hard_mode.GetBool())
		gamemodeFlags |= GM_FLAG_ARENA_HARDMODE;

	if (CanUseSkills() && bb2_hard_scaling.GetBool())
		gamemodeFlags |= GM_FLAG_EXTREME_SCALING;
	
	m_iGamemodeFlags.Set(gamemodeFlags);

	// Check if map is workshop related, if so set download ID!
	GameBaseServer()->SetCurrentMapAddon(szCurrentMap);

#endif
}

const CViewVectors* CHL2MPRules::GetViewVectors()const
{
	return &g_HL2MPViewVectors;
}

const HL2MPViewVectors* CHL2MPRules::GetHL2MPViewVectors()const
{
	return &g_HL2MPViewVectors;
}

CHL2MPRules::~CHL2MPRules( void )
{
#ifndef CLIENT_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
#endif
}

#ifndef CLIENT_DLL
int CHL2MPRules::GetNewTeam(int wishTeam, bool bNewRound)
{
	int iTeamToJoin = wishTeam;

	int iSize = 0;
	// Check if the team we want to join has too many players:
	if (wishTeam == TEAM_HUMANS)
	{
		iSize = (GetTeamSize(TEAM_HUMANS) - GetTeamSize(TEAM_DECEASED));
		if (bNewRound)
			iSize--;

		if (iSize >= mp_limitteams.GetInt())
			iTeamToJoin = TEAM_DECEASED;
	}
	else if (wishTeam == TEAM_DECEASED)
	{
		iSize = (GetTeamSize(TEAM_DECEASED) - GetTeamSize(TEAM_HUMANS));
		if (bNewRound)
			iSize--;

		if (iSize >= mp_limitteams.GetInt())
			iTeamToJoin = TEAM_HUMANS;
	}

	return iTeamToJoin;
}

float CHL2MPRules::GetPlayerRespawnTime(CHL2MP_Player *pPlayer)
{
	if (!pPlayer)
		return 0.0f;

	if (GameBaseServer()->IsStoryMode())
		return (gpGlobals->curtime + bb2_story_respawn_time.GetFloat());

	if (!IsTeamplay())
		return (gpGlobals->curtime + bb2_deathmatch_respawn_time.GetFloat());

	float flDefaultTime = bb2_elimination_respawn_time.GetFloat();
	float flExtraTime = bb2_elimination_respawn_time_scale.GetFloat();
	int teamSize = GetTeamSize(pPlayer->GetSelectedTeam());

	return (gpGlobals->curtime + (flDefaultTime + (flExtraTime * (float)teamSize)));
}
#endif

int CHL2MPRules::GetTeamSize(int team)
{
#ifdef CLIENT_DLL
	if (!g_PR)
		return 0;
#endif

	// Calculate the proper team size:
	int playerCount = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
#ifdef CLIENT_DLL
		if (g_PR->IsConnected(i) && (g_PR->GetSelectedTeam(i) == team))
			playerCount++;
#else
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		if (pPlayer->GetSelectedTeam() == team)
			playerCount++;
#endif
	}

	return playerCount;
}

int CHL2MPRules::GetPlayersInGame(void)
{
#ifdef CLIENT_DLL
	if (!g_PR)
		return 0;
#endif

	int playerCount = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
#ifdef CLIENT_DLL
		if (g_PR->IsConnected(i))
			playerCount++;
#else
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		playerCount++;
#endif
	}

	return playerCount;
}

const char *CHL2MPRules::GetNameForCombatCharacter(int index)
{
	if (index <= 0)
		return "";

	CBaseEntity *pCharacter = NULL;

#ifdef CLIENT_DLL
	if (!g_PR)
		return "";

	if ((index >= 1) && (index <= MAX_PLAYERS))
		return g_PR->GetPlayerName(index);

	pCharacter = ClientEntityList().GetEnt( index );
#else
	pCharacter = UTIL_EntityByIndex(index);
#endif

	if (!pCharacter || !pCharacter->MyCombatCharacterPointer())
		return "";

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pCharacter);

#ifndef CLIENT_DLL
	CAI_BaseNPC *pNPC = pCharacter->MyNPCPointer();
	if (pPlayer)
		return pPlayer->GetPlayerName();
	else if (pNPC)
		return pNPC->GetFriendlyName();
#else
	C_AI_BaseNPC *pNPC = pCharacter->MyNPCPointer();
	if (pPlayer)
		return g_PR->GetPlayerName(index);
	else if (pNPC)
		return pNPC->GetNPCName();
#endif

	return "";
}

float CHL2MPRules::GetTimeLeft()
{
	float flTime = (m_flServerStartTime + (GetTimelimitValue() * 60.0f)) - gpGlobals->curtime;
	if (flTime > 0)
		return flTime;

	return 0.0f;
}

float CHL2MPRules::GetTimelimitValue()
{
	switch (GetCurrentGamemode())
	{
	case MODE_ARENA:
		return mp_timelimit_arena.GetFloat();
	case MODE_DEATHMATCH:
		return mp_timelimit_deathmatch.GetFloat();
	case MODE_ELIMINATION:
		return mp_timelimit_elimination.GetFloat();
	}

	return  mp_timelimit_objective.GetFloat();
}

float CHL2MPRules::GetReinforcementRespawnTime()
{
	float flTime = m_flRespawnTime - gpGlobals->curtime;

	if (flTime > 0)
		return flTime;
	else
	{
#ifndef CLIENT_DLL
		m_flRespawnTime = gpGlobals->curtime + bb2_arena_respawn_time.GetFloat();
#endif
		return 0.0f;
	}
}

#ifndef CLIENT_DLL
int CHL2MPRules::GetRewardFromRoundWin(CHL2MP_Player *pPlayer, int winnerTeam, bool gameOver)
{
	if (CanUseSkills() == false)
		return 0;

	float xpReward = 0.0f;
	float xpRequired = ((float)pPlayer->m_BB2Local.m_iSkill_XPLeft);
	float timeLeft = GetTimeLeft();
	if (GetCurrentGamemode() == MODE_OBJECTIVE)
	{
		if ((pPlayer->HasPlayerEscaped() || ((pPlayer->GetTeamNumber() == winnerTeam) && pPlayer->HasFullySpawned() && pPlayer->IsAlive())) && gameOver)		
			xpReward = xpRequired * (GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPGameWinObjective / 100.0f);
	}
	else if (GetCurrentGamemode() == MODE_ARENA)
	{
		if ((winnerTeam == TEAM_HUMANS) && pPlayer->HasFullySpawned())
		{
			xpReward = xpRequired * ((gameOver ? GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPGameWinArena : GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPRoundWinArena) / 100.0f);
			GameBaseShared()->GetAchievementManager()->WriteToAchievement(pPlayer, "ACH_GM_ARENA_WIN");

			if (timeLeft <= 0)
				xpReward = xpRequired * (GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flXPRoundWinArena / 100.0f);
		}
	}

	int iXPToGive = abs((int)(ceil(xpReward)));
	if (iXPToGive > 0)
	{
		bool bRet = pPlayer->CanLevelUp(iXPToGive, NULL);
		if (bRet)
		{
			char pchArg1[16];
			Q_snprintf(pchArg1, 16, "%i", iXPToGive);
			GameBaseServer()->SendToolTip("#TOOLTIP_XP_REWARD", GAME_TIP_DEFAULT, pPlayer->entindex(), pchArg1);
		}
	}

	return iXPToGive;
}

void CHL2MPRules::DisplayScores(int iWinner)
{
	if (m_bShouldShowScores || (GetTimeLeft() <= 0.0f))
		return;

	m_flScoreBoardTime = gpGlobals->curtime + mp_chattime.GetFloat();
	m_bShouldShowScores = true;
	ResetVote(true);

	IGameEvent * event = gameeventmanager->CreateEvent("round_end");
	if (event)
	{
		event->SetInt("team", iWinner);
		gameeventmanager->FireEvent(event);
	}

	KeyValues *data = new KeyValues("data");
	data->SetInt("winner", iWinner);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		pPlayer->AddFlag(FL_FROZEN | FL_GODMODE);

		if (pPlayer->IsBot())
			continue;

		GetRewardFromRoundWin(pPlayer, iWinner, false);
		pPlayer->ShowViewPortPanel(PANEL_ENDSCORE, true, data);
	}

	data->deleteThis();
}

void CHL2MPRules::NewRoundInit(int iPlayersInGame)
{
	if (iPlayersInGame <= 0)
		return;

	// In elimination mode we don't start the round before one of the teams have players!
	if (GetCurrentGamemode() == MODE_ELIMINATION)
	{
		if (GetTeamSize(TEAM_HUMANS) <= 0 && GetTeamSize(TEAM_DECEASED) <= 0)
			return;
	}

	float flCountdownTime = bb2_roundstart_freezetime.GetFloat();
	if (GameBaseServer()->IsTutorialModeEnabled() || GameBaseServer()->IsStoryMode())
		flCountdownTime = 1.0f;

	bool bCountdownStarted = (m_flRoundStartTime > 0.0f);

	if (!m_bRoundStarted && !bCountdownStarted)
	{
		m_flRoundStartTime = gpGlobals->curtime + flCountdownTime;
		RestartGame();
		return;
	}

	// Countdown started:
	if (bCountdownStarted)
	{
		// Decrement per sec:
		if (m_flRoundStartTime > gpGlobals->curtime)
			m_iRoundCountdown = (m_flRoundStartTime - gpGlobals->curtime);
		else
		{
			if (m_bRoundStarted)
				return;

			m_flRoundStartTime = 0.0f;
			m_bRoundStarted = true;
			m_flTimeRoundStarted = gpGlobals->curtime;

			if (GetCurrentGamemode() == MODE_ARENA)
			{
				m_iNumReinforcements = IsGamemodeFlagActive(GM_FLAG_ARENA_HARDMODE) ? 0 : bb2_arena_reinforcement_count.GetInt();
				m_flRespawnTime = (gpGlobals->curtime + bb2_arena_respawn_time.GetFloat());
			}

			// Fire out an event!
			// Broadcast to whoever wants to know that we're ready to 'play'.
			IGameEvent * event = gameeventmanager->CreateEvent("round_started");
			if (event)
				gameeventmanager->FireEvent(event);

			GameAnnouncer->Reset();

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				if (!pPlayer)
					continue;

				pPlayer->SetLastTimeRanCommand(gpGlobals->curtime);

				if (pPlayer->IsObserver() || !pPlayer->IsAlive() || (pPlayer->GetTeamNumber() < TEAM_HUMANS))
					continue;

				if (ShouldHideHUDDuringRoundWait())
					pPlayer->RemoveFlag(FL_FROZEN | FL_GODMODE);

				if (!pPlayer->IsBot() && !GameBaseServer()->IsTutorialModeEnabled() && (GetCurrentGamemode() != MODE_ELIMINATION)
					&& (GetCurrentGamemode() != MODE_DEATHMATCH))
				{
					CSingleUserRecipientFilter filter(pPlayer);
					pPlayer->EmitSound(filter, pPlayer->entindex(), "Music.Round.Start");
				}
			}

			return;
		}

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
			if (!pPlayer || pPlayer->IsObserver() || !pPlayer->IsAlive() || (pPlayer->GetTeamNumber() < TEAM_HUMANS))
				continue;

			if (!m_bRoundStarted && pPlayer->GetGroundEntity() && ShouldHideHUDDuringRoundWait() && (!(pPlayer->GetFlags() & FL_FROZEN) || !(pPlayer->GetFlags() & FL_GODMODE)))
				pPlayer->AddFlag(FL_FROZEN | FL_GODMODE);
		}
	}
}
#endif

void CHL2MPRules::CreateStandardEntities( void )
{
#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.

	BaseClass::CreateStandardEntities();

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt = 
#endif
		CBaseEntity::Create( "hl2mp_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

bool CHL2MPRules::IsIntermission( void )
{
#ifndef CLIENT_DLL
	return m_flIntermissionEndTime > gpGlobals->curtime;
#endif

	return false;
}

void CHL2MPRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	if ( IsIntermission() )
		return;

	BaseClass::PlayerKilled( pVictim, info );
#endif
}

bool CHL2MPRules::IsTeamplay(void)
{
	if (m_iCurrentGamemode == MODE_DEATHMATCH)
		return false;

	return true;
}

// This method handles transmitting sound scripts to run on the client instead of the server, so that we allow custom sounds/diff sounds for each player in brief terms.
#ifndef CLIENT_DLL
void CHL2MPRules::EmitSoundToClient(CBaseEntity *pAnnouncer, const char *szOriginalSound, int iType, bool bGenderMale, int playerIndex)
{
	// Disable stuff when waiting for transfer.
	if (IsGameoverOrScoresVisible())
		return;

	int entindex = -1;
	const char *survivorModel = "", *survivorPrefix = "";
	if (iType != BB2_SoundTypes::TYPE_ANNOUNCER)
	{
		if (!pAnnouncer)
			return;

		// If the emitter is some NPC then check the sound penalty time:
		if (pAnnouncer->IsNPC() && pAnnouncer->MyNPCPointer() && !pAnnouncer->MyNPCPointer()->IsBoss() && strcmp(szOriginalSound, "Death") && strcmp(szOriginalSound, "Die"))
		{
			if (pAnnouncer->MyNPCPointer()->GetSoundPenaltyTime() > gpGlobals->curtime)
				return;

			// Prevent spamming.
			if (TryTheLuck(0.35) && (!strcmp(szOriginalSound, "Pain") || !strcmp(szOriginalSound, "Taunt")))
				return;

			pAnnouncer->MyNPCPointer()->SetSoundPenaltyTime(4.0f);
		}

		CHL2MP_Player *pPlayer = ToHL2MPPlayer(pAnnouncer);
		if (pPlayer)
		{
			// Don't play any sounds yet...
			if (!pPlayer->HasFullySpawned())
				return;

			survivorModel = pPlayer->GetSoundsetSurvivorLink();
			survivorPrefix = pPlayer->GetSoundsetPrefix();
		}

		entindex = pAnnouncer->entindex();
	}

	// Send our data to the client, let the client decide the rest, we've done our part:
	IGameEvent * event = gameeventmanager->CreateEvent("client_sound_transmit");
	if (event)
	{
		event->SetInt("entity", entindex);
		event->SetString("original", szOriginalSound);
		event->SetBool("gender", bGenderMale);
		event->SetInt("type", iType);
		event->SetString("survivorprefix", survivorPrefix);
		event->SetInt("playerindex", playerIndex);
		event->SetString("survivorlink", survivorModel);
		gameeventmanager->FireEvent(event);
	}
}
#endif

// Are we allowed to spawn more zombies?
#ifndef CLIENT_DLL
bool CHL2MPRules::CanSpawnZombie(void)
{
	return (g_pZombiesInWorld <= bb2_zombie_max.GetInt());
}

void CHL2MPRules::EndRound(bool bRestart)
{
	if (IsGameoverOrScoresVisible())
		return;

	bool bEndGameNow = false;
	if (GetTimeLeft() <= 0.0f)
		bEndGameNow = true;

	int iWinner = bRestart ? TEAM_DECEASED : TEAM_HUMANS;

	if (GetCurrentGamemode() == MODE_ELIMINATION)
	{
		CTeam *pTeam = GetGlobalTeam(iWinner);
		if (pTeam && (pTeam->GetScore() >= bb2_elimination_fraglimit.GetInt()))
			bEndGameNow = true;
	}

	if (bEndGameNow)
		GoToIntermission(iWinner);
	else
		DisplayScores(iWinner);
}

void CHL2MPRules::GameModeSharedThink(void)
{
	int clientsInGame = 0;
	int humansInGame = 0;
	int playersInGame = 0;
	int zombiesInGame = 0;
	int escapedPlayers = 0;

	int eliminationZombiePlayers = 0;
	int eliminationHumanPlayers = 0;

	// How many is participating in the game?
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		clientsInGame++;

		if ((pPlayer->GetSelectedTeam() == TEAM_HUMANS) && pPlayer->HasFullySpawned())
			eliminationHumanPlayers++;

		if ((pPlayer->GetSelectedTeam() == TEAM_DECEASED) && pPlayer->HasFullySpawned())
			eliminationZombiePlayers++;

		if (pPlayer->GetTeamNumber() == TEAM_HUMANS)
			humansInGame++;

		if (pPlayer->GetTeamNumber() == TEAM_DECEASED)
			zombiesInGame++;

		if ((pPlayer->GetTeamNumber() != TEAM_UNASSIGNED) && pPlayer->HasJoinedGame())
			playersInGame++;

		if (pPlayer->HasPlayerEscaped())
			escapedPlayers++;
	}

	if (m_bRoundStarted && !IsIntermission() && !m_bShouldShowScores && !g_fGameOver && !GameBaseServer()->IsTutorialModeEnabled())
	{
		// In Elimination mode we restart the game when everyone on one of the teams are dead.
		// In classic/objective mode extracted survivors will tip the scale.
		// In Arena there has to be one who's alive.
		if ((GetCurrentGamemode() == MODE_OBJECTIVE) && (humansInGame <= 0) && (escapedPlayers > 0))
			GoToIntermission();
		else if (((GetCurrentGamemode() == MODE_ELIMINATION) || !IsTeamplay()) && (clientsInGame > 0))
		{
			if (IsTeamplay())
			{
				CTeam *pHumans = GetGlobalTeam(TEAM_HUMANS);
				if (pHumans)
					pHumans->Think();

				CTeam *pDeceased = GetGlobalTeam(TEAM_DECEASED);
				if (pDeceased)
					pDeceased->Think();

				int humanScore = pHumans ? pHumans->GetScore() : 0;
				int deceasedScore = pDeceased ? pDeceased->GetScore() : 0;

				// Check if we're done playing in surival mode:
				float flTimeLeft = GetTimeLeft();
				if (flTimeLeft <= 0 ||
					(bb2_elimination_fraglimit.GetInt() <= humanScore) ||
					(bb2_elimination_fraglimit.GetInt() <= deceasedScore))
				{
					int winnerTeam = TEAM_DECEASED;
					if (humanScore > deceasedScore)
						winnerTeam = TEAM_HUMANS;

					GoToIntermission(winnerTeam);
					return;
				}

				// Check for exterminations:
				if (eliminationHumanPlayers > 1 && eliminationZombiePlayers > 1)
				{
					if (humansInGame <= 0)
					{
						if (pDeceased)
							pDeceased->AddScore(bb2_elimination_score_from_extermination.GetInt());

						EndRound(true);
						return;
					}
					else if (zombiesInGame <= 0)
					{
						if (pHumans)
							pHumans->AddScore(bb2_elimination_score_from_extermination.GetInt());

						EndRound(false);
						return;
					}
				}
			}
			else
			{
				// Get the highest score:
				int highestScore = 0;
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
					if (!pPlayer)
						continue;

					int playerScore = pPlayer->GetTotalScore();
					if (playerScore > highestScore)
						highestScore = playerScore;
				}

				// Check if we're done playing or if the fraglimit has been reached.
				float flTimeLeft = GetTimeLeft();
				if (flTimeLeft <= 0 ||
					(bb2_deathmatch_fraglimit.GetInt() <= highestScore))
				{
					GoToIntermission();
					return;
				}

				GameAnnouncer->Think((round(flTimeLeft)));
			}

			// Check if we should respawn some people:
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				if (!pPlayer || pPlayer->IsBot() || !pPlayer->HasLoadedStats())
					continue;

				if ((pPlayer->GetRespawnTime() < gpGlobals->curtime) && (pPlayer->GetRespawnTime() > 0.0f))
				{
					pPlayer->SetRespawnTime(0.0f);
					pPlayer->HandleCommand_JoinTeam((IsTeamplay() ? pPlayer->GetSelectedTeam() : TEAM_HUMANS), true);
					pPlayer->ForceRespawn();
				}
			}
		}
		else if (!GameBaseServer()->IsClassicMode() && (GetCurrentGamemode() != MODE_ELIMINATION) && (GetCurrentGamemode() != MODE_DEATHMATCH) && humansInGame <= 0 && clientsInGame > 0) // If everyone is dead / is zombies we restart.
			EndRound(true);
		else if ((GetCurrentGamemode() == MODE_OBJECTIVE) && (GetTimeLeft() <= 0) && !GameBaseServer()->IsStoryMode() && (clientsInGame > 0))
			GoToIntermission(TEAM_DECEASED);
		else if (clientsInGame <= 0) // Reset the game itself if there's no one left! (connected)
		{
			m_bRoundStarted = false;
			m_iRoundCountdown = 0;

			CTeam *pHumans = GetGlobalTeam(TEAM_HUMANS);
			if (pHumans)
			{
				pHumans->SetScore(0);
				pHumans->SetRoundsWon(0);
			}

			CTeam *pZombies = GetGlobalTeam(TEAM_DECEASED);
			if (pZombies)
			{
				pZombies->SetScore(0);
				pZombies->SetRoundsWon(0);
			}

			IGameEvent * event = gameeventmanager->CreateEvent("round_end");
			if (event)
			{
				event->SetInt("team", TEAM_DECEASED);
				gameeventmanager->FireEvent(event);
			}

			RestartGame();
			return;
		}

		// Respawn waiting reinforcements.
		if (GetCurrentGamemode() == MODE_ARENA)
		{
			if ((GetReinforcementRespawnTime() <= 0.0f) && (m_iNumReinforcements > 0))
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					if (m_iNumReinforcements <= 0)
						break;

					CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
					if (!pPlayer || !pPlayer->HasLoadedStats() || (pPlayer->GetTeamNumber() != TEAM_SPECTATOR))
						continue;

					pPlayer->HandleCommand_JoinTeam(TEAM_HUMANS, true);
					pPlayer->ForceRespawn();
					m_iNumReinforcements--;
				}
			}
		}
		else if (GameBaseServer()->IsStoryMode())
		{
			// Check if we should respawn some people:
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
				if (!pPlayer)
					continue;

				if (pPlayer->IsBot() || !pPlayer->HasLoadedStats())
					continue;

				if ((pPlayer->GetRespawnTime() < gpGlobals->curtime) && (pPlayer->GetRespawnTime() > 0.0f))
				{
					pPlayer->SetRespawnTime(0.0f);
					pPlayer->HandleCommand_JoinTeam(TEAM_HUMANS, true);
					pPlayer->ForceRespawn();
				}
			}
		}
	}
	// If the server is empty and the timelimit runs out then change map...
	else if (clientsInGame <= 0 && (GetTimeLeft() <= 0.0f))
		GoToIntermission(TEAM_DECEASED);

	// If the clients in the game is above required we will start the round if not already started.
	NewRoundInit(playersInGame);

	// Update the base shared server mode:
	GameBaseServer()->OnUpdate(clientsInGame);

	// Show scores for x sec when a round has ended/failed then we call newroundinit to start a new round!
	if (m_bShouldShowScores && !g_fGameOver)
	{
		if (m_flScoreBoardTime < gpGlobals->curtime)
		{
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
				if (!pPlayer)
					continue;

				pPlayer->RemoveFlag(FL_FROZEN | FL_GODMODE);

				if (pPlayer->IsBot())
					continue;

				pPlayer->ShowViewPortPanel(PANEL_ENDSCORE, false);
			}

			m_flScoreBoardTime = 0.0f;
			m_bShouldShowScores = false;
			m_bRoundStarted = false;
		}
	}
}

void CHL2MPRules::VoteSystemThink(void)
{
	if (m_iCurrentVoteType)
	{
		int amountOfVotes = (m_iCurrentYesVotes + m_iCurrentNoVotes);
		if ((gpGlobals->curtime > m_flTimeUntilVoteEnds) || (amountOfVotes >= m_iAmountOfVoters))
		{
			int voteType = m_iCurrentVoteType;
			int iVotesLeft = m_iAmountOfVoters - m_iCurrentYesVotes;
			m_iCurrentNoVotes = iVotesLeft;

			float yesVotes = (((float)m_iCurrentYesVotes) / ((float)m_iAmountOfVoters)) * 100.0f;
			bool bVoteStatus = false;
			if (yesVotes >= ((voteType == VOTE_TYPE_MAP) ? bb2_vote_required_percentage.GetFloat() : bb2_vote_required_percentage_kickban.GetFloat()))
			{
				bVoteStatus = true;

				CBasePlayer *pTarget = NULL;
				if (voteType != VOTE_TYPE_MAP)
					pTarget = UTIL_PlayerBySteamID(m_ullSteamIDToKickOrBan);

				char pchServerCMD[80];
				switch (voteType)
				{

				case VOTE_TYPE_KICK:
				{
					if ((bb2_vote_kick_ban_time.GetInt() > 0) && (m_ullSteamIDToKickOrBan > 0))
					{
						Q_snprintf(pchServerCMD, 80, "banid %i %llu\n", bb2_vote_kick_ban_time.GetInt(), m_ullSteamIDToKickOrBan);
						engine->ServerCommand(pchServerCMD);
					}
					break;
				}

				case VOTE_TYPE_BAN:
				{
					if (m_ullSteamIDToKickOrBan > 0)
					{
						Q_snprintf(pchServerCMD, 80, "banid %i %llu;writeid;writeip\n", bb2_ban_time.GetInt(), m_ullSteamIDToKickOrBan);
						engine->ServerCommand(pchServerCMD);
					}
					break;
				}

				case VOTE_TYPE_MAP:
					GameBaseServer()->DoMapChange(pchMapToChangeTo);
					break;

				}

				if (pTarget)
				{
					Q_snprintf(pchServerCMD, 80, "kickid %i\n", pTarget->GetUserID());
					engine->ServerCommand(pchServerCMD);
				}
			}

			if (bVoteStatus)
				GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_SUCCESS", GAME_TIP_WARNING);
			else
				GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_FAILURE", GAME_TIP_WARNING);
		
			DispatchVoteEvent(0, 0, true);
			ResetVote();
			m_flNextVoteTime = gpGlobals->curtime + bb2_vote_frequency_time.GetFloat();
		}
	}
}

void CHL2MPRules::ResetVote(bool bFullReset)
{
	m_iCurrentYesVotes = 0;
	m_iCurrentNoVotes = 0;
	m_iAmountOfVoters = 0;
	m_iCurrentVoteType = 0;
	m_ullSteamIDToKickOrBan = 0;
	pchMapToChangeTo[0] = 0;

	if (bFullReset)	
		m_flNextVoteTime = 0.0f;	
}

bool CHL2MPRules::CanCreateVote(CBasePlayer *pVoter)
{
	if (!pVoter || IsGameoverOrScoresVisible())
		return false;

	if (m_iCurrentVoteType)
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_DENY_ACTIVE", GAME_TIP_WARNING, pVoter->entindex());
		return false;
	}

	float timePassed = (m_flNextVoteTime - gpGlobals->curtime);
	if ((bb2_vote_roundstart_delay.GetFloat() > 0.0f) && ((gpGlobals->curtime - m_flTimeRoundStarted) < bb2_vote_roundstart_delay.GetFloat()))
		timePassed += abs(bb2_vote_roundstart_delay.GetFloat() - (gpGlobals->curtime - m_flTimeRoundStarted));

	if (timePassed > 0.0f)
	{
		char pchTime[32];
		Q_snprintf(pchTime, 32, "%i", (int)timePassed);
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_DENY_WAIT", GAME_TIP_WARNING, pVoter->entindex(), pchTime);
		return false;
	}

	CHL2MP_Player *pClient = ToHL2MPPlayer(pVoter);
	if (pClient && CanUseSkills() && engine->IsDedicatedServer())
	{
		if (pClient->GetPlayerLevel() < bb2_vote_required_level.GetInt())
		{
			char pchLevel[32];
			Q_snprintf(pchLevel, 32, "%i", bb2_vote_required_level.GetInt());
			GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_DENY_LEVEL", GAME_TIP_WARNING, pVoter->entindex(), pchLevel);
			return false;
		}
	}

	return true;
}

void CHL2MPRules::CreateBanKickVote(CBasePlayer *pVoter, CBasePlayer *pTarget, bool bBan)
{
	if (!pVoter || !pTarget)
		return;

	if (pVoter == pTarget)
		return;

	if ((pVoter->LastTimePlayerTalked() + 0.5f) > gpGlobals->curtime)
		return;

	pVoter->NotePlayerTalked();

	if (bb2_vote_disable_ban.GetBool() && bBan)
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_BAN_DISABLED", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	if (bb2_vote_disable_kick.GetBool() && !bBan)
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_KICK_DISABLED", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	if (!m_bRoundStarted)
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_WAIT_GAME", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	CHL2MP_Player *pTargetClient = ToHL2MPPlayer(pTarget);
	if (pTargetClient && pTargetClient->IsAdminOnServer())
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_KICKBAN_ADMIN", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	if (!CanCreateVote(pVoter))
		return;

	ResetVote();
	SetupVote(pVoter->entindex());
	m_iCurrentVoteType = bBan ? VOTE_TYPE_BAN : VOTE_TYPE_KICK;
	m_ullSteamIDToKickOrBan = ((unsigned long long)pTarget->GetSteamIDAsUInt64());
	
	GameBaseServer()->GameAnnouncement((bBan ? "#Vote_Ban_Player" : "#Vote_Kick_Player"), pVoter->GetPlayerName(), pTarget->GetPlayerName());
	DispatchVoteEvent(pVoter->entindex(), pTarget->entindex());
}

void CHL2MPRules::CreateMapVote(CBasePlayer *pVoter, const char *map)
{
	if (!pVoter)
		return;

	if ((pVoter->LastTimePlayerTalked() + 0.5f) > gpGlobals->curtime)
		return;

	pVoter->NotePlayerTalked();

	if (bb2_vote_disable_map.GetBool())
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_MAP_DISABLED", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	if (!m_bRoundStarted)
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_WAIT_GAME", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	char pszMapPath[80];
	Q_snprintf(pszMapPath, 80, "maps/%s.bsp", map);
	if (!filesystem->FileExists(pszMapPath, "MOD"))
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_MAP_ERROR", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	bool bFound = IsMapInMapCycle(map);
	if (!bFound)
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_VOTE_MAP_ERROR_CYCLE", GAME_TIP_WARNING, pVoter->entindex());
		return;
	}

	if (!CanCreateVote(pVoter))
		return;

	ResetVote();
	SetupVote(pVoter->entindex());
	m_iCurrentVoteType = VOTE_TYPE_MAP;
	Q_strncpy(pchMapToChangeTo, map, MAX_MAP_NAME);

	GameBaseServer()->GameAnnouncement("#Vote_MapChange", pVoter->GetPlayerName(), pchMapToChangeTo);
	DispatchVoteEvent(pVoter->entindex(), 0);
}

void CHL2MPRules::SetupVote(int indexOfVoter)
{
	int iAmountOfVoters = 0;
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		int plrIndex = (i - 1);
		m_bPlayersAllowedToVote[plrIndex] = false;
		m_bPlayersVoted[plrIndex] = false;

		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		m_bPlayersAllowedToVote[plrIndex] = true;
		iAmountOfVoters++;
	}
	m_iAmountOfVoters = iAmountOfVoters;

	m_flTimeVoteStarted = gpGlobals->curtime;
	m_flTimeUntilVoteEnds = gpGlobals->curtime + bb2_vote_time.GetFloat();

	if (indexOfVoter >= 1 && indexOfVoter <= gpGlobals->maxClients)
		m_bPlayersVoted[indexOfVoter - 1] = true;

	m_iCurrentYesVotes++;
}

void CHL2MPRules::PlayerVote(CBasePlayer *pPlayer, bool bYes)
{
	if (!pPlayer || !m_iCurrentVoteType)
		return;

	int voteIndex = (pPlayer->entindex() - 1);
	if ((voteIndex < 0) || (voteIndex >= MAX_PLAYERS))
		return;

	bool bCanVote = m_bPlayersAllowedToVote[voteIndex];
	bool bHasVoted = m_bPlayersVoted[voteIndex];
	if (bCanVote && !bHasVoted)
	{
		if (bYes)
			m_iCurrentYesVotes++;
		else
			m_iCurrentNoVotes++;

		m_bPlayersVoted[voteIndex] = true;
	}
}

void CHL2MPRules::DispatchVoteEvent(int indexOfVoter, int targetIndex, bool bVoteEnd)
{
	if (bVoteEnd)
	{
		IGameEvent *event = gameeventmanager->CreateEvent("votesys_end");
		if (event)
			gameeventmanager->FireEvent(event);

		return;
	}

	IGameEvent *event = gameeventmanager->CreateEvent("votesys_start");
	if (event)
	{
		event->SetInt("index", indexOfVoter);
		event->SetInt("type", m_iCurrentVoteType);
		event->SetInt("targetID", targetIndex);
		event->SetString("mapName", pchMapToChangeTo);
		gameeventmanager->FireEvent(event);
	}
}

void CHL2MPRules::GameEndVoteThink(void)
{
	if (!g_fGameOver)
		return;

	if (m_flIntermissionEndTime < gpGlobals->curtime)
	{
		if (!m_bChangelevelDone)
		{
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CHL2MP_Player *pPlayer = (CHL2MP_Player*)UTIL_PlayerByIndex(i);
				if (!pPlayer)
					continue;

				pPlayer->Reset();
				pPlayer->ShowViewPortPanel(PANEL_ENDSCORE, false);
			}

			if (GetPlayersInGame() > 0)
				StartEndMapVote();
			else
				ChangeLevel();

			m_bChangelevelDone = true;
		}
		else if (m_bEndMapVotingEnabled)
		{
			if (m_flEndVoteTimeEnd < gpGlobals->curtime)
			{				
				bool bDoReset = true;
				int voteChoice = GetVoteTypeWithMostVotes();

				switch (voteChoice)
				{

				case ENDMAP_VOTE_OBJECTIVE:
				{
					GameBaseServer()->DoMapChange(pchMapOptions[0]);
					break;
				}

				case ENDMAP_VOTE_ARENA:
				{
					GameBaseServer()->DoMapChange(pchMapOptions[1]);
					break;
				}

				case ENDMAP_VOTE_ELIMINATION:
				{
					GameBaseServer()->DoMapChange(pchMapOptions[2]);
					break;
				}

				case ENDMAP_VOTE_DEATHMATCH:
				{
					GameBaseServer()->DoMapChange(pchMapOptions[3]);
					break;
				}

				case ENDMAP_VOTE_RETRY:
				{
					GameBaseServer()->DoMapChange(szCurrentMap);
					break;
				}

				case ENDMAP_VOTE_REFRESH:
				{
					bDoReset = false;
					StartEndMapVote(true);
					break;
				}

				default: // No one voted... 
				{
					ChangeLevel();
					break;
				}

				}

				if (bDoReset)
					ResetEndMapVoting();
			}
		}
	}
}

void CHL2MPRules::StartEndMapVote(bool bRefresh)
{
	ResetEndMapVoting();

	m_bEndMapVotingEnabled = true;
	m_flEndVoteTimeEnd = gpGlobals->curtime + bb2_vote_time_endgame.GetFloat();

	bool bShouldShowRandom = ShouldShowRandomMaps();
	Q_strncpy(pchMapOptions[0], GetRandomMapForVoteSys(MODE_OBJECTIVE, bShouldShowRandom), MAX_MAP_NAME);
	Q_strncpy(pchMapOptions[1], GetRandomMapForVoteSys(MODE_ARENA, bShouldShowRandom), MAX_MAP_NAME);
	Q_strncpy(pchMapOptions[2], GetRandomMapForVoteSys(MODE_ELIMINATION, bShouldShowRandom), MAX_MAP_NAME);
	Q_strncpy(pchMapOptions[3], GetRandomMapForVoteSys(MODE_DEATHMATCH, bShouldShowRandom), MAX_MAP_NAME);

	int iMapChoices = 0;
	for (int i = 0; i < 4; i++)
	{
		if (pchMapOptions[i] && pchMapOptions[i][0])
			iMapChoices++;
	}

	KeyValues *data = new KeyValues("data");
	data->SetBool("refresh", bRefresh);
	data->SetInt("mapChoices", iMapChoices);

	data->SetString("map1", pchMapOptions[0]);
	data->SetString("map2", pchMapOptions[1]);
	data->SetString("map3", pchMapOptions[2]);
	data->SetString("map4", pchMapOptions[3]);

	data->SetFloat("timeleft", m_flEndVoteTimeEnd);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		if (pPlayer->IsBot())
			continue;

		pPlayer->ShowViewPortPanel(PANEL_ENDVOTE, true, data);
	}

	data->deleteThis();
}

void CHL2MPRules::ResetEndMapVoting(void)
{
	m_bEndMapVotingEnabled = false;
	m_flEndVoteTimeEnd = 0.0f;

	for (int i = 0; i < MAX_PLAYERS; i++)
		m_iEndVotePlayerChoices[i] = 0;

	for (int i = 0; i < ENDMAP_VOTE_TYPES_MAX; i++)
		m_iEndMapVotesForType.Set(i, 0);

	pchMapOptions[0][0] = 0;
	pchMapOptions[1][0] = 0;
	pchMapOptions[2][0] = 0;
	pchMapOptions[3][0] = 0;
}

void CHL2MPRules::RecalculateEndMapVotes(void)
{
	for (int i = 0; i < ENDMAP_VOTE_TYPES_MAX; i++)
	{
		int votes = 0;
		for (int plr = 0; plr < MAX_PLAYERS; plr++)
		{
			if (m_iEndVotePlayerChoices[plr] == (i + 1))
				votes++;
		}

		m_iEndMapVotesForType.Set(i, votes);
	}
}

int CHL2MPRules::GetVoteTypeWithMostVotes(void)
{
	int iVotes = 0;
	for (int i = 0; i < ENDMAP_VOTE_TYPES_MAX; i++)
	{
		if (m_iEndMapVotesForType.Get(i) > iVotes)
			iVotes = m_iEndMapVotesForType.Get(i);
	}

	if (iVotes > 0)
	{
		CUtlVector<int> pAvailableChoices;
		for (int i = 0; i < ENDMAP_VOTE_TYPES_MAX; i++)
		{
			if (iVotes == m_iEndMapVotesForType.Get(i))
				pAvailableChoices.AddToTail((i + 1));
		}

		if (!pAvailableChoices.Count())
		{
			pAvailableChoices.Purge();
			return 0;
		}

		int choice = pAvailableChoices[random->RandomInt(0, (pAvailableChoices.Count() - 1))];
		pAvailableChoices.Purge();
		return choice;
	}

	return 0; 
}

bool CHL2MPRules::ShouldShowRandomMaps(void)
{
	bool bMapTypes[MODE_TOTAL];
	for (int mapType = 0; mapType < MODE_TOTAL; mapType++)
		bMapTypes[mapType] = false;

	for (int i = 0; i < m_MapList.Count(); i++)
	{
		if (!Q_stristr(m_MapList[i], "bba_") && !Q_stristr(m_MapList[i], "bbe_") && !Q_stristr(m_MapList[i], "bbd_"))
			bMapTypes[0] = true;
		else if (Q_stristr(m_MapList[i], "bba_"))
			bMapTypes[1] = true;
		else if (Q_stristr(m_MapList[i], "bbe_"))
			bMapTypes[2] = true;
		else if (Q_stristr(m_MapList[i], "bbd_"))
			bMapTypes[3] = true;
	}

	return !(bMapTypes[0] && bMapTypes[1] && bMapTypes[2] && bMapTypes[3]);
}

const char *CHL2MPRules::GetRandomMapForVoteSys(int mode, bool bRandom)
{
	CUtlVector<char*> pNewMapList;

	for (int i = 0; i < m_MapList.Count(); i++)
	{
		if (!strcmp(szCurrentMap, m_MapList[i]))
			continue;

		bool bAdd = false;
		if (bRandom)
		{
			bAdd = true;
			for (int x = 0; x < 4; x++)
			{
				if (pchMapOptions[x] && pchMapOptions[x][0] && !strcmp(pchMapOptions[x], m_MapList[i]))
					bAdd = false;
			}

			if (bAdd)
				pNewMapList.AddToTail(m_MapList[i]);
		}
		else
		{
			if (!Q_stristr(m_MapList[i], "bba_") && !Q_stristr(m_MapList[i], "bbe_") && !Q_stristr(m_MapList[i], "bbd_") && (mode == MODE_OBJECTIVE))
				bAdd = true;
			else if (Q_stristr(m_MapList[i], "bba_") && (mode == MODE_ARENA))
				bAdd = true;
			else if (Q_stristr(m_MapList[i], "bbe_") && (mode == MODE_ELIMINATION))
				bAdd = true;
			else if (Q_stristr(m_MapList[i], "bbd_") && (mode == MODE_DEATHMATCH))
				bAdd = true;

			if (bAdd)
				pNewMapList.AddToTail(m_MapList[i]);
		}
	}

	if (pNewMapList.Count() <= 0)
		return "";

	int index = random->RandomInt(0, (pNewMapList.Count() - 1));
	char pchOut[MAX_MAP_NAME];
	Q_strncpy(pchOut, pNewMapList[index], MAX_MAP_NAME);
	pNewMapList.RemoveAll();

	const char *mapName = pchOut;
	return mapName;
}
#endif

void CHL2MPRules::Think( void )
{
#ifndef CLIENT_DLL

	CGameRules::Think();

	VoteSystemThink();
	GameModeSharedThink();
	GameEndVoteThink();

#endif
}

void CHL2MPRules::GoToIntermission(int iWinner)
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )
		return;

	g_fGameOver = true;
	m_bShouldShowScores = true;
	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetFloat();
	ResetVote(true);

	IGameEvent * event = gameeventmanager->CreateEvent("round_end");
	if (event)
	{
		event->SetInt("team", iWinner);
		gameeventmanager->FireEvent(event);
	}

	KeyValues *data = new KeyValues("data");
	data->SetInt("winner", iWinner);
	data->SetBool("timeRanOut", (GetTimeLeft() <= 0.0f));

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		pPlayer->AddFlag(FL_FROZEN | FL_GODMODE);

		if (pPlayer->IsBot())
			continue;

		GetRewardFromRoundWin(pPlayer, iWinner, true);
		pPlayer->ShowViewPortPanel(PANEL_ENDSCORE, true, data);

		// Tell our clients that they should do a 'last' save of their stats if we're allowed to save and if the client has loaded his stats. (so we don't overwrite his current stats)
		GameBaseShared()->GetAchievementManager()->SaveGlobalStats(pPlayer);

		pPlayer->HandleLocalProfile(true);
	}

	data->deleteThis();
	GameBaseShared()->OnGameOver(GetTimeLeft(), iWinner);
#endif
}

bool CHL2MPRules::CheckGameOver()
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			m_bShouldShowScores = false;
			ChangeLevel(); // intermission is over			
		}

		return true;
	}
#endif

	return false;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	CWeaponHL2MPBase *pHL2Weapon = dynamic_cast< CWeaponHL2MPBase*>( pWeapon );
	if ( pHL2Weapon )
		return pHL2Weapon->GetOriginalSpawnOrigin();
#endif

	return pWeapon->GetAbsOrigin();
}

#ifndef CLIENT_DLL
//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

//=========================================================
// What angles should this item use to respawn?
//=========================================================
QAngle CHL2MPRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHL2MPRules::FlItemRespawnTime( CItem *pItem )
{
	return 1;
}

#endif

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHL2MPRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
	{
		return GR_WEAPON_RESPAWN_NO;
	}
#endif

	return GR_WEAPON_RESPAWN_YES;
}

bool CHL2MPRules::CanUseSkills(void)
{
	if ((GetCurrentGamemode() == MODE_DEATHMATCH) || (GetCurrentGamemode() == MODE_ELIMINATION))
		return false;

	return true;
}

bool CHL2MPRules::IsFastPacedGameplay(void)
{
	if ((GetCurrentGamemode() == MODE_DEATHMATCH) || (GetCurrentGamemode() == MODE_ELIMINATION))
		return true;

	return false;
}

bool CHL2MPRules::CanUseGameAnnouncer(void)
{
	if (GetCurrentGamemode() == MODE_DEATHMATCH)
		return true;

	return false;
}

bool CHL2MPRules::IsPowerupsAllowed(void)
{
	if (GetCurrentGamemode() == MODE_DEATHMATCH)
		return true;

	return false;
}

bool CHL2MPRules::CanPlayersRespawnIndividually(void)
{
	if ((GetCurrentGamemode() == MODE_ELIMINATION) || !IsTeamplay())
		return true;

#ifndef CLIENT_DLL
		if (GameBaseServer()->IsStoryMode())
			return true;
#endif

	return false;
}

bool CHL2MPRules::ShouldHideHUDDuringRoundWait(void)
{
	if (GetCurrentGamemode() == MODE_ARENA)
		return false;

	return true;
}

bool CHL2MPRules::ShouldDrawHeadLabels()
{
	bool bRet = BaseClass::ShouldDrawHeadLabels();

	if (!IsTeamplay())
		return false;

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CHL2MPRules::ClientDisconnected(edict_t *pClient)
{
#ifndef CLIENT_DLL

	CHL2MP_Player *pPlayer = (CHL2MP_Player *)CBaseEntity::Instance(pClient);
	if (pPlayer)
	{
		if (pPlayer->m_ullCachedSteamID)
			m_uDisconnectedClients.AddToTail(pPlayer->m_ullCachedSteamID);

		GameAnnouncer->RemoveItemsForPlayer(pPlayer->entindex());

		if (pPlayer->GetTeamNumber() == TEAM_HUMANS)
			pPlayer->DropAllWeapons();

		pPlayer->RemovePowerups();
		if (engine->IsDedicatedServer())
			pPlayer->HandleLocalProfile(true);

		GameBaseShared()->GetAchievementManager()->SaveGlobalStats(pPlayer);
		GameBaseShared()->RemoveInventoryItem(pPlayer->entindex(), pPlayer->GetAbsOrigin());

		// Remove the player from his team
		if (pPlayer->GetTeam())
			pPlayer->GetTeam()->RemovePlayer(pPlayer);
	}

	BaseClass::ClientDisconnected(pClient);

#endif
}

//=========================================================
// Deathnotice for players and npcs.
//=========================================================
void CHL2MPRules::DeathNotice(CBaseEntity *pVictim, const CTakeDamageInfo &info)
{
#ifndef CLIENT_DLL
	if (!pVictim || IsGameoverOrScoresVisible())
		return;

	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CHL2MP_Player *pKillerPlayer = ToHL2MPPlayer(pKiller);
	CHL2MP_Player *pVictimPlayer = ToHL2MPPlayer(pVictim);

	if (!pKiller || !pInflictor)
		return;

	// If an NPC suicides we don't want to broadcast it.
	if (pVictim->IsNPC() && (pVictim == pKiller))
		return;

	GameBaseShared()->EntityKilledByPlayer(pKiller, pVictim, pInflictor, info.GetForcedWeaponID());
	GameAnnouncer->DeathNotice(info, pVictim, pKiller);

	int iLastHitGroup = HITGROUP_GENERIC;
	if (pVictim->MyCombatCharacterPointer())
		iLastHitGroup = pVictim->MyCombatCharacterPointer()->LastHitGroup();

	int iKillerID = pKiller->entindex();
	int iVictimID = pVictim->entindex();

	const char *weaponName = "world";

	// Why? Because this npc is about to be deleted right now, which means we can't fetch the npc's name on the client, only in some cases if the npc is not out of the local client's PVS...
	const char *npcKillerName = "";
	const char *npcVictimName = "";

	if (pKiller->IsNPC() && pKiller->MyNPCPointer())
		npcKillerName = pKiller->MyNPCPointer()->GetFriendlyName();

	if (pVictim->IsNPC() && pVictim->MyNPCPointer())
		npcVictimName = pVictim->MyNPCPointer()->GetFriendlyName();

	// If the inflictor is the killer then the active weapon is the 'killer'.
	if (pInflictor == pKiller)
	{
		CBaseCombatWeapon *pActiveWep = GetActiveWeaponFromEntity(pKiller);
		if (pActiveWep)
			weaponName = pActiveWep->GetClassname();
	}
	else
		weaponName = pInflictor->GetClassname(); // Get the inflictors classname...

	// strip the NPC_* or weapon_* from the inflictor's classname
	if (strncmp(weaponName, "weapon_", 7) == 0)
		weaponName += 7;
	else if (strncmp(weaponName, "npc_", 4) == 0)
		weaponName += 4;
	else if (strncmp(weaponName, "func_", 5) == 0)
		weaponName += 5;
	else if (strstr(weaponName, "physics"))
		weaponName = "physics";

	if (FClassnameIs(pKiller, "npc_m1a1"))
		weaponName = "m1a1";
	else if (info.GetDamageType() & DMG_BURN)
		weaponName = "fire";

	int customDMGType = info.GetDamageCustom();
	if (customDMGType != 0)
	{
		if (customDMGType & DMG_BURN)
			weaponName = "fire";
		else if (customDMGType & DMG_SLASH)
			weaponName = "bleed";
		else if (customDMGType & DMG_CLUB)
			weaponName = "kick";
	}

	if (pKiller->IsZombie(true))
		weaponName = "zombie";

	// If the killer is a player then award the player:
	if (pKillerPlayer && (pVictim != pKiller) && m_bRoundStarted)
	{
		int experience = GetExperienceReward(pVictim);
		pKillerPlayer->IncrementTotalScore();
		pKillerPlayer->IncrementRoundScore();
		pKillerPlayer->IncrementFragCount(experience);

		// Add extra zombie credit for zombie skills:
		// Add one more kill to our current kills as a zombie. We then check these kills when you die, if they're over 3 you'll be respawned as a human. :)
		if (pKillerPlayer->IsZombie())
		{
			pKillerPlayer->m_iZombKills++;
			pKillerPlayer->m_BB2Local.m_iZombieCredits += GetZombieCredits(pVictim);
			pKillerPlayer->CheckCanRespawnAsHuman();
		}
		else // Humans gain EXP from killing.
			pKillerPlayer->CanLevelUp(experience, pVictim);

		CTeam *pPlayerTeam = pKillerPlayer->GetTeam();
		if (pPlayerTeam)
			pPlayerTeam->AddScore(1);
	}

	if (pVictimPlayer)
	{
		pVictimPlayer->RemoveGlowEffect();
		if (GameBaseShared()->GetPlayerLoadoutHandler())
			GameBaseShared()->GetPlayerLoadoutHandler()->RemoveDataForPlayer(pVictimPlayer);
	}

	IGameEvent *event = gameeventmanager->CreateEvent("death_notice");
	if (event)
	{
		event->SetString("npcVictim", npcVictimName);
		event->SetString("npcKiller", npcKillerName);
		event->SetString("weapon", weaponName);
		event->SetInt("victimID", iVictimID);
		event->SetInt("killerID", iKillerID);
		event->SetInt("hitgroup", iLastHitGroup);
		event->SetInt("priority", 7);
		gameeventmanager->FireEvent(event);
	}
#endif
}

void CHL2MPRules::ClientSettingsChanged(CBasePlayer *pPlayer)
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer(pPlayer);
	if (pHL2Player == NULL)
		return;

	if (sv_report_client_settings.GetInt() == 1)
		UTIL_LogPrintf("\"%s\" cl_cmdrate = \"%s\"\n", pHL2Player->GetPlayerName(), engine->GetClientConVarValue(pHL2Player->entindex(), "cl_cmdrate"));

	BaseClass::ClientSettingsChanged(pPlayer);
#endif
}

int CHL2MPRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// In survival we will have teamplay OFF which means we have to force zombie players to be friends:
	if (pPlayer && pTarget)
	{
		if ((pPlayer->IsZombie()) && (pTarget->IsZombie()))
			return GR_TEAMMATE;
	}

	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if (pPlayer->GetTeamNumber() == pTarget->GetTeamNumber())
		return GR_TEAMMATE;
#endif

	return GR_NOTTEAMMATE;
}

const char *CHL2MPRules::GetGameDescription( void )
{ 
	return "BrainBread 2";
} 

bool CHL2MPRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPRules::Precache( void )
{
}

bool CHL2MPRules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	if (collisionGroup0 > collisionGroup1) // swap so that lowest is always first
		V_swap(collisionGroup0, collisionGroup1);

	// Nothing collides with spawning zombos!
	if ((collisionGroup0 == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING) || (collisionGroup1 == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING))
		return false;

	// NPC & Players don't collide with weps. And neither with reality phasers.
	if ((collisionGroup0 == COLLISION_GROUP_WEAPON) || (collisionGroup1 == COLLISION_GROUP_WEAPON) || (collisionGroup0 == COLLISION_GROUP_PLAYER_REALITY_PHASE) || (collisionGroup1 == COLLISION_GROUP_PLAYER_REALITY_PHASE))
	{
		if ((collisionGroup0 >= COLLISION_GROUP_NPC_ACTOR) || (collisionGroup1 >= COLLISION_GROUP_NPC_ACTOR)) // Any NPCs + PLRS will not collide with weps.
			return false;

		if (
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ||
			collisionGroup0 == COLLISION_GROUP_NPC
			)
			return false;
	}

	if ((collisionGroup0 == COLLISION_GROUP_WEAPON) && (collisionGroup1 == COLLISION_GROUP_WEAPON))
		return false;

	if ((collisionGroup0 == COLLISION_GROUP_PLAYER_REALITY_PHASE) && (collisionGroup1 == COLLISION_GROUP_PLAYER_REALITY_PHASE))
		return false;

	// Military NPCS
	if ((collisionGroup0 == COLLISION_GROUP_NPC_MILITARY) || (collisionGroup1 == COLLISION_GROUP_NPC_MILITARY))
	{
		if (
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup1 == COLLISION_GROUP_PLAYER_REALITY_PHASE
			)
			return false;
	}

	// Zombie NPCS
	if (collisionGroup0 == COLLISION_GROUP_PLAYER_ZOMBIE && collisionGroup1 == COLLISION_GROUP_NPC_ZOMBIE)
		return false;

	if (collisionGroup0 == COLLISION_GROUP_PLAYER_ZOMBIE && collisionGroup1 == COLLISION_GROUP_NPC_ZOMBIE_BOSS)
		return false;

	if (collisionGroup0 == COLLISION_GROUP_NPC_ZOMBIE && collisionGroup1 == COLLISION_GROUP_NPC_ZOMBIE_BOSS)
		return false;

	// Zombo Bosses won't be blocked by human NPCs.
	if (collisionGroup0 == COLLISION_GROUP_NPC_ZOMBIE_BOSS && ((collisionGroup1 == COLLISION_GROUP_NPC_MERCENARY) || (collisionGroup1 == COLLISION_GROUP_NPC_MILITARY)))
		return false;

	// Zombie Players
	if (collisionGroup0 == COLLISION_GROUP_PLAYER_ZOMBIE && collisionGroup1 == COLLISION_GROUP_PLAYER_ZOMBIE)
		return false;

	// Human Players
	if ((collisionGroup0 == COLLISION_GROUP_PLAYER && collisionGroup1 == COLLISION_GROUP_PLAYER) && IsTeamplay())
		return false;

	// HL2 treats movement and tracing against players the same, so just remap here
	if (collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup0 == COLLISION_GROUP_PLAYER_ZOMBIE || collisionGroup0 == COLLISION_GROUP_PLAYER_REALITY_PHASE)
		collisionGroup0 = COLLISION_GROUP_PLAYER;

	if (collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup1 == COLLISION_GROUP_PLAYER_ZOMBIE || collisionGroup1 == COLLISION_GROUP_PLAYER_REALITY_PHASE)
		collisionGroup1 = COLLISION_GROUP_PLAYER;

	// If collisionGroup0 is not a player then NPC_ACTOR behaves just like an NPC.
	if (collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER)
		collisionGroup1 = COLLISION_GROUP_NPC;

	if (collisionGroup1 == HL2COLLISION_GROUP_CROW)
	{
		if (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_NPC ||
			collisionGroup0 == COLLISION_GROUP_NPC_ZOMBIE ||
			collisionGroup0 == COLLISION_GROUP_NPC_ZOMBIE_BOSS ||
			collisionGroup0 == COLLISION_GROUP_NPC_ZOMBIE_SPAWNING ||
			collisionGroup0 == COLLISION_GROUP_NPC_MILITARY ||
			collisionGroup0 == COLLISION_GROUP_NPC_MERCENARY ||
			collisionGroup0 == HL2COLLISION_GROUP_CROW)
			return false;
	}

	// gunships don't collide with other gunships
	if (collisionGroup0 == HL2COLLISION_GROUP_GUNSHIP && collisionGroup1 == HL2COLLISION_GROUP_GUNSHIP)
		return false;

	// players don't collide against NPC Actors.
	// I could've done this up where I check if collisionGroup0 is NOT a player but I decided to just
	// do what the other checks are doing in this function for consistency sake.
	if (collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER)
		return false;

	// In cases where NPCs are playing a script which causes them to interpenetrate while riding on another entity,
	// such as a train or elevator, you need to disable collisions between the actors so the mover can move them.
	if (collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED)
		return false;

	return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);
}

bool CHL2MPRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
#ifndef CLIENT_DLL
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;

	if (GameBaseShared()->ClientCommand(args))
		return true;

	CHL2MP_Player *pPlayer = (CHL2MP_Player *) pEdict;
	if ( pPlayer->ClientCommand( args ) )
		return true;

	if (pPlayer && (args.ArgC() == 2) && m_bEndMapVotingEnabled && FStrEq(args[0], "player_vote_endmap_choice"))
	{
		int playerIndex = (pPlayer->entindex() - 1);
		if (playerIndex >= 0 && playerIndex < MAX_PLAYERS)
		{
			m_iEndVotePlayerChoices[playerIndex] = atoi(args[1]);
			RecalculateEndMapVotes();
		}
		return true;
	}
#endif

	return false;
}

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

#ifdef BB2_AI
		def.AddAmmoType("Sniper", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(2500, 9500), 7, 12);
		def.AddAmmoType("Rifle", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(1200, 5000));
		def.AddAmmoType("Minigun", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(4000, 12500), 8, 14);
		def.AddAmmoType("SMG", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(1500, 5500));
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(850, 2400));
		def.AddAmmoType("Magnum", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(800, 5000));
		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, BULLET_IMPULSE(800, 2000), 9, 15);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, 0.0f, 10, 15);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, 0.0f, 7, 12);
		def.AddAmmoType("Propane", DMG_BURN, TRACER_NONE, 0.0f, 12, 16);
		def.AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0.0f);
		def.AddAmmoType("Cannister", DMG_BURN, TRACER_NONE, 0.0f);

		// func_tank related.
		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(200, 1225), 6, 10);
#endif //BB2_AI
	}

	return &def;
}

#ifdef CLIENT_DLL

#else

// Handler for the "bot" command.
void Bot_h()
{		
	BotPutInServer(false, TEAM_HUMANS);
}

void Bot_z()
{
	BotPutInServer(false, TEAM_DECEASED);
}

ConCommand cc_BotZ("bot_add_zombie", Bot_z, "Add a zombie bot.", FCVAR_CHEAT);
ConCommand cc_BotH("bot_add_human", Bot_h, "Add a human bot.", FCVAR_CHEAT);

#endif

#ifndef CLIENT_DLL

void CHL2MPRules::RestartGame()
{
	g_bFirstTimeSpawnedNPC = false;
	m_uDisconnectedClients.Purge();

	CleanUpMap();

	// now respawn all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );
		if (!pPlayer || !pPlayer->HasLoadedStats())
			continue;

		pPlayer->RemoveAllItems();

		// Everyone will become a human on game restart.
		if (GetCurrentGamemode() != MODE_ELIMINATION)
			pPlayer->HandleCommand_JoinTeam(TEAM_HUMANS, true);
		else
		{
			int iWantedTeam = pPlayer->GetSelectedTeam();
			int iTeam = GetNewTeam(iWantedTeam, true);
			pPlayer->SetSelectedTeam(iTeam);
			pPlayer->HandleCommand_JoinTeam(iTeam, true);

			if (iWantedTeam != iTeam)
				GameBaseServer()->SendToolTip("#TOOLTIP_TEAM_SWAPPED", GAME_TIP_DEFAULT, pPlayer->entindex());
		}

		pPlayer->Reset();
		pPlayer->SetRespawnTime(0.0f);
		pPlayer->ForceRespawn();
	}

	CTeam *pHumans = GetGlobalTeam(TEAM_HUMANS);
	if (pHumans)
		pHumans->ResetTeamPerks();

	CTeam *pZombies = GetGlobalTeam(TEAM_DECEASED);
	if (pZombies)
		pZombies->ResetTeamPerks();

	// Respawn entities (glass, doors, etc..)
	m_flIntermissionEndTime = 0;

	IGameEvent * event = gameeventmanager->CreateEvent( "round_start" );
	if ( event )
	{
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted
		gameeventmanager->FireEvent( event );
	}

	GameBaseShared()->RemoveInventoryItems();
}

void CHL2MPRules::CleanUpMap()
{
	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase*>(pCur);
		// Weapons with owners don't want to be removed..
		if ( pWeapon )
		{
			if ( !pWeapon->GetPlayerOwner() )
			{
				UTIL_Remove( pCur );
			}
		}
		// remove entities that has to be restored on roundrestart (breakables etc)
		else if ( !FindInList( s_PreserveEnts, pCur->GetClassname() ) )
		{
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	// Cancel all queued events, in case a func_bomb_target fired some delayed outputs that
	// could kill respawning CTs
	g_EventQueue.Clear();

	// Now reload the map entities.
	class CHL2MPMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( !FindInList( s_PreserveEnts, pClassname ) )
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
					m_iIterator = g_MapEntityRefs.Next( m_iIterator );

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CHL2MPMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CHL2MPRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == TRUE )
	{
		const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
		if ( chatLocation && *chatLocation )
		{
			pszFormat = "HL2MP_Chat_Team_Loc";
		}
		else
		{
			pszFormat = "HL2MP_Chat_Team";
		}
	}
	// everyone
	else
	{
		pszFormat = "HL2MP_Chat_All";	
	}

	return pszFormat;
}

#ifdef BB2_AI
void CHL2MPRules::InitDefaultAIRelationships( void )
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i = 0; i < NUM_AI_CLASSES; i++)
	{
		for (j = 0; j < NUM_AI_CLASSES; j++)
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship((Class_T)i, (Class_T)j, D_NU, 0);
		}
	}

	// ------------------------------------------------------------
	//	> CLASS_BULLSEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MILITARY_VEHICLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_ZOMB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ZOMBIE_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_ZOMB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MILITARY_VEHICLE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER_INFECTED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_BULLSEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_CONSCRIPT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MILITARY_VEHICLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_FLARE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MILITARY_VEHICLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER_ZOMB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ZOMBIE_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MILITARY_VEHICLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER_INFECTED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MILITARY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_MISSILE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MILITARY_VEHICLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER_ZOMB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ZOMBIE_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MILITARY_VEHICLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_ZOMB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ZOMBIE_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MILITARY_VEHICLE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BULLSEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MILITARY_VEHICLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER_ZOMB, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER_INFECTED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ZOMBIE_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE_BOSS
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_MILITARY_VEHICLE, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_PLAYER, D_HT, 2);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_PLAYER_ZOMB, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_PLAYER_INFECTED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_COMBINE, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_CONSCRIPT, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_MILITARY, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_ZOMBIE_BOSS, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE_BOSS, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ZOMB
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_MILITARY_VEHICLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_PLAYER_ZOMB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_ZOMBIE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_ZOMBIE_BOSS, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ZOMB, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY_VEHICLE
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_MILITARY_VEHICLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_PLAYER_INFECTED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_VEHICLE, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_INFECTED
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_MILITARY_VEHICLE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_INFECTED, CLASS_EARTH_FAUNA, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	//
	// Hates pretty much everything equally except other earth fauna.
	// This will make the critter choose the nearest thing as its enemy.
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_NONE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER_INFECTED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_FLARE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MISSILE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER_ZOMB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ZOMBIE_BOSS, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MILITARY_VEHICLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_EARTH_FAUNA, D_NU, 0);
}
#endif // BB2_AI

void CHL2MPRules::AddBreakableDoor(CBaseEntity *pEntity)
{
	if (m_hBreakableDoors.Find(pEntity) == -1)
	{
		m_hBreakableDoors.AddToTail(pEntity);
	}
}

void CHL2MPRules::RemoveBreakableDoor(CBaseEntity *pEntity)
{
	if (m_hBreakableDoors.Find(pEntity) != -1)
	{
		m_hBreakableDoors.FindAndRemove(pEntity);
	}
}

bool CHL2MPRules::IsBreakableDoor(CBaseEntity *pEntity)
{
	bool bIsBreakableDoor = (m_hBreakableDoors.Find(pEntity) != -1);
	if (bIsBreakableDoor)
	{
		CBasePropDoor *pDoor = dynamic_cast<CBasePropDoor*> (pEntity);
		if (pDoor && (pDoor->m_iHealth > 0) && (pDoor->IsDoorClosed() || pDoor->IsDoorLocked()))
			return true;
	}

	return false;
}

CBaseEntity *CHL2MPRules::GetNearbyBreakableDoorEntity(CBaseEntity *pChecker)
{
	if (pChecker && m_hBreakableDoors.Count())
	{
		const Vector &vecPos = pChecker->GetAbsOrigin();
		for (int i = 0; i < m_hBreakableDoors.Count(); i++)
		{
			CBaseEntity *pObstruction = m_hBreakableDoors[i].Get();
			if (!pObstruction)
				continue;

			CBasePropDoor *pDoor = dynamic_cast<CBasePropDoor*> (pObstruction);
			if (!pDoor)
				continue;

			if (pDoor->m_iHealth <= 0 ||
				!(pDoor->IsDoorClosed() || pDoor->IsDoorClosing() || pDoor->IsDoorLocked()))
				continue;

			const Vector &vecDoor = pDoor->GetLocalOrigin();
			float dist = vecPos.AsVector2D().DistTo(vecDoor.AsVector2D());
			float zDist = (vecDoor.z - vecPos.z);
			float collisionHeightDoor = abs(pDoor->WorldAlignMaxs().z) + abs(pDoor->WorldAlignMins().z);

			if ((dist > 120.0f) || (zDist <= -10.0f) || (zDist >= ((collisionHeightDoor / 2.0f) + 10.0f)))
				continue;

			return pObstruction;
		}
	}

	return NULL;
}

#endif 

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Should props emit DLIGHT when on fire?
//-----------------------------------------------------------------------------
bool CHL2MPRules::ShouldBurningPropsEmitLight()
{
	return false;
}

#endif
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Game Manager - UTILS, Misc & Good features for controlling your map.
//
//========================================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Server.h"
#include "GameEventListener.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGameManager : public CLogicalEntity, public CGameEventListener
{
public:
	DECLARE_CLASS(CGameManager, CLogicalEntity);
	DECLARE_DATADESC();

	CGameManager();
	void Spawn();

private:
	void FireGameEvent(IGameEvent *event);

	void InputEndGame(inputdata_t &inputData);
	void InputEndRound(inputdata_t &inputData);

	COutputEvent m_OnEndGame;
	COutputEvent m_OnEndRound;
	COutputEvent m_OnStartGame;
	COutputEvent m_OnStartGameClassic;
	COutputEvent m_OnFailedRound;
	COutputEvent m_OnGameOver;

	COutputEvent m_OnNewRound[10];

	int m_iRoundsToPlay;
	int m_iRoundsPlayed;
	int m_iTriesPerRound;
	int m_iCurrentTries;
};

BEGIN_DATADESC(CGameManager)
DEFINE_INPUTFUNC(FIELD_INTEGER, "EndGame", InputEndGame),
DEFINE_INPUTFUNC(FIELD_INTEGER, "EndRound", InputEndRound),
DEFINE_KEYFIELD(m_iRoundsToPlay, FIELD_INTEGER, "Rounds"),
DEFINE_KEYFIELD(m_iTriesPerRound, FIELD_INTEGER, "Tries"),
DEFINE_OUTPUT(m_OnEndGame, "OnEndGame"),
DEFINE_OUTPUT(m_OnEndRound, "OnEndRound"),
DEFINE_OUTPUT(m_OnStartGame, "OnStartGame"),
DEFINE_OUTPUT(m_OnStartGameClassic, "OnStartGameClassic"),
DEFINE_OUTPUT(m_OnFailedRound, "OnRoundFail"),
DEFINE_OUTPUT(m_OnGameOver, "OnGameOver"),
DEFINE_OUTPUT(m_OnNewRound[0], "OnRound1"),
DEFINE_OUTPUT(m_OnNewRound[1], "OnRound2"),
DEFINE_OUTPUT(m_OnNewRound[2], "OnRound3"),
DEFINE_OUTPUT(m_OnNewRound[3], "OnRound4"),
DEFINE_OUTPUT(m_OnNewRound[4], "OnRound5"),
DEFINE_OUTPUT(m_OnNewRound[5], "OnRound6"),
DEFINE_OUTPUT(m_OnNewRound[6], "OnRound7"),
DEFINE_OUTPUT(m_OnNewRound[7], "OnRound8"),
DEFINE_OUTPUT(m_OnNewRound[8], "OnRound9"),
DEFINE_OUTPUT(m_OnNewRound[9], "OnRound10"),
DEFINE_FIELD(m_iRoundsPlayed, FIELD_INTEGER),
DEFINE_FIELD(m_iCurrentTries, FIELD_INTEGER),
END_DATADESC()

LINK_ENTITY_TO_CLASS(game_manager, CGameManager)

CGameManager::CGameManager()
{
	m_iRoundsToPlay = 0;
	m_iRoundsPlayed = 0;
	m_iTriesPerRound = 1;
	m_iCurrentTries = 0;

	ListenForGameEvent("round_started");
	ListenForGameEvent("round_end");
}

void CGameManager::Spawn()
{
	BaseClass::Spawn();

	if (m_iTriesPerRound < 0)
		m_iTriesPerRound = 0;
}

void CGameManager::InputEndGame(inputdata_t &inputData)
{
	int iWinner = (inputData.value.Int() >= 1) ? TEAM_DECEASED : TEAM_HUMANS;
	HL2MPRules()->GoToIntermission(iWinner);
	m_OnEndGame.FireOutput(this, this);
}

void CGameManager::InputEndRound(inputdata_t &inputData)
{
	m_OnEndRound.FireOutput(this, this);
	int iWinner = inputData.value.Int();

	if ((m_iRoundsToPlay > 0) && (iWinner <= 0))
	{
		m_iRoundsPlayed++;
		if (m_iRoundsPlayed >= m_iRoundsToPlay)
		{
			HL2MPRules()->GoToIntermission();
			return;
		}
	}

	HL2MPRules()->EndRound((iWinner >= 1) ? true : false);
}

void CGameManager::FireGameEvent(IGameEvent *event)
{
	const char * type = event->GetName();

	if (!strcmp(type, "round_started"))
	{
		// If Pure Classic Mode is on we'll allow the mapper to use this output to make some changes if necessary.
		if (GameBaseServer()->IsClassicMode())
			m_OnStartGameClassic.FireOutput(this, this);

		m_OnStartGame.FireOutput(this, this);

		if (m_iRoundsToPlay > 0)
		{
			if (m_iRoundsPlayed < m_iRoundsToPlay)
				m_OnNewRound[m_iRoundsPlayed].FireOutput(this, this);
		}
	}
	else if (!strcmp(type, "round_end"))
	{
		int team = event->GetInt("team", TEAM_DECEASED);
		if (team == TEAM_DECEASED) // We failed...
		{
			m_iCurrentTries++;
			if (m_iCurrentTries > m_iTriesPerRound)
			{
				m_iCurrentTries = 0;
				m_iRoundsPlayed = 0;
				m_OnFailedRound.FireOutput(this, this);
			}
		}
		else
		{
			m_iCurrentTries = 0;
		}

		m_OnGameOver.FireOutput(this, this);
	}
}
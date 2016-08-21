//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "mp_shareddefs.h"
#include "basemultiplayerplayer.h"

// Minimum interval between rate-limited commands that players can run.
#define COMMAND_MAX_RATE  0.3

CBaseMultiplayerPlayer::CBaseMultiplayerPlayer()
{
	m_iCurrentConcept = MP_CONCEPT_NONE;
	m_flConnectionTime = gpGlobals->curtime;
}

CBaseMultiplayerPlayer::~CBaseMultiplayerPlayer()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Expresser *CBaseMultiplayerPlayer::CreateExpresser(void)
{
	m_pExpresser = new CMultiplayer_Expresser(this);
	if (!m_pExpresser)
		return NULL;

	m_pExpresser->Connect(this);
	return m_pExpresser;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::PostConstructor(const char *szClassname)
{
	BaseClass::PostConstructor(szClassname);
	CreateExpresser();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet)
{
	BaseClass::ModifyOrAppendCriteria(criteriaSet);

	ModifyOrAppendPlayerCriteria(criteriaSet);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::SpeakIfAllowed(AIConcept_t concept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
{
	if (!IsAlive())
		return false;

	//if ( IsAllowedToSpeak( concept, bRespondingToPlayer ) )
	return Speak(concept, modifiers, pszOutResponseChosen, bufsize, filter);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CBaseMultiplayerPlayer::GetResponseSystem()
{
	return BaseClass::GetResponseSystem();
	// NOTE: This is where you would hook your custom responses.
	//	return <*>GameRules()->m_ResponseRules[iIndex].m_ResponseSystems[m_iCurrentConcept];
}

//-----------------------------------------------------------------------------
// Purpose: Doesn't actually speak the concept. Just finds a response in the system. You then have to play it yourself.
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::SpeakConcept( AI_Response &response, int iConcept )
{
	// Save the current concept.
	m_iCurrentConcept = iConcept;
	return SpeakFindResponse( response, g_pszMPConcepts[iConcept] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::SpeakConceptIfAllowed(int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
{
	// Save the current concept.
	m_iCurrentConcept = iConcept;
	return SpeakIfAllowed(g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::CanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
	// can always hear the console unless we're ignoring all chat
	if (!pPlayer)
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if (m_iIgnoreGlobalChat == CHAT_IGNORE_ALL)
		return false;

	// check if we're ignoring all but teammates
	if (m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship(this, pPlayer) != GR_TEAMMATE)
		return false;

	// can't hear dead players if we're alive
	if (pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand( const CCommand &args )
{
	return ShouldRunRateLimitedCommand( args[0] );
}

bool CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand( const char *pszCommand )
{
	const char *pcmd = pszCommand;

	int i = m_RateLimitLastCommandTimes.Find(pcmd);
	if (i == m_RateLimitLastCommandTimes.InvalidIndex())
	{
		m_RateLimitLastCommandTimes.Insert(pcmd, gpGlobals->curtime);
		return true;
	}
	else if ((gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < COMMAND_MAX_RATE)
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::ClientCommand(const CCommand &args)
{
	const char *pcmd = args[0];

	if (FStrEq(pcmd, "ignoremsg"))
	{
		if (ShouldRunRateLimitedCommand(args))
		{
			m_iIgnoreGlobalChat = (m_iIgnoreGlobalChat + 1) % 3;
			switch (m_iIgnoreGlobalChat)
			{
			case CHAT_IGNORE_NONE:
				ClientPrint(this, HUD_PRINTTALK, "#Accept_All_Messages");
				break;
			case CHAT_IGNORE_ALL:
				ClientPrint(this, HUD_PRINTTALK, "#Ignore_Broadcast_Messages");
				break;
			case CHAT_IGNORE_TEAM:
				ClientPrint(this, HUD_PRINTTALK, "#Ignore_Broadcast_Team_Messages");
				break;
			default:
				break;
			}
		}
		return true;
	}

	return BaseClass::ClientCommand(args);
}

bool CBaseMultiplayerPlayer::ShouldShowVoiceSubtitleToEnemy(void)
{
	return false;
}

void CBaseMultiplayerPlayer::Spawn(void)
{
	BaseClass::Spawn();
}
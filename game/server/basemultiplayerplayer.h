//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#ifndef BASEMULTIPLAYERPLAYER_H
#define BASEMULTIPLAYERPLAYER_H
#pragma once

#include "player.h"
#include "ai_speech.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseMultiplayerPlayer : public CAI_ExpresserHost < CBasePlayer >
{
	DECLARE_CLASS(CBaseMultiplayerPlayer, CAI_ExpresserHost<CBasePlayer>);

public:

	CBaseMultiplayerPlayer();
	~CBaseMultiplayerPlayer();

	virtual void		Spawn(void);

	virtual void		PostConstructor(const char *szClassname);
	virtual void		ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet);

	virtual bool			SpeakIfAllowed(AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL);
	virtual IResponseSystem *GetResponseSystem();
	bool					SpeakConcept( AI_Response& response, int iConcept );
	virtual bool			SpeakConceptIfAllowed(int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL);

	virtual bool		CanHearAndReadChatFrom(CBasePlayer *pPlayer);
	virtual bool		CanSpeak(void) { return true; }

	virtual void		Precache(void)
	{
		PrecacheParticleSystem("achieved");
		BaseClass::Precache();
	}

	virtual bool		ClientCommand(const CCommand &args);

	virtual bool		CanSpeakVoiceCommand(void) { return true; }
	virtual bool		ShouldShowVoiceSubtitleToEnemy(void);
	virtual void		NoteSpokeVoiceCommand(const char *pszScenePlayed) {}

	virtual void OnAchievementEarned(int iAchievement) {}

	enum
	{
		CHAT_IGNORE_NONE = 0,
		CHAT_IGNORE_ALL,
		CHAT_IGNORE_TEAM,
	};

	int m_iIgnoreGlobalChat;

	//---------------------------------
	// Speech support
	virtual CAI_Expresser *GetExpresser() { return m_pExpresser; }
	virtual CMultiplayer_Expresser *GetMultiplayerExpresser() { return m_pExpresser; }

	float GetConnectionTime(void) { return m_flConnectionTime; }

	// Command rate limiting.
	bool ShouldRunRateLimitedCommand(const CCommand &args);
	bool ShouldRunRateLimitedCommand( const char *pszCommand );

protected:
	virtual CAI_Expresser *CreateExpresser(void);

	int		m_iCurrentConcept;
private:
	//---------------------------------
	CMultiplayer_Expresser		*m_pExpresser;

	float m_flConnectionTime;

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float, int>	m_RateLimitLastCommandTimes;
};

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CBaseMultiplayerPlayer *ToBaseMultiplayerPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;
#if _DEBUG
	return dynamic_cast<CBaseMultiplayerPlayer *>( pEntity );
#else
	return static_cast<CBaseMultiplayerPlayer *>(pEntity);
#endif
}

#endif	// BASEMULTIPLAYERPLAYER_H
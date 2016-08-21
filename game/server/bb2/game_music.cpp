//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Allows mappers to override FMOD to crossfade in their own specific soundtracks. (will loop until another game music entity overrides it or if you stop it! / round restart)
//
//========================================================================================//

#include "cbase.h"
#include "game_music.h"
#include "baseentity.h"
#include "baseentity_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CGameMusic)
DEFINE_KEYFIELD(szSoundFile, FIELD_SOUNDNAME, "FilePath"),
DEFINE_KEYFIELD(m_flSoundDuration, FIELD_FLOAT, "SoundDuration"),
DEFINE_INPUTFUNC(FIELD_VOID, "Play", PlayMusic),
DEFINE_INPUTFUNC(FIELD_VOID, "Stop", StopMusic),
DEFINE_INPUTFUNC(FIELD_STRING, "File", SetFileToPlay),
DEFINE_OUTPUT(m_OnPlay, "OnPlay"),
DEFINE_OUTPUT(m_OnStop, "OnStop"),
DEFINE_OUTPUT(m_OnEnd, "OnEnd"),
DEFINE_THINKFUNC(SoundThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(game_music, CGameMusic);

CGameMusic::CGameMusic()
{
	szSoundFile = NULL_STRING;
	m_bIsPlayingSound = false;
	m_flSoundDuration = 0.0f;
}

void CGameMusic::Spawn()
{
	BaseClass::Spawn();

	if (szSoundFile == NULL_STRING)
		Warning("Missing sound file for game_music entity!\nIndex: %i\nName %s\n", entindex(), STRING(GetEntityName()));
}

void CGameMusic::PlayMusic(inputdata_t &inputdata)
{
	if (!m_bIsPlayingSound)
		Play();
	else
		Warning("'%s' is already playing!\n", STRING(GetEntityName()));
}

void CGameMusic::StopMusic(inputdata_t &inputdata)
{
	if (m_bIsPlayingSound)
		Stop();
	else
		Warning("'%s' is not playing any sound!\n", STRING(GetEntityName()));
}

void CGameMusic::SetFileToPlay(inputdata_t &inputdata)
{
	szSoundFile = inputdata.value.StringID();
}

void CGameMusic::Play(void)
{
	if (m_bIsPlayingSound)
		return;

	// If we play a sound then make sure we stop all the other music ent's or else their state will never be reset (it will never reset m_bIsPlayingSound).
	CGameMusic *pOther = dynamic_cast<CGameMusic*> (gEntList.FindEntityByClassname(NULL, this->GetClassname()));
	while (pOther)
	{
		if (pOther != this)
			pOther->Stop();

		pOther = dynamic_cast<CGameMusic*> (gEntList.FindEntityByClassname(pOther, this->GetClassname()));
	}

	m_bIsPlayingSound = true;
	m_OnPlay.FireOutput(this, this);

	IGameEvent *event = gameeventmanager->CreateEvent("game_music");
	if (event)
	{
		event->SetString("file", STRING(szSoundFile));
		event->SetBool("stop", false);
		gameeventmanager->FireEvent(event);
	}

	// Non-Looping? If so we stop the sound when the duration is over.
	if (m_flSoundDuration > 0.0f)
	{
		SetThink(&CGameMusic::SoundThink);
		SetNextThink(gpGlobals->curtime + m_flSoundDuration);
	}
}

void CGameMusic::Stop(void)
{
	if (!m_bIsPlayingSound)
		return;

	m_bIsPlayingSound = false;
	m_OnStop.FireOutput(this, this);

	IGameEvent *event = gameeventmanager->CreateEvent("game_music");
	if (event)
	{
		event->SetString("file", STRING(szSoundFile));
		event->SetBool("stop", true);
		gameeventmanager->FireEvent(event);
	}

	SetThink(NULL);
}

void CGameMusic::SoundThink(void)
{
	m_OnEnd.FireOutput(this, this);
	Stop();
}
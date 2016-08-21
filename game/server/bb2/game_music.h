//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Allows mappers to override FMOD to crossfade in their own specific soundtracks. (will loop until another game music entity overrides it or if you stop it! / round restart)
//
//========================================================================================//

#ifndef GAME_MUSIC_H
#define GAME_MUSIC_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "baseentity_shared.h"

class CGameMusic : public CLogicalEntity
{
public:

	DECLARE_CLASS(CGameMusic, CLogicalEntity);
	DECLARE_DATADESC();

	CGameMusic();

	void Spawn();

	void Play(void);
	void Stop(void);
	void SoundThink(void);

	bool IsPlaying(void) { return m_bIsPlayingSound; }
	const char *GetSoundFile(void) { return STRING(szSoundFile); }

private:

	COutputEvent m_OnPlay;
	COutputEvent m_OnStop;
	COutputEvent m_OnEnd;

	void PlayMusic(inputdata_t &inputdata);
	void StopMusic(inputdata_t &inputdata);
	void SetFileToPlay(inputdata_t &inputdata);

	bool m_bIsPlayingSound;
	float m_flSoundDuration;

	string_t szSoundFile;
};

inline CGameMusic *ToGameMusic(CBaseEntity *pEnt)
{
	if (!pEnt)
		return NULL;

	return dynamic_cast<CGameMusic*>(pEnt);
}

#endif // GAME_MUSIC_H
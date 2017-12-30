//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: FMOD - Sound System. Allows full fading in and out. (Fading won't work if the game is paused)
// Auto-Searches for all .mp3 and .wav files in the soundtrack folder under sound/ and adds them to a list to be played randomly in-game. Use TransitAmbientSound to fade out an active sound and fade in a new one.
// Notice: Looping is set by game_music @server.
//
//========================================================================================//

#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"

class CFMODManager
{
public:
	CFMODManager();
	~CFMODManager();

	void InitFMOD();
	void ExitFMOD();
	void FadeThink();

	bool PlayAmbientSound(const char *szSoundPath, bool bLoop = false);
	void PlayLoadingMusic(const char *szSoundPath);
	void StopAmbientSound(void);
	bool TransitionAmbientSound(const char *szSoundPath, bool bLoop = false);
	void SetSoundVolume(float vol) { m_flSoundVolume = vol; }

private:
	ConVar *m_pVarMusicVolume;
	ConVar *m_pVarMuteSoundFocus;

	const char *GetFullPathToSound(const char *pathToFileFromModFolder);
	const char *GetCurrentSoundName(void);

	char szActiveSound[MAX_WEAPON_STRING];
	char szTransitSound[MAX_WEAPON_STRING];

	bool bShouldPlayInSequence;
	bool bShouldMuteFMOD;
	bool bIsFMODMuted;

	bool m_bFadeIn;
	bool m_bFadeOut;
	bool m_bIsPlayingSound;
	bool m_bShouldLoop;

	float m_flVolume; // Main Volume (100% vol)
	float m_flSoundVolume; // Percent of master vol above.
	float m_flLerp; // The value to update per millisec.

	float m_flFadeOutTime; // When will we start to fade out?

	float m_flSoundLength; // Length of the active sound decremented by the frametime = start time (dynamic).
	float m_flTimeConstant; // Length of the active sound. 
};

extern CFMODManager *FMODManager();

#endif //FMOD_MANAGER_H
//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: FMOD - Sound System. Allows full fading in and out. (Fading won't work if the game is paused)
// Auto-Searches for all .mp3 and .wav files in the soundtrack folder under sound/ and adds them to a list to be played randomly in-game. Use TransitAmbientSound to fade out an active sound and fade in a new one.
// Notice: Looping is set by game_music @server.
//
//========================================================================================//

#include "cbase.h"
#include "fmod_manager.h"
#include "filesystem.h"
#include "GameBase_Client.h"
#include "music_system.h"

using namespace FMOD;

#define FMOD_FADE_TIME 1.5f

System			*pSystem;
Sound			*pSound;
SoundGroup		*pSoundGroup;
Channel			*pChannel;
ChannelGroup	*pChannelGroup;
FMOD_RESULT		result;

CFMODManager gFMODMng;
CFMODManager* FMODManager()
{
	return &gFMODMng;
}

CFMODManager::CFMODManager()
{
	m_flVolume = 0.0f;
	m_flSoundVolume = 1.0f;
	m_bFadeIn = false;
	m_bFadeOut = false;
	m_bIsPlayingSound = false;
	m_bShouldLoop = false;
	m_flTime = m_flFadeOutTime = 0.0f;
}

CFMODManager::~CFMODManager()
{
}

void CFMODManager::InitFMOD(void)
{
	result = System_Create(&pSystem); // Create the main system object.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System creation failed!\n");
	else
		DevMsg("FMOD system successfully created.\n");

	result = pSystem->init(100, FMOD_INIT_NORMAL, 0);   // Initialize FMOD system.
	if (result != FMOD_OK)
		Warning("FMOD ERROR: Failed to initialize properly!\n");
	else
		DevMsg("FMOD initialized successfully.\n");

	m_pVarMusicVolume = cvar->FindVar("snd_musicvolume");
	m_pVarMuteSoundFocus = cvar->FindVar("snd_mute_losefocus");
}

void CFMODManager::ExitFMOD(void)
{
	result = pSystem->release();
	if (result != FMOD_OK)
		Warning("FMOD ERROR: System did not terminate properly!\n");
	else
		DevMsg("FMOD system terminated successfully.\n");
}

// Returns the full path of a specified sound file in the /sounds folder
const char *CFMODManager::GetFullPathToSound(const char *pathToFileFromModFolder)
{
	char fullPath[512], relativePath[256];
	Q_snprintf(relativePath, 256, "sound/%s", pathToFileFromModFolder);
	filesystem->RelativePathToFullPath(relativePath, "MOD", fullPath, sizeof(fullPath));
	int iLength = strlen(fullPath);

	// convert backwards slashes to forward slashes
	for (int i = 0; i < iLength; i++)
	{
		if (fullPath[i] == '\\')
			fullPath[i] = '/';
	}

	const char *results = fullPath;
	return results;
}

// Returns the current sound playing.
const char *CFMODManager::GetCurrentSoundName(void)
{
	return szActiveSound;
}

// Handles all fade-related sound stuffs.
// Called every frame.
void CFMODManager::FadeThink(void)
{
	// Do we wish to play the in game soundtracks? If not, play on demand. (forced from main menu (transit only))
	bool bShouldPlayInSequence = GameBaseClient->IsInGame();
	bool bShouldMuteFMOD = (engine->IsPaused() || (m_pVarMuteSoundFocus && m_pVarMuteSoundFocus->GetBool() && !engine->IsActiveApp()));
	bool bIsMuted = false;

	if (pChannel)
	{
		pChannel->getMute(&bIsMuted);
		if (bIsMuted != bShouldMuteFMOD)
			pChannel->setMute(bShouldMuteFMOD);
	}

	// Fading out uses this volume as 100%...
	if (m_pVarMusicVolume)
		m_flVolume = m_pVarMusicVolume->GetFloat();
	else
		m_flVolume = 1.0f;

	m_flVolume *= m_flSoundVolume;

	if (m_bFadeIn || m_bFadeOut)
	{
		float timeFraction = clamp(((engine->Time() - m_flTime) / FMOD_FADE_TIME), 0.0f, 1.0f);
		if (timeFraction >= 1.0f)
		{
			if (m_bFadeIn)
			{
				m_bFadeIn = false;
				m_bIsPlayingSound = true; // wait for 'final' countdown.
			}

			if (m_bFadeOut)
			{
				m_bFadeOut = false;
				Q_strncpy(szActiveSound, "", MAX_WEAPON_STRING); // clear active sound.

				// find the next sound, if we have a transit sound, prio that one.
				if (szTransitSound && szTransitSound[0])
					PlayAmbientSound(szTransitSound, m_bShouldLoop);
				else if (bShouldPlayInSequence) // If there's no transit sound and we're in game (no bg map) then continue playing the soundtrack!				
					GetMusicSystem->RunAmbientSoundTrack();
			}
		}
		else
			pChannel->setVolume((m_bFadeIn ? timeFraction : (1.0f - timeFraction)) * m_flVolume);
	}

	// Update our volume and count down to the point where we need to fade out.
	if (m_bIsPlayingSound)
	{
		pChannel->setVolume(m_flVolume);
		if (engine->Time() >= m_flFadeOutTime)
			StopAmbientSound(true);
	}

	if (pSystem)
		pSystem->update();
}

void CFMODManager::PlayLoadingMusic(const char *szSoundPath)
{
	m_bFadeIn = false;
	m_bFadeOut = false;
	m_bIsPlayingSound = false;

	Q_strncpy(szActiveSound, "", MAX_WEAPON_STRING); // clear active sound.
	Q_strncpy(szTransitSound, "", MAX_WEAPON_STRING); // clear transit sound.

	result = pSystem->createStream(GetFullPathToSound(szSoundPath), FMOD_DEFAULT, 0, &pSound);
	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", szSoundPath, result);
		return;
	}

	result = pSystem->playSound(FMOD_CHANNEL_REUSE, pSound, false, &pChannel);
	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", szSoundPath, result);
		return;
	}

	pChannel->setVolume((m_flVolume * m_flSoundVolume));
}

// Fades in and sets all needed params for playing a sound through FMOD.
bool CFMODManager::PlayAmbientSound(const char *szSoundPath, bool bLoop)
{
	// We don't want to play the same sound or any other before it is done!
	if (m_bFadeOut || m_bFadeIn || m_bIsPlayingSound || !strcmp(szSoundPath, szActiveSound))
		return false;

	result = pSystem->createStream(GetFullPathToSound(szSoundPath), FMOD_DEFAULT, 0, &pSound);
	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", szSoundPath, result);
		return false;
	}

	result = pSystem->playSound(FMOD_CHANNEL_REUSE, pSound, false, &pChannel);
	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", szSoundPath, result);
		return false;
	}

	pChannel->setVolume(0.0f); // we fade in no matter what, we will now go ahead and play a new file (.wav).

	// Get the length of the sound and set the timer to countdown.
	uint lengthOfSound;
	pSound->getLength(&lengthOfSound, FMOD_TIMEUNIT_MS);
	float flLengthOfSound = ((float)lengthOfSound);
	flLengthOfSound /= 1000.0f;
	flLengthOfSound = floor(flLengthOfSound);

	m_flTime = engine->Time();
	m_flFadeOutTime = m_flTime + flLengthOfSound - FMOD_FADE_TIME; // We fade out X sec before the sound is over.	
	Q_strncpy(szActiveSound, szSoundPath, MAX_WEAPON_STRING);

	// WE only loop transit sounds.
	if (!bLoop)
		Q_strncpy(szTransitSound, "", MAX_WEAPON_STRING); // clear transit sound.

	m_bFadeIn = true;
	return true;
}

// Abruptly stops playing current ambient sound.
void CFMODManager::StopAmbientSound(bool force)
{
	// If the active sound is NULL then don't care.
	if (!force && (!szActiveSound || !szActiveSound[0]))
		return;

	m_bIsPlayingSound = false;
	m_bFadeIn = false;
	m_bFadeOut = true;
	m_flTime = engine->Time();
}

// We store a transit char which will be looked up right before the sound is fully faded out. (swapping) 
// This allow us to override a current playing song without interferring too much.
bool CFMODManager::TransitionAmbientSound(const char *szSoundPath, bool bLoop)
{
	m_bShouldLoop = bLoop;

	// If the active sound is NULL, allow us to play right away.
	if (!szActiveSound || !szActiveSound[0])
	{
		PlayAmbientSound(szSoundPath, bLoop);
		return true;
	}

	// We don't want to play the same sound before it is done!
	if (!strcmp(szSoundPath, szActiveSound) || !strcmp(szSoundPath, szTransitSound))
		return false;

	Q_strncpy(szTransitSound, szSoundPath, MAX_WEAPON_STRING);

	if (m_bFadeOut)
		return false;

	StopAmbientSound();
	return true;
}
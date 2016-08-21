//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
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
	bShouldPlayInSequence = false;
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
	char fullPath[512];

	Q_snprintf(fullPath, 512, "%s/sound/%s", engine->GetGameDirectory(), pathToFileFromModFolder);
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

// Handles all fade-related sound stuffs
// Called every frame when the client is in-game
void CFMODManager::FadeThink(void)
{
	// Do we wish to play the in game soundtracks? If not, play on demand. (forced from main menu (transit only))
	bShouldPlayInSequence = GameBaseClient->IsInGame();

	// Fading out uses this volume as 100%...
	if (m_pVarMusicVolume)
		m_flVolume = m_pVarMusicVolume->GetFloat();
	else
		m_flVolume = 1.0f;

	m_flVolume *= m_flSoundVolume;

	if (m_bFadeIn || m_bFadeOut)
	{
		float flMilli = MAX(0.0f, 1000.0f - m_flLerp);
		float flSec = flMilli * 0.001f;
		if ((flSec >= 1.0))
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
				if (strlen(szTransitSound) > 0)
				{
					PlayAmbientSound(szTransitSound, m_bShouldLoop);
				}
				else if (bShouldPlayInSequence) // If there's no transit sound and we're in game (no bg map) then continue playing the soundtrack!
				{
					GetMusicSystem->RunAmbientSoundTrack();
				}
			}
		}
		else
		{
			float flFrac = SimpleSpline(flSec / 1.0);
			pChannel->setVolume((m_bFadeIn ? flFrac : (1.0f - flFrac)) * m_flVolume);
		}
	}

	// Update our volume and count down to the point where we need to fade out.
	if (m_bIsPlayingSound)
	{
		pChannel->setVolume(m_flVolume);

		float flMilli = MAX(0.0f, m_flTimeConstant - m_flSoundLength);
		float flSec = flMilli * 0.001f;
		if ((flSec >= m_flFadeOutTime))
		{
			m_bFadeOut = true;
			m_bIsPlayingSound = false;
			m_flLerp = 1000.0f;
		}

		//Msg("Time Left : %i sec\nTime Elapsed : %i sec\n", (int)(m_flSoundLength / 1000), (int)flSec);
	}

	// Update our timer which we use to interpolate the fade in and out *delay* from 0 to 100% volume.
	float frame_msec = 1000.0f * gpGlobals->frametime;

	if (m_flLerp > 0)
	{
		m_flLerp -= frame_msec;
		if (m_flLerp < 0)
			m_flLerp = 0;
	}

	if (m_flSoundLength > 0)
	{
		m_flSoundLength -= frame_msec;
		if (m_flSoundLength < 0)
			m_flSoundLength = 0;
	}
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
	if (!strcmp(szSoundPath, szActiveSound) || m_bFadeOut || m_bFadeIn || m_bIsPlayingSound)
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

	pChannel->setVolume(0.0); // we fade in no matter what, we will now go ahead and play a new file (.wav).

	// Get the length of the sound and set the timer to countdown.
	uint lengthOfSound;
	pSound->getLength(&lengthOfSound, FMOD_TIMEUNIT_MS);

	m_flLerp = 1000.0f; // Fade Time.
	m_flSoundLength = ((float)lengthOfSound);
	m_flTimeConstant = m_flSoundLength;
	m_flFadeOutTime = ((m_flSoundLength - 1000.0f) / 1000); // We fade out 1 sec before the sound is over.

	Q_strncpy(szActiveSound, szSoundPath, MAX_WEAPON_STRING);

	// WE only loop transit sounds.
	if (!bLoop)
		Q_strncpy(szTransitSound, "", MAX_WEAPON_STRING); // clear transit sound.

	m_bFadeIn = true;

	return true;
}

// Abruptly stops playing current ambient sound.
void CFMODManager::StopAmbientSound(void)
{
	// If the active sound is NULL then don't care.
	if (!szActiveSound || (strlen(szActiveSound) <= 0))
		return;

	m_bIsPlayingSound = false;
	m_bFadeIn = false;
	m_bFadeOut = true;
	m_flLerp = 1000.0f;
}

// We store a transit char which will be looked up right before the sound is fully faded out. (swapping) 
// This allow us to override a current playing song without interferring too much.
bool CFMODManager::TransitionAmbientSound(const char *szSoundPath, bool bLoop)
{
	m_bShouldLoop = bLoop;

	// If the active sound is NULL, allow us to play right away.
	if (!szActiveSound || (strlen(szActiveSound) <= 0))
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
//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Extended implementation of FMOD, allows proper fading in and out transitions between sounds.
// It also allows direct play using PlayLoadingSound to skip fading functions. Fading doesn't work when you're in the main menu and in game when not in a background map because the frametime and curtime is frozen (paused). 
// Unless there's any other way to interpolate fading, such as using the animation controller or a tick signal, I don't think it will be as smooth and at least the anim controller may freeze during pause as well.
// Notice: These fade functions work so much better in multiplayer because you can't pause the game in mp.
// 
//=============================================================================//

#include "cbase.h"
#include "fmod/fmod.hpp"
#include "fmod_ambience.h"
#include "fmod_manager.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CFMODAmbience::CFMODAmbience()
{
	m_pSound = NULL;
	m_pChannel = NULL;
	m_flVolume = 0.0f;
	m_pchSoundFile[0] = 0;
}

CFMODAmbience::~CFMODAmbience()
{
	Destroy();
}

void CFMODAmbience::Restart(void)
{
	if (m_pchSoundFile && m_pchSoundFile[0] && (m_pSound == NULL) && (m_pChannel == NULL))
		PlaySoundInternal();
}

void CFMODAmbience::PlaySound(const char* pSoundPath)
{
	Q_strncpy(m_pchSoundFile, FMODManager()->GetFullPathToSound(pSoundPath), sizeof(m_pchSoundFile));
	PlaySoundInternal();
}

void CFMODAmbience::PlaySoundInternal(void)
{
	FMOD_RESULT result = FMODManager()->GetFMODSystem()->createStream(m_pchSoundFile, FMOD_LOOP_NORMAL | FMOD_2D | FMOD_HARDWARE, 0, &m_pSound);

	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to create stream for sound '%s' ! (ERROR NUMBER: %i)\n", m_pchSoundFile, result);
		return;
	}

	FMOD_CHANNELINDEX index = (m_pChannel == NULL) ? FMOD_CHANNEL_FREE : FMOD_CHANNEL_REUSE;
	result = FMODManager()->GetFMODSystem()->playSound(index, m_pSound, false, &m_pChannel);

	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", m_pchSoundFile, result);
		return;
	}

	m_pChannel->setVolume(0.0f);
}

void CFMODAmbience::StopSound(void)
{
	SetVolume(0.0f);
}

void CFMODAmbience::SetVolume(float volume)
{
	m_flVolume = clamp(volume, 0.0f, 1.0f);
	if (m_pChannel && (m_flVolume <= 0.0f))
		m_pChannel->setVolume(0.0f);
}

void CFMODAmbience::Think(void)
{
	if (m_pChannel == NULL)
		return;

	bool bShouldMute = (engine->IsPaused() || !engine->IsActiveApp());
	bool bIsMuted = false;

	m_pChannel->getMute(&bIsMuted);

	if (bIsMuted != bShouldMute)
		m_pChannel->setMute(bShouldMute);

	m_pChannel->setVolume(bShouldMute ? 0.0f : (m_flVolume * FMODManager()->GetMasterVolume()));
}

void CFMODAmbience::Destroy(void)
{
	SetVolume(0.0f);

	if (m_pSound != NULL)
		m_pSound->release();

	if (m_pChannel != NULL)
		m_pChannel->stop();

	m_pSound = NULL;
	m_pChannel = NULL;
}
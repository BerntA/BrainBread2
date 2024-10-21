//========= Copyright Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Extended implementation of FMOD, allows proper fading in and out transitions between sounds.
// It also allows direct play using PlayLoadingSound to skip fading functions. Fading doesn't work when you're in the main menu and in game when not in a background map because the frametime and curtime is frozen (paused). 
// Unless there's any other way to interpolate fading, such as using the animation controller or a tick signal, I don't think it will be as smooth and at least the anim controller may freeze during pause as well.
// Notice: These fade functions work so much better in multiplayer because you can't pause the game in mp.
// 
//=============================================================================//

#ifndef FMOD_AMBIENCE_H
#define FMOD_AMBIENCE_H

#ifdef _WIN32
#pragma once
#endif

namespace FMOD
{
	class Sound;
	class Channel;
}

class CFMODAmbience
{
public:
	CFMODAmbience();
	virtual ~CFMODAmbience();

	void Restart(void);
	void PlaySound(const char* pSoundPath);
	void StopSound(void);
	void SetVolume(float volume);
	void Think(void);
	void Destroy(void);

private:
	void PlaySoundInternal(void);

private:
	FMOD::Sound* m_pSound;
	FMOD::Channel* m_pChannel;
	float m_flVolume;
	char m_pchSoundFile[512];
};

#endif //FMOD_AMBIENCE_H
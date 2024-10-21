//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_SOUNDSCAPE_H
#define C_SOUNDSCAPE_H

#ifdef _WIN32
#pragma once
#endif

class IGameSystem;
struct audioparams_t;

extern IGameSystem* ClientSoundscapeSystem();

// call when audio params may have changed
extern void Soundscape_Update(audioparams_t& audio);

// Called on round restart, otherwise the soundscape system thinks all its
// sounds are still playing when they're not.
void Soundscape_OnStopAllSounds();

extern void Soundscape_StopFMODSounds();
extern void Soundscape_RestartFMODSounds();

#endif // C_SOUNDSCAPE_H
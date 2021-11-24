//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Music System: Utilizes FMOD, this class handles playing sounds using fmod (mostly music), this class also parses sound data which can hold:
// loading music queues, random ambient tracks, specific sound events which can be triggered by game_music, you can still use raw file paths in game_music but I recommend using event stuff, it will lookup a file with the same map name.
//
//========================================================================================//

#ifndef MUSIC_SYSTEM_H
#define MUSIC_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod_manager.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "GameEventListener.h"

#define MAX_MUSIC_PATH 64
extern ConVar bb2_music_system_shuffle;

struct musicDataItem_t
{
	char pchMapLink[MAX_MUSIC_PATH];
	char pchFilePath[MAX_MUSIC_PATH];
	float flVolume;
};

struct musicEventDataItem_t
{
	char pchEventName[MAX_MUSIC_PATH];
	char pchFilePath[MAX_MUSIC_PATH];
	float flVolume;
	bool bShouldLoop;
};

class CMusicData;
class CMusicData
{
public:

	CMusicData()
	{
		m_iActiveIndex = 0;
		Cleanup();
	}

	~CMusicData()
	{
		Cleanup();
	}

	void Cleanup(void)
	{
		m_pAmbientTracks.Purge();
		m_pMusicEvents.Purge();
	}

	const musicEventDataItem_t *GetMusicEvent(const char *eventName) const
	{
		if (eventName && eventName[0])
		{
			for (int i = 0; i < m_pMusicEvents.Count(); i++)
			{
				if (!strcmp(eventName, m_pMusicEvents[i].pchEventName))
					return &m_pMusicEvents[i];
			}
		}
		return NULL;
	}

	void AddAmbientTrack(const char *soundPath, float volume)
	{
		musicDataItem_t item;
		item.pchMapLink[0] = 0;
		Q_strncpy(item.pchFilePath, soundPath, MAX_MUSIC_PATH);
		item.flVolume = volume;
		m_pAmbientTracks.AddToTail(item);
	}

	void AddMusicEvent(const char *eventName, const char *soundPath, float volume, bool shouldLoop)
	{
		musicEventDataItem_t item;
		Q_strncpy(item.pchEventName, eventName, MAX_MUSIC_PATH);
		Q_strncpy(item.pchFilePath, soundPath, MAX_MUSIC_PATH);
		item.flVolume = volume;
		item.bShouldLoop = shouldLoop;
		m_pMusicEvents.AddToTail(item);
	}

	bool PlayRandomMusic(bool reset = false)
	{
		if (bb2_music_system_shuffle.GetBool() && (m_pAmbientTracks.Count() > 1))
			m_iActiveIndex = GetRandIdxExcluded(m_pAmbientTracks.Count(), m_iActiveIndex);
		else if (reset || (m_iActiveIndex >= m_pAmbientTracks.Count()))
			m_iActiveIndex = 0;

		if (m_pAmbientTracks.Count())
		{
			FMODManager()->SetSoundVolume(m_pAmbientTracks[m_iActiveIndex].flVolume);
			FMODManager()->TransitionAmbientSound(m_pAmbientTracks[m_iActiveIndex].pchFilePath);
			if (!bb2_music_system_shuffle.GetBool())
				m_iActiveIndex++;
		}

		return (m_pAmbientTracks.Count() > 0);
	}

	bool RunMusicEvent(const char *eventName)
	{
		const musicEventDataItem_t *pEvent = GetMusicEvent(eventName);
		if (pEvent)
		{
			FMODManager()->SetSoundVolume(pEvent->flVolume);
			FMODManager()->TransitionAmbientSound(pEvent->pchFilePath, pEvent->bShouldLoop);
		}
		return (pEvent != NULL);
	}

	int m_iActiveIndex;
	CUtlVector<musicDataItem_t> m_pAmbientTracks;
	CUtlVector<musicEventDataItem_t> m_pMusicEvents;
};

class CMusicSystem;
class CMusicSystem : public CGameEventListener
{
public:
	CMusicSystem();
	virtual ~CMusicSystem();

	void InitializeSystem(const char *activeEventName);
	void Cleanup(void);
	void CleanupMapMusic(void);
	void ParseMusicData(void);
	bool ParseMusicData(CMusicData *pMusicData, KeyValues *pkvData);
	bool ParseLoadingMusic(const char *pMapName, KeyValues *pkvData);
	void ParseMapMusicData(KeyValues *pkvData = NULL);
	void RunAmbientSoundTrack(bool bReset = false);
	void RunLoadingSoundTrack(const char *pMapName);
	void RunSoundEvent(const char *eventName);

	CMusicData *GetMusicDataForMap(void) { return m_pMapMusicData; }
	CMusicData *GetDefaultMusicData(void) { return m_pDefaultData; }

private:

	CMusicData *m_pDefaultData;
	CMusicData *m_pMapMusicData;
	CUtlVector<musicDataItem_t> m_pLoadingTracks;

protected:
	void FireGameEvent(IGameEvent *event);
};

extern CMusicSystem *GetMusicSystem;

#endif // MUSIC_SYSTEM_H
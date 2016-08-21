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

extern ConVar bb2_music_system_shuffle;

struct musicDataItem_t
{
	char pchFilePath[80];
	float flVolume;
};

struct musicEventDataItem_t
{
	char pchEventName[64];
	char pchFilePath[80];
	float flVolume;
	bool bShouldLoop;
};

enum musicTypes_t
{
	MUSIC_TYPE_LOADING_TRACK = 0,
	MUSIC_TYPE_AMBIENT_TRACK,
	MUSIC_TYPE_EVENT,
};

class CMusicData;
class CMusicData
{
public:

	CMusicData()
	{
		m_iActiveIndex = 0;
		m_pLoadingTracks.Purge();
		m_pAmbientTracks.Purge();
		m_pMusicEvents.Purge();
	}

	~CMusicData()
	{
		m_pLoadingTracks.Purge();
		m_pAmbientTracks.Purge();
		m_pMusicEvents.Purge();
	}

	bool HasLoadingMusic(void)
	{
		return m_pLoadingTracks.Count();
	}

	bool HasAmbientMusic(void)
	{
		return m_pAmbientTracks.Count();
	}

	int GetIndexForEvent(const char *eventName)
	{
		for (int i = 0; i < m_pMusicEvents.Count(); i++)
		{
			if (!strcmp(eventName, m_pMusicEvents[i].pchEventName))
				return i;
		}

		return -1;
	}

	bool DoesSoundEventExist(const char *eventName)
	{
		return (GetIndexForEvent(eventName) != -1);
	}

	void AddLoadingTrack(const char *soundPath, float volume)
	{
		musicDataItem_t item;
		Q_strncpy(item.pchFilePath, soundPath, 80);
		item.flVolume = volume;
		m_pLoadingTracks.AddToTail(item);
	}

	void AddAmbientTrack(const char *soundPath, float volume)
	{
		musicDataItem_t item;
		Q_strncpy(item.pchFilePath, soundPath, 80);
		item.flVolume = volume;
		m_pAmbientTracks.AddToTail(item);
	}

	void AddMusicEvent(const char *eventName, const char *soundPath, float volume, bool shouldLoop)
	{
		musicEventDataItem_t item;
		Q_strncpy(item.pchEventName, eventName, 64);
		Q_strncpy(item.pchFilePath, soundPath, 80);
		item.flVolume = volume;
		item.bShouldLoop = shouldLoop;
		m_pMusicEvents.AddToTail(item);
	}

	bool PlayRandomMusic(int type, bool reset = false)
	{
		bool bRet = false;

		if (bb2_music_system_shuffle.GetBool() && (m_pAmbientTracks.Count() > 1) && (type == MUSIC_TYPE_AMBIENT_TRACK))
		{
			int iCurrentIndexInAmbientList = m_iActiveIndex;
			while (iCurrentIndexInAmbientList == m_iActiveIndex)
			{
				iCurrentIndexInAmbientList = random->RandomInt(0, (m_pAmbientTracks.Count() - 1));
			}
			m_iActiveIndex = iCurrentIndexInAmbientList;
		}
		else
		{
			if (reset)
				m_iActiveIndex = 0;

			if (m_iActiveIndex >= m_pAmbientTracks.Count())
				m_iActiveIndex = 0;
		}

		switch (type)
		{
		case MUSIC_TYPE_LOADING_TRACK:
			if (HasLoadingMusic())
			{
				int index = random->RandomInt(0, (m_pLoadingTracks.Count() - 1));
				FMODManager()->SetSoundVolume(m_pLoadingTracks[index].flVolume);
				FMODManager()->PlayLoadingMusic(m_pLoadingTracks[index].pchFilePath);

				bRet = true;
			}
			break;

		case MUSIC_TYPE_AMBIENT_TRACK:
			if (HasAmbientMusic())
			{
				FMODManager()->SetSoundVolume(m_pAmbientTracks[m_iActiveIndex].flVolume);
				FMODManager()->TransitionAmbientSound(m_pAmbientTracks[m_iActiveIndex].pchFilePath);
				if (!bb2_music_system_shuffle.GetBool())
					m_iActiveIndex++;

				bRet = true;
			}
			break;
		}

		return bRet;
	}

	bool RunMusicEvent(const char *eventName)
	{
		int index = GetIndexForEvent(eventName);
		if (index != -1)
		{
			FMODManager()->SetSoundVolume(m_pMusicEvents[index].flVolume);
			FMODManager()->TransitionAmbientSound(m_pMusicEvents[index].pchFilePath, m_pMusicEvents[index].bShouldLoop);
			return true;
		}

		return false;
	}

	int m_iActiveIndex;

	CUtlVector<musicDataItem_t> m_pLoadingTracks;
	CUtlVector<musicDataItem_t> m_pAmbientTracks;
	CUtlVector<musicEventDataItem_t> m_pMusicEvents;
};

class CMusicSystem;
class CMusicSystem : public CGameEventListener
{
public:
	CMusicSystem();
	~CMusicSystem();

	void InitializeSystem(const char *activeEventName);
	void Cleanup(void);
	void CleanupMapMusic(void);
	void ParseMusicData(void);
	void ParseMapMusicData(KeyValues *pkvData = NULL);
	void RunAmbientSoundTrack(bool bReset = false);
	void RunLoadingSoundTrack(void);
	void RunSoundEvent(const char *eventName);

	CMusicData *GetMusicDataForMap(void) { return m_pMapMusicData; }
	CMusicData *GetDefaultMusicData(void) { return m_pDefaultData; }

private:

	CMusicData *m_pDefaultData;
	CMusicData *m_pMapMusicData;

protected:
	void FireGameEvent(IGameEvent *event);
};

extern CMusicSystem *GetMusicSystem;

#endif // MUSIC_SYSTEM_H
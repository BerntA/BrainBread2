//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Music System: Utilizes FMOD, this class handles playing sounds using fmod (mostly music), this class also parses sound data which can hold:
// loading music queues, random ambient tracks, specific sound events which can be triggered by game_music, you can still use raw file paths in game_music but I recommend using event stuff, it will lookup a file with the same map name.
//
//========================================================================================//

#include "cbase.h"
#include "music_system.h"
#include "filesystem.h"
#include "GameBase_Client.h"
#include "hud_macros.h"

ConVar bb2_music_system_shuffle("bb2_music_system_shuffle", "0", FCVAR_ARCHIVE, "Enable shuffling for in-game ambient music.", true, 0.0f, true, 1.0f);

static void __MsgFunc_PlayerInit(bf_read &msg)
{
	char soundEvent[MAX_WEAPON_STRING];
	msg.ReadString(soundEvent, MAX_WEAPON_STRING);
	GetMusicSystem->InitializeSystem(soundEvent);
}

CMusicSystem *GetMusicSystem = NULL;
CMusicSystem::CMusicSystem()
{
	GetMusicSystem = this;
	m_pDefaultData = NULL;
	m_pMapMusicData = NULL;

	ListenForGameEvent("round_start");
	ListenForGameEvent("game_music");
	HOOK_MESSAGE(PlayerInit);
}

CMusicSystem::~CMusicSystem()
{
	Cleanup();
	GetMusicSystem = NULL;
}

void CMusicSystem::InitializeSystem(const char *activeEventName)
{
	if (activeEventName && activeEventName[0])
		RunSoundEvent(activeEventName);
	else
		RunAmbientSoundTrack(true);
}

void CMusicSystem::Cleanup(void)
{
	if (m_pMapMusicData != NULL)
	{
		delete m_pMapMusicData;
		m_pMapMusicData = NULL;
	}

	if (m_pDefaultData != NULL)
	{
		delete m_pDefaultData;
		m_pDefaultData = NULL;
	}

	m_pLoadingTracks.Purge();
}

void CMusicSystem::CleanupMapMusic(void)
{
	if (m_pMapMusicData != NULL)
	{
		delete m_pMapMusicData;
		m_pMapMusicData = NULL;
	}
}

void CMusicSystem::ParseMusicData(void)
{
	Cleanup();
	KeyValues *pkvDefault = new KeyValues("DefaultData");
	if (pkvDefault->LoadFromFile(filesystem, "data/game/game_music.txt", "MOD"))
	{
		m_pDefaultData = new CMusicData();
		ParseMusicData(m_pDefaultData, pkvDefault);
		ParseLoadingMusic("DEFAULT", pkvDefault);
	}
	pkvDefault->deleteThis();
}

bool CMusicSystem::ParseMusicData(CMusicData *pMusicData, KeyValues *pkvData)
{
	if ((pMusicData == NULL) || (pkvData == NULL))
		return false;

	KeyValues *pkvSub = NULL;

	pkvSub = pkvData->FindKey("AmbientMusic");
	if (pkvSub)
	{
		for (KeyValues *sub = pkvSub->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			pMusicData->AddAmbientTrack(sub->GetName(), sub->GetFloat());
	}

	pkvSub = pkvData->FindKey("Events");
	if (pkvSub)
	{
		for (KeyValues *sub = pkvSub->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			pMusicData->AddMusicEvent(sub->GetName(), sub->GetString("path"), sub->GetFloat("volume"), (sub->GetInt("loop") >= 1));
	}

	return true;
}

bool CMusicSystem::ParseLoadingMusic(const char *pMapName, KeyValues *pkvData)
{
	if (pkvData == NULL)
		return false;

	KeyValues *pkvSub = pkvData->FindKey("LoadingMusic");
	if (pkvSub)
	{
		for (KeyValues *sub = pkvSub->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			musicDataItem_t item;
			Q_strncpy(item.pchMapLink, pMapName, MAX_MUSIC_PATH);
			Q_strncpy(item.pchFilePath, sub->GetName(), MAX_MUSIC_PATH);
			item.flVolume = sub->GetFloat();
			m_pLoadingTracks.AddToTail(item);
		}
	}

	return (pkvSub != NULL);
}

void CMusicSystem::ParseMapMusicData(KeyValues *pkvData)
{
	if (pkvData)
	{
		m_pMapMusicData = new CMusicData();
		ParseMusicData(m_pMapMusicData, pkvData);
	}
}

void CMusicSystem::RunAmbientSoundTrack(bool bReset)
{
	CMusicData *data = GetMusicDataForMap();
	if (data && data->PlayRandomMusic(bReset))
		return;

	data = GetDefaultMusicData();
	if (data)
		data->PlayRandomMusic(bReset);
}

void CMusicSystem::RunLoadingSoundTrack(const char *pMapName)
{
	const int size = m_pLoadingTracks.Count();
	if (!pMapName || !pMapName[0] || !size)
		return;

	CUtlVector<int> indexes;

	// Look for matching map load snd.
	for (int i = 0; i < size; i++)
	{
		const musicDataItem_t *pItem = &m_pLoadingTracks[i];
		if (!strcmp(pMapName, pItem->pchMapLink))
			indexes.AddToTail(i);
	}

	// Revert to default.
	if (indexes.Count() == 0)
	{
		for (int i = 0; i < size; i++)
		{
			const musicDataItem_t *pItem = &m_pLoadingTracks[i];
			if (!strcmp("DEFAULT", pItem->pchMapLink))
				indexes.AddToTail(i);
		}
	}

	// Still no loading music?
	if (indexes.Count() == 0)
		return;

	int index = indexes[random->RandomInt(0, (indexes.Count() - 1))];
	FMODManager()->SetSoundVolume(m_pLoadingTracks[index].flVolume);
	FMODManager()->PlayLoadingMusic(m_pLoadingTracks[index].pchFilePath);
}

void CMusicSystem::RunSoundEvent(const char *eventName)
{
	FMODManager()->SetSoundVolume(1.0f);

	CMusicData *data = GetMusicDataForMap();
	if (data && data->RunMusicEvent(eventName))
		return;

	data = GetDefaultMusicData();
	if (data && data->RunMusicEvent(eventName))
		return;

	// The event we wanted to play didn't exist, we expect that the 'eventName' is a raw path to some sound file.
	FMODManager()->TransitionAmbientSound(eventName, true);
}

void CMusicSystem::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (!strcmp(type, "round_start"))
		RunAmbientSoundTrack(true);
	else if (!strcmp(type, "game_music"))
	{
		const char *soundEvent = event->GetString("file");
		bool bStop = event->GetBool("stop");
		if (bStop)
			RunAmbientSoundTrack();
		else
			RunSoundEvent(soundEvent);
	}
}
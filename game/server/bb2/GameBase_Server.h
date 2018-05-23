//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Server Handler - Handles kicking, voting, baning, map change, etc...
//
//========================================================================================//

#ifndef GAME_BASE_SERVER_H
#define GAME_BASE_SERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "hl2mp_player.h"
#include "html_data_handler.h"

struct sharedDataItem_t
{
	char szInfo[128];
	int iType;
};

enum sharedDataTypes
{
	DATA_SECTION_DEVELOPER = 0,
	DATA_SECTION_DONATOR,
	DATA_SECTION_TESTER,
	DATA_SECTION_BANNED,
	DATA_SECTION_SERVER_BLACKLIST,
	DATA_SECTION_SERVER_ADMIN,
};

enum profileSystemType
{
	PROFILE_NONE = 0,
	PROFILE_GLOBAL,
	PROFILE_LOCAL,
};

class CGameBaseServer
{
public:
	CGameBaseServer();
	~CGameBaseServer();

	void Init();
	void Release();
	void PostInit();
	void PostLoad(float flDelay) { m_flPostLoadTimer = engine->Time() + flDelay; }

	// Global
	void LoadSharedInfo(void);
	void AddItemToSharedList(const char *str, int type);
	bool FindItemInSharedList(const char *str, int type);
	void LoadServerTags(void);
	void CheckMapData(void);
	void GameAnnouncement(const char *format, const char *arg1, const char *arg2);
	void NewPlayerConnection(CHL2MP_Player *pClient);
	void SendToolTip(const char *message, int type = 0, int index = -1, const char *arg1 = "", const char *arg2 = "", const char *arg3 = "", const char *arg4 = "");
	void SendToolTip(const char *message, const char *keybind, float duration, int type = 0, int index = -1, const char *arg1 = "", const char *arg2 = "", const char *arg3 = "", const char *arg4 = "");
	void SendAchievement(const char *szAchievement, int iReceipentID = 0);
	float GetDamageScaleForEntity(CBaseEntity *pAttacker, CBaseEntity *pVictim, int damageType, int customDamageType);

	bool IsTutorialModeEnabled(void);
	bool IsStoryMode(void);
	bool IsClassicMode(void);
	bool IsUsingDBSystem(void);

	// Plugin Iterator
	void IterateAddonsPath(const char *path);

	// Shared
	// Does this server allow you to store your skills on round end/joining? (global DB)
	int CanStoreSkills();
	bool HasIllegalConVarValues();
	// Think func, updated from hl2mp g-rules.
	void OnUpdate(int iClientsInGame);

	void DoMapChange(const char *map);

	bool IsServerBlacklisted(void) { return bIsServerBlacklisted; }
	void SetServerBlacklisted(bool value) { bIsServerBlacklisted = value; }

private:

	// Other
	bool bIsServerBlacklisted;
	bool bFoundCheats;
	bool bFoundIllegalPlugin;
	bool bAllowStatsForMap;
	float m_flPostLoadTimer;

	CUtlVector<sharedDataItem_t> m_pSharedDataList;

	char szNextMap[MAX_MAP_NAME];
	bool m_bShouldChangeMap;
	float m_flChangeLevelTimeLerp;
	float m_flLastProfileSystemStatusUpdateCheck;

	// Server Blacklist - Get the IP of the server and check it via the LIBCURL data.
	inline const char *GetPublicIP(uint32 unIP) const;
};

extern CGameBaseServer *GameBaseServer();

#endif // GAME_BASE_SERVER_H
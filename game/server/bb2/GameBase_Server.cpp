//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Server Handler - Handles shared data, npc/player hitgroup scaling, map change, etc...
//
//========================================================================================//

#include "cbase.h"
#include "GameBase_Server.h"
#include "filesystem.h"
#include "game_music.h"
#include "GameBase_Shared.h"
#include "inetchannelinfo.h"
#include "ai_basenpc.h"
#include "movevars_shared.h"
#include "world.h"
#include "tier0/icommandline.h"

ConVar bb2_tutorial_mode("bb2_tutorial_mode", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "Force-enable tutorial mode for any map, useful for testing.", true, 0.0f, true, 1.0f);

static CGameBaseServer gServerMode;
CGameBaseServer *GameBaseServer()
{
	return &gServerMode;
}

CGameBaseServer::CGameBaseServer()
{
}

CGameBaseServer::~CGameBaseServer()
{
}

void CGameBaseServer::Init()
{
	m_flPostLoadTimer = 0.0f;
	bIsServerBlacklisted = false;
	bFoundCheats = false;
	bFoundIllegalPlugin = false;
	m_bShouldChangeMap = false;
	bAllowStatsForMap = false;
	szNextMap[0] = 0;

	m_pSharedDataList.Purge();
}

void CGameBaseServer::Release()
{
	bFoundIllegalPlugin = false;
	m_bShouldChangeMap = false;
	bAllowStatsForMap = false;
	szNextMap[0] = 0;
	m_pSharedDataList.Purge();
}

// Handle global stuff here:

void CGameBaseServer::LoadSharedInfo(void)
{
	m_pSharedDataList.Purge();

	KeyValues *pkvAdminData = new KeyValues("AdminList");
	if (pkvAdminData->LoadFromFile(filesystem, "data/server/admins.txt", "MOD"))
	{
		for (KeyValues *sub = pkvAdminData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			AddItemToSharedList(sub->GetString(), DATA_SECTION_SERVER_ADMIN);
	}
	pkvAdminData->deleteThis();
}

void CGameBaseServer::AddItemToSharedList(const char *str, int type)
{
	sharedDataItem_t item;
	Q_strncpy(item.szInfo, str, 128);
	item.iType = type;
	m_pSharedDataList.AddToTail(item);
}

bool CGameBaseServer::FindItemInSharedList(const char *str, int type)
{
	if (!m_pSharedDataList.Count())
		return false;

	for (int i = 0; i < m_pSharedDataList.Count(); i++)
	{
		if ((type == m_pSharedDataList[i].iType) && (!strcmp(str, m_pSharedDataList[i].szInfo)))
			return true;
	}

	return false;
}

void CGameBaseServer::LoadServerTags(void)
{
	int iProfileSavingType = CanStoreSkills();
	char pszTagList[128], pszTagCommand[256], pszParam[32];
	Q_snprintf(pszTagList, 128, "Version %s", GameBaseShared()->GetGameVersion());

	if (iProfileSavingType >= PROFILE_GLOBAL)
	{
		Q_snprintf(pszParam, 32, ", savedata %i", iProfileSavingType);
		Q_strncat(pszTagList, pszParam, sizeof(pszTagList), COPY_ALL_CHARACTERS);
	}

	if (bb2_enable_ban_list.GetBool() && engine->IsDedicatedServer())
		Q_strncat(pszTagList, ", banlist", sizeof(pszTagList), COPY_ALL_CHARACTERS);

	Q_snprintf(pszTagCommand, 256, "sv_tags %s\n", pszTagList);
	engine->ServerCommand(pszTagCommand);
}

void CGameBaseServer::CheckMapData(void)
{
	if (GameBaseShared()->GetSharedMapData())
	{
		gameMapItem_t *currentMapData = GameBaseShared()->GetSharedMapData()->GetMapData(HL2MPRules()->szCurrentMap);
		if (currentMapData)
			bAllowStatsForMap = ((HL2MPRules()->m_ulMapSize > 0) && (HL2MPRules()->m_ulMapSize == currentMapData->ulFileSize) && (currentMapData->iMapVerification >= MAP_VERIFIED_WHITELISTED));
	}
}

// Set current workshop ID regardless of SteamUGC state!
// Use workshop file to find the right ID.
void CGameBaseServer::SetCurrentMapAddon(const char *map)
{
	if (engine->IsDedicatedServer() == false)
	{
		bb2_active_workshop_item.SetValue(0);
		return;
	}

	char pchWorkshopID[80], pchTargetFile[MAX_PATH];
	Q_strncpy(pchWorkshopID, "0", 80);

	CUtlVector<PublishedFileId_t> workshopMaps;
	if (GameBaseShared() && GameBaseShared()->GetServerWorkshopData())
		GameBaseShared()->GetServerWorkshopData()->GetListOfAddons(workshopMaps, true);

	for (int i = 0; i < workshopMaps.Count(); i++)
	{
		Q_snprintf(pchTargetFile, MAX_PATH, "workshop/content/346330/%llu/maps/%s.bsp", workshopMaps[i], map);
		if (filesystem->FileExists(pchTargetFile, "MOD"))
		{
			Q_snprintf(pchWorkshopID, 80, "%llu", workshopMaps[i]);
			break;
		}
	}

	workshopMaps.Purge();

	bb2_active_workshop_item.SetValue(pchWorkshopID);
}

// Send a message to all the clients in-game.
void CGameBaseServer::GameAnnouncement(const char *format, const char *arg1, const char *arg2)
{
	CRecipientFilter filter;
	filter.AddAllPlayers();
	filter.MakeReliable();
	UTIL_ClientPrintFilter(filter, HUD_PRINTTALK, format, arg1, arg2);
}

// When a new player joins we need to see if there's an active sound playing @game_music, if so we play that one or else we just play the default soundtracks.
void CGameBaseServer::NewPlayerConnection(CHL2MP_Player *pClient)
{
	if (!pClient)
		return;

	char szSoundFile[MAX_WEAPON_STRING];
	Q_strncpy(szSoundFile, "", MAX_WEAPON_STRING);

	CGameMusic *pGameMusic = ToGameMusic(gEntList.FindEntityByClassname(NULL, "game_music"));
	while (pGameMusic)
	{
		// Find the first entity playing music:
		if (pGameMusic->IsPlaying())
		{
			Q_strncpy(szSoundFile, pGameMusic->GetSoundFile(), MAX_WEAPON_STRING);
			break;
		}

		pGameMusic = ToGameMusic(gEntList.FindEntityByClassname(pGameMusic, "game_music"));
	}

	CSingleUserRecipientFilter filter(pClient);
	filter.MakeReliable();
	UserMessageBegin(filter, "PlayerInit");
	WRITE_STRING(szSoundFile);
	MessageEnd();

	GameBaseShared()->NewPlayerConnection(false);

	if (engine->IsDedicatedServer())
	{
		char steamID[80];
		Q_snprintf(steamID, 80, "%llu", pClient->GetSteamIDAsUInt64());

		const char *ipAddress = "";
		if (FindItemInSharedList(steamID, DATA_SECTION_DEVELOPER))
			pClient->AddGroupIDFlag(GROUPID_IS_DEVELOPER);

		if (FindItemInSharedList(steamID, DATA_SECTION_DONATOR))
			pClient->AddGroupIDFlag(GROUPID_IS_DONATOR);

		if (FindItemInSharedList(steamID, DATA_SECTION_TESTER))
			pClient->AddGroupIDFlag(GROUPID_IS_TESTER);

		if (FindItemInSharedList(steamID, DATA_SECTION_SERVER_ADMIN))
			pClient->SetAdminStatus(true);

		if (bb2_enable_ban_list.GetBool())
		{
			INetChannelInfo *info = engine->GetPlayerNetInfo(pClient->entindex());
			if (info)
			{
				char pszAddress[128];
				Q_strncpy(pszAddress, info->GetName(), 128);

				int iSize = strlen(pszAddress);
				int indexToStripStart = 0;
				for (int i = 0; i < iSize; i++)
				{
					if (pszAddress[i] == ':')
					{
						indexToStripStart = i;
						break;
					}
				}

				pszAddress[indexToStripStart] = 0;
				ipAddress = pszAddress;
			}

			if (FindItemInSharedList(ipAddress, DATA_SECTION_BANNED) ||
				FindItemInSharedList(steamID, DATA_SECTION_BANNED))
			{
				char kickCmd[128];
				Q_snprintf(kickCmd, 128, "kickid %i You're on the ban list!\n", pClient->GetUserID());
				engine->ServerCommand(kickCmd);
			}
		}
	}
}

float CGameBaseServer::GetDamageScaleForEntity(CBaseEntity *pAttacker, CBaseEntity *pVictim, int damageType, int customDamageType)
{
	if (!pAttacker || !pVictim)
		return 1.0f;

	float flDamageScale = 1.0f;

	// Bernt: Scale the damage depending on which entity we hit.
	const char *pchNPCName = NULL;
	const char*pchWeaponClassname = NULL;
	CBaseCombatWeapon *pActiveWep = NULL;
	if (pAttacker->IsNPC() && pAttacker->MyNPCPointer())
	{
		pchNPCName = pAttacker->MyNPCPointer()->GetFriendlyName();
		pActiveWep = pAttacker->MyNPCPointer()->GetActiveWeapon();
		if (pActiveWep)
			pchWeaponClassname = pActiveWep->GetClassname();
		else if (pAttacker->IsZombie(true))
			pchWeaponClassname = "zombie";
	}
	else if (pAttacker->IsPlayer())
	{
		// NO SCALING FOR KICKING.
		if (customDamageType == DMG_CLUB)
			return 1.0f;

		CBasePlayer *pOwner = ToBasePlayer(pAttacker);
		if (pOwner)
			pActiveWep = pOwner->GetActiveWeapon();

		if (pActiveWep)
		{
			// IF our wep is not a melee wep and our dmg type is not a firearm type, return no scaling, assuming a bash attack or triggered attack of some kind.
			if (!pActiveWep->IsMeleeWeapon() && !(damageType & (DMG_BULLET | DMG_BUCKSHOT | DMG_BLAST | DMG_BURN)))
				return 1.0f;

			pchWeaponClassname = pActiveWep->GetClassname();
		}
	}

	if (pchWeaponClassname != NULL)
	{
		int entityType = -1;

		if (pVictim->IsHumanBoss())
			entityType = DAMAGE_SCALE_TO_NPC_HUMAN_BOSSES;
		else if (pVictim->IsZombieBoss())
			entityType = DAMAGE_SCALE_TO_NPC_ZOMBIE_BOSSES;
		else if (pVictim->IsHuman())
			entityType = DAMAGE_SCALE_TO_PLAYER;
		else if (pVictim->IsHuman(true))
			entityType = DAMAGE_SCALE_TO_NPC_HUMANS;
		else if (pVictim->IsZombie())
			entityType = DAMAGE_SCALE_TO_PLAYER_ZOMBIE;
		else if (pVictim->IsZombie(true))
			entityType = DAMAGE_SCALE_TO_NPC_ZOMBIES;

		if (entityType == -1)
			return flDamageScale;

		if (pchNPCName != NULL)
			flDamageScale = GameBaseShared()->GetNPCData()->GetFirearmDamageScale(pchNPCName, pchWeaponClassname, entityType);
		else
			flDamageScale = GameBaseShared()->GetSharedGameDetails()->GetPlayerFirearmDamageScale(pchWeaponClassname, entityType, pAttacker->GetTeamNumber());
	}

	return flDamageScale;
}

bool CGameBaseServer::IsTutorialModeEnabled(void)
{
	if (engine->IsDedicatedServer())
		return false;

	return ((GetWorldEntity() && GetWorldEntity()->IsTutorialMap()) || bb2_tutorial_mode.GetBool());
}

bool CGameBaseServer::IsStoryMode(void)
{
	return (GetWorldEntity() && GetWorldEntity()->IsStoryMap() && (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE)));
}

bool CGameBaseServer::IsClassicMode(void)
{
	if (IsStoryMode())
		return false;

	return (HL2MPRules() && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE) && bb2_classic_mode_enabled.GetBool());
}

bool CGameBaseServer::IsUsingDBSystem(void)
{
	return engine->IsDedicatedServer();
}

void CGameBaseServer::IterateAddonsPath(const char *path)
{
	char pszAbsPath[256], pszNextPath[256];
	Q_snprintf(pszAbsPath, 256, "%s/*.*", path);

	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx(pszAbsPath, "MOD", &findHandle);
	while (pFilename != NULL)
	{
		if (strlen(pFilename) > 2)
		{
			if (filesystem->FindIsDirectory(findHandle))
			{
				Q_snprintf(pszNextPath, 256, "%s/%s", path, pFilename);
				IterateAddonsPath(pszNextPath);
			}
			else
			{
				char pchFile[128];
				Q_strncpy(pchFile, pFilename, 128);
				Q_strlower(pchFile);

				if (Q_strstr(pchFile, ".so") || Q_strstr(pchFile, ".srv") || Q_strstr(pchFile, ".dylib") || Q_strstr(pchFile, ".dll"))
				{
					bFoundIllegalPlugin = true;
					Warning("The plugin '%s' is not allowed to be used with Global Saving, the global profile system will be disabled until you remove this plugin!\n", pchFile);
				}
			}
		}

		pFilename = filesystem->FindNext(findHandle);
	}

	filesystem->FindClose(findHandle);
}

// Sends a tip to the desired client.
void CGameBaseServer::SendToolTip(const char *message, int type, int index, const char *arg1, const char *arg2, const char *arg3, const char *arg4)
{
	if (!(message && message[0]))
	{
		Warning("Invalid tooltip sent, ignoring!\n");
		return;
	}

	if (index == -1)
	{
		CRecipientFilter filter;
		filter.AddAllPlayers();
		filter.MakeReliable();

		UserMessageBegin(filter, "ToolTip");
		WRITE_SHORT(type);
		WRITE_STRING(message);
		WRITE_STRING(arg1);
		WRITE_STRING(arg2);
		WRITE_STRING(arg3);
		WRITE_STRING(arg4);
		MessageEnd();
	}
	else
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(index);
		if (pPlayer)
		{
			CSingleUserRecipientFilter filter(pPlayer);
			filter.MakeReliable();

			UserMessageBegin(filter, "ToolTip");
			WRITE_SHORT(type);
			WRITE_STRING(message);
			WRITE_STRING(arg1);
			WRITE_STRING(arg2);
			WRITE_STRING(arg3);
			WRITE_STRING(arg4);
			MessageEnd();
		}
	}
}

void CGameBaseServer::SendToolTip(const char *message, const char *keybind, float duration, int type, int index, const char *arg1, const char *arg2, const char *arg3, const char *arg4)
{
	if ((type == 0) && (strlen(keybind) <= 0))
	{
		Warning("Invalid keybind input in game_tip ent!\n");
		return;
	}

	if (index == -1)
	{
		CRecipientFilter filter;
		filter.AddAllPlayers();
		filter.MakeReliable();

		UserMessageBegin(filter, "GameTip");
		WRITE_SHORT(type);
		WRITE_FLOAT(duration);
		WRITE_STRING(message);
		WRITE_STRING(keybind);
		WRITE_STRING(arg1);
		WRITE_STRING(arg2);
		WRITE_STRING(arg3);
		WRITE_STRING(arg4);
		MessageEnd();
	}
	else
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(index);
		if (pPlayer)
		{
			CSingleUserRecipientFilter filter(pPlayer);
			filter.MakeReliable();

			UserMessageBegin(filter, "GameTip");
			WRITE_SHORT(type);
			WRITE_FLOAT(duration);
			WRITE_STRING(message);
			WRITE_STRING(keybind);
			WRITE_STRING(arg1);
			WRITE_STRING(arg2);
			WRITE_STRING(arg3);
			WRITE_STRING(arg4);
			MessageEnd();
		}
	}
}

// Add achievement to achieved for the user (-1 index = all). 
void CGameBaseServer::SendAchievement(const char *szAchievement, int iReceipentID)
{
	// Give Tutorial Finished Achiev:
	if (IsTutorialModeEnabled() && !strcmp(szAchievement, "ACH_TUTORIAL_COMPLETE") && !engine->IsDedicatedServer() && steamapicontext)
	{
		bool bAchieved = false;
		steamapicontext->SteamUserStats()->GetAchievement(szAchievement, &bAchieved);
		if (!bAchieved)
		{
			steamapicontext->SteamUserStats()->SetAchievement(szAchievement);
			steamapicontext->SteamUserStats()->StoreStats();
			steamapicontext->SteamUserStats()->RequestCurrentStats();
		}

		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer)
			continue;

		if ((iReceipentID > 0) && (i != iReceipentID))
			continue;

		GameBaseShared()->GetAchievementManager()->WriteToAchievement(pPlayer, szAchievement, ACHIEVEMENT_TYPE_MAP);
	}
}

// Late init.
void CGameBaseServer::PostInit()
{
	bFoundCheats = false;
	m_bShouldChangeMap = false;
	bAllowStatsForMap = false;
	m_flLastProfileSystemStatusUpdateCheck = 0.0f;

	LoadSharedData();
	CheckMapData();
	LoadServerTags();
}

// Are you allowed to store your skills?
int CGameBaseServer::CanStoreSkills()
{
	// Are we even allowed to use skills?
	if (HL2MPRules() && !HL2MPRules()->CanUseSkills())
		return PROFILE_NONE;

	int profileType = bb2_allow_profile_system.GetInt();

	// We won't allow profile saving or score saving if we're having cheats enabled. (only global)
	// profile_local will allow the connected player to read a keyvalue file from the server's folder which contains the stats for the steam id of the connected player.
	// Global stats only works on dedicated servers!
	if (profileType == PROFILE_GLOBAL)
	{
		// Blacklisted? Using sourcemod or similar? Using cheats? Not hosting whitelisted maps? Exploiting through convars like gravity? etc...
		// Did this server ever have cheats on? If so we'll not allow this server to load any stats until you restart the map with sv_cheats off.
		if (IsServerBlacklisted() || bFoundIllegalPlugin || !bAllowStatsForMap || HasIllegalConVarValues() ||
			bFoundCheats || (gpGlobals->maxClients <= 1) || (sv_cheats && sv_cheats->GetBool()) || !engine->IsDedicatedServer())
			return PROFILE_NONE;
	}

	if (IsTutorialModeEnabled())
		return PROFILE_NONE;

	return profileType;
}

bool CGameBaseServer::HasIllegalConVarValues()
{
	// Check if certain convars have been changed:
	if (sv_gravity.GetFloat() != 600.0f)
		return true;

	return false;
}

// Think func for the server mode:
void CGameBaseServer::OnUpdate(int iClientsInGame)
{
	// Hunt for sv_cheats & sv_lan here: (disables skill saving throughout the game) //
	if (sv_cheats->GetBool())
		bFoundCheats = true;

	if (gpGlobals->curtime > m_flLastProfileSystemStatusUpdateCheck)
	{
		bb2_profile_system_status.SetValue(CanStoreSkills());
		m_flLastProfileSystemStatusUpdateCheck = gpGlobals->curtime + 1.0f;
	}

	// PostLoad - When we run a dedicated server we want to re-load the map data when SteamAPI is up running...
	if ((m_flPostLoadTimer > 0) && (m_flPostLoadTimer < engine->Time()))
	{
		m_flPostLoadTimer = 0.0f;

		if (GameBaseShared()->GetSharedMapData())
		{
			// If we didn't fetch workshop data, but only regular map data via data/maps/*, load tags and check stuff right away.
			if (!GameBaseShared()->GetSharedMapData()->FetchMapData())
			{
				CheckMapData();
				LoadServerTags();
			}
		}

		if (GameBaseShared()->GetServerWorkshopData())
			GameBaseShared()->GetServerWorkshopData()->Initialize();

		uint32 unIP = steamgameserverapicontext->SteamGameServer()->GetPublicIP();
		bIsServerBlacklisted = FindItemInSharedList(GetPublicIP(unIP), DATA_SECTION_SERVER_BLACKLIST);
		if (bIsServerBlacklisted)
			Warning("This IP has been blacklisted, your servers will not be visible for the public nor will you be able to enable global profile saving.\n");

		IterateAddonsPath("addons");
	}

	// Check for map changes:
	if (m_bShouldChangeMap && (gpGlobals->curtime > m_flTimeToChangeLevel))
	{
		m_bShouldChangeMap = false;
		engine->ChangeLevel(szNextMap, NULL);
		return;
	}

	if (GameBaseShared()->GetServerWorkshopData())
		GameBaseShared()->GetServerWorkshopData()->DownloadThink();
}

void CGameBaseServer::DoMapChange(const char *map)
{
	Q_strncpy(szNextMap, map, MAX_MAP_NAME);

	IGameEvent *event = gameeventmanager->CreateEvent("changelevel");
	if (event)
	{
		event->SetString("map", szNextMap);
		gameeventmanager->FireEvent(event);
	}

	m_bShouldChangeMap = true;
	m_flTimeToChangeLevel = gpGlobals->curtime + 1.25f; // Wait X sec until changing lvl.
}

inline const char *CGameBaseServer::GetPublicIP(uint32 unIP) const
{
	static char s[4][64];
	static int nBuf = 0;
	unsigned char *ipByte = (unsigned char *)&unIP;
	Q_snprintf(s[nBuf], sizeof(s[nBuf]), "%u.%u.%u.%u", (int)(ipByte[3]), (int)(ipByte[2]), (int)(ipByte[1]), (int)(ipByte[0]));
	const char *pchRet = s[nBuf];
	++nBuf;
	nBuf %= ((sizeof(s) / sizeof(s[0])));
	return pchRet;
}

CON_COMMAND_F(player_vote_map, "Vote for a map.", FCVAR_HIDDEN)
{
	if (args.ArgC() != 2)
		return;

	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || !HL2MPRules())
		return;

	const char *map = args[1];
	if (!(map && map[0]))
		return;

	HL2MPRules()->CreateMapVote(pClient, map);
}

CON_COMMAND_F(player_vote_kickban, "Vote to kick or ban someone.", FCVAR_HIDDEN)
{
	if (args.ArgC() != 3)
		return;

	int iTarget = atoi(args[1]);
	int iType = atoi(args[2]);

	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient || !HL2MPRules())
		return;

	HL2MPRules()->CreateBanKickVote(pClient, UTIL_PlayerByUserId(iTarget), (iType != 0));
}

CON_COMMAND(admin_kick_id, "Admin Kick Command")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (pClient->IsBot() || !pClient->IsAdminOnServer())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "You are not an admin on this server!");
		return;
	}

	if (args.ArgC() < 3)
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "Please specify the <userID> and the <reason>!");
		return;
	}

	int userID = atoi(args[1]);
	const char *reason = args.ArgS();
	reason += strlen(args[1]); // strip away userID.

	char pchCommand[80];
	Q_snprintf(pchCommand, 80, "kickid %i \"%s\"\n", userID, reason);
	engine->ServerCommand(pchCommand);
}

CON_COMMAND(admin_kick_bots, "Kick all bots from the server.")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (pClient->IsBot() || !pClient->IsAdminOnServer())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "You are not an admin on this server!");
		return;
	}

	char pchServerCMD[80];
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || !pPlayer->IsBot())
			continue;

		Q_snprintf(pchServerCMD, 80, "kickid %i\n", pPlayer->GetUserID());
		engine->ServerCommand(pchServerCMD);
	}
}

CON_COMMAND(admin_ban_id, "Admin Ban Command")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (pClient->IsBot() || !pClient->IsAdminOnServer())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "You are not an admin on this server!");
		return;
	}

	if (args.ArgC() < 4)
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "Please specify the <ban time in minutes> <userID> and the <reason>!");
		return;
	}

	int banTime = atoi(args[1]);
	int userID = atoi(args[2]);
	const char *reason = args.ArgS();
	reason += strlen(args[1]);
	reason += strlen(args[2]);

	char pchCommand[80];

	Q_snprintf(pchCommand, 80, "banid %i %i \"%s\"\n", banTime, userID, reason);
	engine->ServerCommand(pchCommand);
	engine->ServerExecute(); // Add to ban list right away.

	Q_snprintf(pchCommand, 80, "kickid %i \"%s\"\n", userID, reason);
	engine->ServerCommand(pchCommand); // Kick afterwards.

	engine->ServerCommand("writeid;writeip\n"); // Save the new ids already, in case the server crashes b4 level change..
}

CON_COMMAND(admin_changelevel, "Admin Changelevel Command")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (pClient->IsBot() || !pClient->IsAdminOnServer())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "You are not an admin on this server!");
		return;
	}

	if (args.ArgC() != 2)
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "Please specify the <mapName> to change to!");
		return;
	}

	const char *mapName = args[1];

	char pszMapPath[80];
	Q_snprintf(pszMapPath, 80, "maps/%s.bsp", mapName);
	if (!filesystem->FileExists(pszMapPath, "MOD"))
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "The map you specified doesn't exist on the server!");
		return;
	}

	if (mapName && mapName[0])
		GameBaseServer()->DoMapChange(mapName);
}

CON_COMMAND(admin_spectate, "Admin Spectate Command")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (pClient->IsBot() || !pClient->IsAdminOnServer())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "You are not an admin on this server!");
		return;
	}

	if (!HL2MPRules()->IsFastPacedGameplay())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "This command may only be used in Deathmatch & Elimination mode!");
		return;
	}

	pClient->m_bWantsToDeployAsHuman = true;
	pClient->SetRespawnTime(0.0f);
	pClient->HandleCommand_JoinTeam(TEAM_SPECTATOR, true);
	pClient->ForceRespawn();
}

CON_COMMAND(admin_joinhuman, "Admin Join Human Command")
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(UTIL_GetCommandClient());
	if (!pClient)
		return;

	if (pClient->IsBot() || !pClient->IsAdminOnServer())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "You are not an admin on this server!");
		return;
	}

	if (!HL2MPRules()->IsFastPacedGameplay())
	{
		ClientPrint(pClient, HUD_PRINTCONSOLE, "This command may only be used in Deathmatch & Elimination mode!");
		return;
	}

	pClient->m_bWantsToDeployAsHuman = true;
	pClient->SetRespawnTime(3.0f);
	pClient->HandleCommand_JoinTeam(TEAM_SPECTATOR, true);
	pClient->ForceRespawn();
}
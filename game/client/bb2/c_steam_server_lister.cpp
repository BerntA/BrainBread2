//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Server Browsing, Get Server Details, Etc... 
// Notice: Used for direct connection, populating the server browser, etc...
//
//========================================================================================//

#include "cbase.h"
#include "c_steam_server_lister.h"
#include "GameBase_Client.h"
#include "GameBase_Shared.h"

CSteamServerLister::CSteamServerLister()
{
	m_iConnectRedirect = CONNECTION_WAITING;
	m_bRequestingServers = false;
	m_hServerListRequest = NULL;
}

CSteamServerLister::~CSteamServerLister()
{
}

//-----------------------------------------------------------------------------
// Purpose: Callback from Steam telling us about a server that has responded
//-----------------------------------------------------------------------------
void CSteamServerLister::ServerResponded(HServerListRequest hReq, int iServer)
{
	gameserveritem_t *pServer = steamapicontext->SteamMatchmakingServers()->GetServerDetails(hReq, iServer);
	if (pServer)
	{
		// Filter out servers that don't match our appid here (might get these in LAN calls since we can't put more filters on it)
		if (pServer->m_nAppID == steamapicontext->SteamUtils()->GetAppID())
			AddServer(pServer);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Callback from Steam telling us about a server that has failed to respond
//-----------------------------------------------------------------------------
void CSteamServerLister::ServerFailedToRespond(HServerListRequest hReq, int iServer)
{
}

//-----------------------------------------------------------------------------
// Purpose: Callback from Steam telling us a refresh is complete
//-----------------------------------------------------------------------------
void CSteamServerLister::RefreshComplete(HServerListRequest hReq, EMatchMakingServerResponse response)
{
	m_bRequestingServers = false;

	// If another request is outstanding, make sure we release it properly
	if (m_hServerListRequest)
	{
		steamapicontext->SteamMatchmakingServers()->ReleaseRequest(m_hServerListRequest);
		m_hServerListRequest = NULL;
	}

	if (m_iConnectRedirect == CONNECTION_SEARCH_INTERNET) // We used direct connect to find our server through the internet, now we try through LAN.
	{
		m_iConnectRedirect = CONNECTION_SEARCH_LAN;
		RequestServersByType(SERVER_REQUEST_LAN);
	}
	else if (m_iConnectRedirect == CONNECTION_SEARCH_LAN) // LAN and Internet didn't give us any results so we try to connect anyways without a proper setup of our loading screen.
	{
		m_iConnectRedirect = CONNECTION_WAITING;
		GameBaseClient->RunMap("", szDirectConnectIP);
	}
	else // Callback to our GUI!
		GameBaseClient->ServerRefreshCompleted();
}

//-----------------------------------------------------------------------------
// Purpose: Initiate a refresh of a type of servers.
//-----------------------------------------------------------------------------
void CSteamServerLister::RequestServersByType(int type)
{
	if (!steamapicontext || !steamapicontext->SteamMatchmakingServers() || !steamapicontext->SteamUtils())
		return;

	// If we are still finishing the previous refresh, then ignore this new request
	if (m_bRequestingServers)
		return;

	// If another request is outstanding, make sure we release it properly
	if (m_hServerListRequest)
	{
		steamapicontext->SteamMatchmakingServers()->ReleaseRequest(m_hServerListRequest);
		m_hServerListRequest = NULL;
	}

	// Track that we are now in a refresh, what type of refresh, and reset our server count
	m_bRequestingServers = true;

	MatchMakingKeyValuePair_t pFilters[1];
	MatchMakingKeyValuePair_t *pFilter = pFilters;

	Q_strncpy(pFilters[0].m_szKey, "gamedir", sizeof(pFilters[0].m_szKey));
	Q_strncpy(pFilters[0].m_szValue, "brainbread2", sizeof(pFilters[0].m_szValue));

	switch (type)
	{
	case SERVER_REQUEST_INTERNET:
		m_hServerListRequest = steamapicontext->SteamMatchmakingServers()->RequestInternetServerList(steamapicontext->SteamUtils()->GetAppID(), &pFilter, ARRAYSIZE(pFilters), this);
		break;
	case SERVER_REQUEST_LAN:
		m_hServerListRequest = steamapicontext->SteamMatchmakingServers()->RequestLANServerList(steamapicontext->SteamUtils()->GetAppID(), this);
		break;
	case SERVER_REQUEST_HISTORY:
		m_hServerListRequest = steamapicontext->SteamMatchmakingServers()->RequestHistoryServerList(steamapicontext->SteamUtils()->GetAppID(), &pFilter, ARRAYSIZE(pFilters), this);
		break;
	case SERVER_REQUEST_FAVORITES:
		m_hServerListRequest = steamapicontext->SteamMatchmakingServers()->RequestFavoritesServerList(steamapicontext->SteamUtils()->GetAppID(), &pFilter, ARRAYSIZE(pFilters), this);
		break;
	case SERVER_REQUEST_FRIENDS:
		m_hServerListRequest = steamapicontext->SteamMatchmakingServers()->RequestFriendsServerList(steamapicontext->SteamUtils()->GetAppID(), &pFilter, ARRAYSIZE(pFilters), this);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Received a number of servers.
//-----------------------------------------------------------------------------
void CSteamServerLister::AddServer(gameserveritem_t *pGameServerItem)
{
	if (m_iConnectRedirect >= CONNECTION_SEARCH_INTERNET)
	{
		if (!strcmp(pGameServerItem->m_NetAdr.GetConnectionAddressString(), szDirectConnectIP))
		{
			Q_strncpy(szDirectConnectMap, pGameServerItem->m_szMap, MAX_MAP_NAME);
			m_iConnectRedirect = CONNECTION_WAITING;

			if (pGameServerItem->m_bPassword)
				GameBaseClient->ShowPasswordDialog(true);
			else
				GameBaseClient->RunMap(szDirectConnectMap, szDirectConnectIP);
		}
	}
	else // WE probably requested this for our GUI server browser!
		GameBaseClient->AddServerBrowserItem(pGameServerItem);
}

void CSteamServerLister::DirectConnect(const char *ip)
{
	m_iConnectRedirect = CONNECTION_SEARCH_INTERNET;
	Q_strncpy(szDirectConnectIP, ip, MAX_WEAPON_STRING);
	RequestServersByType(SERVER_REQUEST_INTERNET);
}

void CSteamServerLister::DirectConnectWithPassword(const char *password)
{
	engine->ClientCmd_Unrestricted(VarArgs("password %s\n", password));
	GameBaseClient->RunMap(szDirectConnectMap, szDirectConnectIP);
	GameBaseClient->ShowPasswordDialog(false);
}

void CSteamServerLister::StopCurrentRequest(void)
{
	// Make sure that we're busy:
	if (!m_bRequestingServers)
		return;

	// Release the current request, if any...
	if (m_hServerListRequest)
	{
		steamapicontext->SteamMatchmakingServers()->ReleaseRequest(m_hServerListRequest);
		m_hServerListRequest = NULL;
	}

	m_bRequestingServers = false;
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Server Browsing, Get Server Details, Etc... 
// Notice: Used for direct connection, populating the server browser, etc...
//
//========================================================================================//

#ifndef STEAM_SERVER_LISTER_H
#define STEAM_SERVER_LISTER_H

#ifdef _WIN32
#pragma once
#endif

#include <steam/steam_api.h>
#include "steam/isteamapps.h"

enum ServerRequestType_t
{
	SERVER_REQUEST_INTERNET = 0,
	SERVER_REQUEST_LAN,
	SERVER_REQUEST_HISTORY,
	SERVER_REQUEST_FAVORITES,
	SERVER_REQUEST_FRIENDS,
};

class CSteamServerLister : public ISteamMatchmakingServerListResponse
{
public:

	CSteamServerLister();
	~CSteamServerLister();
	void AddServer(gameserveritem_t *pGameServerItem);
	void ServerResponded(HServerListRequest hReq, int iServer);
	void ServerFailedToRespond(HServerListRequest hReq, int iServer);
	void RefreshComplete(HServerListRequest hReq, EMatchMakingServerResponse response);
	void RequestServersByType(int type);
	void DirectConnect(const char *ip);
	void DirectConnectWithPassword(const char *password);
	void StopCurrentRequest(void);
	bool IsBusy(void) { return m_bRequestingServers; }

private:

	HServerListRequest m_hServerListRequest;
	bool m_bRequestingServers;
	int m_iConnectRedirect;
	char szDirectConnectIP[MAX_WEAPON_STRING];
	char szDirectConnectMap[MAX_MAP_NAME];
};

#endif //STEAM_SERVER_LISTER_H
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles player loadout loading/saving @ Story mode. (when transiting between maps)
//
//========================================================================================//

#ifndef PLAYER_LOADOUT_HANDLER_H
#define PLAYER_LOADOUT_HANDLER_H

#ifdef _WIN32
#pragma once
#endif

#include "hl2mp_player.h"

#define PLAYER_MAX_CARRY_WEPS 4

struct PlayerLoadoutItem_t
{
	unsigned long long ullSteamID;

	char pchActiveWeapon[MAX_WEAPON_STRING];
	char pchLastWeapon[MAX_WEAPON_STRING];

	char pchWeapon[PLAYER_MAX_CARRY_WEPS][MAX_WEAPON_STRING];
	int iAmmo[PLAYER_MAX_CARRY_WEPS];
	int iPrimaryClip[PLAYER_MAX_CARRY_WEPS];
	int iSecondaryClip[PLAYER_MAX_CARRY_WEPS];
	bool bIsBloody[PLAYER_MAX_CARRY_WEPS];

	int iHealth;
	int iMaxHealth;
	int iArmorValue;
	int iArmorType;
};

class CPlayerLoadoutHandler;
class CPlayerLoadoutHandler
{
public:
	CPlayerLoadoutHandler();
	~CPlayerLoadoutHandler();

	bool CanHandle();
	void SaveLoadoutData();
	bool LoadDataForPlayer(CHL2MP_Player *pPlayer);
	void RemoveDataForPlayer(CBasePlayer *pPlayer);
	void SetMapTransit(bool value) { m_bIsTransiting = value; }
	bool IsMapTransit() { return m_bIsTransiting; }

private:

	bool m_bIsTransiting;
	CUtlVector<PlayerLoadoutItem_t> m_pLoadoutList;
};

#endif // PLAYER_LOADOUT_HANDLER_H
//=========       Copyright � Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles player loadout loading/saving @ Story mode. (when transiting between maps)
//
//========================================================================================//

#include "cbase.h"
#include "player_loadout_handler.h"
#include "hl2mp_gamerules.h"
#include "world.h"

CPlayerLoadoutHandler::CPlayerLoadoutHandler()
{
	m_pLoadoutList.Purge();
	m_bIsTransiting = false;
}

CPlayerLoadoutHandler::~CPlayerLoadoutHandler()
{
	m_pLoadoutList.Purge();
	m_bIsTransiting = false;
}

bool CPlayerLoadoutHandler::CanHandle()
{
	if (HL2MPRules() && GetWorldEntity() && GetWorldEntity()->IsStoryMap() && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE))
		return true;

	return false;
}

void CPlayerLoadoutHandler::SaveLoadoutData()
{
	m_pLoadoutList.Purge();

	if (!CanHandle())
		return;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pPlayer || pPlayer->IsBot() || !pPlayer->HasFullySpawned() || !pPlayer->IsAlive())
			continue;

		PlayerLoadoutItem_t item;

		item.ullSteamID = pPlayer->GetSteamIDAsUInt64();
		for (int wep = 0; wep < PLAYER_MAX_CARRY_WEPS; wep++)
		{
			const char *weaponClassname = "";
			CBaseCombatWeapon *pWeapon = pPlayer->Weapon_GetSlot(wep);
			if (pWeapon && pWeapon->VisibleInWeaponSelection())
			{
				weaponClassname = pWeapon->GetClassname();

				item.iAmmo[wep] = pWeapon->GetAmmoCount();
				item.iClip[wep] = pWeapon->m_iClip;

				item.bIsBloody[wep] = pWeapon->m_bIsBloody;
			}
			else
			{
				item.iAmmo[wep] = 0;
				item.iClip[wep] = 0;
				item.bIsBloody[wep] = false;
			}

			Q_strncpy(item.pchWeapon[wep], weaponClassname, MAX_WEAPON_STRING);
		}

		Q_strncpy(item.pchActiveWeapon, "", MAX_WEAPON_STRING);
		Q_strncpy(item.pchLastWeapon, "", MAX_WEAPON_STRING);

		CBaseCombatWeapon *pActiveWep = pPlayer->GetActiveWeapon();
		if (pActiveWep)
			Q_strncpy(item.pchActiveWeapon, pActiveWep->GetClassname(), MAX_WEAPON_STRING);

		CBaseCombatWeapon *pLastWep = pPlayer->Weapon_GetLast();
		if (pLastWep)
			Q_strncpy(item.pchLastWeapon, pLastWep->GetClassname(), MAX_WEAPON_STRING);

		item.iHealth = pPlayer->GetHealth();
		item.iMaxHealth = pPlayer->GetMaxHealth();
		item.iArmorValue = pPlayer->ArmorValue();
		item.iArmorType = pPlayer->m_BB2Local.m_iActiveArmorType.Get();

		m_pLoadoutList.AddToTail(item);
	}
}

bool CPlayerLoadoutHandler::LoadDataForPlayer(CHL2MP_Player *pPlayer)
{
	if (CanHandle() && pPlayer)
	{
		unsigned long long steamID = pPlayer->GetSteamIDAsUInt64();
		for (int i = (m_pLoadoutList.Count() - 1); i >= 0; i--)
		{
			if (m_pLoadoutList[i].ullSteamID == steamID)
			{
				pPlayer->SetHealth(m_pLoadoutList[i].iHealth);
				pPlayer->SetMaxHealth(m_pLoadoutList[i].iMaxHealth);
				pPlayer->ApplyArmor(((float)m_pLoadoutList[i].iArmorValue), m_pLoadoutList[i].iArmorType);

				for (int wep = 0; wep < PLAYER_MAX_CARRY_WEPS; wep++)
				{
					if (m_pLoadoutList[i].pchWeapon[wep] && m_pLoadoutList[i].pchWeapon[wep][0])
					{
						if (pPlayer->GiveItem(m_pLoadoutList[i].pchWeapon[wep], true))
						{
							CBaseCombatWeapon *pWeapon = pPlayer->Weapon_OwnsThisType(m_pLoadoutList[i].pchWeapon[wep]);
							if (pWeapon)
							{
								pWeapon->SetAmmoCount(m_pLoadoutList[i].iAmmo[wep]);
								pWeapon->m_iClip = m_pLoadoutList[i].iClip[wep];
								pWeapon->m_bIsBloody = m_pLoadoutList[i].bIsBloody[wep];
							}
						}
					}
				}

				CBaseCombatWeapon *pActiveWep = pPlayer->Weapon_OwnsThisType(m_pLoadoutList[i].pchActiveWeapon);
				if (pActiveWep)
					pPlayer->Weapon_Switch(pActiveWep, true);

				CBaseCombatWeapon *pLastWep = pPlayer->Weapon_OwnsThisType(m_pLoadoutList[i].pchLastWeapon);
				if (pLastWep)
					pPlayer->Weapon_SetLast(pLastWep);

				//m_pLoadoutList.Remove(i);
				return true;
			}
		}
	}

	return false;
}

void CPlayerLoadoutHandler::RemoveDataForPlayer(CBasePlayer *pPlayer)
{
	if (m_bIsTransiting)
		return;

	if (pPlayer && CanHandle())
	{
		unsigned long long steamID = pPlayer->GetSteamIDAsUInt64();
		for (int i = (m_pLoadoutList.Count() - 1); i >= 0; i--)
		{
			if (m_pLoadoutList[i].ullSteamID == steamID)
				m_pLoadoutList.Remove(i);
		}
	}
}
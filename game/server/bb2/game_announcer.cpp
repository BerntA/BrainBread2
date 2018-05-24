//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Deathmatch Announcer - Not really an 'entity'.
//
//========================================================================================//

#include "cbase.h"
#include "game_announcer.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar bb2_deathmatch_announcer_maxtime("bb2_deathmatch_announcer_maxtime", "10", FCVAR_REPLICATED, "If you strike another kill and it has gone this many sec it will reset your kill value. 0 = Ignore!", true, 0.0f, true, 20.0f);

static CGameAnnouncer g_GameAnnouncer;
CGameAnnouncer *GameAnnouncer = &g_GameAnnouncer;

struct announcerItem_t
{
	char pchWeaponUsed[64];
	int iKillsWithWeapon;
	int iPlayerIndex;
};

static CUtlVector<announcerItem_t> m_pAnnouncerList;

int GetIndexForPlayerWithWeapon(const char *weapon, int playerIndex)
{
	if (!weapon || (weapon && (strlen(weapon) <= 0)))
		return -1;

	for (int i = (m_pAnnouncerList.Count() - 1); i >= 0; i--)
	{
		if ((m_pAnnouncerList[i].iPlayerIndex == playerIndex) && (!strcmp(weapon, m_pAnnouncerList[i].pchWeaponUsed)))
			return i;
	}

	return -1;
}

void CGameAnnouncer::Reset(void)
{
	m_bFirstBlood = false;
	m_flIsBusy = 0.0f;
	m_iLastTime = 0;
	m_pAnnouncerList.Purge();

	if (HL2MPRules() && !HL2MPRules()->CanUseGameAnnouncer())
		return;

	HL2MPRules()->EmitSoundToClient(NULL, "GameStart", BB2_SoundTypes::TYPE_ANNOUNCER, false);
}

void CGameAnnouncer::RemoveItemsForPlayer(int index)
{
	if (!m_pAnnouncerList.Count())
		return;

	for (int i = (m_pAnnouncerList.Count() - 1); i >= 0; i--)
	{
		if (m_pAnnouncerList[i].iPlayerIndex == index)
			m_pAnnouncerList.Remove(i);
	}
}

bool CGameAnnouncer::HandleClientAnnouncerSound(const CTakeDamageInfo &info, CBaseEntity *pVictim, CBaseEntity *pKiller)
{
	if (HL2MPRules() && !HL2MPRules()->CanUseGameAnnouncer())
		return false;

	CHL2MP_Player *pPlayerKiller = ToHL2MPPlayer(pKiller);
	CHL2MP_Player *pPlayerVictim = ToHL2MPPlayer(pVictim);
	CBaseEntity *pInflictor = info.GetInflictor();

	if (!pPlayerKiller || !pPlayerVictim)
		return false;

	if (pPlayerKiller == pPlayerVictim)
		return false;

	const char *weaponUsed = NULL;
	int playerIndex = pPlayerKiller->entindex();
	CHL2MP_Player *pInflictorPlayer = ToHL2MPPlayer(pInflictor);

	CBaseCombatWeapon *pActiveWeapon = NULL;
	if (pInflictorPlayer && pInflictorPlayer->GetActiveWeapon())
	{
		pActiveWeapon = pInflictorPlayer->GetActiveWeapon();
		weaponUsed = pActiveWeapon->GetClassname();
	}
	else if (pInflictor && !pInflictor->IsPlayer())
		weaponUsed = pInflictor->GetClassname();

	int customDMGType = info.GetDamageCustom();
	if (customDMGType != 0)
	{
		if (customDMGType & DMG_CLUB)
			weaponUsed = "weapon_kick";
	}

	if ((weaponUsed == NULL) || (playerIndex <= 0))
		return false;

	int indexInList = GetIndexForPlayerWithWeapon(weaponUsed, playerIndex);
	if (indexInList == -1)
	{
		announcerItem_t item;
		item.iKillsWithWeapon = 0;
		item.iPlayerIndex = playerIndex;
		Q_strncpy(item.pchWeaponUsed, weaponUsed, 64);
		indexInList = m_pAnnouncerList.AddToTail(item);
	}

	int kills = m_pAnnouncerList[indexInList].iKillsWithWeapon + 1;
	const char *soundscriptToRun = NULL;

	CHL2MP_Player *pLastKiller = ToHL2MPPlayer(pPlayerKiller->m_hLastKiller.Get());
	if (pLastKiller && (pLastKiller == pPlayerVictim))
	{
		pPlayerKiller->m_hLastKiller = NULL;
		soundscriptToRun = "PayBack";
	}
	else if (!strcmp(weaponUsed, "weapon_kick") && (kills == 4))
		soundscriptToRun = "KickMaster";
	else if (!strcmp(weaponUsed, "weapon_hands"))
	{
		if (kills == 1)
			soundscriptToRun = "Fister";
		else if (kills == 2)
			soundscriptToRun = "DoubleFist";
		else if (kills == 4)
			soundscriptToRun = "FistMaster";
	}
	else if (!strcmp(weaponUsed, "weapon_baseballbat") && (kills == 4))
		soundscriptToRun = "BaseballBatMaster";
	else if (pActiveWeapon && (kills == 7) && (!strcmp(weaponUsed, pActiveWeapon->GetClassname())))
	{
		if (!pActiveWeapon->IsMeleeWeapon())
		{
			if ((pActiveWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE) || (pActiveWeapon->GetWeaponType() == WEAPON_TYPE_SMG))
				soundscriptToRun = "RifleMaster";
			else if (pActiveWeapon->GetWeaponType() == WEAPON_TYPE_SHOTGUN)
				soundscriptToRun = "ShotgunMaster";
			else if (pActiveWeapon->GetWeaponType() == WEAPON_TYPE_PISTOL)
				soundscriptToRun = "PistolMaster";
			else if (pActiveWeapon->GetWeaponType() == WEAPON_TYPE_REVOLVER)
				soundscriptToRun = "357Master";
		}
	}

	m_pAnnouncerList[indexInList].iKillsWithWeapon = kills;

	if (soundscriptToRun == NULL)
		return false;

	HL2MPRules()->EmitSoundToClient(NULL, soundscriptToRun, BB2_SoundTypes::TYPE_ANNOUNCER, false, playerIndex);
	return true;
}

void CGameAnnouncer::DeathNotice(const CTakeDamageInfo &info, CBaseEntity *pVictim, CBaseEntity *pKiller)
{
	if (HL2MPRules() && !HL2MPRules()->CanUseGameAnnouncer())
		return;

	if (m_flIsBusy > gpGlobals->curtime)
		return;

	CHL2MP_Player *pPlayerKiller = ToHL2MPPlayer(pKiller);
	CHL2MP_Player *pPlayerVictim = ToHL2MPPlayer(pVictim);
	CHL2MP_Player *pInflictor = ToHL2MPPlayer(info.GetInflictor());

	if (!pPlayerKiller || !pPlayerVictim)
		return;

	if (pPlayerKiller == pPlayerVictim)
	{
		HL2MPRules()->EmitSoundToClient(NULL, "Suicider", BB2_SoundTypes::TYPE_ANNOUNCER, false, pPlayerKiller->entindex());
		return;
	}

	float timeVar = bb2_deathmatch_announcer_maxtime.GetFloat();
	if (timeVar != 0.0f)
	{
		float timeCheck = gpGlobals->curtime - pPlayerKiller->m_flDMTimeSinceLastKill;
		if (timeCheck >= timeVar)
			pPlayerKiller->m_iDMKills = 0;
	}

	pPlayerKiller->m_iDMKills++;

	const char *soundscriptToRun = NULL;

	if (!m_bFirstBlood)
	{
		m_bFirstBlood = true;
		soundscriptToRun = "FirstBlood";
	}
	else
	{
		const int lastHitgroup = pPlayerVictim->LastHitGroup();

		const char *weaponString = "";
		CBaseCombatWeapon *pWeaponUsed = pPlayerKiller->GetActiveWeapon();
		if (pWeaponUsed && (pInflictor == pPlayerKiller))
			weaponString = pWeaponUsed->GetClassname();

		if (info.GetDamageType() & DMG_BLAST)
			soundscriptToRun = "FatalDeath";
		else if (pPlayerKiller->m_iDMKills == 2)
			soundscriptToRun = "MultiKill";
		else if (pPlayerKiller->m_iDMKills == 3)
			soundscriptToRun = "TripleKill";
		else if (pPlayerKiller->m_iDMKills == 6)
			soundscriptToRun = "Domination";
		else if (pPlayerKiller->m_iDMKills == 9)
			soundscriptToRun = "KillingSpree";
		else if (pPlayerKiller->m_iDMKills == 12)
			soundscriptToRun = "Massacre";
		else if (pPlayerKiller->m_iDMKills == 15)
			soundscriptToRun = "Slaughter";
		else if (pPlayerKiller->m_iDMKills == 20)
			soundscriptToRun = "MassSlaughter";
		else if (!HandleClientAnnouncerSound(info, pVictim, pKiller))
		{
			if (lastHitgroup == HITGROUP_HEAD)
			{
				if (info.GetDamageType() & DMG_BULLET)
					soundscriptToRun = "Headshot";
				else
					soundscriptToRun = "Headhunter";
			}
		}
	}

	if (soundscriptToRun != NULL)
	{
		HL2MPRules()->EmitSoundToClient(NULL, soundscriptToRun, BB2_SoundTypes::TYPE_ANNOUNCER, false, pPlayerKiller->entindex());
		m_flIsBusy = gpGlobals->curtime + 1.5f;
	}

	pPlayerKiller->m_flDMTimeSinceLastKill = gpGlobals->curtime;
	pPlayerVictim->m_hLastKiller = pPlayerKiller;
	RemoveItemsForPlayer(pPlayerVictim->entindex());
}

void CGameAnnouncer::Think(int timeleft)
{
	if (HL2MPRules() && !HL2MPRules()->CanUseGameAnnouncer())
		return;

	if (timeleft <= 0 || (m_iLastTime == timeleft))
		return;

	// Time in seconds:
	const char *soundScript = NULL;
	switch (timeleft)
	{
	case 300:
		soundScript = "Countdown5Min";
		break;
	case 180:
		soundScript = "Countdown3Min";
		break;
	case 60:
		soundScript = "Countdown1Min";
		break;
	case 30:
		soundScript = "Countdown30Sec";
		break;
	case 10:
		soundScript = "Countdown10Sec";
		break;
	case 5:
		soundScript = "Countdown5Sec";
		break;
	}

	m_iLastTime = timeleft;

	if (soundScript != NULL)
		HL2MPRules()->EmitSoundToClient(NULL, soundScript, BB2_SoundTypes::TYPE_ANNOUNCER, false);
}
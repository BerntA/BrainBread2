//========= Copyright Reperio-Studios Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Draws the death notices.
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time("hud_deathnotice_time", "6", 0);
static ConVar hud_deathnotice_npcs("hud_deathnotice_npcs", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Display kills by NPCs in the killfeed.\n", true, 0, true, 1);

// Contents of each entry in our list of death notices
struct DeathNoticeItem
{
	char killerName[MAX_PLAYER_NAME_LENGTH];
	char victimName[MAX_PLAYER_NAME_LENGTH];
	CHudTexture *iconDeath;
	CHudTexture *iconHeadshot;
	Color       colItemKiller;
	Color       colItemVictim;
	int			iSuicide;
	float		flDisplayTime;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudDeathNotice, vgui::Panel);
public:
	CHudDeathNotice(const char *pElementName);

	void Init(void);
	void VidInit(void);
	virtual bool ShouldDraw(void);
	virtual void Paint(void);
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

	void RetireExpiredDeathNotices(void);

	virtual void FireGameEvent(IGameEvent * event);

private:

	CPanelAnimationVarAliasType(float, m_flLineHeight, "LineHeight", "15", "proportional_float");
	CPanelAnimationVar(float, m_flMaxDeathNotices, "MaxDeathNotices", "4");
	CPanelAnimationVar(bool, m_bRightJustify, "RightJustify", "1");
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "Default");

	// Texture for skull symbol
	CHudTexture		*m_iconD_skull;

	CUtlVector<DeathNoticeItem> m_DeathNotices;
};

using namespace vgui;

DECLARE_HUDELEMENT(CHudDeathNotice);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDeathNotice::CHudDeathNotice(const char *pElementName) :
CHudElement(pElementName), BaseClass(NULL, "HudDeathNotice")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_iconD_skull = NULL;

	SetHiddenBits(HIDEHUD_MISCSTATUS | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetPaintBackgroundEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init(void)
{
	ListenForGameEvent("death_notice");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit(void)
{
	m_iconD_skull = gHUD.GetIcon("d_skull");
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && (m_DeathNotices.Count()));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	if (!m_iconD_skull || !g_PR)
		return;

	int yStart = GetClientModeHL2MPNormal()->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(GameResources()->GetTeamColor(0));

	int iCount = m_DeathNotices.Count();
	for (int i = 0; i < iCount; i++)
	{
		CHudTexture *icon = m_DeathNotices[i].iconDeath;
		CHudTexture *hshotIcon = m_DeathNotices[i].iconHeadshot;
		if (!icon)
			continue;

		wchar_t victim[256];
		wchar_t killer[256];

		g_pVGuiLocalize->ConvertANSIToUnicode(m_DeathNotices[i].victimName, victim, sizeof(victim));
		g_pVGuiLocalize->ConvertANSIToUnicode(m_DeathNotices[i].killerName, killer, sizeof(killer));

		// Get the local position for this notice
		int len = UTIL_ComputeStringWidth(m_hTextFont, victim);
		int y = yStart + (m_flLineHeight * i);

		int x;
		if (m_bRightJustify)
			x = GetWide() - len - icon->Width() - ((hshotIcon != NULL) ? hshotIcon->Width() : 0);
		else
			x = 0;

		// Only draw killers name if it wasn't a suicide
		if (!m_DeathNotices[i].iSuicide)
		{
			if (m_bRightJustify)
				x -= UTIL_ComputeStringWidth(m_hTextFont, killer);

			surface()->DrawSetTextColor(m_DeathNotices[i].colItemKiller);

			// Draw killer's name
			surface()->DrawSetTextPos(x, y);
			surface()->DrawSetTextFont(m_hTextFont);
			surface()->DrawUnicodeString(killer);
			surface()->DrawGetTextPos(x, y);
		}

		Color iconColor(255, 255, 255, 255);

		// Draw death weapon
		//If we're using a font char, this will ignore iconTall and iconWide
		//icon->DrawSelf( x, y, iconWide, iconTall, iconColor );
		surface()->DrawSetColor(iconColor);
		surface()->DrawSetTexture(icon->textureId);
		surface()->DrawTexturedRect(x, y, icon->Width() + x, icon->Height() + y);

		x += icon->Width();

		if (hshotIcon)
		{
			surface()->DrawSetColor(iconColor);
			surface()->DrawSetTexture(hshotIcon->textureId);
			surface()->DrawTexturedRect(x, y, hshotIcon->Width() + x, hshotIcon->Height() + y);
			x += hshotIcon->Width();
		}

		surface()->DrawSetTextColor(m_DeathNotices[i].colItemVictim);

		// Draw victims name
		surface()->DrawSetTextPos(x, y);
		surface()->DrawSetTextFont(m_hTextFont);	//reset the font, draw icon can change it
		surface()->DrawUnicodeString(victim);
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::RetireExpiredDeathNotices(void)
{
	// Loop backwards because we might remove one
	int iSize = m_DeathNotices.Size();
	for (int i = iSize - 1; i >= 0; i--)
	{
		if (m_DeathNotices[i].flDisplayTime < gpGlobals->curtime)
			m_DeathNotices.Remove(i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CHudDeathNotice::FireGameEvent(IGameEvent * event)
{
	if (!g_PR || !HL2MPRules() || (hud_deathnotice_time.GetFloat() == 0))
		return;

	int victimID = event->GetInt("victimID");
	int killerID = event->GetInt("killerID");
	int hitgroup = event->GetInt("hitgroup");

	const char *victimName = HL2MPRules()->GetNameForCombatCharacter(victimID);
	const char *killerName = HL2MPRules()->GetNameForCombatCharacter(killerID);
	const char *npcVictimName = event->GetString("npcVictim");
	const char *npcKillerName = event->GetString("npcKiller");
	const char *weaponName = event->GetString("weapon");

	if (!hud_deathnotice_npcs.GetBool() && !IsPlayerIndex(killerID) && !IsPlayerIndex(victimID))
		return;

	if (npcVictimName && npcVictimName[0])
		victimName = npcVictimName;

	if (npcKillerName && npcKillerName[0])
		killerName = npcKillerName;

	// Hacky name fixes:
	if (!strcmp(weaponName, "m1a1"))
		killerName = "M1A1";

	char killedWith[128];
	if (weaponName && *weaponName)
		Q_snprintf(killedWith, sizeof(killedWith), "death_%s", weaponName);
	else
		killedWith[0] = 0;

	// Do we have too many death messages in the queue?
	if (m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() >= (int)m_flMaxDeathNotices)
	{
		// Remove the oldest one in the queue, which will always be the first
		m_DeathNotices.Remove(0);
	}

	// Make a new death notice
	DeathNoticeItem deathMsg;

	deathMsg.colItemKiller = GameResources()->GetTeamColor(g_PR->GetTeam(killerID));
	deathMsg.colItemVictim = GameResources()->GetTeamColor(g_PR->GetTeam(victimID));

	Q_strncpy(deathMsg.killerName, killerName, MAX_PLAYER_NAME_LENGTH);
	Q_strncpy(deathMsg.victimName, victimName, MAX_PLAYER_NAME_LENGTH);

	deathMsg.flDisplayTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.iconHeadshot = NULL;

	if (victimID == killerID)
		deathMsg.iSuicide = 1;
	else
		deathMsg.iSuicide = 0;

	// Try and find the death identifier in the icon list
	CHudTexture *m_pHeadShotTexture = NULL;
	if ((hitgroup == HITGROUP_HEAD) && (victimID != killerID) && killerName && killerName[0] && victimName && victimName[0])
	{
		int playerTeam = g_PR->GetTeam(victimID);
		if (playerTeam)
		{
			if (playerTeam == TEAM_HUMANS)
				m_pHeadShotTexture = gHUD.GetIcon("headshot_player_human");
			else if (playerTeam == TEAM_DECEASED)
				m_pHeadShotTexture = gHUD.GetIcon("headshot_player_deceased");
		}
		else
		{
			char pszDeathIcon[64];
			Q_snprintf(pszDeathIcon, 64, "headshot_%s", victimName);
			Q_strlower(pszDeathIcon);
			m_pHeadShotTexture = gHUD.GetIcon(pszDeathIcon);
		}
	}

	deathMsg.iconHeadshot = m_pHeadShotTexture;
	deathMsg.iconDeath = gHUD.GetIcon(killedWith);

	if (!deathMsg.iconDeath || deathMsg.iSuicide)
	{
		deathMsg.iconDeath = m_iconD_skull;
	}

	// Add it to our list of death notices
	m_DeathNotices.AddToTail(deathMsg);

	// PRINT TO CONSOLE, EVENT LOGGING, ETC...
	char sDeathMsg[512];

	// Record the death notice in the console
	if (deathMsg.iSuicide)
	{
		if (!strcmp(killedWith, "death_world"))
			Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%s died.\n", victimName);
		else
			Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%s suicided.\n", victimName);
	}
	else
	{
		Q_snprintf(sDeathMsg, sizeof(sDeathMsg), "%s killed %s with %s.\n", killerName, victimName, weaponName);
	}

	Msg("%s", sDeathMsg);
}
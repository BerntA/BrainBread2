//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Draws your Avatar in the top left corner with the xp bar around it, displays your health, armor and level. We also display perks and specials such as team bonus.
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "vgui_avatarimage.h"
#include "c_playerresource.h"
#include "hl2mp_gamerules.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "GameBase_Shared.h"
#include "GameBase_Client.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

enum SharedHUDTextureIcons
{
	SHARED_HUD_ICON_XPCIRCLE = 0,
	SHARED_HUD_ICON_INFECTION_NO,
	SHARED_HUD_ICON_INFECTION_FULL,
	SHARED_HUD_ICON_ARMOR_NONE,
	SHARED_HUD_ICON_ARMOR_LIGHT,
	SHARED_HUD_ICON_ARMOR_MEDIUM,
	SHARED_HUD_ICON_ARMOR_HEAVY,
	SHARED_HUD_ICON_KICK_CD_BG,
	SHARED_HUD_ICON_KICK_CD_FG,
	SHARED_HUD_ICON_SLIDE_CD_BG,
	SHARED_HUD_ICON_SLIDE_CD_FG,
	SHARED_HUD_ICON_QUEST,

	SHARED_HUD_ICON_COUNT
};

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudProfileStats : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudProfileStats, vgui::Panel);

public:

	CHudProfileStats(const char * pElementName);
	virtual ~CHudProfileStats();

	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);
	virtual void DisplayQuestNotification(int status);

protected:

	virtual void UpdatePlayer(C_HL2MP_Player *pClient);
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	bool m_bIsZombie;

	int m_iKillCount;
	int m_iPointsUntilLevel;
	int m_iLevel;

	int m_iHealth;
	int m_iMaxHealth;
	int m_iArmor;
	int m_iMaxArmor;

	// Health Icon
	int m_nTexture_HealthIcon;

	// Perk Icons
	// 0 - Agi
	// 1 - Str
	// 2 - Pro
	// 3 - Zombie Rage
	int m_nTexture_Perk[4];
	int m_nTexture_PerkHint;

	int m_nTexture_Background;
	int m_nTexture_DeathmatchBackground;
	int m_nTexture_InfectionBackground;

	// 0 - Health
	// 1 - Armor
	// TODO: Add special / energy?
	int m_nTextureBars[2];

	// Bonus icon
	int m_nTexture_Bonus;

	// HUD Texture Icons:
	CHudTexture *m_pSharedHUDIcons[SHARED_HUD_ICON_COUNT];

	// Quest Notification Icon:
	float m_flQuestNotificationTime;
	Color m_colQuestNotification;

	CPanelAnimationVarAliasType(float, level_x, "level_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, level_y, "level_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, level_w, "level_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, level_h, "level_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, credits_x, "credits_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, credits_y, "credits_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, credits_w, "credits_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, credits_h, "credits_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, xp_ypos, "xp_ypos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, text_offset, "text_offset", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, height_offset, "height_offset", "0", "proportional_float");

	/* Health */
	CPanelAnimationVarAliasType(float, health_icon_x, "health_icon_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, health_icon_y, "health_icon_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, health_icon_w, "health_icon_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, health_icon_h, "health_icon_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, health_bar_x, "health_bar_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, health_bar_y, "health_bar_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, health_bar_w, "health_bar_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, health_bar_h, "health_bar_h", "0", "proportional_float");
	/* Armor */
	CPanelAnimationVarAliasType(float, armor_bar_x, "armor_bar_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, armor_bar_y, "armor_bar_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, armor_bar_w, "armor_bar_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, armor_bar_h, "armor_bar_h", "0", "proportional_float");
	/* PERK */
	CPanelAnimationVarAliasType(float, perk_icon_x, "perk_icon_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, perk_icon_y, "perk_icon_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, perk_icon_w, "perk_icon_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, perk_icon_h, "perk_icon_h", "0", "proportional_float");

	// No-Skill Mode:
	CPanelAnimationVarAliasType(float, dm_x, "dm_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dm_y, "dm_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dm_w, "dm_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dm_h, "dm_h", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dm_offset_x, "dm_offset_x", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, powerup_w, "powerup_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, powerup_h, "powerup_h", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, powerup_x, "powerup_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, powerup_y, "powerup_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, dm_cooldown_xpos, "dm_cooldown_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dm_cooldown_ypos, "dm_cooldown_ypos", "0", "proportional_float");

	// Misc
	CPanelAnimationVarAliasType(float, misc_x, "misc_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, misc_y, "misc_y", "0", "proportional_float");

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "BB2_PANEL");
	CPanelAnimationVar(vgui::HFont, m_hXPFont, "XPFont", "Default");
	CPanelAnimationVar(vgui::HFont, m_hSmallFont, "DefFontSmall", "DefaultVerySmall");
};

static CHudProfileStats *g_pProfileStatsHUD = NULL;
void OnNotifyQuestState(int state) // HACK
{
	if (g_pProfileStatsHUD)
		g_pProfileStatsHUD->DisplayQuestNotification(state);
}

DECLARE_HUDELEMENT(CHudProfileStats);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudProfileStats::CHudProfileStats(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudProfileStats")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	const char *szPerkIcons[] =
	{
		"vgui/skills/humans/ico_reality_phase",
		"vgui/skills/humans/ico_blood_rage",
		"vgui/skills/humans/ico_gunslinger",
		"vgui/skills/zombies/ico_damage",
	};

	const char *szBars[] =
	{
		"vgui/hud/base/health_full",
		"vgui/hud/base/armor_full",
	};

	m_nTexture_HealthIcon = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_HealthIcon, "vgui/hud/icons/health", true, false);

	for (int i = 0; i < _ARRAYSIZE(m_nTexture_Perk); i++)
	{
		m_nTexture_Perk[i] = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_nTexture_Perk[i], szPerkIcons[i], true, false);
	}

	for (int i = 0; i < _ARRAYSIZE(m_nTextureBars); i++)
	{
		m_nTextureBars[i] = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_nTextureBars[i], szBars[i], true, false);
	}

	m_nTexture_Background = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_Background, "vgui/hud/base/background", true, false);

	m_nTexture_DeathmatchBackground = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_DeathmatchBackground, "vgui/hud/base/background_dm", true, false);

	m_nTexture_InfectionBackground = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_InfectionBackground, "vgui/hud/base/infection_empty", true, false);

	m_nTexture_Bonus = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_Bonus, "vgui/hud/icons/team_bonus", true, false);

	m_nTexture_PerkHint = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_PerkHint, "vgui/hud/icons/icon_tip", true, false);

	for (int i = 0; i < SHARED_HUD_ICON_COUNT; i++)
		m_pSharedHUDIcons[i] = NULL;

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);

	g_pProfileStatsHUD = this;
}

CHudProfileStats::~CHudProfileStats()
{
	g_pProfileStatsHUD = NULL;
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudProfileStats::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose: Load shared icon textures:
//-----------------------------------------------------------------------
void CHudProfileStats::VidInit(void)
{
	const char *iconTextures[SHARED_HUD_ICON_COUNT] =
	{
		"xp_circle_max",
		"infection_circle_empty",
		"infection_circle_max",
		"armor_none",
		"armor_light",
		"armor_medium",
		"armor_heavy",
		"kick_cd_bg",
		"kick_cd_fg",
		"slide_cd_bg",
		"slide_cd_fg",
		"quest",
	};

	for (int i = 0; i < SHARED_HUD_ICON_COUNT; i++)
		m_pSharedHUDIcons[i] = gHUD.GetIcon(iconTextures[i]);
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudProfileStats::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
	m_flQuestNotificationTime = 0.0f;
	m_colQuestNotification = GetFgColor();
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
bool CHudProfileStats::ShouldDraw(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !g_PR || !HL2MPRules() || !CHudElement::ShouldDraw())
		return false;

	if (pPlayer->GetTeamNumber() < TEAM_HUMANS)
		return false;

	UpdatePlayer(pPlayer);
	return true;
}

void CHudProfileStats::DisplayQuestNotification(int status)
{
	m_flQuestNotificationTime = gpGlobals->curtime + 3.5f;
	switch (status)
	{
	case STATUS_SUCCESS:
		m_colQuestNotification = Color(45, 185, 45, 255);
		break;
	case STATUS_FAILED:
		m_colQuestNotification = Color(170, 0, 0, 255);
		break;
	default:
		m_colQuestNotification = Color(255, 255, 255, 255);
		break;
	}
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudProfileStats::Paint()
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !g_PR || !HL2MPRules())
		return;

	bool bSkillsEnabled = HL2MPRules()->CanUseSkills();
	bool bCanRespawnAsHuman = (pPlayer->m_BB2Local.m_bCanRespawnAsHuman && !pPlayer->m_BB2Local.m_bHasPlayerEscaped);

	float xposZombie = misc_x;
	float yposZombie = misc_y;

	float xPosOffset = 0.0f;
	if (!bSkillsEnabled || m_bIsZombie)
		xPosOffset = dm_offset_x;

	bool bCanBeInfected = (!m_bIsZombie && (HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE));
	int m_iTeamBonus = pPlayer->m_BB2Local.m_iPerkTeamBonus;
	float flXPTextX, flXPTextY, flScale;
	flScale = 0.0f;
	flXPTextX = 0.0f;
	flXPTextY = 0.0f;

	if (bSkillsEnabled && !m_bIsZombie)
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(m_nTexture_Background);
		surface()->DrawTexturedRect(0, 0, GetWide(), GetTall() - height_offset);
	}
	else
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(m_nTexture_DeathmatchBackground);
		surface()->DrawTexturedRect(dm_x, dm_y, dm_x + dm_w, dm_y + dm_h);
	}

	CHudTexture *pHudIcon = NULL;
	if (!m_bIsZombie && bSkillsEnabled)
	{
		pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_XPCIRCLE];
		if (pHudIcon && (m_iLevel < MAX_PLAYER_LEVEL))
		{
			flXPTextX = pHudIcon->GetOrigPosX() + (pHudIcon->GetOrigWide() / 2);
			flXPTextY = pHudIcon->GetOrigPosY() + pHudIcon->GetOrigTall() + xp_ypos;
			flScale = (float)((float)m_iKillCount / (float)m_iPointsUntilLevel);
			pHudIcon->DrawCircularProgression(GetFgColor(), pHudIcon->GetOrigPosX(), pHudIcon->GetOrigPosY(), pHudIcon->GetOrigWide(), pHudIcon->GetOrigTall(), flScale);
		}

		if (bCanBeInfected)
		{
			pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_INFECTION_NO];
			if (pHudIcon)
			{
				surface()->DrawSetColor(GetFgColor());
				surface()->DrawSetTexture(pHudIcon->textureId);
				surface()->DrawTexturedRect(pHudIcon->GetOrigPosX(), pHudIcon->GetOrigPosY(), pHudIcon->GetOrigPosX() + pHudIcon->GetOrigWide(), pHudIcon->GetOrigPosY() + pHudIcon->GetOrigTall());
			}

			if (g_PR->IsInfected(pPlayer->entindex()))
			{
				float flInfectionTime = GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->flInfectionDuration; // Max time in sec.
				float flTimeElapsed = pPlayer->m_BB2Local.m_flInfectionTimer - gpGlobals->curtime; // Elapsed time in sec.
				if (flTimeElapsed < 0)
					flTimeElapsed = 0.0f;

				float flFraction = 1.0f - (flTimeElapsed / flInfectionTime); // Reverse the fraction.
				pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_INFECTION_FULL];
				if (pHudIcon)
				{
					flXPTextX = pHudIcon->GetOrigPosX() + (pHudIcon->GetOrigWide() / 2);
					flXPTextY = pHudIcon->GetOrigPosY() + pHudIcon->GetOrigTall() + xp_ypos;
					pHudIcon->DrawCircularProgression(GetFgColor(), pHudIcon->GetOrigPosX(), pHudIcon->GetOrigPosY(), pHudIcon->GetOrigWide(), pHudIcon->GetOrigTall(), flFraction);
				}
			}
		}
	}

	// Armor Bar FG
	flScale = (((float)m_iArmor) / 100.0f);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(m_nTextureBars[1]);
	surface()->DrawTexturedSubRect(armor_bar_x + xPosOffset, armor_bar_y, xPosOffset + armor_bar_x + (int)(armor_bar_w * flScale), armor_bar_y + armor_bar_h,
		0.0f, 0.0f, flScale, 1.0f);

	// Armor Icon
	switch (pPlayer->m_BB2Local.m_iActiveArmorType)
	{
	case TYPE_LIGHT:
		pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_ARMOR_LIGHT];
		break;
	case TYPE_MED:
		pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_ARMOR_MEDIUM];
		break;
	case TYPE_HEAVY:
		pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_ARMOR_HEAVY];
		break;
	default:
		pHudIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_ARMOR_NONE];
		break;
	}

	if (pHudIcon)
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(pHudIcon->textureId);
		surface()->DrawTexturedRect(pHudIcon->GetOrigPosX() + xPosOffset, pHudIcon->GetOrigPosY(), xPosOffset + pHudIcon->GetOrigPosX() + pHudIcon->GetOrigWide(), pHudIcon->GetOrigPosY() + pHudIcon->GetOrigTall());
	}

	// Health Bar FG
	flScale = ((float)m_iHealth / (float)m_iMaxHealth);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(m_nTextureBars[0]);
	surface()->DrawTexturedSubRect(xPosOffset + health_bar_x, health_bar_y, xPosOffset + health_bar_x + (int)(health_bar_w * flScale), health_bar_y + health_bar_h,
		0.0f, 0.0f, flScale, 1.0f);

	// HP Icon
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(m_nTexture_HealthIcon);
	surface()->DrawTexturedRect(xPosOffset + health_icon_x, health_icon_y, xPosOffset + health_icon_x + health_icon_w, health_icon_y + health_icon_h);

	// TEXT -
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTextFont(m_hSmallFont);
	surface()->DrawSetTextColor(GetFgColor());

	wchar_t unicode[64];
	int iStringLen = 0;

	float textX = 0, textY = 0;
	textX = (level_x + level_w) / 2.0f;
	textY = (level_y + level_h) / 2.0f;

	if (bSkillsEnabled)
	{
		if (m_bIsZombie)
		{
			wchar_t wszArg1[10];
			V_swprintf_safe(wszArg1, L"%i", pPlayer->m_BB2Local.m_iZombieCredits);
			g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), g_pVGuiLocalize->Find("#VGUI_SkillPoints"), 1, wszArg1);

			surface()->DrawSetTextFont(m_hSmallFont);
			surface()->DrawSetTextPos(xposZombie, yposZombie);
			surface()->DrawPrintText(unicode, wcslen(unicode));

			if (bCanRespawnAsHuman)
			{
				g_pVGuiLocalize->ConvertANSIToUnicode("UNBOUND", wszArg1, sizeof(wszArg1));
				const char *pchKeyPress = engine->Key_LookupBinding("classic_respawn_ashuman");
				if (pchKeyPress)
				{
					char pszKey[32];
					Q_strncpy(pszKey, pchKeyPress, 32);
					Q_strupr(pszKey);

					g_pVGuiLocalize->ConvertANSIToUnicode(pszKey, wszArg1, sizeof(wszArg1));
				}

				wchar_t longTip[128];
				g_pVGuiLocalize->ConstructString(longTip, sizeof(longTip), g_pVGuiLocalize->Find("#HUD_ZombieHumanRespawnTip"), 1, wszArg1);
				yposZombie -= surface()->GetFontTall(m_hSmallFont);
				surface()->DrawSetTextPos(xposZombie, yposZombie);
				surface()->DrawPrintText(longTip, wcslen(longTip));
			}
		}
		else
		{
			surface()->DrawSetTextFont(m_hTextFont);
			V_swprintf_safe(unicode, L"%i", m_iLevel);

			iStringLen = UTIL_ComputeStringWidth(m_hTextFont, unicode);
			textX -= (iStringLen / 2.0f);
			textY -= (surface()->GetFontTall(m_hTextFont) / 2.0f);

			surface()->DrawSetTextPos(textX, textY);
			surface()->DrawPrintText(unicode, wcslen(unicode));

			textX = (credits_x + credits_w) / 2.0f;
			textY = (credits_y + credits_h) / 2.0f;

			V_swprintf_safe(unicode, L"%i", pPlayer->m_BB2Local.m_iSkill_Talents);

			iStringLen = UTIL_ComputeStringWidth(m_hTextFont, unicode);
			textX -= (iStringLen / 2.0f);
			textY -= (surface()->GetFontTall(m_hTextFont) / 2.0f);

			surface()->DrawSetTextPos(textX, textY);
			surface()->DrawPrintText(unicode, wcslen(unicode));
		}
	}

	surface()->DrawSetTextFont(m_hSmallFont);

	if (bb2_show_details.GetBool())
	{
		if (!m_bIsZombie && bSkillsEnabled)
		{
			if (m_iLevel < MAX_PLAYER_LEVEL)
			{
				V_swprintf_safe(unicode, L"%i / %i", m_iKillCount, m_iPointsUntilLevel);
				iStringLen = UTIL_ComputeStringWidth(m_hSmallFont, unicode);
				surface()->DrawSetTextPos(flXPTextX - (iStringLen / 2), flXPTextY);
				surface()->DrawPrintText(unicode, wcslen(unicode));
			}
		}

		V_swprintf_safe(unicode, L"%i / 100", m_iArmor);
		iStringLen = UTIL_ComputeStringWidth(m_hSmallFont, unicode);
		surface()->DrawSetTextPos(xPosOffset + armor_bar_x + (armor_bar_w / 2) - (iStringLen / 2), armor_bar_y + text_offset);
		surface()->DrawPrintText(unicode, wcslen(unicode));

		V_swprintf_safe(unicode, L"%i / %i", m_iHealth, m_iMaxHealth);
		iStringLen = UTIL_ComputeStringWidth(m_hSmallFont, unicode);
		surface()->DrawSetTextPos(xPosOffset + health_bar_x + (health_bar_w / 2) - (iStringLen / 2), health_bar_y + text_offset);
		surface()->DrawPrintText(unicode, wcslen(unicode));
	}

	// Draw Perk Icon if available:
	float flPerkYOffset = perk_icon_y;
	float flPerkXTextOffset = XRES(2);
	float flPerkYTextOffset = flPerkYOffset + (perk_icon_h / 2);

	if (pPlayer->m_BB2Local.m_bCanActivatePerk && bSkillsEnabled)
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(m_nTexture_PerkHint);

		if (m_bIsZombie)
		{
			yposZombie -= (perk_icon_h + 1);
			surface()->DrawTexturedRect(xposZombie, yposZombie, xposZombie + perk_icon_w, yposZombie + perk_icon_h);
			V_swprintf_safe(unicode, L"Rage Mode Available!");
			surface()->DrawSetTextPos(xposZombie + perk_icon_w + flPerkXTextOffset, yposZombie + (perk_icon_h / 2) - (surface()->GetFontTall(m_hSmallFont) / 2));
			surface()->DrawPrintText(unicode, wcslen(unicode));
		}
		else
		{
			surface()->DrawTexturedRect(perk_icon_x, flPerkYOffset, perk_icon_x + perk_icon_w, flPerkYOffset + perk_icon_h);

			flPerkYTextOffset -= (surface()->GetFontTall(m_hSmallFont) / 2);
			V_swprintf_safe(unicode, L"Perks Available!");
			surface()->DrawSetTextPos(perk_icon_x + perk_icon_w + flPerkXTextOffset, flPerkYTextOffset);
			surface()->DrawPrintText(unicode, wcslen(unicode));

			flPerkYOffset -= (perk_icon_h + 1.0f);
			flPerkYTextOffset = flPerkYOffset + (perk_icon_h / 2);
		}
	}

	if (pPlayer->GetPerkFlags())
	{
		const float percent = pPlayer->GetPerkFraction(
			m_bIsZombie ? GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData()->flDuration : GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedData()->flPerkTime
			);

		if (m_bIsZombie)
		{
			if (pPlayer->IsPerkFlagActive(PERK_ZOMBIE_RAGE))
			{
				yposZombie -= (perk_icon_h + 1);
				CHudTexture::DrawCircularProgression(GetFgColor(), Color(25, 255, 25, 255), m_nTexture_Perk[3], m_nTexture_Perk[3], xposZombie, yposZombie, perk_icon_w, perk_icon_h, percent);
				V_swprintf_safe(unicode, L"RAGE Mode Active!");
				surface()->DrawSetTextPos(xposZombie + perk_icon_w + flPerkXTextOffset, yposZombie + (perk_icon_h / 2) - (surface()->GetFontTall(m_hSmallFont) / 2));
				surface()->DrawPrintText(unicode, wcslen(unicode));
			}
		}
		else if (bSkillsEnabled)
		{
			int iActivePerk = 0;

			if (pPlayer->IsPerkFlagActive(PERK_HUMAN_REALITYPHASE))
			{
				iActivePerk = PLAYER_SKILL_HUMAN_REALITY_PHASE;
				CHudTexture::DrawCircularProgression(GetFgColor(), Color(25, 255, 25, 255), m_nTexture_Perk[0], m_nTexture_Perk[0], perk_icon_x, flPerkYOffset, perk_icon_w, perk_icon_h, percent);
			}

			if (pPlayer->IsPerkFlagActive(PERK_HUMAN_BLOODRAGE))
			{
				iActivePerk = PLAYER_SKILL_HUMAN_BLOOD_RAGE;
				CHudTexture::DrawCircularProgression(GetFgColor(), Color(25, 255, 25, 255), m_nTexture_Perk[1], m_nTexture_Perk[1], perk_icon_x, flPerkYOffset, perk_icon_w, perk_icon_h, percent);
			}

			if (pPlayer->IsPerkFlagActive(PERK_HUMAN_GUNSLINGER))
			{
				iActivePerk = PLAYER_SKILL_HUMAN_GUNSLINGER;
				CHudTexture::DrawCircularProgression(GetFgColor(), Color(25, 255, 25, 255), m_nTexture_Perk[2], m_nTexture_Perk[2], perk_icon_x, flPerkYOffset, perk_icon_w, perk_icon_h, percent);
			}

			if (iActivePerk)
			{
				flPerkYTextOffset -= (surface()->GetFontTall(m_hSmallFont) / 2.0f);
				V_swprintf_safe(unicode, L"Perk x%i", pPlayer->GetSkillValue(iActivePerk));
				surface()->DrawSetTextPos(perk_icon_x + perk_icon_w + flPerkXTextOffset, flPerkYTextOffset);
				surface()->DrawPrintText(unicode, wcslen(unicode));
				flPerkYOffset -= (perk_icon_h + 1.0f);
				flPerkYTextOffset = flPerkYOffset + (perk_icon_h / 2);
			}
		}
	}

	if ((m_iTeamBonus > 0) && bSkillsEnabled)
	{
		if (m_bIsZombie)
		{
			float teamBonusX = xposZombie, teamBonusY = yposZombie - surface()->GetFontTall(m_hSmallFont), teamBonusTextY = yposZombie + (perk_icon_h / 2) - surface()->GetFontTall(m_hSmallFont);

			surface()->DrawSetColor(GetFgColor());
			surface()->DrawSetTexture(m_nTexture_Bonus);
			surface()->DrawTexturedRect(teamBonusX, teamBonusY, perk_icon_w + teamBonusX, teamBonusY + perk_icon_h);

			teamBonusTextY -= (surface()->GetFontTall(m_hSmallFont) / 2);
			V_swprintf_safe(unicode, L"Team Bonus x%i", m_iTeamBonus);
			surface()->DrawSetTextPos(perk_icon_w + teamBonusX + flPerkXTextOffset, teamBonusTextY);
			surface()->DrawPrintText(unicode, wcslen(unicode));
		}
		else
		{
			surface()->DrawSetColor(GetFgColor());
			surface()->DrawSetTexture(m_nTexture_Bonus);
			surface()->DrawTexturedRect(perk_icon_x, flPerkYOffset, perk_icon_w + perk_icon_x, flPerkYOffset + perk_icon_h);

			flPerkYTextOffset -= (surface()->GetFontTall(m_hSmallFont) / 2);
			V_swprintf_safe(unicode, L"Team Bonus x%i", m_iTeamBonus);
			surface()->DrawSetTextPos(perk_icon_w + perk_icon_x + flPerkXTextOffset, flPerkYTextOffset);
			surface()->DrawPrintText(unicode, wcslen(unicode));
		}
	}

	if (!bSkillsEnabled && pPlayer->GetPerkFlags())
	{
		int perkFlag = 0;
		if (pPlayer->IsPerkFlagActive(PERK_POWERUP_CRITICAL))
			perkFlag = PERK_POWERUP_CRITICAL;
		else if (pPlayer->IsPerkFlagActive(PERK_POWERUP_CHEETAH))
			perkFlag = PERK_POWERUP_CHEETAH;
		else if (pPlayer->IsPerkFlagActive(PERK_POWERUP_NANITES))
			perkFlag = PERK_POWERUP_NANITES;
		else if (pPlayer->IsPerkFlagActive(PERK_POWERUP_PAINKILLER))
			perkFlag = PERK_POWERUP_PAINKILLER;
		else if (pPlayer->IsPerkFlagActive(PERK_POWERUP_PREDATOR))
			perkFlag = PERK_POWERUP_PREDATOR;

		if (perkFlag != 0)
		{
			const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(perkFlag);
			if (data)
			{
				const float percent = pPlayer->GetPerkFraction(data->flPerkDuration);
				CHudTexture *pPowerupIcon = NULL;

				pPowerupIcon = data->pIconPowerupInactive;
				if (pPowerupIcon)
				{
					surface()->DrawSetColor(GetFgColor());
					surface()->DrawSetTexture(pPowerupIcon->textureId);
					surface()->DrawTexturedRect(powerup_x, powerup_y, powerup_x + powerup_w, powerup_y + powerup_h);
				}

				pPowerupIcon = data->pIconPowerupActive;
				if (pPowerupIcon)
					pPowerupIcon->DrawCircularProgression(GetFgColor(), powerup_x, powerup_y, powerup_w, powerup_h, percent);
			}
		}
	}

	float cdxpos = 0.0f, cdypos = 0.0f;
	if (!bSkillsEnabled)
	{
		cdxpos = dm_cooldown_xpos;
		cdypos = dm_cooldown_ypos;
	}

	if (!m_bIsZombie)
	{
		if (pPlayer->m_BB2Local.m_flSlideKickCooldownEnd > gpGlobals->curtime)
		{
			float flEndTime = pPlayer->m_BB2Local.m_flSlideKickCooldownEnd;
			float flStartTime = pPlayer->m_BB2Local.m_flSlideKickCooldownStart;
			float timeToTake = flEndTime - flStartTime;
			float percentage = 1.0f - ((flEndTime - gpGlobals->curtime) / timeToTake);
			percentage = clamp(percentage, 0.0f, 1.0f);

			CHudTexture *pCooldownIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_KICK_CD_BG];
			if (pCooldownIcon)
			{
				surface()->DrawSetColor(GetFgColor());
				surface()->DrawSetTexture(pCooldownIcon->textureId);
				surface()->DrawTexturedRect(pCooldownIcon->GetOrigPosX() + cdxpos, pCooldownIcon->GetOrigPosY() + cdypos, cdxpos + pCooldownIcon->GetOrigPosX() + pCooldownIcon->GetOrigWide(), cdypos + pCooldownIcon->GetOrigPosY() + pCooldownIcon->GetOrigTall());
			}

			pCooldownIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_SLIDE_CD_BG];
			if (pCooldownIcon)
			{
				surface()->DrawSetColor(GetFgColor());
				surface()->DrawSetTexture(pCooldownIcon->textureId);
				surface()->DrawTexturedRect(pCooldownIcon->GetOrigPosX() + cdxpos, pCooldownIcon->GetOrigPosY() + cdypos, cdxpos + pCooldownIcon->GetOrigPosX() + pCooldownIcon->GetOrigWide(), cdypos + pCooldownIcon->GetOrigPosY() + pCooldownIcon->GetOrigTall());
			}

			pCooldownIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_KICK_CD_FG];
			if (pCooldownIcon)
				pCooldownIcon->DrawCircularProgression(GetFgColor(), pCooldownIcon->GetOrigPosX() + cdxpos, pCooldownIcon->GetOrigPosY() + cdypos, pCooldownIcon->GetOrigWide(), pCooldownIcon->GetOrigTall(), percentage);

			pCooldownIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_SLIDE_CD_FG];
			if (pCooldownIcon)
				pCooldownIcon->DrawCircularProgression(GetFgColor(), pCooldownIcon->GetOrigPosX() + cdxpos, pCooldownIcon->GetOrigPosY() + cdypos, pCooldownIcon->GetOrigWide(), pCooldownIcon->GetOrigTall(), percentage);

			cdypos -= (pCooldownIcon ? pCooldownIcon->GetOrigTall() : 0);
		}

		// Flash quest notification(s)!
		CHudTexture *pQuestIcon = m_pSharedHUDIcons[SHARED_HUD_ICON_QUEST];
		if (pQuestIcon && (m_flQuestNotificationTime >= gpGlobals->curtime))
		{
			surface()->DrawSetColor(m_colQuestNotification);
			surface()->DrawSetTexture(pQuestIcon->textureId);
			surface()->DrawTexturedRect(pQuestIcon->GetOrigPosX() + cdxpos, pQuestIcon->GetOrigPosY() + cdypos, cdxpos + pQuestIcon->GetOrigPosX() + pQuestIcon->GetOrigWide(), cdypos + pQuestIcon->GetOrigPosY() + pQuestIcon->GetOrigTall());
		}
	}
}

void CHudProfileStats::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CHudProfileStats::UpdatePlayer(C_HL2MP_Player *pClient)
{
	if (!pClient || !g_PR)
		return;

	m_bIsZombie = (pClient->GetTeamNumber() == TEAM_DECEASED);
	m_iLevel = g_PR->GetLevel(pClient->entindex());
	m_iKillCount = pClient->m_BB2Local.m_iSkill_XPCurrent;
	m_iPointsUntilLevel = pClient->m_BB2Local.m_iSkill_XPLeft;
	m_iHealth = MAX(pClient->m_iHealth, 0);
	m_iMaxHealth = MAX(pClient->GetMaxHealth(), 0);
	m_iArmor = MAX(pClient->GetArmorValue(), 0);
}
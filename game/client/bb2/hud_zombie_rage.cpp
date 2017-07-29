//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the zombie rage bar.
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "GameBase_Shared.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "hl2mp_gamerules.h"
#include "vgui_controls/Panel.h"
#include "usermessages.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "c_team.h"
#include "vgui/ILocalize.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

enum RageBarTextures_t
{
	RAGE_TEX_BG = 0,
	RAGE_TEX_FG,
	RAGE_TEX_ACTIVE,
	RAGE_TEX_COUNT,
};

class CHudZombieRage : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudZombieRage, vgui::Panel);

public:

	CHudZombieRage(const char * pElementName);

	void Init(void);
	void Reset(void);
	bool ShouldDraw(void);
	void Paint();
	void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	int m_nTextureBars[RAGE_TEX_COUNT];
	bool m_bIsDrawing;

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "Default");
};

DECLARE_HUDELEMENT(CHudZombieRage);

CHudZombieRage::CHudZombieRage(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudZombieRage")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	const char *pchTextures[] =
	{
		"vgui/hud/zombierage/ragebar_bg",
		"vgui/hud/zombierage/ragebar_fg",
		"vgui/hud/zombierage/ragebar_active",
	};

	for (int i = 0; i < RAGE_TEX_COUNT; i++)
	{
		m_nTextureBars[i] = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_nTextureBars[i], pchTextures[i], true, false);
	}

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

void CHudZombieRage::Init()
{
	Reset();
}

void CHudZombieRage::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(0);
	m_bIsDrawing = false;
}

bool CHudZombieRage::ShouldDraw(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !g_PR)
		return false;

	bool bShouldDraw = (CHudElement::ShouldDraw() && (pPlayer->GetTeamNumber() == TEAM_DECEASED));
	if (bShouldDraw)
	{
		if (m_bIsDrawing)
		{
			if (pPlayer->m_BB2Local.m_flZombieRageThresholdDamage <= 0.0f && !pPlayer->IsPerkFlagActive(PERK_ZOMBIE_RAGE))
			{
				m_bIsDrawing = false;
				GetAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_LINEAR);
			}
		}
		else if (!m_bIsDrawing)
		{
			if ((pPlayer->m_BB2Local.m_flZombieRageThresholdDamage > 0.0f) || pPlayer->IsPerkFlagActive(PERK_ZOMBIE_RAGE))
			{
				m_bIsDrawing = true;
				GetAnimationController()->RunAnimationCommand(this, "alpha", 256.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_LINEAR);
			}
		}
	}

	return bShouldDraw;
}

void CHudZombieRage::Paint()
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return;

	float requiredDamageThreshold = GameBaseShared()->GetSharedGameDetails()->GetPlayerZombieRageData().flRequiredDamageThreshold;
	float plrDamageThreshold = clamp(pPlayer->m_BB2Local.m_flZombieRageThresholdDamage, 0.0f, requiredDamageThreshold);
	bool bInRageMode = pPlayer->IsPerkFlagActive(PERK_ZOMBIE_RAGE);

	if (bInRageMode)
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(m_nTextureBars[RAGE_TEX_ACTIVE]);
		surface()->DrawTexturedRect(0, 0, GetWide(), GetTall());
	}
	else
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(m_nTextureBars[RAGE_TEX_BG]);
		surface()->DrawTexturedRect(0, 0, GetWide(), GetTall());

		float flFraction = clamp((plrDamageThreshold / requiredDamageThreshold), 0.0f, 1.0f);
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(m_nTextureBars[RAGE_TEX_FG]);
		surface()->DrawTexturedSubRect(0, 0, (int)(GetWide() * flFraction), GetTall(),
			0.0f, 0.0f, flFraction, 1.0f);
	}

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTextFont(m_hTextFontDef);
	surface()->DrawSetTextColor(GetFgColor());

	wchar_t unicode[64];

	if (bInRageMode)
		V_swprintf_safe(unicode, L"RAGE ACTIVE");
	else
		V_swprintf_safe(unicode, L"%i / %i", ((int)plrDamageThreshold), ((int)requiredDamageThreshold));

	int iStringW, iStringH;
	surface()->GetTextSize(m_hTextFontDef, unicode, iStringW, iStringH);
	vgui::surface()->DrawSetTextPos(
		(GetWide() / 2.0f) - (((float)iStringW) / 2.0f),
		(GetTall() / 2.0f) - (((float)iStringH) / 2.0f)
		);
	surface()->DrawPrintText(unicode, wcslen(unicode));
}

void CHudZombieRage::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
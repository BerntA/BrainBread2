//=========       Copyright © Reperio Studios 2015-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the ammo left in your gun and the clips left.
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "hl2mp_gamerules.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>

using namespace vgui;

#include "tier0/memdbgon.h" 

#define MAX_AMMO_HUD_TYPES 9
#define MAX_AMMO_HUD_TEX_COUNT 20

struct AmmoBaseData_t
{
	const char *prefix;
	int capacity;
};

static AmmoBaseData_t ammoBaseDataList[MAX_AMMO_HUD_TYPES] =
{
	{ "mag_beretta_", 9 },
	{ "mag_ak47_", 15 },
	{ "mag_trapper_", 8 },
	{ "mag_remington_", 8 },
	{ "mag_sawedoff_", 2 },
	{ "mag_mac11_", 16 },
	{ "mag_sniper_", 5 },
	{ "mag_revolver_6_", 6 },
	{ "mag_revolver_5_", 5 },
};

static CHudTexture* ammoDefinitions[MAX_AMMO_HUD_TYPES][MAX_AMMO_HUD_TEX_COUNT];

CHudTexture *GetAmmoTextureForAmmo(int type, float bulletsLeft, float clipCapacity)
{
	type = clamp(type, 0, (MAX_AMMO_HUD_TYPES - 1));
	AmmoBaseData_t *ammoBase = &ammoBaseDataList[type];
	float flIndex = clamp((bulletsLeft / clipCapacity), 0.0f, 1.0f) * ammoBase->capacity;
	int wantedIndex = ((int)flIndex);
	wantedIndex = clamp(wantedIndex, 0, (MAX_AMMO_HUD_TEX_COUNT - 1));
	return ammoDefinitions[type][wantedIndex];
}

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudBaseAmmo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudBaseAmmo, vgui::Panel);

public:
	CHudBaseAmmo(const char * pElementName);

	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "HUD_STATUS");
	CPanelAnimationVarAliasType(float, special_xpos, "special_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, special_ypos, "special_ypos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, special_wide, "special_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, special_tall, "special_tall", "0", "proportional_float");
};

DECLARE_HUDELEMENT(CHudBaseAmmo);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudBaseAmmo::CHudBaseAmmo(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudBaseAmmo")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_INVEHICLE | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudBaseAmmo::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose: Load textrues.
//-----------------------------------------------------------------------
void CHudBaseAmmo::VidInit(void)
{
	char pchTextureIcon[64];
	pchTextureIcon[0] = 0;

	for (int i = 0; i < MAX_AMMO_HUD_TYPES; i++)
	{
		int max = ammoBaseDataList[i].capacity;
		for (int x = 0; x < MAX_AMMO_HUD_TEX_COUNT; x++)
		{
			if (x <= max)
			{
				Q_snprintf(pchTextureIcon, 64, "%s%i", ammoBaseDataList[i].prefix, x);
				ammoDefinitions[i][x] = gHUD.GetIcon(pchTextureIcon);
			}
			else
				ammoDefinitions[i][x] = NULL;
		}
	}
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudBaseAmmo::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
bool CHudBaseAmmo::ShouldDraw(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !CHudElement::ShouldDraw())
		return false;

	if (pPlayer->GetTeamNumber() != TEAM_HUMANS)
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if (!pWeapon)
		return false;

	if (pWeapon->IsMeleeWeapon() || !pWeapon->UsesClipsForAmmo1() || !pWeapon->VisibleInWeaponSelection())
		return false;

	return true;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudBaseAmmo::Paint()
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return;

	C_BaseCombatWeapon *pLocalWeapon = pPlayer->GetActiveWeapon();
	if (!pLocalWeapon)
		return;

	int xpos = 0, ypos = 0, wide = 1, tall = 1, textX = 0, textY = 0;
	Color fgColor = GetFgColor();
	Color iconColor = GetFgColor();

	// Special BAR for special weapons:
	if (pLocalWeapon->GetWeaponType() == WEAPON_TYPE_SPECIAL)
	{
		float ammoLeft = (float)pLocalWeapon->m_iClip1, maxAmmo = (float)pLocalWeapon->GetMaxClip1();
		float percentOfWidth = (ammoLeft / maxAmmo);
		percentOfWidth = clamp(percentOfWidth, 0.0f, 1.0f);
		float barWide = (percentOfWidth * special_wide);

		Color fullWhite = Color(255, 255, 255, fgColor.a());
		DrawHollowBox(special_xpos, special_ypos, special_wide, special_tall, fullWhite, 1.0f, 1, 1);

		surface()->DrawSetColor(fullWhite);
		surface()->DrawFilledRect(special_xpos, special_ypos, special_xpos + barWide, special_ypos + special_tall);
		return;
	}

	CHudTexture *ammoIconPrimary = NULL, *ammoIconSecondary = NULL;

	bool bIsDual = pLocalWeapon->IsAkimboWeapon();
	if (bIsDual)
	{
		iconColor = Color(fgColor.r(), fgColor.g(), fgColor.b(), 110);
		ammoIconSecondary = GetAmmoTextureForAmmo(pLocalWeapon->GetWpnData().m_iAmmoHUDIndex,
			((float)pLocalWeapon->m_iClip2), ((float)pLocalWeapon->GetMaxClip2()));
	}

	ammoIconPrimary = GetAmmoTextureForAmmo(pLocalWeapon->GetWpnData().m_iAmmoHUDIndex,
		((float)pLocalWeapon->m_iClip1), ((float)pLocalWeapon->GetMaxClip1()));

	if (ammoIconSecondary)
	{
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(ammoIconSecondary->textureId);

		wide = ammoIconSecondary->GetOrigWide();
		tall = ammoIconSecondary->GetOrigTall();
		xpos = ammoIconSecondary->GetOrigPosX();
		ypos = ammoIconSecondary->GetOrigPosY();

		surface()->DrawTexturedRect(xpos, ypos, xpos + wide, ypos + tall);
	}

	if (ammoIconPrimary)
	{
		surface()->DrawSetColor(iconColor);
		surface()->DrawSetTexture(ammoIconPrimary->textureId);

		wide = ammoIconPrimary->GetOrigWide();
		tall = ammoIconPrimary->GetOrigTall();
		xpos = ammoIconPrimary->GetOrigPosX();
		ypos = ammoIconPrimary->GetOrigPosY();

		surface()->DrawTexturedRect(xpos, ypos, xpos + wide, ypos + tall);
	}

	// Draw Mags Left
	if (ammoIconPrimary || ammoIconSecondary)
	{
		int iAmmoCount = (pPlayer->GetAmmoCount(pLocalWeapon->m_iPrimaryAmmoType));
		int m_iMagsLeft = iAmmoCount;
		if (pLocalWeapon->GetWpnData().m_bShowAsMagsLeft)
		{
			int iMags = 0;
			if (iAmmoCount > 0)
				iMags = ((iAmmoCount < pLocalWeapon->GetDefaultClip1() && iAmmoCount > 0) ? 1 : (iAmmoCount / pLocalWeapon->GetDefaultClip1()));

			m_iMagsLeft = iMags;
		}

		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTextColor(GetFgColor());
		surface()->DrawSetTextFont(m_hTextFont);

		wchar_t unicode[32];
		V_swprintf_safe(unicode, L"x%i", m_iMagsLeft);
		textX = (xpos + wide) + scheme()->GetProportionalScaledValue(2);
		textY = ypos + ((tall / 2) - (surface()->GetFontTall(m_hTextFont) / 2));
		surface()->DrawSetTextPos(textX, textY);
		surface()->DrawPrintText(unicode, wcslen(unicode));
	}
}

void CHudBaseAmmo::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
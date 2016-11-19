//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
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
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include "c_basehlplayer.h"
#include "hl2mp_gamerules.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>

using namespace vgui;

#include "tier0/memdbgon.h" 

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudBaseAmmo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudBaseAmmo, vgui::Panel);

public:

	CHudBaseAmmo(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:

	virtual void UpdatePlayer(C_HL2MP_Player *pClient, C_BaseCombatWeapon *pWeapon);
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

	int        m_iMagsLeft;
	int        m_iActiveWeaponType;

private:

	CHudTexture *ammoIconPrimary;
	CHudTexture *ammoIconSecondary;

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

	// Initialize Textures:
	ammoIconPrimary = NULL;
	ammoIconSecondary = NULL;

	m_iMagsLeft = 0;
	m_iActiveWeaponType = -1;

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

	if (pWeapon->IsMeleeWeapon() || !pWeapon->UsesClipsForAmmo1())
		return false;

	UpdatePlayer(pPlayer, pWeapon);
	return true;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudBaseAmmo::Paint()
{
	int xpos = 0, ypos = 0, wide = 1, tall = 1, textX = 0, textY = 0;
	Color fgColor = GetFgColor();
	Color iconColor = GetFgColor();
	bool bIsDual = (ammoIconPrimary && ammoIconSecondary);
	if (bIsDual)
		iconColor = Color(fgColor.r(), fgColor.g(), fgColor.b(), 110);

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

	// Special BAR for special weapons:
	if (m_iActiveWeaponType == WEAPON_TYPE_SPECIAL)
	{
		C_BaseCombatWeapon *pLocalWeapon = GetActiveWeapon();
		if (pLocalWeapon)
		{
			float ammoLeft = (float)m_iMagsLeft, maxAmmo = (float)pLocalWeapon->GetMaxClip1();
			float percentOfWidth = (ammoLeft / maxAmmo);
			percentOfWidth = clamp(percentOfWidth, 0.0f, 1.0f);
			float barWide = (percentOfWidth * special_wide);

			Color fullWhite = Color(255, 255, 255, fgColor.a());
			DrawHollowBox(special_xpos, special_ypos, special_wide, special_tall, fullWhite, 1.0f, 1, 1);

			surface()->DrawSetColor(fullWhite);
			surface()->DrawFilledRect(special_xpos, special_ypos, special_xpos + barWide, special_ypos + special_tall);
		}
	}
}

void CHudBaseAmmo::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CHudBaseAmmo::UpdatePlayer(C_HL2MP_Player *pClient, C_BaseCombatWeapon *pWeapon)
{
	ammoIconPrimary = NULL;
	ammoIconSecondary = NULL;
	m_iActiveWeaponType = -1;

	if (!pClient || !pWeapon)
		return;

	m_iActiveWeaponType = pWeapon->GetWeaponType();
	if (!pWeapon->UsesClipsForAmmo1())
		return;

	bool bAkimbo = pWeapon->IsAkimboWeapon();

	int iAmmoCount = (pClient->GetAmmoCount(pWeapon->m_iPrimaryAmmoType));
	float percentLeft = (((float)pWeapon->m_iClip1 / (float)pWeapon->GetMaxClip1()));
	float percentRight = (((float)pWeapon->m_iClip2 / (float)pWeapon->GetMaxClip2()));

	const char *szWep = pWeapon->GetClassname();
	if (strncmp(szWep, "weapon_", 7) == 0)
		szWep += 7;

	switch (pWeapon->GetWeaponType())
	{

	case WEAPON_TYPE_SHOTGUN:
	{
		m_iMagsLeft = iAmmoCount;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_%s_%i", szWep, pWeapon->m_iClip1));
		break;
	}

	case WEAPON_TYPE_RIFLE:
	{
		float bulletCount = percentLeft * 15.0f;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_ak47_%i", (int)bulletCount));
		break;
	}

	case WEAPON_TYPE_SMG:
	{
		float bulletCount = percentLeft * 16.0f;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_mac11_%i", (int)bulletCount));
		break;
	}

	case WEAPON_TYPE_SNIPER:
	{
		float bulletCount = percentLeft * 5.0f;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_sniper_%i", (int)bulletCount));
		break;
	}

	case WEAPON_TYPE_PISTOL:
	{
		float bulletsRight = percentRight * 9.0f;
		float bulletsLeft = percentLeft * 9.0f;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_beretta_%i", (int)bulletsLeft));
		if (bAkimbo)
			ammoIconSecondary = gHUD.GetIcon(VarArgs("mag_beretta_%i", (int)bulletsRight));
		break;
	}

	case WEAPON_TYPE_REVOLVER:
	{
		float bulletsLeft = percentLeft * 6.0f;
		float bulletsRight = percentRight * 6.0f;

		m_iMagsLeft = iAmmoCount;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_revolver_%i_%i", pWeapon->GetMaxClip1(), (int)bulletsLeft));

		if (bAkimbo)
			ammoIconSecondary = gHUD.GetIcon(VarArgs("mag_revolver_%i_%i", pWeapon->GetMaxClip2(), (int)bulletsRight));
		break;
	}

	case WEAPON_TYPE_SPECIAL:
	{
		m_iMagsLeft = pWeapon->m_iClip1;
		break;
	}

	}

	if ((pWeapon->GetWeaponType() == WEAPON_TYPE_PISTOL) || (pWeapon->GetWeaponType() == WEAPON_TYPE_RIFLE) || (pWeapon->GetWeaponType() == WEAPON_TYPE_SMG) || (pWeapon->GetWeaponType() == WEAPON_TYPE_SNIPER))
	{
		int iMags = 0;
		if (iAmmoCount > 0)
			iMags = ((iAmmoCount < pWeapon->GetDefaultClip1() && iAmmoCount > 0) ? 1 : (iAmmoCount / pWeapon->GetDefaultClip1()));

		m_iMagsLeft = iMags;
	}

	if (!strcmp(szWep, "winchester1894"))
	{
		m_iMagsLeft = iAmmoCount;
		ammoIconPrimary = gHUD.GetIcon(VarArgs("mag_trapper_%i", pWeapon->m_iClip1));
	}
}
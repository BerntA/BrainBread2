//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the active weapon + text. (image preview)
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "hl2mp_gamerules.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <vgui/ILocalize.h>

using namespace vgui;

#include "tier0/memdbgon.h" 

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudWeaponView : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudWeaponView, vgui::Panel);

public:

	CHudWeaponView(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:

	virtual void UpdatePlayer(C_HL2MP_Player *pClient, C_BaseCombatWeapon *pWeapon);
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	wchar_t szCurrWep[64];
	char pchActiveWeaponClassname[64];
	CHudTexture *MaxIconAnim;

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "HUD_STATUS");
};

DECLARE_HUDELEMENT(CHudWeaponView);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudWeaponView::CHudWeaponView(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudWeaponView")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();

	SetParent(pParent);

	// Initialize Textures:
	MaxIconAnim = NULL;

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_INVEHICLE | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudWeaponView::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudWeaponView::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
	Q_strncpy(pchActiveWeaponClassname, "", 64);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
bool CHudWeaponView::ShouldDraw(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !CHudElement::ShouldDraw())
		return false;

	if (pPlayer->GetTeamNumber() != TEAM_HUMANS)
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if (!pWeapon)
		return false;

	if (!pWeapon->VisibleInWeaponSelection())
		return false;

	UpdatePlayer(pPlayer, pWeapon);
	return true;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudWeaponView::Paint()
{
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextFont(m_hTextFont);

	int iTextXPos = 0, iTextYPos = 0;
	if (MaxIconAnim)
	{
		// Draw Icon
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(MaxIconAnim->textureId);

		int xpos, ypos, wide, tall, textX, textY;
		wide = MaxIconAnim->GetOrigWide();
		tall = MaxIconAnim->GetOrigTall();
		xpos = MaxIconAnim->GetOrigPosX();
		ypos = MaxIconAnim->GetOrigPosY();

		surface()->DrawTexturedRect(xpos, ypos, xpos + wide, ypos + tall);

		// Draw Text
		textX = xpos + ((wide / 2) - (UTIL_ComputeStringWidth(m_hTextFont, szCurrWep) / 2));
		textY = ypos - surface()->GetFontTall(m_hTextFont) - scheme()->GetProportionalScaledValue(1);
		surface()->DrawSetTextPos(textX, textY);
		surface()->DrawUnicodeString(szCurrWep);
		iTextXPos = textX;
		iTextYPos = textY;
	}

	C_BaseCombatWeapon *pActiveWep = GetActiveWeapon();
	if (pActiveWep && pActiveWep->IsMeleeWeapon() && (pActiveWep->m_flMeleeCooldown > 0.0f))
	{
		float timeToUse = (pActiveWep->m_flNextSecondaryAttack - pActiveWep->m_flMeleeCooldown);
		float percentage = ((pActiveWep->m_flNextSecondaryAttack - gpGlobals->curtime) / timeToUse);
		percentage = 1.0f - clamp(percentage, 0.0f, 1.0f);

		CHudTexture *pCooldownIcon = gHUD.GetIcon("melee_cd_bg");
		if (pCooldownIcon)
		{
			iTextXPos -= pCooldownIcon->GetOrigWide();
			surface()->DrawSetColor(GetFgColor());
			surface()->DrawSetTexture(pCooldownIcon->textureId);
			surface()->DrawTexturedRect(pCooldownIcon->GetOrigPosX() + iTextXPos, pCooldownIcon->GetOrigPosY() + iTextYPos, iTextXPos + pCooldownIcon->GetOrigPosX() + pCooldownIcon->GetOrigWide(), iTextYPos + pCooldownIcon->GetOrigPosY() + pCooldownIcon->GetOrigTall());
		}

		pCooldownIcon = gHUD.GetIcon("melee_cd_fg");
		if (pCooldownIcon)
			pCooldownIcon->DrawCircularProgression(GetFgColor(), pCooldownIcon->GetOrigPosX() + iTextXPos, pCooldownIcon->GetOrigPosY() + iTextYPos, pCooldownIcon->GetOrigWide(), pCooldownIcon->GetOrigTall(), percentage);
	}
}

void CHudWeaponView::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CHudWeaponView::UpdatePlayer(C_HL2MP_Player *pClient, C_BaseCombatWeapon *pWeapon)
{
	if (!pClient || !pWeapon)
		return;

	if (pWeapon->GetSlot() > 4)
	{
		MaxIconAnim = NULL;
		return;
	}

	const char *szWep = pWeapon->GetClassname();
	if (strncmp(szWep, "weapon_", 7) == 0)
		szWep += 7;

	if (strcmp(szWep, pchActiveWeaponClassname))
	{
		Q_strncpy(pchActiveWeaponClassname, szWep, 64);
		MaxIconAnim = gHUD.GetIcon(szWep);
		g_pVGuiLocalize->ConvertANSIToUnicode(pWeapon->GetWpnData().szPrintName, szCurrWep, sizeof(szCurrWep));
	}
}
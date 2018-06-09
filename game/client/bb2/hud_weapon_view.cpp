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
#include "hl2mp_gamerules.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h" 

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudWeaponView : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudWeaponView, vgui::Panel);

public:

	CHudWeaponView(const char * pElementName);

	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	CHudTexture *m_pCooldownBG;
	CHudTexture *m_pCooldownFG;

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

	m_pCooldownBG = m_pCooldownFG = NULL;

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_INVEHICLE | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudWeaponView::Init()
{
	Reset();
}

void CHudWeaponView::VidInit(void)
{
	m_pCooldownBG = gHUD.GetIcon("melee_cd_bg");
	m_pCooldownFG = gHUD.GetIcon("melee_cd_fg");
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudWeaponView::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
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

	return true;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudWeaponView::Paint()
{
	C_BaseCombatWeapon *pActiveWep = GetActiveWeapon();
	if (!pActiveWep)
		return;

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextFont(m_hTextFont);

	int iTextXPos = 0, iTextYPos = 0;
	const CHudTexture *pWeaponIcon = pActiveWep->GetSpriteActive();
	if (pWeaponIcon)
	{
		// Draw Icon
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture(pWeaponIcon->textureId);

		int xpos, ypos, wide, tall, textX, textY;
		wide = pWeaponIcon->GetOrigWide();
		tall = pWeaponIcon->GetOrigTall();
		xpos = pWeaponIcon->GetOrigPosX();
		ypos = pWeaponIcon->GetOrigPosY();

		surface()->DrawTexturedRect(xpos, ypos, xpos + wide, ypos + tall);

		// Draw Text
		wchar_t unicode[MAX_WEAPON_STRING * 2];
		g_pVGuiLocalize->ConvertANSIToUnicode(pActiveWep->GetWpnData().szPrintName, unicode, sizeof(unicode));
		textX = xpos + ((wide / 2) - (UTIL_ComputeStringWidth(m_hTextFont, unicode) / 2));
		textY = ypos - surface()->GetFontTall(m_hTextFont) - scheme()->GetProportionalScaledValue(1);
		surface()->DrawSetTextPos(textX, textY);
		surface()->DrawUnicodeString(unicode);
		iTextXPos = textX;
		iTextYPos = textY;
	}

	if (pActiveWep->IsMeleeWeapon() && (pActiveWep->m_flMeleeCooldown > 0.0f))
	{
		float timeToUse = (pActiveWep->m_flNextSecondaryAttack - pActiveWep->m_flMeleeCooldown);
		float percentage = ((pActiveWep->m_flNextSecondaryAttack - gpGlobals->curtime) / timeToUse);
		percentage = 1.0f - clamp(percentage, 0.0f, 1.0f);

		CHudTexture *pCooldownIcon = m_pCooldownBG;
		if (pCooldownIcon)
		{
			iTextXPos -= pCooldownIcon->GetOrigWide();
			surface()->DrawSetColor(GetFgColor());
			surface()->DrawSetTexture(pCooldownIcon->textureId);
			surface()->DrawTexturedRect(pCooldownIcon->GetOrigPosX() + iTextXPos, pCooldownIcon->GetOrigPosY() + iTextYPos, iTextXPos + pCooldownIcon->GetOrigPosX() + pCooldownIcon->GetOrigWide(), iTextYPos + pCooldownIcon->GetOrigPosY() + pCooldownIcon->GetOrigTall());
		}

		pCooldownIcon = m_pCooldownFG;
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
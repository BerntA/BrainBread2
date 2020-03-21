//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays a 'charge' bar for certain weapons, for example when charging unarmed melee attacks.
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "hl2mp_gamerules.h"
#include "weapon_melee_chargeable.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

#include "tier0/memdbgon.h" 

using namespace vgui;

class CHudWeaponCharger : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudWeaponCharger, vgui::Panel);

public:

	CHudWeaponCharger(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
};

DECLARE_HUDELEMENT(CHudWeaponCharger);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudWeaponCharger::CHudWeaponCharger(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudWeaponCharger")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudWeaponCharger::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudWeaponCharger::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
bool CHudWeaponCharger::ShouldDraw(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !CHudElement::ShouldDraw())
		return false;

	if (pPlayer->GetTeamNumber() < TEAM_HUMANS)
		return false;

	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	if (!pWeapon)
		return false;

	if (!pWeapon->IsMeleeWeapon())
		return false;

	C_HL2MPMeleeChargeable *pChargeableWeapon = dynamic_cast<C_HL2MPMeleeChargeable*> (pWeapon);
	if (!pChargeableWeapon)
		return false;

	if (!pChargeableWeapon->IsChargingWeapon())
		return false;

	if (pChargeableWeapon->GetChargeFraction() <= 0.025f)
		return false;

	return true;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudWeaponCharger::Paint()
{
	C_HL2MPMeleeChargeable *pChargeableWeapon = dynamic_cast<C_HL2MPMeleeChargeable*> (GetActiveWeapon());
	if (pChargeableWeapon)
	{
		Color fgColor = GetFgColor();
		float barWide = (pChargeableWeapon->GetChargeFraction() * ((float)GetWide()));
		Color fullWhite = Color(255, 255, 255, fgColor.a());

		DrawHollowBox(0, 0, GetWide(), GetTall(), fullWhite, 1.0f, 1, 1);
		surface()->DrawSetColor(fullWhite);
		surface()->DrawFilledRect(0, 0, barWide, GetTall());
	}
}

void CHudWeaponCharger::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
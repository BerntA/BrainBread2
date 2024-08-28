//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays a basic scope for sniper weapons.
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
#include "c_playerresource.h"
#include "weapon_base_sniper.h"
#include "c_bb2_player_shared.h"

#include "tier0/memdbgon.h"

using namespace vgui;

ConVar bb2_scope_refraction("bb2_scope_refraction", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Enable refract for sniper scopes, if possible.", true, 0, true, 1);

class CHudScopeView : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudScopeView, vgui::Panel);

public:
	CHudScopeView(const char* pElementName);
	void Init(void);
	bool ShouldDraw(void);

protected:
	void Paint();
	void ApplySchemeSettings(vgui::IScheme* scheme);

private:
	int m_nTexture_Scope;
	int m_nTexture_ScopeRefract;

	CPanelAnimationVarAliasType(float, scope_wide, "scope_wide", "512", "proportional_float");
	CPanelAnimationVarAliasType(float, scope_tall, "scope_tall", "256", "proportional_float");
};

DECLARE_HUDELEMENT_DEPTH(CHudScopeView, 100);

CHudScopeView::CHudScopeView(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudScopeView")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_nTexture_Scope = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_Scope, "effects/scope", true, false);

	m_nTexture_ScopeRefract = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_ScopeRefract, "effects/scope_refract", true, false);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudScopeView::Init()
{
}

bool CHudScopeView::ShouldDraw(void)
{
	BB2PlayerGlobals->SetSniperScopeActive(false);
	if (!CHudElement::ShouldDraw())
		return false;

	C_BaseCombatWeapon* pActiveWeapon = GetActiveWeapon();
	if (!pActiveWeapon || !g_PR)
		return false;

	if (pActiveWeapon->GetWeaponType() != WEAPON_TYPE_SNIPER)
		return false;

	C_HL2MPSniperRifle* pSniperRifle = dynamic_cast<C_HL2MPSniperRifle*> (pActiveWeapon);
	if (!pSniperRifle)
		return false;

	if (!pSniperRifle->GetZoomLevel())
		return false;

	BB2PlayerGlobals->SetSniperScopeActive(true);
	return true;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudScopeView::Paint()
{
	const int textureID = bb2_scope_refraction.GetBool() ? m_nTexture_ScopeRefract : m_nTexture_Scope;

	int wide, tall;
	GetSize(wide, tall);

	// DRAW SCOPE
	int xpos = (wide / 2) - (scope_wide / 2);
	int ypos = (tall / 2) - (scope_tall / 2);

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(textureID);
	surface()->DrawTexturedRect(xpos, ypos, xpos + scope_wide, ypos + scope_tall);

	// Fill in the sides
	int sWide = scope_wide - scheme()->GetProportionalScaledValue(2);
	int sTall = scope_tall - scheme()->GetProportionalScaledValue(2);

	int left = (wide / 2) - (sWide / 2);
	int right = left + sWide;

	// LEFT fill
	surface()->DrawSetColor(COLOR_BLACK);
	surface()->DrawFilledRect(0, 0, left, tall);

	// RIGHT fill
	surface()->DrawSetColor(COLOR_BLACK);
	surface()->DrawFilledRect(right, 0, right + (wide - right), tall);

	left = (tall / 2) - (sTall / 2);
	right = left + sTall;

	// MIDDLE FILL up
	surface()->DrawSetColor(COLOR_BLACK);
	surface()->DrawFilledRect(0, 0, wide, left);

	// MIDDLE FILL down
	surface()->DrawSetColor(COLOR_BLACK);
	surface()->DrawFilledRect(0, right, wide, right + (tall - right));
}

void CHudScopeView::ApplySchemeSettings(vgui::IScheme* scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}
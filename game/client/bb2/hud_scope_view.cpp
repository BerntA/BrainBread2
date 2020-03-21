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
	CHudScopeView(const char * pElementName);
	void Init(void);
	bool ShouldDraw(void);

protected:
	void Paint();
	void ApplySchemeSettings(vgui::IScheme *scheme);

private:
	int m_nTexture_Scope;
	int m_nTexture_ScopeRefract;
};

DECLARE_HUDELEMENT_DEPTH(CHudScopeView, 100);

CHudScopeView::CHudScopeView(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudScopeView")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
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

	C_BaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
	if (!pActiveWeapon || !g_PR)
		return false;

	if (pActiveWeapon->GetWeaponType() != WEAPON_TYPE_SNIPER)
		return false;

	C_HL2MPSniperRifle *pSniperRifle = dynamic_cast<C_HL2MPSniperRifle*> (pActiveWeapon);
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
	int textureID = m_nTexture_Scope;
	if (bb2_scope_refraction.GetBool())
		textureID = m_nTexture_ScopeRefract;

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(textureID);
	surface()->DrawTexturedRect(0, 0, GetWide(), GetTall());
}

void CHudScopeView::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}
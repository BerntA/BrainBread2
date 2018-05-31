//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie Infection Effect/Overlay
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "hl2mp_gamerules.h"
#include "c_hl2mp_player.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

//-----------------------------------------------------------------------------
// Purpose: Draws Infected overlay. TODO : Replace with shader or add a custom shader for this?
//-----------------------------------------------------------------------------
class CHudHumanInfected : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudHumanInfected, vgui::Panel);

public:

	CHudHumanInfected(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:

	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual void Paint();

private:

	int m_nTexture_FG;
};

DECLARE_HUDELEMENT_DEPTH(CHudHumanInfected, 100);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudHumanInfected::CHudHumanInfected(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudHumanInfected")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_nTexture_FG = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_FG, "effects/infected", true, false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

void CHudHumanInfected::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudHumanInfected::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudHumanInfected::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

bool CHudHumanInfected::ShouldDraw(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return false;

	return (CHudElement::ShouldDraw() && (pPlayer->GetTeamNumber() == TEAM_DECEASED));
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudHumanInfected::Paint()
{
	int w, h;
	GetHudSize(w, h);

	if (GetWide() != w)
		SetWide(w);

	if (GetTall() != h)
		SetTall(h);

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(m_nTexture_FG);
	surface()->DrawTexturedRect(0, 0, w, h);
}
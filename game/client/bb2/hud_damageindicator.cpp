//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Draws a simple damage overlay when taking damage.
//
//========================================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>
#include "c_baseplayer.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHudDamageIndicator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudDamageIndicator, vgui::Panel);

public:

	CHudDamageIndicator(const char * pElementName);
	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

	void MsgFunc_Damage(bf_read &msg);

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	bool m_bShouldFadeIn;
	bool m_bShouldRender;
	int m_nTexture_Left;
	int m_nTexture_Right;

	CPanelAnimationVarAliasType(float, dmg_left_x, "dmg_left_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dmg_right_x, "dmg_right_x", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, dmg_size_w, "dmg_size_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, dmg_size_h, "dmg_size_h", "0", "proportional_float");
};

DECLARE_HUDELEMENT(CHudDamageIndicator);
DECLARE_HUD_MESSAGE(CHudDamageIndicator, Damage);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudDamageIndicator::CHudDamageIndicator(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudDamageIndicator")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_nTexture_Left = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_Left, "effects/indicator_left", true, false);

	m_nTexture_Right = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_Right, "effects/indicator_right", true, false);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDamageIndicator::Init(void)
{
	HOOK_HUD_MESSAGE(CHudDamageIndicator, Damage);
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudDamageIndicator::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && m_bShouldRender);
}

//------------------------------------------------------------------------
// Purpose: Set default layout
//-----------------------------------------------------------------------
void CHudDamageIndicator::Reset(void)
{
	m_bShouldFadeIn = m_bShouldRender = false;
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(0);
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudDamageIndicator::Paint()
{
	int ypos = (GetTall() / 2) - (dmg_size_h / 2);

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTexture(m_nTexture_Left);
	surface()->DrawTexturedRect(dmg_left_x, ypos, dmg_size_w + dmg_left_x, dmg_size_h + ypos);

	surface()->DrawSetTexture(m_nTexture_Right);
	surface()->DrawTexturedRect(GetWide() - dmg_right_x, ypos, dmg_size_w + (GetWide() - dmg_right_x), dmg_size_h + ypos);

	if (m_bShouldFadeIn && (GetAlpha() >= 255))
	{
		m_bShouldFadeIn = false;
		vgui::GetAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.4f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
	}

	if (m_bShouldRender && (GetAlpha() <= 1))
		m_bShouldRender = false;
}

void CHudDamageIndicator::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

void CHudDamageIndicator::MsgFunc_Damage(bf_read &msg)
{
	m_bShouldRender = m_bShouldFadeIn = msg.ReadByte();
	vgui::GetAnimationController()->RunAnimationCommand(this, "alpha", 256.0f, 0.0f, 0.25f, AnimationController::INTERPOLATOR_LINEAR);
}
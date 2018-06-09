//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: When a new game starts it will display X sec until the round starts!
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "hl2mp_gamerules.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h" 

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Default
//-----------------------------------------------------------------------------
class CHudRoundTimer : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudRoundTimer, vgui::Panel);

public:

	CHudRoundTimer(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	int m_iRoundTimeLeft;
	bool m_bShouldRender;

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "BB2_PANEL");
};

DECLARE_HUDELEMENT(CHudRoundTimer);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudRoundTimer::CHudRoundTimer(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudRoundTimer")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudRoundTimer::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudRoundTimer::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(0);
	m_bShouldRender = false;
}

//------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------
bool CHudRoundTimer::ShouldDraw(void)
{
	if (!CHudElement::ShouldDraw() || !HL2MPRules())
		return false;

	m_iRoundTimeLeft = MAX(HL2MPRules()->m_iRoundCountdown, 0);
	if ((m_iRoundTimeLeft > 0) && !HL2MPRules()->m_bRoundStarted)
	{
		if (!m_bShouldRender)
		{
			m_bShouldRender = true;
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 256.0f, 0.0f, 0.4f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else
	{
		if (m_bShouldRender)
		{
			m_bShouldRender = false;
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}

	if (g_pClientMode->GetViewportAnimationController()->IsPanelBeingAnimated(this))
		return true;

	return m_bShouldRender;
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudRoundTimer::Paint()
{
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextFont(m_hTextFont);

	wchar_t unicode[64];
	wchar_t wszTimeLeft[10];
	V_swprintf_safe(wszTimeLeft, L"%i", m_iRoundTimeLeft);
	g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), g_pVGuiLocalize->Find("#HUD_RoundCountdown"), 1, wszTimeLeft);

	int xpos, ypos;
	xpos = (GetWide() / 2) - (UTIL_ComputeStringWidth(m_hTextFont, unicode) / 2);
	ypos = (GetTall() / 2) - (surface()->GetFontTall(m_hTextFont) / 2);

	surface()->DrawSetTextPos(xpos, ypos);
	surface()->DrawPrintText(unicode, wcslen(unicode));
}

void CHudRoundTimer::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
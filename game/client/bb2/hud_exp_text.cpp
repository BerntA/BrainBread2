//=========       Copyright © Reperio Studios 2022 @ Bernt Andreas Eide!       ============//
//
// Purpose: Fancy arcade XP view
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

#include "tier0/memdbgon.h" 

using namespace vgui;

ConVar bb2_render_xp_text("bb2_render_xp_text", "1", FCVAR_ARCHIVE, "Display XP received when dealing damage, from achievements, etc.", true, 0.0f, true, 1.0f);

struct ExpItem_t
{
	int value;
	float timeAdded;
};

class CHudExperienceText : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudExperienceText, vgui::Panel);

public:

	CHudExperienceText(const char * pElementName);

	void Init(void);
	void VidInit(void);
	bool ShouldDraw(void);
	void Reset(void);
	void MsgFunc_ExperienceTextInfo(bf_read &msg);

protected:

	void ApplySchemeSettings(vgui::IScheme *scheme);
	void Paint();

private:

	CUtlVector<ExpItem_t> m_pItems;
	float m_flLastTimeRecv;
	CPanelAnimationVar(vgui::HFont, m_hDefaultFont, "DefaultFont", "Default");
	CPanelAnimationVar(Color, m_hXPColor, "XPColor", "15 240 15 255");
	CPanelAnimationVar(float, m_flLifeCycle, "LifeCycle", "1.25f");
	CPanelAnimationVar(float, m_flMergeTime, "MergeTime", "0.18f");
};

DECLARE_HUDELEMENT_DEPTH(CHudExperienceText, 80);
DECLARE_HUD_MESSAGE(CHudExperienceText, ExperienceTextInfo);

CHudExperienceText::CHudExperienceText(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudExperienceText")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

void CHudExperienceText::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CHudExperienceText::Init()
{
	HOOK_HUD_MESSAGE(CHudExperienceText, ExperienceTextInfo);
	Reset();
}

void CHudExperienceText::VidInit(void)
{
	m_pItems.Purge();
}

bool CHudExperienceText::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && m_pItems.Count() && bb2_render_xp_text.GetBool());
}

void CHudExperienceText::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
	m_flLastTimeRecv = 0.0f;
}

void CHudExperienceText::Paint()
{
	const int fontTall = surface()->GetFontTall(m_hDefaultFont);
	const int ypos = GetTall() + fontTall + scheme()->GetProportionalScaledValue(2);
	const int xpos = scheme()->GetProportionalScaledValue(2);
	wchar_t unicode[64];

	for (int i = (m_pItems.Count() - 1); i >= 0; i--)
	{
		const ExpItem_t &item = m_pItems[i];

		const float delta = (gpGlobals->curtime - item.timeAdded) / m_flLifeCycle;
		const float currPos = (1.0f - clamp(delta, 0.0f, 1.0f)) * ypos - fontTall;

		V_swprintf_safe(unicode, L"+%i XP", item.value);
		surface()->DrawSetColor(m_hXPColor);
		surface()->DrawSetTextColor(m_hXPColor);
		surface()->DrawSetTextFont(m_hDefaultFont);
		surface()->DrawSetTextPos(xpos, ((int)currPos));
		surface()->DrawUnicodeString(unicode);

		if (delta > 1.15f)
			m_pItems.Remove(i);
	}
}

void CHudExperienceText::MsgFunc_ExperienceTextInfo(bf_read &msg)
{
	const int xp = msg.ReadShort();

	// Prevent spam.
	if ((m_pItems.Count() > 0) && ((gpGlobals->curtime - m_flLastTimeRecv) <= m_flMergeTime))
	{
		m_pItems[m_pItems.Count() - 1].value += xp;
		return;
	}

	// Add new exp item to render.
	ExpItem_t item;
	item.value = xp;
	item.timeAdded = gpGlobals->curtime;

	m_pItems.AddToTail(item);
	m_flLastTimeRecv = gpGlobals->curtime;
}
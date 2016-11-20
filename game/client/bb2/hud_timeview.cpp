//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the timeleft of the entire game.
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "hl2mp_gamerules.h"
#include "vgui_controls/Panel.h"
#include "usermessages.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "c_team.h"
#include "vgui/ILocalize.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

class CHudTimeView : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudTimeView, vgui::Panel);

public:

	CHudTimeView(const char * pElementName);

	void Init(void);
	void Reset(void);
	bool ShouldDraw(void);
	void Paint();
	void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	int m_nTextureTimeLeft;
	float GetSecondsFromTimeByPercentage(int iPercent);

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "Default");
	CPanelAnimationVarAliasType(float, time_wide, "time_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, time_tall, "time_tall", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, time_xpos, "time_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, time_ypos, "time_ypos", "0", "proportional_float");
};

DECLARE_HUDELEMENT(CHudTimeView);

//------------------------------------------------------------------------
// Purpose: Constructor 
//------------------------------------------------------------------------
CHudTimeView::CHudTimeView(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudTimeView")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_nTextureTimeLeft = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTextureTimeLeft, "vgui/spectating/stopwatch", true, false);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//------------------------------------------------------------------------
// Purpose: Initialize = Start 
//------------------------------------------------------------------------
void CHudTimeView::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose: Reset - Constructor - Spawn similar stuff...
//-----------------------------------------------------------------------
void CHudTimeView::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

//------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------
bool CHudTimeView::ShouldDraw(void)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient || !CHudElement::ShouldDraw() || !HL2MPRules())
		return false;

	if ((HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION) || (!HL2MPRules()->m_bRoundStarted) || (pClient->GetTeamNumber() < TEAM_HUMANS))
		return false;

	return true;
}

float CHudTimeView::GetSecondsFromTimeByPercentage(int iPercent)
{
	if (HL2MPRules())
		return (((HL2MPRules()->GetTimelimitValue() * 60) / 100) * iPercent);

	return 0.0f;
}

//------------------------------------------------------------------------
// Purpose: Handles the drawing and setting text itself...
//------------------------------------------------------------------------
void CHudTimeView::Paint()
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !HL2MPRules())
		return;

	int iAlpha = GetFgColor().a();
	Color baseTimerColor = GetFgColor();
	int timer = HL2MPRules()->GetTimeLeft();

	if (GetSecondsFromTimeByPercentage(10) > timer)
		baseTimerColor = Color(255, 0, 0, iAlpha);
	else if (GetSecondsFromTimeByPercentage(20) > timer)
		baseTimerColor = Color(255, 255, 0, iAlpha);

	surface()->DrawSetTexture(m_nTextureTimeLeft);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawTexturedRect(time_xpos, time_ypos, time_xpos + time_wide, time_ypos + time_tall);

	wchar_t unicode[32];
	surface()->DrawSetColor(baseTimerColor);
	surface()->DrawSetTextFont(m_hTextFontDef);
	surface()->DrawSetTextColor(baseTimerColor);

	V_swprintf_safe(unicode, L"%d:%02d", (timer / 60), (timer % 60));
	surface()->DrawSetTextPos(time_xpos + time_wide + 2, time_ypos + (time_tall / 2) - (surface()->GetFontTall(m_hTextFontDef) / 2));
	surface()->DrawPrintText(unicode, wcslen(unicode));
}

//------------------------------------------------------------------------
// Purpose: Handles the background...
//------------------------------------------------------------------------
void CHudTimeView::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
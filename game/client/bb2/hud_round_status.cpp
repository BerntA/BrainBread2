//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Draws the score for each team and the power bar for the team you're on @Elimination.
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
#include "c_playerresource.h"
#include "c_team.h"

#include "tier0/memdbgon.h" 

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Declarations
//-----------------------------------------------------------------------------
class CHudRoundStatus : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudRoundStatus, vgui::Panel);

public:

	CHudRoundStatus(const char * pElementName);

	void Init(void);
	void Reset(void);
	bool ShouldDraw(void);
	void Paint();
	void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	int m_nTextureBarBackground;
	int m_nTextureBarForeground;

	int m_nTextureScore[2];
	int m_nTextureTimeLeft;

	float GetSecondsFromTimeByPercentage(int iPercent);

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "HUD_STATUS");

	CPanelAnimationVarAliasType(float, bar_wide, "bar_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bar_tall, "bar_tall", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bar_xpos, "bar_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, bar_ypos, "bar_ypos", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, score_wide, "score_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, score_tall, "score_tall", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, score_offset, "score_offset", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, time_wide, "time_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, time_tall, "time_tall", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, time_xpos, "time_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, time_ypos, "time_ypos", "0", "proportional_float");
};

DECLARE_HUDELEMENT(CHudRoundStatus);

//------------------------------------------------------------------------
// Purpose: Constructor 
//------------------------------------------------------------------------
CHudRoundStatus::CHudRoundStatus(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudRoundStatus")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_nTextureBarBackground = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTextureBarBackground, "vgui/hud/elimination/team_bar_bg", true, false);

	m_nTextureBarForeground = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTextureBarForeground, "vgui/hud/elimination/team_bar_full", true, false);

	for (int i = 0; i < _ARRAYSIZE(m_nTextureScore); i++)
		m_nTextureScore[i] = surface()->CreateNewTextureID();

	surface()->DrawSetTextureFile(m_nTextureScore[0], "vgui/hud/elimination/human_score", true, false);
	surface()->DrawSetTextureFile(m_nTextureScore[1], "vgui/hud/elimination/deceased_score", true, false);

	m_nTextureTimeLeft = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTextureTimeLeft, "vgui/hud/elimination/time_bg", true, false);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
}

//------------------------------------------------------------------------
// Purpose: Initialize = Start 
//------------------------------------------------------------------------
void CHudRoundStatus::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose: Reset - Constructor - Spawn similar stuff...
//-----------------------------------------------------------------------
void CHudRoundStatus::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

//------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------
bool CHudRoundStatus::ShouldDraw(void)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient || !CHudElement::ShouldDraw() || !HL2MPRules())
		return false;

	if ((HL2MPRules()->GetCurrentGamemode() != MODE_ELIMINATION) || (!HL2MPRules()->m_bRoundStarted) || (pClient->GetTeamNumber() < TEAM_HUMANS))
		return false;

	return true;
}

float CHudRoundStatus::GetSecondsFromTimeByPercentage(int iPercent)
{
	if (HL2MPRules())
		return (((HL2MPRules()->GetTimelimitValue() * 60.0f) / 100.0f) * iPercent);

	return 0.0f;
}

//------------------------------------------------------------------------
// Purpose: Handles the drawing and setting text itself...
//------------------------------------------------------------------------
void CHudRoundStatus::Paint()
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !HL2MPRules())
		return;

	int xWide, xTall;
	GetSize(xWide, xTall);

	float newXPOS = (((float)xWide / 2) - (bar_wide / 2)) + bar_xpos;
	surface()->DrawSetTexture(m_nTextureBarBackground);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawTexturedRect(newXPOS, bar_ypos, newXPOS + bar_wide, bar_ypos + bar_tall);

	int iHumanScore = 0, iZombieScore = 0;
	float flExtraScore = 0.0f;

	C_Team *pHumanTeam = GetGlobalTeam(TEAM_HUMANS);
	if (pHumanTeam)
		iHumanScore = pHumanTeam->Get_Score();

	C_Team *pZombieTeam = GetGlobalTeam(TEAM_DECEASED);
	if (pZombieTeam)
		iZombieScore = pZombieTeam->Get_Score();

	C_Team *pMyTeam = GetGlobalTeam(pPlayer->GetTeamNumber());
	if (pMyTeam)
		flExtraScore = (float)pMyTeam->GetExtraScore();

	float _progress = flExtraScore / bb2_elimination_teamperk_kills_required.GetFloat();
	if (_progress > 1)
		_progress = 1.0f;

	surface()->DrawSetTexture(m_nTextureBarForeground);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawTexturedSubRect(newXPOS, bar_ypos, newXPOS + (int)(bar_wide * _progress), bar_ypos + bar_tall,
		0.0f, 0.0f, _progress, 1.0f);

	int iAlpha = GetFgColor().a();
	Color baseTimerColor = GetFgColor();
	int timer = HL2MPRules()->GetTimeLeft();

	if (GetSecondsFromTimeByPercentage(20) > timer)
		baseTimerColor = Color(255, 0, 0, iAlpha);
	else if (GetSecondsFromTimeByPercentage(40) > timer)
		baseTimerColor = Color(255, 255, 0, iAlpha);

	surface()->DrawSetColor(GetFgColor());
	surface()->DrawSetTextFont(m_hTextFontDef);
	surface()->DrawSetTextColor(GetFgColor());

	wchar_t unicode[32];
	int iLen = 0;

	// Draw Frags:
	newXPOS = (((float)xWide / 2) - (bar_wide / 2)) + bar_xpos;

	surface()->DrawSetTexture(m_nTextureScore[0]);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawTexturedRect(newXPOS - score_wide - score_offset, bar_ypos, (newXPOS - score_wide - score_offset) + score_wide, bar_ypos + score_tall);

	surface()->DrawSetTexture(m_nTextureScore[1]);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawTexturedRect(newXPOS + bar_wide + score_offset, bar_ypos, newXPOS + bar_wide + score_wide + score_offset, bar_ypos + score_tall);

	V_swprintf_safe(unicode, L"%i", iHumanScore);
	iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
	surface()->DrawSetTextPos(newXPOS - score_offset - (score_wide / 2) - (iLen / 2), bar_ypos + (score_tall / 2) - (surface()->GetFontTall(m_hTextFontDef) / 2));
	surface()->DrawPrintText(unicode, wcslen(unicode));

	V_swprintf_safe(unicode, L"%i", iZombieScore);
	iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
	surface()->DrawSetTextPos(newXPOS + bar_wide + score_offset + (score_wide / 2) - (iLen / 2), bar_ypos + (score_tall / 2) - (surface()->GetFontTall(m_hTextFontDef) / 2));
	surface()->DrawPrintText(unicode, wcslen(unicode));

	newXPOS = (((float)xWide / 2) - (time_wide / 2)) + time_xpos;
	surface()->DrawSetTexture(m_nTextureTimeLeft);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawTexturedRect(newXPOS, time_ypos, newXPOS + time_wide, time_tall + time_ypos);

	surface()->DrawSetColor(baseTimerColor);
	surface()->DrawSetTextFont(m_hTextFontDef);
	surface()->DrawSetTextColor(baseTimerColor);

	V_swprintf_safe(unicode, L"%d:%02d", (timer / 60), (timer % 60));
	iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
	surface()->DrawSetTextPos(newXPOS + (time_wide / 2) - (iLen / 2), time_ypos + (time_tall / 2) - (surface()->GetFontTall(m_hTextFontDef) / 2));
	surface()->DrawPrintText(unicode, wcslen(unicode));
}

//------------------------------------------------------------------------
// Purpose: Handles the background...
//------------------------------------------------------------------------
void CHudRoundStatus::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
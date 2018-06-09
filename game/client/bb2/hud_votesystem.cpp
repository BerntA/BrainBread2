//========= Copyright Reperio-Studios Bernt Andreas Eide, All rights reserved. ============//
//
// Purpose: Vote System.
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include "c_bb2_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudVoteSystem : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudVoteSystem, vgui::Panel);
public:
	CHudVoteSystem(const char *pElementName);

	void Init(void);
	void VidInit(void);
	void Reset(void);
	bool ShouldDraw(void);
	void Paint(void);
	void PaintBackground();
	void ApplySchemeSettings(vgui::IScheme *scheme);
	void FireGameEvent(IGameEvent * event);

private:

	int m_nTextureAnswer[2];

	CHudTexture *m_pStopWatchBG;
	CHudTexture *m_pStopWatchFG;

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "Default");

	// Positions
	CPanelAnimationVarAliasType(float, divider_xpos, "divider_xpos", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, divider_wide, "divider_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, divider_tall, "divider_tall", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, divider_ypos_1, "divider_ypos_1", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, divider_ypos_2, "divider_ypos_2", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, vote_header_x, "vote_header_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, vote_header_y, "vote_header_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, vote_target_x, "vote_target_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, vote_target_y, "vote_target_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, yes_info_x, "yes_info_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, yes_info_y, "yes_info_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, no_info_x, "no_info_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, no_info_y, "no_info_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, yes_box_x, "yes_box_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, yes_box_y, "yes_box_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, no_box_x, "no_box_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, no_box_y, "no_box_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, vote_count_x, "vote_count_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, vote_count_y, "vote_count_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, vote_timer_x, "vote_timer_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, vote_timer_y, "vote_timer_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, checkbox_wide, "checkbox_wide", "12", "proportional_float");
	CPanelAnimationVarAliasType(float, checkbox_tall, "checkbox_tall", "12", "proportional_float");
	CPanelAnimationVar(float, checkbox_thick, "checkbox_thick", "2");

	wchar_t szVoteInfoString[80];
	wchar_t szVoteTargetString[80];
};

using namespace vgui;

DECLARE_HUDELEMENT(CHudVoteSystem);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudVoteSystem::CHudVoteSystem(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudVoteSystem")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	for (int i = 0; i < 2; i++)
		m_nTextureAnswer[i] = surface()->CreateNewTextureID();

	surface()->DrawSetTextureFile(m_nTextureAnswer[0], "vgui/hud/vote_yes", true, false);
	surface()->DrawSetTextureFile(m_nTextureAnswer[1], "vgui/hud/vote_no", true, false);

	m_pStopWatchBG = m_pStopWatchFG = NULL;

	SetHiddenBits(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVoteSystem::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVoteSystem::Init(void)
{
	ListenForGameEvent("votesys_start");
	ListenForGameEvent("votesys_end");
	ListenForGameEvent("round_end");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVoteSystem::VidInit(void)
{
	m_pStopWatchBG = gHUD.GetIcon("stopwatch_bg");
	m_pStopWatchFG = gHUD.GetIcon("stopwatch_fg");
}

void CHudVoteSystem::Reset(void)
{
}

bool CHudVoteSystem::ShouldDraw(void)
{
	bool bWantsDraw = (HL2MPRules() && g_PR && CHudElement::ShouldDraw() && g_pClientMode && g_pClientMode->GetViewportAnimationController());
	if (!bWantsDraw)
		return false;

	return (BB2PlayerGlobals->IsVotePanelActive() || g_pClientMode->GetViewportAnimationController()->IsPanelBeingAnimated(this));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVoteSystem::Paint()
{
	Color fgColor = GetFgColor();

	float flFontTall = (float)surface()->GetFontTall(m_hTextFont);
	surface()->DrawSetColor(fgColor);
	surface()->DrawFilledRect(divider_xpos, divider_ypos_1, divider_xpos + divider_wide, divider_ypos_1 + divider_tall);
	surface()->DrawFilledRect(divider_xpos, divider_ypos_2, divider_xpos + divider_wide, divider_ypos_2 + divider_tall);

	Color baseColor = Color(30, 30, 240, 255);
	baseColor[3] = fgColor[3];
	if (BB2PlayerGlobals->GetPlayerVoteResponse() == 1) // YES
	{
		surface()->DrawSetColor(baseColor);
		surface()->DrawFilledRect(divider_xpos, yes_info_y, divider_xpos + divider_wide, yes_info_y + flFontTall);
	}
	else if (BB2PlayerGlobals->GetPlayerVoteResponse() == 2) // NO
	{
		surface()->DrawSetColor(baseColor);
		surface()->DrawFilledRect(divider_xpos, no_info_y, divider_xpos + divider_wide, no_info_y + flFontTall);
	}

	surface()->DrawSetColor(fgColor);
	surface()->DrawSetTextColor(fgColor);
	surface()->DrawSetTextFont(m_hTextFont);

	wchar_t unicode[80];

	surface()->DrawSetTextPos(vote_header_x, vote_header_y);
	surface()->DrawUnicodeString(szVoteInfoString);

	surface()->DrawSetTextPos(vote_target_x, vote_target_y);
	surface()->DrawUnicodeString(szVoteTargetString);

	g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), g_pVGuiLocalize->Find("#VOTE_INFO_YES"), 0);
	surface()->DrawSetTextPos(yes_info_x, yes_info_y);
	surface()->DrawUnicodeString(unicode);

	g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), g_pVGuiLocalize->Find("#VOTE_INFO_NO"), 0);
	surface()->DrawSetTextPos(no_info_x, no_info_y);
	surface()->DrawUnicodeString(unicode);

	DrawHollowBox(yes_box_x, yes_box_y, checkbox_wide, checkbox_tall, fgColor, 1.0f, checkbox_thick, checkbox_thick);
	DrawHollowBox(no_box_x, no_box_y, checkbox_wide, checkbox_tall, fgColor, 1.0f, checkbox_thick, checkbox_thick);

	surface()->DrawSetColor(fgColor);
	surface()->DrawSetTextColor(fgColor);
	surface()->DrawSetTextFont(m_hTextFont);

	wchar_t argument1[16];

	V_swprintf_safe(argument1, L"%i", HL2MPRules()->m_iCurrentYesVotes);
	surface()->DrawSetTextPos(yes_box_x + checkbox_wide + XRES(6), yes_box_y + (checkbox_tall / 2) - (surface()->GetFontTall(m_hTextFont) / 2));
	surface()->DrawUnicodeString(argument1);

	V_swprintf_safe(argument1, L"%i", HL2MPRules()->m_iCurrentNoVotes);
	surface()->DrawSetTextPos(no_box_x + checkbox_wide + XRES(6), no_box_y + (checkbox_tall / 2) - (surface()->GetFontTall(m_hTextFont) / 2));
	surface()->DrawUnicodeString(argument1);

	g_pVGuiLocalize->ConstructString(unicode, sizeof(unicode), g_pVGuiLocalize->Find("#VOTE_INFO_COUNT"), 0);
	surface()->DrawSetTextPos(vote_count_x, vote_count_y);
	surface()->DrawUnicodeString(unicode);

	surface()->DrawSetColor(fgColor);
	surface()->DrawSetTexture(m_nTextureAnswer[0]);
	surface()->DrawTexturedRect(yes_box_x, yes_box_y, yes_box_x + checkbox_wide, yes_box_y + checkbox_tall);

	surface()->DrawSetColor(fgColor);
	surface()->DrawSetTexture(m_nTextureAnswer[1]);
	surface()->DrawTexturedRect(no_box_x, no_box_y, no_box_x + checkbox_wide, no_box_y + checkbox_tall);

	// Draw Time Left:
	float timeToTake = (HL2MPRules()->m_flTimeUntilVoteEnds - HL2MPRules()->m_flTimeVoteStarted);
	float timeLeft = MAX((HL2MPRules()->m_flTimeUntilVoteEnds - gpGlobals->curtime), 0.0f);
	float percentage = (timeLeft / timeToTake);
	percentage = 1.0f - clamp(percentage, 0.0f, 1.0f);

	CHudTexture *pStopwatchIcon = m_pStopWatchBG;
	if (pStopwatchIcon)
	{
		surface()->DrawSetColor(fgColor);
		surface()->DrawSetTexture(pStopwatchIcon->textureId);
		surface()->DrawTexturedRect(
			pStopwatchIcon->GetOrigPosX(),
			pStopwatchIcon->GetOrigPosY(),
			pStopwatchIcon->GetOrigPosX() + pStopwatchIcon->GetOrigWide(),
			pStopwatchIcon->GetOrigPosY() + pStopwatchIcon->GetOrigTall()
			);
	}

	pStopwatchIcon = m_pStopWatchFG;
	if (pStopwatchIcon)
		pStopwatchIcon->DrawCircularProgression(fgColor, pStopwatchIcon->GetOrigPosX(), pStopwatchIcon->GetOrigPosY(), pStopwatchIcon->GetOrigWide(), pStopwatchIcon->GetOrigTall(), percentage);

	surface()->DrawSetColor(fgColor);
	surface()->DrawSetTextColor(fgColor);
	surface()->DrawSetTextFont(m_hTextFont);

	V_swprintf_safe(argument1, L"%i", (int)timeLeft);
	surface()->DrawSetTextPos(vote_timer_x, vote_timer_y);
	surface()->DrawUnicodeString(argument1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVoteSystem::PaintBackground()
{
	SetBgColor(Color(10, 8, 12, 200));
	SetPaintBorderEnabled(true);
	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVoteSystem::FireGameEvent(IGameEvent *event)
{
	if (!g_PR || !HL2MPRules())
		return;

	const char *type = event->GetName();
	if (!strcmp(type, "votesys_start"))
	{
		BB2PlayerGlobals->SetVotePanelActive(true);

		int x, y;
		GetPos(x, y);
		SetPos(-GetWide(), y);
		SetAlpha(0);

		BB2PlayerGlobals->SetPlayerVoteResponse(0);
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 256.0f, 0.0f, 1.0f, AnimationController::INTERPOLATOR_LINEAR);
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "xpos", 0.0f, 0.0f, 0.5f, AnimationController::INTERPOLATOR_LINEAR);

		vgui::surface()->PlaySound("common/wpn_hudoff.wav");

		int voteType = event->GetInt("type");
		int voterIndex = event->GetInt("index");
		int kickBanIndex = event->GetInt("targetID");
		const char *newMap = event->GetString("mapName");

		if (voterIndex == GetLocalPlayerIndex())
			BB2PlayerGlobals->SetPlayerVoteResponse(1);

		const char *voterName = g_PR->GetPlayerName(voterIndex);
		wchar_t wszVoterName[32], wszTargetName[32];
		g_pVGuiLocalize->ConvertANSIToUnicode(voterName, wszVoterName, sizeof(wszVoterName));
		g_pVGuiLocalize->ConstructString(szVoteInfoString, sizeof(szVoteInfoString), g_pVGuiLocalize->Find("#VOTE_INFO_CALLER"), 1, wszVoterName);

		switch (voteType)
		{

		case 1:
		case 2:
		{
			const char *targetName = g_PR->GetPlayerName(kickBanIndex);
			wchar_t *token = g_pVGuiLocalize->Find("#VOTE_INFO_KICK");
			if (voteType == 2)
				token = g_pVGuiLocalize->Find("#VOTE_INFO_BAN");

			g_pVGuiLocalize->ConvertANSIToUnicode(targetName, wszTargetName, sizeof(wszTargetName));
			g_pVGuiLocalize->ConstructString(szVoteTargetString, sizeof(szVoteTargetString), token, 1, wszTargetName);
			break;
		}
		case 3:
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(newMap, wszTargetName, sizeof(wszTargetName));
			g_pVGuiLocalize->ConstructString(szVoteTargetString, sizeof(szVoteTargetString), g_pVGuiLocalize->Find("#VOTE_INFO_MAP"), 1, wszTargetName);
			break;
		}

		}
	}
	else if (!strcmp(type, "votesys_end") && BB2PlayerGlobals->IsVotePanelActive())
	{
		BB2PlayerGlobals->SetVotePanelActive(false);

		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, 1.0f, AnimationController::INTERPOLATOR_LINEAR);
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "xpos", -GetWide(), 0.0f, 0.5f, AnimationController::INTERPOLATOR_LINEAR);

		vgui::surface()->PlaySound("common/wpn_moveselect.wav");
	}
	else if (!strcmp(type, "round_end")) // Reset vote HUD!
	{
		BB2PlayerGlobals->SetVotePanelActive(false);
		BB2PlayerGlobals->SetPlayerVoteResponse(0);

		int x, y;
		GetPos(x, y);

		SetAlpha(0);
		SetPos(-GetWide(), y);
	}
}
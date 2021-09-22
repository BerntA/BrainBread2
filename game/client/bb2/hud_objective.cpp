//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective HUD - Gets data from the logic_objective & trigger_capturepoint entity.
//
//========================================================================================//

#include "cbase.h"
#include "hud_objective.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "hl2mp_gamerules.h"
#include "GameBase_Client.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ITEM_TRANSIT_TIME 1.0f

static CHudObjective *m_pObjectiveHUD = NULL;

DECLARE_HUDELEMENT(CHudObjective);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudObjective::CHudObjective(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudObjective")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);
	m_pObjectiveHUD = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudObjective::Init(void)
{
	ListenForGameEvent("objective_run");
	ListenForGameEvent("objective_update");
	ListenForGameEvent("round_end");
	ListenForGameEvent("round_start");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudObjective::VidInit(void)
{
	pszObjectiveItems.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one item in the list
//-----------------------------------------------------------------------------
bool CHudObjective::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && pszObjectiveItems.Count());
}

//------------------------------------------------------------------------
// Purpose: Set default layout
//-----------------------------------------------------------------------
void CHudObjective::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudObjective::Paint()
{
	if (pszObjectiveItems.Count() <= 0)
	{
		m_flYPos = 0.0f;
		return;
	}

	bool bIsArenaMode = false;
	int iObject = -1, iOtherObject = -1;
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (pClient && HL2MPRules())
	{
		bIsArenaMode = ((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) && (!GameBaseClient->IsViewPortPanelVisible(PANEL_INFO)));
		for (int i = (pszObjectiveItems.Count() - 1); i >= 0; i--)
		{
			if ((pszObjectiveItems[i].m_iTeam == pClient->GetTeamNumber() || (bIsArenaMode && (pszObjectiveItems[i].m_iTeam == TEAM_HUMANS))) && (iObject == -1))
				iObject = i;

			if (pszObjectiveItems[i].m_iTeam != pClient->GetTeamNumber() && (iOtherObject == -1))
				iOtherObject = i;
		}
	}

	if (iObject == -1)
	{
		m_flYPos = 0.0f;
		SetAlpha(0);
		iObject = iOtherObject;
		if (iObject == -1)
			return;
	}
	else
		SetAlpha(255);

	float flFraction = 1.0f;
	if (!HasObjectiveFlag(iObject, OBJ_FADED_IN))
	{
		pszObjectiveItems[iObject].m_flLerp = 1000.0f;
		AddObjectiveFlag(iObject, OBJ_FADED_IN);
	}

	if (HasObjectiveFlag(iObject, OBJ_IS_FINISHED) && !HasObjectiveFlag(iObject, OBJ_FADING_OUT))
	{
		pszObjectiveItems[iObject].m_flLerp = 1000.0f;
		AddObjectiveFlag(iObject, OBJ_FADING_OUT);
	}

	bool bFadeOut = HasObjectiveFlag(iObject, OBJ_FADING_OUT);
	if (HasObjectiveFlag(iObject, OBJ_FADING_OUT) || (HasObjectiveFlag(iObject, OBJ_FADED_IN) && !HasObjectiveFlag(iObject, OBJ_IS_VISIBILE)))
	{
		float flInterpolation = (MAX(0.0f, 1000.0f - pszObjectiveItems[iObject].m_flLerp)) * 0.001f;
		if (flInterpolation >= ITEM_TRANSIT_TIME)
		{
			if (HasObjectiveFlag(iObject, OBJ_IS_FINISHED))
			{
				pszObjectiveItems.Remove(iObject);
				return;
			}
			else if (HasObjectiveFlag(iObject, OBJ_FADED_IN))
				AddObjectiveFlag(iObject, OBJ_IS_VISIBILE);
		}
		else
		{
			flFraction = SimpleSpline(flInterpolation / ITEM_TRANSIT_TIME);
			if (bFadeOut)
				flFraction = 1.0f - SimpleSpline(flInterpolation / ITEM_TRANSIT_TIME);
		}
	}

	// If we're done then move the layout up and end the stuff.
	if ((pszObjectiveItems[iObject].m_iStatus == STATUS_SUCCESS) && !HasObjectiveFlag(iObject, OBJ_IS_FINISHED))
		AddObjectiveFlag(iObject, OBJ_IS_FINISHED);

	m_TextColorTim[3] = GetAlpha();
	m_TextColorObj[3] = GetAlpha();

	float flYPos = (-60.0f * (1.0f - flFraction));
	if (bIsArenaMode && pClient && pClient->IsObserver())
		flYPos = (-60.0f * (1.0f - flFraction)) + (YRES(40) * flFraction);

	surface()->DrawSetColor(m_TextColorObj);
	surface()->DrawSetTextFont(m_hObjFont);
	surface()->DrawSetTextColor(m_TextColorObj);

	// Sizes
	wchar_t unicode_long[80];
	wchar_t unicode_short[10];
	wchar_t sIDString[100];
	sIDString[0] = 0;

	V_swprintf_safe(unicode_short, L"%i", pszObjectiveItems[iObject].m_iKillsLeft);
	g_pVGuiLocalize->ConvertANSIToUnicode(pszObjectiveItems[iObject].szObjective, unicode_long, sizeof(unicode_long));
	g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), unicode_long, 1, unicode_short);

	bool bShouldDrawTime = (pszObjectiveItems[iObject].m_flTime > 0);

	// Dynamic Bounds:
	int iWide, iTall, xPos;
	iWide = UTIL_ComputeStringWidth(m_hObjFont, sIDString);
	iTall = surface()->GetFontTall(m_hObjFont);
	xPos = (GetWide() / 2) - (iWide / 2);

	// Draw objective text
	surface()->DrawSetTextPos(xPos, flYPos);
	surface()->DrawPrintText(sIDString, wcslen(sIDString));

	if (iObject != iOtherObject)
		m_flYPos = (flYPos + (iTall + scheme()->GetProportionalScaledValue(1)));

	// Do we want to draw the time left for this obj.?
	if (bShouldDrawTime)
	{
		surface()->DrawSetColor(m_TextColorTim);
		surface()->DrawSetTextFont(m_hTimFont);
		surface()->DrawSetTextColor(m_TextColorTim);

		int iTime = MAX(((int)(pszObjectiveItems[iObject].m_flTime - gpGlobals->curtime)), 0);

		V_swprintf_safe(unicode_long, L"Time: %d:%02d", (iTime / 60), (iTime % 60));

		int iStrLen = UTIL_ComputeStringWidth(m_hTimFont, unicode_long);
		surface()->DrawSetTextPos((GetWide() / 2) - (iStrLen / 2), flYPos + (iTall + scheme()->GetProportionalScaledValue(1)));
		surface()->DrawPrintText(unicode_long, wcslen(unicode_long));

		if (iObject != iOtherObject)
			m_flYPos += (surface()->GetFontTall(m_hTimFont) + scheme()->GetProportionalScaledValue(3));
	}

	float frame_msec = 1000.0f * gpGlobals->frametime;
	if (pszObjectiveItems[iObject].m_flLerp > 0)
	{
		pszObjectiveItems[iObject].m_flLerp -= frame_msec;
		if (pszObjectiveItems[iObject].m_flLerp < 0)
			pszObjectiveItems[iObject].m_flLerp = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudObjective::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

//-----------------------------------------------------------------------------
// Purpose: Check if we have the desired item in our item list.
//-----------------------------------------------------------------------------
ObjectiveItem_t *CHudObjective::GetObjectiveItem(int index)
{
	for (int i = 0; i < pszObjectiveItems.Count(); i++)
	{
		if (pszObjectiveItems[i].m_iIndex == index)
			return &pszObjectiveItems[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that we have to update or add an existing objective item.
//-----------------------------------------------------------------------------
void CHudObjective::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (!strcmp(type, "objective_run"))
	{
		int iIndex = event->GetInt("index"),
			iTeamLink = event->GetInt("team", TEAM_HUMANS),
			iStatus = event->GetInt("status"),
			iIconLocation = event->GetInt("icon_location"),
			iKillsLeft = event->GetInt("kills_left");
		float flTime = event->GetFloat("time");
		const char *szObjective = event->GetString("objective", "");
		const char *szIconTexture = event->GetString("icon_texture", "");
		bool bFullUpdate = event->GetBool("update");

		// Make sure we don't add a duplicate: However this may be a unique item if the mapper forgot to setup the indexes for his logic_objective's...
		ObjectiveItem_t *existingItem = GetObjectiveItem(iIndex);
		if (existingItem)
		{
			// We call objective_run when we start and end so we don't want to leave just yet, we want to set the stuff we can set:
			existingItem->m_iStatus = iStatus; // The status controls the state of the item, if it becomes finished we'll start animating iex : fade out, then fade in the next item.

			// Re-Scale Update...
			if (bFullUpdate)
			{
				existingItem->m_iKillsLeft = iKillsLeft;
				existingItem->m_flTime = flTime;
			}

			return;
		}

		// Create our new obj. item!
		ObjectiveItem_t objItem;
		objItem.m_iIndex = iIndex;
		objItem.m_iTeam = iTeamLink;
		objItem.m_iStatus = iStatus;
		objItem.m_iIconLocationIndex = iIconLocation;
		objItem.m_iKillsLeft = iKillsLeft;
		objItem.m_flTime = flTime;
		objItem.m_nDisplayFlags = 0;
		objItem.m_flLerp = 0.0f;
		Q_strncpy(objItem.szObjective, szObjective, 80);
		Q_strncpy(objItem.szIconTexture, szIconTexture, 80);

		pszObjectiveItems.AddToHead(objItem);
	}
	else if (!strcmp(type, "objective_update"))
	{
		int iIndex = event->GetInt("index"),
			iKillsLeft = event->GetInt("kills_left");

		ObjectiveItem_t *existingItem = GetObjectiveItem(iIndex);
		if (existingItem == NULL)
		{
			Warning("Tried to update a non existant objective item!\n");
			return;
		}

		// Update the kills left and the string is drawn in the paint function so it will be updated accordingly.
		existingItem->m_iKillsLeft = iKillsLeft;
	}
	else if (!strcmp(type, "round_end") || !strcmp(type, "round_start")) // Reset / Refresh :
	{
		pszObjectiveItems.Purge();
		engine->ClientCmd_Unrestricted("r_cleardecals\n"); // Remove decals.
	}
}

bool CHudObjective::HasObjectiveFlag(int index, int nFlag)
{
	if (!pszObjectiveItems.Count())
		return false;

	if (index >= pszObjectiveItems.Count() || index < 0)
		return false;

	return ((pszObjectiveItems[index].m_nDisplayFlags & nFlag) != 0);
}

void CHudObjective::AddObjectiveFlag(int index, int nFlag)
{
	if (!pszObjectiveItems.Count())
		return;

	if (index >= pszObjectiveItems.Count() || index < 0)
		return;

	pszObjectiveItems[index].m_nDisplayFlags |= nFlag;
}

//-----------------------------------------------------------------------------
// Purpose: Capture Progress HUD
//-----------------------------------------------------------------------------
DECLARE_HUDELEMENT(CHudCaptureProgressBar);
DECLARE_HUD_MESSAGE(CHudCaptureProgressBar, CapturePointProgress);

CHudCaptureProgressBar::CHudCaptureProgressBar(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudCaptureProgress")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

void CHudCaptureProgressBar::Init(void)
{
	HOOK_HUD_MESSAGE(CHudCaptureProgressBar, CapturePointProgress);
	Reset();
}

void CHudCaptureProgressBar::VidInit(void)
{
}

bool CHudCaptureProgressBar::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && m_bShouldDrawProgress);
}

void CHudCaptureProgressBar::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
	m_bShouldDrawProgress = false;
	m_flProgressPercent = 0.0f;
}

void CHudCaptureProgressBar::Paint()
{
	int x, y;
	GetPos(x, y);

	CHudObjective *pElement = m_pObjectiveHUD;
	if (pElement)
		y = (pElement->GetYPosOffset() + YRES(2));

	SetPos(x, y);

	// Draw Progress:	
	surface()->DrawSetColor(m_BarColor);
	surface()->DrawFilledRect(0, 0, (int)(((float)GetWide()) * m_flProgressPercent), GetTall());
	DrawHollowBox(0, 0, GetWide(), GetTall(), GetFgColor(), 1.0f, 1, 1);

	surface()->DrawSetColor(m_TextColor);
	surface()->DrawSetTextColor(m_TextColor);
	surface()->DrawSetTextFont(m_hDefaultFont);

	wchar_t unicodeMessage[32];
	g_pVGuiLocalize->ConvertANSIToUnicode("SECURING", unicodeMessage, sizeof(unicodeMessage));

	int txtWide, txtTall;
	surface()->GetTextSize(m_hDefaultFont, unicodeMessage, txtWide, txtTall);

	surface()->DrawSetTextPos((GetWide() / 2) - (txtWide / 2), (GetTall() / 2) - (txtTall / 2));
	surface()->DrawPrintText(unicodeMessage, wcslen(unicodeMessage));
}

void CHudCaptureProgressBar::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CHudCaptureProgressBar::MsgFunc_CapturePointProgress(bf_read &msg)
{
	m_bShouldDrawProgress = msg.ReadByte();
	m_flProgressPercent = msg.ReadFloat();
}
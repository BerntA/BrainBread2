//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: 3D NPC Health Bars, handles all health bars for npcs
//
//========================================================================================//

#include "cbase.h"
#include "hud_npc_health_bar.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "hl2mp_gamerules.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_playerresource.h"
#include "GameBase_Shared.h"
#include "GameBase_Client.h"
#include "c_ai_basenpc.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

DECLARE_HUDELEMENT_DEPTH(CHudNPCHealthBar, 90);

static CHudNPCHealthBar *pHealthBar = NULL;
CHudNPCHealthBar *GetHealthBarHUD() { return pHealthBar; }

#define FADE_TIME 0.25f
#define DIST_START_FADE_PERCENT 1.2f // When we stray X% away from the dist required, start fading!

//------------------------------------------------------------------------
// Purpose: Constructor 
//------------------------------------------------------------------------
CHudNPCHealthBar::CHudNPCHealthBar(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudNPCHealthBar")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_nTextureBackground = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTextureBackground, "vgui/hud/npc_health_bar", true, false);

	m_nTexture_Bar = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTexture_Bar, "vgui/hud/random_bar_1", true, false);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);

	pHealthBar = this;
}

CHudNPCHealthBar::~CHudNPCHealthBar()
{
	Cleanup();
	pHealthBar = NULL;
}

//------------------------------------------------------------------------
// Purpose: Initialize = Start 
//------------------------------------------------------------------------
void CHudNPCHealthBar::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose: Initialize = Start 
//------------------------------------------------------------------------
void CHudNPCHealthBar::VidInit(void)
{
}

//------------------------------------------------------------------------
// Purpose: Reset - Constructor - Spawn similar stuff...
//-----------------------------------------------------------------------
void CHudNPCHealthBar::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

bool CHudNPCHealthBar::ShouldDraw(void)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return false;

	return (CHudElement::ShouldDraw() && pszNPCHealthBarList.Count() && !pClient->IsInVGuiInputMode());
}

//------------------------------------------------------------------------
// Purpose: Handles the drawing and setting text itself...
//------------------------------------------------------------------------
void CHudNPCHealthBar::Paint()
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return;

	Vector localPlayerPos = pClient->GetLocalOrigin();

	int itemCount = pszNPCHealthBarList.Count();
	for (int i = (itemCount - 1); i >= 0; i--)
	{
		const HealthBarItem_t *item = &pszNPCHealthBarList[i];
		C_AI_BaseNPC *pObject = dynamic_cast<C_AI_BaseNPC*> (ClientEntityList().GetBaseEntity(item->index));
		if (pObject == NULL) // Shouldn't happen!
			continue;

		if (pObject->IsDormant() || !pObject->ShouldDraw() || ((pObject->m_nRenderFX == kRenderFxNone) && (pObject->GetRenderColor().a < 255)))
			continue;

		float flAlpha = clamp(((gpGlobals->curtime - item->flTime) / FADE_TIME), 0.0f, 1.0f) * 255.0f;

		int health = MAX(pObject->m_iHealth, 0);
		int healthMax = MAX(pObject->m_iMaxHealth, 0);
		bool bIsBoss = pObject->IsBoss();

		// Do we allow hp bars for non-bosses?
		if (!bIsBoss && !bb2_enable_healthbar_for_all.GetBool())
			continue;

		Vector vecNewPos = pObject->GetLocalOrigin();

		// Dynamically change the alpha value, based on dist from npc.
		if (!((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) && bIsBoss) && ((gpGlobals->curtime - item->flTime) >= FADE_TIME))
		{
			float distFromNPC = localPlayerPos.DistTo(vecNewPos);
			float minDistance = MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_REGULAR;
			float range = (MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_REGULAR * DIST_START_FADE_PERCENT) - minDistance;

			if (bIsBoss && (distFromNPC > MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_BOSS))
			{
				minDistance = MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_BOSS;
				range = (MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_BOSS * DIST_START_FADE_PERCENT) - minDistance;
				flAlpha = 255.0f * (1.0f - clamp(((distFromNPC - minDistance) / range), 0.0, 1.0f));
			}
			else if (!bIsBoss && (distFromNPC > MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_REGULAR))
				flAlpha = 255.0f * (1.0f - clamp(((distFromNPC - minDistance) / range), 0.0, 1.0f));
		}

		int iAlpha = ((int)flAlpha);
		if (iAlpha <= 0)
			continue;

		wchar_t unicode[32];
		vecNewPos.z += (item->vecMaxs.z - item->vecMins.z) + 6.0f;

		int xpos, ypos;
		GetVectorInScreenSpace(vecNewPos, xpos, ypos, NULL);

		xpos -= (bg_w / 2); // Move the health bar panel to be displayed in the middle of the npcs bounds. / pos above the head.

		surface()->DrawSetColor(Color(255, 255, 255, iAlpha));
		surface()->DrawSetTexture(m_nTextureBackground);
		surface()->DrawTexturedRect(xpos, ypos, bg_w + xpos, bg_h + ypos);

		// Draw Text
		surface()->DrawSetColor(Color(255, 255, 255, iAlpha));
		surface()->DrawSetTextFont(m_hTextFontDef);
		surface()->DrawSetTextColor(Color(255, 255, 255, iAlpha));

		g_pVGuiLocalize->ConvertANSIToUnicode(pObject->GetNPCName(), unicode, sizeof(unicode));
		int iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
		iLen = (bg_w / 2) - (iLen / 2);

		surface()->DrawSetTextPos(xpos + iLen, ypos);
		surface()->DrawUnicodeString(unicode);

		ypos += ((int)bar_y);

		float flScale = 0.0f;
		surface()->DrawSetColor(Color(0, 255, 0, iAlpha));
		surface()->DrawSetTexture(m_nTexture_Bar);
		flScale = ((bg_w / ((float)healthMax)) * ((float)health));
		surface()->DrawTexturedRect(xpos, ypos, xpos + ((int)flScale), ypos + bar_h);

		// Draw Text:
		surface()->DrawSetColor(Color(255, 255, 255, iAlpha));
		surface()->DrawSetTextFont(m_hTextFontDef);
		surface()->DrawSetTextColor(Color(255, 255, 255, iAlpha));

		V_swprintf_safe(unicode, L"%i / %i", health, healthMax);
		iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
		iLen = (bg_w / 2) - (iLen / 2);

		surface()->DrawSetTextPos(xpos + iLen, ypos);
		surface()->DrawPrintText(unicode, wcslen(unicode));
	}
}

//------------------------------------------------------------------------
// Purpose: Add an item to our health bar item list.
//------------------------------------------------------------------------
void CHudNPCHealthBar::AddHealthBarItem(C_BaseEntity *pEntity, int index, bool bIsBoss)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return;

	float flTime = gpGlobals->curtime;
	float distFromNPC = pClient->GetLocalOrigin().DistTo(pEntity->GetLocalOrigin());
	if ((!((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) && bIsBoss)) &&
		((bIsBoss && (distFromNPC > MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_BOSS)) || (!bIsBoss && (distFromNPC > MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_REGULAR))))
		flTime = 0.0f;

	HealthBarItem_t pItem;
	pItem.index = index;
	pItem.flTime = flTime;
	pEntity->GetRenderBounds(pItem.vecMins, pItem.vecMaxs);

	pszNPCHealthBarList.AddToTail(pItem);
}

void CHudNPCHealthBar::RemoveHealthBarItem(int index)
{
	for (int i = (pszNPCHealthBarList.Count() - 1); i >= 0; i--)
	{
		if (pszNPCHealthBarList[i].index == index)
			pszNPCHealthBarList.Remove(i);
	}
}

void CHudNPCHealthBar::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}
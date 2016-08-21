//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: 3D NPC Health Bars, handles all health bars for npcs
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_npc_health_bar.h"
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
#include "GameBase_Shared.h"
#include "GameBase_Client.h"
#include "vgui/ILocalize.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

DECLARE_HUDELEMENT_DEPTH(CHudNPCHealthBar, 90);

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

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);

	// Hide only if the player is dead...
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING);
}

//------------------------------------------------------------------------
// Purpose: Initialize = Start 
//------------------------------------------------------------------------
void CHudNPCHealthBar::Init()
{
	ListenForGameEvent("round_started");
	Reset();
}

//------------------------------------------------------------------------
// Purpose: Initialize = Start 
//------------------------------------------------------------------------
void CHudNPCHealthBar::VidInit(void)
{
	pszNPCHealthBarList.Purge();
}

//------------------------------------------------------------------------
// Purpose: Reset - Constructor - Spawn similar stuff...
//-----------------------------------------------------------------------
void CHudNPCHealthBar::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(0);
	m_bShouldRender = false;
	pszNPCHealthBarList.Purge();
}

//------------------------------------------------------------------------
// Purpose: Handles think function, generic intervals that is...
//------------------------------------------------------------------------
void CHudNPCHealthBar::OnThink(void)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return;

	bool bIsWithinRange = false;

	for (int i = (pszNPCHealthBarList.Count() - 1); i >= 0; i--)
	{
		C_BaseEntity *pObject = ClientEntityList().GetBaseEntity(pszNPCHealthBarList[i].m_iEntIndex);
		if (!pObject)
			pszNPCHealthBarList.Remove(i); // Apparently this npc is dead so we remove it!
		else if (!pObject->IsNPC() || pObject->IsDormant())
			pszNPCHealthBarList.Remove(i); // These entities are supposed to be npcs!!!
		else
		{
			// Are we within distance?
			bool bCanShow = false;
			float flDistance = pClient->GetAbsOrigin().DistTo(pObject->GetAbsOrigin());

			if (pszNPCHealthBarList[i].m_bIsBoss)
			{
				if (flDistance < MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_BOSS)
					bCanShow = true;
			}
			else
			{
				if (flDistance < MIN_DISTANCE_TO_DRAW_HEALTHBAR_HUD_REGULAR)
					bCanShow = true;
			}

			// Boss Health Bars will be visible all the time in arena mode.
			if ((HL2MPRules()->GetCurrentGamemode() == MODE_ARENA) && pszNPCHealthBarList[i].m_bIsBoss)
				bCanShow = true;

			if (bCanShow)
				bIsWithinRange = true;

			pszNPCHealthBarList[i].m_bShouldShow = bCanShow;
		}
	}

	if (bIsWithinRange)
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
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, 0.4f, AnimationController::INTERPOLATOR_LINEAR);
		}
	}
}

//------------------------------------------------------------------------
// Purpose: Handles the drawing and setting text itself...
//------------------------------------------------------------------------
void CHudNPCHealthBar::Paint()
{
	int w, h;
	GetHudSize(w, h);

	if (GetWide() != w)
		SetWide(w);

	if (GetTall() != h)
		SetTall(h);

	for (int i = 0; i < pszNPCHealthBarList.Count(); i++)
	{
		// Do we allow hp bars for non bosses?
		if (!pszNPCHealthBarList[i].m_bIsBoss)
		{
			if (!bb2_enable_healthbar_for_all.GetBool())
			{
				pszNPCHealthBarList[i].m_bShouldShow = false;

				if (!pszNPCHealthBarList[i].m_bDisplaying)
					continue;
			}
		}

		C_BaseEntity *pObject = ClientEntityList().GetBaseEntity(pszNPCHealthBarList[i].m_iEntIndex);
		if (!pObject)
			continue;

		if (!pszNPCHealthBarList[i].m_bShouldShow && pszNPCHealthBarList[i].m_bDisplaying && !pszNPCHealthBarList[i].m_bFadingOut)
		{
			pszNPCHealthBarList[i].m_bFadingOut = true;
			pszNPCHealthBarList[i].m_flLerp = 1000.0f;
		}
		else if (pszNPCHealthBarList[i].m_bShouldShow && !pszNPCHealthBarList[i].m_bDisplaying && !pszNPCHealthBarList[i].m_bFadingIn)
		{
			pszNPCHealthBarList[i].m_bFadingIn = true;
			pszNPCHealthBarList[i].m_flLerp = 1000.0f;
		}

		float m_flAlpha = 0.0f;

		if (pszNPCHealthBarList[i].m_bDisplaying)
			m_flAlpha = 255;

		if (pszNPCHealthBarList[i].m_bFadingIn || pszNPCHealthBarList[i].m_bFadingOut)
		{
			float flMilli = MAX(0.0f, 1000.0f - pszNPCHealthBarList[i].m_flLerp);
			float flSec = flMilli * 0.001f;
			if ((flSec >= 0.6))
			{
				if (pszNPCHealthBarList[i].m_bFadingIn)
				{
					pszNPCHealthBarList[i].m_bFadingIn = false;
					pszNPCHealthBarList[i].m_bDisplaying = true;
					m_flAlpha = 255;
				}
				else if (pszNPCHealthBarList[i].m_bFadingOut)
				{
					pszNPCHealthBarList[i].m_bFadingOut = false;
					pszNPCHealthBarList[i].m_bDisplaying = false;
					m_flAlpha = 0;
				}
			}
			else
			{
				float flFrac = SimpleSpline(flSec / 0.6);
				m_flAlpha = (pszNPCHealthBarList[i].m_bFadingIn ? flFrac : (1.0f - flFrac)) * 255;
			}
		}

		wchar_t unicode[32];
		Vector vecNewPos = pObject->GetAbsOrigin();
		vecNewPos.z += 90;

		int xpos, ypos;
		GetVectorInScreenSpace(vecNewPos, xpos, ypos, NULL);

		xpos -= (bg_w / 2); // Move the health bar panel to be displayed in the middle of the npcs bounds. / pos above the head.

		surface()->DrawSetColor(Color(255, 255, 255, (int)m_flAlpha));
		surface()->DrawSetTexture(m_nTextureBackground);
		surface()->DrawTexturedRect(xpos, ypos, bg_w + xpos, bg_h + ypos);

		// Draw Text
		surface()->DrawSetColor(Color(255, 255, 255, (int)m_flAlpha));
		surface()->DrawSetTextFont(m_hTextFontDef);
		surface()->DrawSetTextColor(Color(255, 255, 255, (int)m_flAlpha));

		g_pVGuiLocalize->ConvertANSIToUnicode(pszNPCHealthBarList[i].szName, unicode, sizeof(unicode));
		int iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
		iLen = (bg_w / 2) - (iLen / 2);

		vgui::surface()->DrawSetTextPos(xpos + iLen, ypos);
		surface()->DrawUnicodeString(unicode);

		ypos += (int)bar_y;

		float flScale = 0.0f;
		surface()->DrawSetColor(Color(0, 255, 0, (int)m_flAlpha));
		surface()->DrawSetTexture(m_nTexture_Bar);
		flScale = ((bg_w / (float)pszNPCHealthBarList[i].m_iMaxHealth) * (float)pszNPCHealthBarList[i].m_iCurrentHealth);
		surface()->DrawTexturedRect(xpos, ypos, xpos + (int)flScale, ypos + bar_h);

		// Draw Text:
		surface()->DrawSetColor(Color(255, 255, 255, (int)m_flAlpha));
		surface()->DrawSetTextFont(m_hTextFontDef);
		surface()->DrawSetTextColor(Color(255, 255, 255, (int)m_flAlpha));

		// Never go below 0.
		int iCurrHP = pszNPCHealthBarList[i].m_iCurrentHealth;

		if (iCurrHP < 0)
			iCurrHP = 0;

		V_swprintf_safe(unicode, L"%i / %i", iCurrHP, pszNPCHealthBarList[i].m_iMaxHealth);
		iLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
		iLen = (bg_w / 2) - (iLen / 2);

		vgui::surface()->DrawSetTextPos(xpos + iLen, ypos);
		surface()->DrawPrintText(unicode, wcslen(unicode));

		// Update fade timer.
		float frame_msec = 1000.0f * gpGlobals->frametime;

		if (pszNPCHealthBarList[i].m_flLerp > 0)
		{
			pszNPCHealthBarList[i].m_flLerp -= frame_msec;
			if (pszNPCHealthBarList[i].m_flLerp < 0)
				pszNPCHealthBarList[i].m_flLerp = 0;
		}
	}
}

//------------------------------------------------------------------------
// Purpose: Add an item to our health bar item list.
//------------------------------------------------------------------------
void CHudNPCHealthBar::AddHealthBarItem(int index, bool bIsBoss, int currHealth, int maxHealth, const char *name)
{
	int iIndex = index;
	bool bBoss = bIsBoss;
	int iCurrHP = currHealth;
	int iTotalHP = maxHealth;

	int iItemID = GetItem(iIndex);
	if (iItemID >= 0)
	{
		pszNPCHealthBarList[iItemID].m_iCurrentHealth = iCurrHP;
		pszNPCHealthBarList[iItemID].m_iMaxHealth = iTotalHP;
		return;
	}

	C_BaseEntity *pObject = ClientEntityList().GetBaseEntity(iIndex);
	if (!pObject)
	{
		DevMsg(2, "Tried to add a health bar item for an npc that doesn't exist!\n");
		return;
	}

	HealthBarItem_t pItem;
	pItem.m_bIsBoss = bBoss;
	pItem.m_iCurrentHealth = iCurrHP;
	pItem.m_iEntIndex = iIndex;
	pItem.m_iMaxHealth = iTotalHP;
	pItem.m_bShouldShow = false;
	pItem.m_bDisplaying = false;
	pItem.m_bFadingIn = false;
	pItem.m_bFadingOut = false;
	pItem.m_flLerp = 1000.0f;
	Q_strncpy(pItem.szName, name, MAX_PLAYER_NAME_LENGTH);

	pszNPCHealthBarList.AddToTail(pItem);
}

int CHudNPCHealthBar::GetItem(int index)
{
	for (int i = 0; i < pszNPCHealthBarList.Count(); i++)
	{
		if (pszNPCHealthBarList[i].m_iEntIndex == index)
			return i;
	}

	return -1;
}

void CHudNPCHealthBar::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (!strcmp(type, "round_started"))
	{
		pszNPCHealthBarList.Purge();
	}
}

void CHudNPCHealthBar::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
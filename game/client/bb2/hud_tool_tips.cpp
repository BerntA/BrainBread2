//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays hints & tool tips in a nice/fancy manner.
//
//========================================================================================//

#include "cbase.h" 
#include "hud.h" 
#include "hud_macros.h" 
#include "c_baseplayer.h"
#include "iclientmode.h" 
#include "vgui/ISurface.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "vgui_controls/AnimationController.h"
#include "basecombatweapon_shared.h"
#include <vgui/ILocalize.h>
#include "c_hl2mp_player.h"
#include "tier0/memdbgon.h" 

using namespace vgui;

// Content in our tool tip
struct TipItem
{
	char szMessage[128];
	CHudTexture *icon;
	int messageLength;
	float m_flLerp;
	float m_flDisplayTime;
	bool m_bFadeIn;
	bool m_bFadeOut;
	bool m_bDraw;
};

//-----------------------------------------------------------------------------
// Purpose: Fade in tool tips & messages from the map/server/client funcs...
//-----------------------------------------------------------------------------
class CHudToolTips : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudToolTips, vgui::Panel);

public:
	CHudToolTips(const char * pElementName);

	void Init(void);
	void VidInit(void);
	void ApplySchemeSettings(vgui::IScheme *scheme);
	bool ShouldDraw(void);

	void MsgFunc_ToolTip(bf_read &msg);

protected:
	void Paint();

private:
	CUtlVector<TipItem> m_ToolTips; // Shows for the amount of time before transcending the items to the remove list.

	CPanelAnimationVarAliasType(float, m_flLineHeight, "LineHeight", "17", "proportional_float");
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "Default");
};

DECLARE_HUDELEMENT_DEPTH(CHudToolTips, 1);
DECLARE_HUD_MESSAGE(CHudToolTips, ToolTip);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudToolTips::CHudToolTips(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudToolTips")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudToolTips::ApplySchemeSettings(IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudToolTips::Init()
{
	HOOK_HUD_MESSAGE(CHudToolTips, ToolTip);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudToolTips::VidInit(void)
{
	m_ToolTips.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one tip to show.
//-----------------------------------------------------------------------------
bool CHudToolTips::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && (m_ToolTips.Count()));
}

//------------------------------------------------------------------------
// Purpose: Draw Items (tips)
//------------------------------------------------------------------------
void CHudToolTips::Paint()
{
	for (int i = (m_ToolTips.Count() - 1); i >= 0; i--)
	{
		int m_iPosX = 0;
		int m_iAlpha = 255;

		if (m_ToolTips[i].m_bFadeIn || m_ToolTips[i].m_bFadeOut)
		{
			float flMilli = MAX(0.0f, 1000.0f - m_ToolTips[i].m_flLerp);
			float flSec = flMilli * 0.001f;
			if ((flSec >= 1.0))
			{
				if (m_ToolTips[i].m_bFadeIn)
				{
					m_ToolTips[i].m_bFadeIn = false;
					m_ToolTips[i].m_bDraw = true;
					m_ToolTips[i].m_flDisplayTime = gpGlobals->curtime + 2; // display for 2 sec.
				}

				if (m_ToolTips[i].m_bFadeOut)
				{
					m_ToolTips[i].m_bFadeOut = false;
					m_ToolTips.Remove(i);
					continue;
				}
			}
			else
			{
				float flFrac = SimpleSpline(flSec / 1.0);
				m_iAlpha = ((m_ToolTips[i].m_bFadeIn ? flFrac : (1.0f - flFrac)) * 255);
				m_iPosX = -(m_ToolTips[i].messageLength * (m_ToolTips[i].m_bFadeIn ? 1.0f - flFrac : flFrac));
			}
		}

		if (m_ToolTips[i].m_bDraw)
		{
			if (m_ToolTips[i].m_flDisplayTime < gpGlobals->curtime)
			{
				m_ToolTips[i].m_flLerp = 1000.0f;
				m_ToolTips[i].m_bDraw = false;
				m_ToolTips[i].m_bFadeOut = true;
			}
		}

		CHudTexture *icon = m_ToolTips[i].icon;

		wchar_t message[256];
		g_pVGuiLocalize->ConvertANSIToUnicode(m_ToolTips[i].szMessage, message, sizeof(message));

		int y = (m_flLineHeight * ((m_ToolTips.Count() - 1) - i));
		int x = 0;

		// Draw the icon if it is not null...
		if (icon)
		{
			surface()->DrawSetColor(Color(255, 255, 255, m_iAlpha));
			surface()->DrawSetTexture(icon->textureId);
			surface()->DrawTexturedRect(m_iPosX, y, icon->Width() + m_iPosX, icon->Height() + y);
			x += icon->Width() + 1;
		}

		surface()->DrawSetColor(Color(255, 255, 255, m_iAlpha));
		surface()->DrawSetTextColor(Color(255, 255, 255, m_iAlpha));
		surface()->DrawSetTextPos(m_iPosX + x, y);
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawUnicodeString(message);

		float frame_msec = 1000.0f * gpGlobals->frametime;

		if (m_ToolTips[i].m_flLerp > 0)
		{
			m_ToolTips[i].m_flLerp -= frame_msec;
			if (m_ToolTips[i].m_flLerp < 0)
				m_ToolTips[i].m_flLerp = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show some tool tip!
//-----------------------------------------------------------------------------
void CHudToolTips::MsgFunc_ToolTip(bf_read &msg)
{
	int iListCount = m_ToolTips.Count();

	// Do we have too many messages in the queue?
	if (iListCount >= 4)
	{
		DevMsg("Overflowing Tip List!\nWait for available slot!\n");
		return;
	}

	int type = msg.ReadShort();
	const char *szIcon = "tip_default"; // Info Icon
	if (type == 1)
		szIcon = "tip_warning";

	char message[128];
	msg.ReadString(message, 128);

	char args[4][32];
	for (int i = 0; i < 4; i++)
		msg.ReadString(args[i], 32);

	char pchMessage[128];

	// Is this a localized msg?
	wchar_t *lookupToken = g_pVGuiLocalize->Find(message);
	if (lookupToken)
	{
		wchar_t wcMessage[128];
		wchar_t arg1[32], arg2[32], arg3[32], arg4[32];

		g_pVGuiLocalize->ConvertANSIToUnicode(args[0], arg1, sizeof(arg1));
		g_pVGuiLocalize->ConvertANSIToUnicode(args[1], arg2, sizeof(arg2));
		g_pVGuiLocalize->ConvertANSIToUnicode(args[2], arg3, sizeof(arg3));
		g_pVGuiLocalize->ConvertANSIToUnicode(args[3], arg4, sizeof(arg4));

		g_pVGuiLocalize->ConstructString(wcMessage, sizeof(wcMessage), lookupToken, 4, arg1, arg2, arg3, arg4);
		g_pVGuiLocalize->ConvertUnicodeToANSI(wcMessage, pchMessage, sizeof(wcMessage));
	}
	else
		Q_strncpy(pchMessage, message, 128);

	TipItem item_msg;
	Q_strncpy(item_msg.szMessage, pchMessage, 128);
	item_msg.icon = gHUD.GetIcon(szIcon);
	item_msg.messageLength = UTIL_ComputeStringWidth(m_hTextFont, pchMessage);
	item_msg.m_bDraw = false;
	item_msg.m_bFadeIn = true;
	item_msg.m_bFadeOut = false;
	item_msg.m_flDisplayTime = 0.0f;
	item_msg.m_flLerp = 1000.0f;
	m_ToolTips.AddToTail(item_msg);
}
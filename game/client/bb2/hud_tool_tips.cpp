//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays hints & tool tips in a nice/fancy manner.
//
//========================================================================================//

#include "cbase.h" 
#include "hud.h" 
#include "hudelement.h"
#include "hud_macros.h" 
#include "c_hl2mp_player.h"
#include "iclientmode.h" 
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h" 

using namespace vgui;

#define FADE_TIME 0.5f
#define STAY_TIME 2.0f

enum TipItemFlags_t
{
	TIP_ITEM_FLAG_FADEIN = 0x01,
	TIP_ITEM_FLAG_FADEOUT = 0x02,
	TIP_ITEM_FLAG_DRAW = 0x04,
};

struct TipItem
{
	char szMessage[128];
	CHudTexture *icon;
	int messageLength;
	int flags;
	float flTime;
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
	return (CHudElement::ShouldDraw() && m_ToolTips.Count());
}

//------------------------------------------------------------------------
// Purpose: Draw Items (tips)
//------------------------------------------------------------------------
void CHudToolTips::Paint()
{
	for (int i = (m_ToolTips.Count() - 1); i >= 0; i--)
	{
		TipItem *item = &m_ToolTips[i];

		int m_iPosX = 0;
		int m_iAlpha = 255;

		if (item->flags & (TIP_ITEM_FLAG_FADEIN | TIP_ITEM_FLAG_FADEOUT))
		{
			float fraction = clamp(((gpGlobals->curtime - item->flTime) / FADE_TIME), 0.0f, 1.0f);
			if (fraction >= 1.0f)
			{
				if (item->flags & TIP_ITEM_FLAG_FADEIN)
				{
					item->flags &= ~TIP_ITEM_FLAG_FADEIN;
					item->flags |= TIP_ITEM_FLAG_DRAW;
				}

				if (item->flags & TIP_ITEM_FLAG_FADEOUT)
				{
					item->flags &= ~TIP_ITEM_FLAG_FADEOUT;
					m_ToolTips.Remove(i);
					continue;
				}
			}
			else
			{
				m_iAlpha = (((item->flags & TIP_ITEM_FLAG_FADEIN) ? fraction : (1.0f - fraction)) * 255.0f);
				m_iPosX = -(item->messageLength * ((item->flags & TIP_ITEM_FLAG_FADEIN) ? (1.0f - fraction) : fraction));
			}
		}

		if (item->flags & TIP_ITEM_FLAG_DRAW)
		{
			if (gpGlobals->curtime >= (item->flTime + FADE_TIME + STAY_TIME))
			{
				item->flTime = gpGlobals->curtime;
				item->flags &= ~TIP_ITEM_FLAG_DRAW;
				item->flags |= TIP_ITEM_FLAG_FADEOUT;
			}
		}

		CHudTexture *icon = item->icon;
		Assert(icon != NULL);

		wchar_t message[256];
		g_pVGuiLocalize->ConvertANSIToUnicode(item->szMessage, message, sizeof(message));

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
	item_msg.flags = TIP_ITEM_FLAG_FADEIN;
	item_msg.flTime = gpGlobals->curtime;

	m_ToolTips.AddToTail(item_msg);
}
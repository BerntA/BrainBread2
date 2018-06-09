//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the name of nearby allies above their head. The color of the text indicates their current health state.
// Green = Good, Yellow = Hurt, Red = Near Death
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_hl2mp_player.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include "hl2mp_gamerules.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h" 

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudTargetIdentifier : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudTargetIdentifier, vgui::Panel);

public:

	CHudTargetIdentifier(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "Default");
};

DECLARE_HUDELEMENT_DEPTH(CHudTargetIdentifier, 90);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudTargetIdentifier::CHudTargetIdentifier(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudTargetIcon")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudTargetIdentifier::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudTargetIdentifier::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudTargetIdentifier::Paint()
{
	C_HL2MP_Player *pMe = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pMe)
		return;

	if (!pMe->IsAlive() || (pMe->GetTeamNumber() < TEAM_HUMANS))
		return;

	if (!HL2MPRules()->IsTeamplay())
		return;

	// Draw all player names if within range and visible:
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		C_HL2MP_Player *pClient = ToHL2MPPlayer(UTIL_PlayerByIndex(i));
		if (!pClient)
			continue;

		if (i == pMe->entindex())
			continue;

		if (!pClient->IsAlive())
			continue;

		if (pClient->GetTeamNumber() < TEAM_HUMANS)
			continue;

		if (pClient->GetTeamNumber() != pMe->GetTeamNumber())
			continue;

		if (pClient->IsDormant())
			continue;

		if (pClient->GetLocalOrigin().DistTo(pMe->GetLocalOrigin()) < MAX_TEAMMATE_DISTANCE)
		{
			int xpos, ypos, strLen;
			wchar_t unicode[256];
			g_pVGuiLocalize->ConvertANSIToUnicode(g_PR->GetPlayerName(i), unicode, sizeof(unicode));

			strLen = UTIL_ComputeStringWidth(m_hTextFontDef, unicode);
			Vector vecNewPos = pClient->GetLocalOrigin();
			vecNewPos.z += 80;

			GetVectorInScreenSpace(vecNewPos, xpos, ypos, NULL);

			xpos -= (strLen / 2);

			Color textColor;

			if (pClient->m_iHealth >= 60)
				textColor = Color(0, 255, 0, 255);
			else if (pClient->m_iHealth < 60 && pClient->m_iHealth > 35)
				textColor = Color(255, 255, 0, 255);
			else
				textColor = Color(139, 0, 0, 255);

			surface()->DrawSetColor(Color(255, 255, 255, 255));
			surface()->DrawSetTextFont(m_hTextFontDef);
			surface()->DrawSetTextColor(textColor);
			surface()->DrawSetTextPos(xpos, ypos);
			surface()->DrawUnicodeString(unicode);
		}
	}
}

void CHudTargetIdentifier::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}
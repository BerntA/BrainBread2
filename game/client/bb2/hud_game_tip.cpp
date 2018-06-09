//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Display a hint for the player - similar to the tool tip hud, however this one is used to draw stuff in the middle and most likely only visual game hints.
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
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

#include "tier0/memdbgon.h" 

struct tipItem_t
{
	wchar_t unicodeMessage[256];
	wchar_t keyBind[16];
	int iTextureIndex;
	float flTimeLeft;
	float flTimeAdded;
	int alpha;
	int type;
};

//-----------------------------------------------------------------------------
// Purpose: Base
//-----------------------------------------------------------------------------
class CHudGameTip : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudGameTip, vgui::Panel);

public:

	CHudGameTip(const char * pElementName);
	virtual ~CHudGameTip();

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

	void MsgFunc_GameTip(bf_read &msg);

private:
	int m_nKeyboardTexture[5];
	int m_nHintTextures[4];
	float GetTimeToWait(void);

protected:

	virtual void Paint();
	virtual void PaintBackground();

	CPanelAnimationVar(vgui::HFont, m_hTextFontDef, "TextFont", "Default");

	CPanelAnimationVarAliasType(float, spacebar_x, "spacebar_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, spacebar_y, "spacebar_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, spacebar_w, "spacebar_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, spacebar_h, "spacebar_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, key_x, "key_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, key_y, "key_y", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, key_w, "key_w", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, key_h, "key_h", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, message_x, "message_x", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, message_y, "message_y", "0", "proportional_float");

	CPanelAnimationVarAliasType(float, panel_wide, "panel_wide", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, panel_ypos, "panel_ypos", "0", "proportional_float");

	CPanelAnimationVar(float, m_flAlpha, "AlphaFade", "0");

	CUtlVector<tipItem_t> m_pTipItems;
};

DECLARE_HUDELEMENT_DEPTH(CHudGameTip, 1);
DECLARE_HUD_MESSAGE(CHudGameTip, GameTip);

#define ITEM_FADE_TIME 0.25f

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudGameTip::CHudGameTip(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudGameTip")
{
	vgui::Panel * pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	for (int i = 0; i < _ARRAYSIZE(m_nKeyboardTexture); i++)
		m_nKeyboardTexture[i] = surface()->CreateNewTextureID();

	for (int i = 0; i < _ARRAYSIZE(m_nHintTextures); i++)
		m_nHintTextures[i] = surface()->CreateNewTextureID();

	surface()->DrawSetTextureFile(m_nKeyboardTexture[0], "vgui/hud/icons/button", true, false);
	surface()->DrawSetTextureFile(m_nKeyboardTexture[1], "vgui/hud/icons/spacebar", true, false);
	surface()->DrawSetTextureFile(m_nKeyboardTexture[2], "vgui/hud/icons/icon_mouse_left", true, false);
	surface()->DrawSetTextureFile(m_nKeyboardTexture[3], "vgui/hud/icons/icon_mouse_right", true, false);
	surface()->DrawSetTextureFile(m_nKeyboardTexture[4], "vgui/hud/icons/icon_mouse_middle", true, false);

	surface()->DrawSetTextureFile(m_nHintTextures[0], "vgui/hud/icons/icon_info", true, false);
	surface()->DrawSetTextureFile(m_nHintTextures[1], "vgui/hud/icons/icon_tip", true, false);
	surface()->DrawSetTextureFile(m_nHintTextures[2], "vgui/hud/icons/exclamation_tip", true, false);
	surface()->DrawSetTextureFile(m_nHintTextures[3], "vgui/hud/icons/icon_defend", true, false);

	SetHiddenBits(0);
}

CHudGameTip::~CHudGameTip()
{
	m_pTipItems.Purge();
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudGameTip::Init()
{
	HOOK_HUD_MESSAGE(CHudGameTip, GameTip);
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudGameTip::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && m_pTipItems.Count());
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudGameTip::Reset(void)
{
	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);
	SetAlpha(0);
	m_flAlpha = 0.0f;
	m_pTipItems.Purge();
}

float CHudGameTip::GetTimeToWait(void)
{
	float flTime = gpGlobals->curtime;
	for (int i = (m_pTipItems.Count() - 1); i >= 0; i--)
	{
		if (m_pTipItems[i].flTimeLeft > flTime)
			flTime = m_pTipItems[i].flTimeLeft;
	}

	return flTime;
}

void CHudGameTip::MsgFunc_GameTip(bf_read &msg)
{
	int type = msg.ReadShort();
	float duration = msg.ReadFloat();

	if (!m_pTipItems.Count())
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 256.0f, 0.0f, 0.2f, AnimationController::INTERPOLATOR_LINEAR);

	char pszMessage[256], pszKeyBind[64], pszOutputBinding[16];

	msg.ReadString(pszMessage, 256);
	msg.ReadString(pszKeyBind, 64);

	char args[4][32];
	for (int i = 0; i < 4; i++)
		msg.ReadString(args[i], 32);

	const char *keybindButton = engine->Key_LookupBinding(pszKeyBind);
	if (keybindButton == NULL)
		keybindButton = "?";

	Q_strncpy(pszOutputBinding, keybindButton, 16);
	Q_strupr(pszOutputBinding);

	// Decide the texture to use:
	int textureIndex = 0;
	if (type == 0)
	{
		if (!strcmp(pszOutputBinding, "SPACE"))
			textureIndex = 1;
		else if (!strcmp(pszOutputBinding, "MOUSE1"))
			textureIndex = 2;
		else if (!strcmp(pszOutputBinding, "MOUSE2"))
			textureIndex = 3;
		else if (!strcmp(pszOutputBinding, "MOUSE3"))
			textureIndex = 4;
	}
	else
	{
		textureIndex = (type - 1);

		if (textureIndex >= _ARRAYSIZE(m_nHintTextures))
		{
			Warning("Tried to add a hint with an invalid type!\n");
			return;
		}
	}

	// Is this a localized msg?
	wchar_t *lookupToken = g_pVGuiLocalize->Find(pszMessage);
	if (lookupToken)
	{
		wchar_t wcMessage[256];
		wchar_t arg1[32], arg2[32], arg3[32], arg4[32];

		g_pVGuiLocalize->ConvertANSIToUnicode(args[0], arg1, sizeof(arg1));
		g_pVGuiLocalize->ConvertANSIToUnicode(args[1], arg2, sizeof(arg2));
		g_pVGuiLocalize->ConvertANSIToUnicode(args[2], arg3, sizeof(arg3));
		g_pVGuiLocalize->ConvertANSIToUnicode(args[3], arg4, sizeof(arg4));

		g_pVGuiLocalize->ConstructString(wcMessage, sizeof(wcMessage), lookupToken, 4, arg1, arg2, arg3, arg4);
		g_pVGuiLocalize->ConvertUnicodeToANSI(wcMessage, pszMessage, sizeof(wcMessage));
	}

	tipItem_t item;
	item.alpha = 0;
	item.type = type;
	item.iTextureIndex = textureIndex;
	item.flTimeLeft = GetTimeToWait() + duration;
	item.flTimeAdded = GetTimeToWait();
	g_pVGuiLocalize->ConvertANSIToUnicode(pszOutputBinding, item.keyBind, 16);
	g_pVGuiLocalize->ConvertANSIToUnicode(pszMessage, item.unicodeMessage, 256);
	m_pTipItems.AddToHead(item);
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudGameTip::Paint()
{
	if (m_pTipItems.Count())
	{
		int index = (m_pTipItems.Count() - 1);

		int iTexture = 0;
		if (m_pTipItems[index].type == 0)
			iTexture = m_nKeyboardTexture[m_pTipItems[index].iTextureIndex];
		else
			iTexture = m_nHintTextures[m_pTipItems[index].iTextureIndex];

		bool bIsSpaceBarBinding = ((m_pTipItems[index].type == 0) && (m_pTipItems[index].iTextureIndex == 1));

		float posX, posY, wide, tall;
		posX = bIsSpaceBarBinding ? spacebar_x : key_x;
		posY = bIsSpaceBarBinding ? spacebar_y : key_y;
		wide = bIsSpaceBarBinding ? spacebar_w : key_w;
		tall = bIsSpaceBarBinding ? spacebar_h : key_h;

		bool bFadeOut = (gpGlobals->curtime >= (m_pTipItems[index].flTimeLeft - ITEM_FADE_TIME));
		bool bFadeIn = (gpGlobals->curtime <= (m_pTipItems[index].flTimeAdded + ITEM_FADE_TIME));
		float flAlpha = m_flAlpha * 255.0f;

		if (bFadeIn)
			vgui::GetAnimationController()->RunAnimationCommand(this, "AlphaFade", 1.0f, 0.0f, ITEM_FADE_TIME, AnimationController::INTERPOLATOR_LINEAR);
		else if (bFadeOut)
			vgui::GetAnimationController()->RunAnimationCommand(this, "AlphaFade", 0.0f, 0.0f, ITEM_FADE_TIME, AnimationController::INTERPOLATOR_LINEAR);

		m_pTipItems[index].alpha = (int)flAlpha;

		Color textColor = Color(20, 225, 15, m_pTipItems[index].alpha);
		Color bindColor = Color(0, 0, 0, m_pTipItems[index].alpha);
		Color col = Color(255, 255, 255, m_pTipItems[index].alpha);

		surface()->DrawSetColor(textColor);
		surface()->DrawSetTextColor(textColor);
		surface()->DrawSetTextFont(m_hTextFontDef);

		surface()->DrawSetTextPos(message_x + wide + posX, message_y);
		surface()->DrawUnicodeString(m_pTipItems[index].unicodeMessage);

		surface()->DrawSetColor(col);
		surface()->DrawSetTexture(iTexture);
		surface()->DrawTexturedRect(posX, posY, posX + wide, posY + tall);

		int strLen = 0;
		if (m_pTipItems[index].type == 0 && m_pTipItems[index].iTextureIndex <= 1)
		{
			surface()->DrawSetColor(bindColor);
			surface()->DrawSetTextColor(bindColor);
			surface()->DrawSetTextFont(m_hTextFontDef);

			strLen = UTIL_ComputeStringWidth(m_hTextFontDef, m_pTipItems[index].keyBind);
			surface()->DrawSetTextPos(posX + (wide / 2) - (strLen / 2), posY + (tall / 2) - (surface()->GetFontTall(m_hTextFontDef) / 2));
			surface()->DrawUnicodeString(m_pTipItems[index].keyBind);
		}

		strLen = UTIL_ComputeStringWidth(m_hTextFontDef, m_pTipItems[index].unicodeMessage) + wide;
		SetWide(panel_wide + strLen);

		SetPos((ScreenWidth() / 2) - (GetWide() / 2), panel_ypos);
	}

	for (int i = (m_pTipItems.Count() - 1); i >= 0; --i)
	{
		if (gpGlobals->curtime > m_pTipItems[i].flTimeLeft)
		{
			m_pTipItems.Remove(i);
			m_flAlpha = 0.0f;
		}
	}

	if (!m_pTipItems.Count())
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, 0.2f, AnimationController::INTERPOLATOR_LINEAR);
}

void CHudGameTip::PaintBackground()
{
	SetBgColor(Color(20, 20, 20, 20));
	SetPaintBorderEnabled(false);
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}
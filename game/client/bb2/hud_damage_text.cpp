//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Damage Indicator - Displays how much damage you dealt towards the target you attacked!
//
//========================================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "hl2mp_gamerules.h"
#include "c_hl2mp_player.h"

#include "tier0/memdbgon.h" 

using namespace vgui;

ConVar bb2_render_damage_text("bb2_render_damage_text", "1", FCVAR_ARCHIVE, "When you attack enemies, display how much damage you dealt.", true, 0.0f, true, 1.0f);

struct damageTextItem_t
{
	Vector vecStartPos;
	int damageDone;
	int index;
	float timeAdded;
};

class CHudDamageText : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudDamageText, vgui::Panel);

public:

	CHudDamageText(const char* pElementName);

	void Init(void);
	void VidInit(void);
	bool ShouldDraw(void);
	void Reset(void);
	void MsgFunc_DamageTextInfo(bf_read& msg);

protected:

	void ApplySchemeSettings(vgui::IScheme* scheme);
	void Paint();

	damageTextItem_t* GetDamageEntry(int index, bool bHealing);

	CUtlVector<damageTextItem_t> m_pItems;

private:

	CPanelAnimationVar(vgui::HFont, m_hDefaultFont, "DefaultFont", "Default");
	CPanelAnimationVar(Color, m_hDamageColor, "DamageColor", "255 20 20 255");
	CPanelAnimationVar(Color, m_hHealColor, "HealingColor", "10 200 10 255");
	CPanelAnimationVar(float, m_flTimeToUse, "ScrollTime", "0.65f");
	CPanelAnimationVar(float, m_flHeightToScroll, "ScrollHeight", "40.0f");
	CPanelAnimationVar(float, m_flMergeTime, "MergeTime", "0.18f");
};

DECLARE_HUDELEMENT_DEPTH(CHudDamageText, 80);
DECLARE_HUD_MESSAGE(CHudDamageText, DamageTextInfo);

//------------------------------------------------------------------------
// Purpose: Constructor
//------------------------------------------------------------------------
CHudDamageText::CHudDamageText(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudDamageText")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ROUNDSTARTING);
}

void CHudDamageText::ApplySchemeSettings(vgui::IScheme* scheme)
{
	BaseClass::ApplySchemeSettings(scheme);
	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudDamageText::Init()
{
	HOOK_HUD_MESSAGE(CHudDamageText, DamageTextInfo);
	Reset();
}

void CHudDamageText::VidInit(void)
{
	m_pItems.Purge();
}

bool CHudDamageText::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && m_pItems.Count() && bb2_render_damage_text.GetBool());
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudDamageText::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
}

void CHudDamageText::Paint()
{
	for (int i = (m_pItems.Count() - 1); i >= 0; i--)
	{
		float delta = (gpGlobals->curtime - m_pItems[i].timeAdded) / m_flTimeToUse;
		float alpha = (1.0f - delta) * 255.0f;
		alpha = clamp(alpha, 0.0f, 255.0f);
		delta = clamp(delta, 0.0f, 1.0f);
		int iDamageOrHealVal = m_pItems[i].damageDone;

		Color colorForText = m_hDamageColor;
		if (iDamageOrHealVal >= 0)
			colorForText = m_hHealColor;

		colorForText[3] = alpha;

		Vector vecCurrPos = m_pItems[i].vecStartPos + (Vector(0, 0, m_flHeightToScroll) * delta);

		int xpos, ypos, strLen;
		wchar_t unicode[64];
		V_swprintf_safe(unicode, L"%i", iDamageOrHealVal);
		strLen = UTIL_ComputeStringWidth(m_hDefaultFont, unicode);

		GetVectorInScreenSpace(vecCurrPos, xpos, ypos, NULL);

		xpos -= (strLen / 2);

		surface()->DrawSetColor(colorForText);
		surface()->DrawSetTextFont(m_hDefaultFont);
		surface()->DrawSetTextColor(colorForText);
		surface()->DrawSetTextPos(xpos, ypos);
		surface()->DrawUnicodeString(unicode);

		// Are we done yet?
		if (delta >= m_flTimeToUse)
			m_pItems.Remove(i);
	}
}

damageTextItem_t* CHudDamageText::GetDamageEntry(int index, bool bHealing)
{
	for (int i = 0; i < m_pItems.Count(); i++)
	{
		damageTextItem_t* pItem = &m_pItems[i];
		if ((pItem->index == index) && ((pItem->damageDone > 0) == bHealing))
			return pItem;
	}
	return NULL;
}

void CHudDamageText::MsgFunc_DamageTextInfo(bf_read& msg)
{
	float xpos, ypos, zpos;
	xpos = msg.ReadFloat();
	ypos = msg.ReadFloat();
	zpos = msg.ReadFloat();

	const int damage = msg.ReadShort();
	const int index = msg.ReadShort();

	float labelHeight = (float)surface()->GetFontTall(m_hDefaultFont);
	if (damage > 0) labelHeight *= 2;

	damageTextItem_t* pExistingItem = GetDamageEntry(index, (damage > 0));
	if ((pExistingItem != NULL) && ((gpGlobals->curtime - pExistingItem->timeAdded) <= m_flMergeTime))
	{
		pExistingItem->damageDone += damage;
		return;
	}

	damageTextItem_t item;
	item.vecStartPos = Vector(xpos, ypos, zpos + labelHeight);
	item.damageDone = damage;
	item.index = index;
	item.timeAdded = gpGlobals->curtime;

	m_pItems.AddToHead(item);
}
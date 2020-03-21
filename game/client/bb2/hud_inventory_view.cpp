//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays your inventory items, only the ones who have a hud icon specified. Important items will always be shown here, if needed.
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
#include "GameBase_Shared.h"
#include "c_playerresource.h"

using namespace vgui;

#include "tier0/memdbgon.h" 

class CHudInventoryView : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudInventoryView, vgui::Panel);

public:

	CHudInventoryView(const char * pElementName);

	virtual void Init(void);
	virtual void Reset(void);
	virtual bool ShouldDraw(void);

	void UserCmd_ItemSwitch(void);
	void UserCmd_ItemUse(void);
	void UserCmd_ItemDrop(void);

	const DataInventoryItem_Base_t* GetActiveItem(void) const;

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	CPanelAnimationVarAliasType(float, item_width, "item_width", "12", "proportional_float");
	CPanelAnimationVarAliasType(float, item_height, "item_height", "12", "proportional_float");

	CPanelAnimationVarAliasType(float, width_spacing, "width_spacing", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, height_spacing, "height_spacing", "0", "proportional_float");

	CPanelAnimationVar(Color, m_colActiveItem, "ActiveItem", "30 255 30 255");

	int m_nTextureDefaultIcon;
	int m_iActiveItem;
	int m_iLastItemCount;
};

DECLARE_HUDELEMENT(CHudInventoryView);

DECLARE_HUD_COMMAND_NAME(CHudInventoryView, ItemSwitch, "CHudInventoryView");
DECLARE_HUD_COMMAND_NAME(CHudInventoryView, ItemUse, "CHudInventoryView");
DECLARE_HUD_COMMAND_NAME(CHudInventoryView, ItemDrop, "CHudInventoryView");

HOOK_COMMAND(item_switch, ItemSwitch);
HOOK_COMMAND(item_use, ItemUse);
HOOK_COMMAND(item_drop, ItemDrop);

CHudInventoryView::CHudInventoryView(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudInventoryView")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);

	m_nTextureDefaultIcon = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nTextureDefaultIcon, "vgui/hud/inventory/unknown", true, false);
}

//------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------
void CHudInventoryView::Init()
{
	Reset();
}

//------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------
void CHudInventoryView::Reset(void)
{
	SetFgColor(Color(255, 255, 255, 255));
	SetAlpha(255);
	m_iActiveItem = m_iLastItemCount = 0;
}

bool CHudInventoryView::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && g_PR && GameBaseShared()->GetSharedGameDetails() && GameBaseShared()->GetGameInventory().Count());
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudInventoryView::Paint()
{
	float xpos = 0, ypos = 0;
	m_iLastItemCount = 0;
	for (int i = 0; i < GameBaseShared()->GetGameInventory().Count(); i++)
	{
		const DataInventoryItem_Base_t* data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(GameBaseShared()->GetGameInventory()[i].m_iItemID, GameBaseShared()->GetGameInventory()[i].bIsMapItem);
		if (!data || (data->bShouldRenderIcon == false))
			continue;

		if (m_iLastItemCount == 6)
		{
			ypos = 0;
			xpos = item_width + width_spacing;
		}

		surface()->DrawSetColor(((i == m_iActiveItem) ? m_colActiveItem : GetFgColor()));
		surface()->DrawSetTexture((data->iHUDTextureID == -1) ? m_nTextureDefaultIcon : data->iHUDTextureID);
		surface()->DrawTexturedRect(xpos, ypos, item_width + xpos, item_height + ypos);
		ypos += (item_height + height_spacing);

		m_iLastItemCount++;
	}

	m_iActiveItem = clamp(m_iActiveItem, 0, (m_iLastItemCount - 1)); // Prevent it from going out of range.
}

void CHudInventoryView::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

void CHudInventoryView::UserCmd_ItemSwitch(void)
{
	if (ShouldDraw() == false)
		return;

	m_iActiveItem++;
	if (m_iActiveItem >= m_iLastItemCount)
		m_iActiveItem = 0;
}

void CHudInventoryView::UserCmd_ItemUse(void)
{
	if (ShouldDraw() == false)
		return;

	const DataInventoryItem_Base_t * item = GetActiveItem();
	if (item == NULL)
		return;

	char pchCommand[MAX_WEAPON_STRING];
	Q_snprintf(pchCommand, MAX_WEAPON_STRING, "bb_inventory_item_use %u %i\n", item->iItemID, (item->bIsMapItem ? 1 : 0));
	engine->ClientCmd_Unrestricted(pchCommand);
}

void CHudInventoryView::UserCmd_ItemDrop(void)
{
	if (ShouldDraw() == false)
		return;

	const DataInventoryItem_Base_t * item = GetActiveItem();
	if (item == NULL)
		return;

	char pchCommand[MAX_WEAPON_STRING];
	Q_snprintf(pchCommand, MAX_WEAPON_STRING, "bb_inventory_item_drop %u %i\n", item->iItemID, (item->bIsMapItem ? 1 : 0));
	engine->ClientCmd_Unrestricted(pchCommand);
}

const DataInventoryItem_Base_t * CHudInventoryView::GetActiveItem(void) const
{
	for (int i = 0; i < GameBaseShared()->GetGameInventory().Count(); i++)
	{
		const DataInventoryItem_Base_t* data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(GameBaseShared()->GetGameInventory()[i].m_iItemID, GameBaseShared()->GetGameInventory()[i].bIsMapItem);
		if (!data || (data->bShouldRenderIcon == false))
			continue;

		if (i == m_iActiveItem)
			return data;
	}

	return NULL;
}
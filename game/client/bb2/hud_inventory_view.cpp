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

protected:

	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);

private:

	CPanelAnimationVarAliasType(float, item_width, "item_width", "12", "proportional_float");
	CPanelAnimationVarAliasType(float, item_height, "item_height", "12", "proportional_float");

	CPanelAnimationVarAliasType(float, width_spacing, "width_spacing", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, height_spacing, "height_spacing", "0", "proportional_float");

	int m_nTextureDefaultIcon;
};

DECLARE_HUDELEMENT(CHudInventoryView);

CHudInventoryView::CHudInventoryView(const char * pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudInventoryView")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_INVEHICLE | HIDEHUD_ZOMBIEMODE | HIDEHUD_ROUNDSTARTING | HIDEHUD_SCOREBOARD);

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
}

bool CHudInventoryView::ShouldDraw(void)
{
	return (CHudElement::ShouldDraw() && g_PR && GameBaseShared()->GetGameInventory().Count());
}

//------------------------------------------------------------------------
// Purpose: Draw Stuff
//------------------------------------------------------------------------
void CHudInventoryView::Paint()
{
	if (!GameBaseShared()->GetSharedGameDetails() || !g_PR)
		return;

	float xpos = 0, ypos = 0;
	int itemsDrawn = 0;
	for (int i = 0; i < GameBaseShared()->GetGameInventory().Count(); i++)
	{
		const DataInventoryItem_Base_t *data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(GameBaseShared()->GetGameInventory()[i].m_iItemID, GameBaseShared()->GetGameInventory()[i].bIsMapItem);
		if (!data)
			continue;

		if (data->bShouldRenderIcon == false)
			continue;

		if (itemsDrawn == 6)
		{
			ypos = 0;
			xpos = item_width + width_spacing;
		}

		surface()->DrawSetColor(GetFgColor());
		surface()->DrawSetTexture((data->iHUDTextureID == -1) ? m_nTextureDefaultIcon : data->iHUDTextureID);
		surface()->DrawTexturedRect(xpos, ypos, item_width + xpos, item_height + ypos);
		itemsDrawn++;
		ypos += item_height + height_spacing;
	}
}

void CHudInventoryView::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}
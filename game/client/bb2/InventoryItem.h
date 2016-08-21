//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Inventory Item Handler - Draws the inv. item in 3D and allows rotating and zooming.
//
//========================================================================================//

#ifndef INVENTORY_ITEM_H
#define INVENTORY_ITEM_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "basemodelpanel.h"
#include <vgui_controls/RichText.h>
#include "vgui_base_panel.h"
#include "MouseInputPanel.h"

namespace vgui
{
	class InventoryItem;
	class InventoryItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(InventoryItem, vgui::Panel);

	public:

		InventoryItem(vgui::Panel *parent, char const *panelName, float flUpdateTime = 1.0f);
		virtual ~InventoryItem();

		virtual void SetSize(int wide, int tall);

		int GetItemID() { return m_iItemID; }

		virtual void SetItemDetails(KeyValues *pkvData, bool bMapItem);

	private:

		uint m_iItemID;
		bool m_bMapItem;

		vgui::ImagePanel *m_pBackground;
		vgui::ImagePanel *m_pHeaderImage;
		vgui::Label *m_pTitle; // Name of item.
		vgui::Label *m_pType; // Type of item. (Misc, Attachment, etc)
		vgui::MouseInputPanel *m_pInputPanel;
		class CModelPanel*  m_pModelPreview;

		float m_flAngleY;
		float m_flOriginX;
		int m_iOriginalCursorXPos;
		bool m_bWantsToRotate;
		float m_flUpdateFreq;
		float m_flUpdateTime;

	protected:

		virtual void OnThink();
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void UpdateLayout();

		virtual void OnMouseReleased(vgui::MouseCode code);
		virtual void OnMousePressed(vgui::MouseCode code);
		virtual void OnMouseDoublePressed(vgui::MouseCode code);
		virtual void OnMouseWheeled(int delta);
	};
}

#endif // INVENTORY_ITEM_H
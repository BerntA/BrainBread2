//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Inventory Item Handler - Draws the inv. item in 3D and allows rotating and zooming.
//
//========================================================================================//

#include "cbase.h"
#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include <vgui/IVGui.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "InventoryItem.h"
#include "inputsystem/iinputsystem.h"
#include "vgui_controls/AnimationController.h"
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "KeyValues.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

InventoryItem::InventoryItem(vgui::Panel *parent, char const *panelName, float flUpdateTime) : vgui::Panel(parent, panelName)
{
	SetScheme("BaseScheme");
	SetProportional(true);

	m_pBackground = new ImagePanel(this, "Background");
	m_pBackground->SetZPos(-1);
	m_pBackground->SetShouldScaleImage(true);
	m_pBackground->SetImage("base/inventory/background");

	m_pHeaderImage = new ImagePanel(this, "Header");
	m_pHeaderImage->SetZPos(10);
	m_pHeaderImage->SetShouldScaleImage(true);

	m_pTitle = new Label(this, "Title", "");
	m_pTitle->SetZPos(55);
	m_pTitle->SetContentAlignment(Label::a_center);
	m_pTitle->SetText("Common");

	m_pType = new Label(this, "Type", "");
	m_pType->SetZPos(55);
	m_pType->SetContentAlignment(Label::a_northwest);
	m_pType->SetText("Weapon");

	m_pModelPreview = new CModelPanel(this, "ItemPreview");
	m_pModelPreview->SetZPos(5);

	m_pInputPanel = new MouseInputPanel(this, "InputPanel");
	m_pInputPanel->SetZPos(65);

	m_bMapItem = false;

	InvalidateLayout();

	PerformLayout();

	m_flUpdateTime = 0.0f;
	m_flUpdateFreq = flUpdateTime;
}

InventoryItem::~InventoryItem()
{
}

void InventoryItem::OnThink()
{
	BaseClass::OnThink();

	if (gpGlobals->curtime > m_flUpdateTime)
	{
		UpdateLayout();
		m_flUpdateTime = gpGlobals->curtime + m_flUpdateFreq;
	}

	if (m_bWantsToRotate && !bb2_inventory_item_debugging.GetBool())
	{
		if (!g_pInputSystem->IsButtonDown(MOUSE_LEFT))
		{
			m_bWantsToRotate = false;
			return;
		}

		int x, y;
		vgui::input()->GetCursorPos(x, y);

		if (x != m_iOriginalCursorXPos)
		{
			int iDiff = x - m_iOriginalCursorXPos;
			m_flAngleY += iDiff;

			// Clamp the angle between 0 - 360 degrees.
			if (m_flAngleY > 360)
				m_flAngleY = 0;
			else if (m_flAngleY < 0)
				m_flAngleY = 360;

			KeyValues *pkvChar = GameBaseShared()->GetInventoryItemDetails(m_iItemID, m_bMapItem);
			if (pkvChar)
			{
				KeyValues *pkvItem = GameBaseShared()->GetInventoryItemByID(pkvChar, m_iItemID);
				if (pkvItem)
				{
					KeyValues *pkvModel = pkvItem->FindKey("model");
					if (pkvModel)
					{
						pkvModel->SetFloat("origin_x", m_flOriginX);
						pkvModel->SetFloat("angles_y", m_flAngleY);
						m_pModelPreview->ParseModelInfo(pkvModel);
					}
				}

				pkvChar->deleteThis();
			}

			m_iOriginalCursorXPos = x;
		}
	}
}

void InventoryItem::SetItemDetails(KeyValues *pkvData, bool bMapItem)
{
	m_iItemID = (uint)atol(pkvData->GetName());
	m_bMapItem = bMapItem;

	int iRarity = pkvData->GetInt("Rarity");
	int iType = pkvData->GetInt("Type");

	m_pTitle->SetText(pkvData->GetString("Title"));
	m_pType->SetText(GameBaseShared()->GetInventoryItemType(iType));
	m_pHeaderImage->SetImage(VarArgs("base/inventory/item_%s", GameBaseShared()->GetInventoryItemRarity(iRarity)));

	KeyValues *pkvChar = pkvData->FindKey("model");
	if (pkvChar)
	{
		m_pModelPreview->ParseModelInfo(pkvChar);
		m_flAngleY = pkvChar->GetFloat("angles_y");
		m_flOriginX = pkvChar->GetFloat("origin_x");
	}

	m_pInputPanel->RequestFocus();

	m_bWantsToRotate = false;
}

void InventoryItem::UpdateLayout()
{
	// Allow us to dynamically update, test and debug our preview.
	if (bb2_inventory_item_debugging.GetBool())
	{
		KeyValues *pkvChar = new KeyValues("model");
		pkvChar->LoadFromBuffer("model", GetInventoryModelTemplate);

		pkvChar->SetFloat("origin_x", bb2_inventory_origin_x.GetFloat());
		pkvChar->SetFloat("origin_y", bb2_inventory_origin_y.GetFloat());
		pkvChar->SetFloat("origin_z", bb2_inventory_origin_z.GetFloat());

		pkvChar->SetFloat("angles_x", bb2_inventory_angle_x.GetFloat());
		pkvChar->SetFloat("angles_y", bb2_inventory_angle_y.GetFloat());
		pkvChar->SetFloat("angles_z", bb2_inventory_angle_z.GetFloat());

		pkvChar->SetString("modelname", bb2_inventory_model.GetString());

		m_pModelPreview->ParseModelInfo(pkvChar);

		pkvChar->deleteThis();
	}
}

void InventoryItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	float flScale = ((float)tall) * 0.1;

	m_pInputPanel->SetPos(0, 0);
	m_pInputPanel->SetSize(wide, tall);

	m_pBackground->SetPos(0, 0);
	m_pBackground->SetSize(wide, (tall - scheme()->GetProportionalScaledValue(flScale)));

	m_pHeaderImage->SetPos(0, tall - scheme()->GetProportionalScaledValue(flScale));
	m_pHeaderImage->SetSize(wide, scheme()->GetProportionalScaledValue(flScale));

	m_pModelPreview->SetPos(0, 0);
	m_pModelPreview->SetSize(wide, (tall - scheme()->GetProportionalScaledValue(flScale)));

	m_pTitle->SetPos(0, tall - scheme()->GetProportionalScaledValue(flScale));
	m_pTitle->SetSize(wide, scheme()->GetProportionalScaledValue(flScale - 5));

	m_pType->SetPos(scheme()->GetProportionalScaledValue(1), tall - scheme()->GetProportionalScaledValue(9));
	m_pType->SetSize(wide, scheme()->GetProportionalScaledValue(8));
}

void InventoryItem::PerformLayout()
{
	BaseClass::PerformLayout();
}

void InventoryItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pTitle->SetFgColor(pScheme->GetColor("InventoryItemTextTitleColor", Color(255, 255, 255, 255)));
	m_pTitle->SetFont(pScheme->GetFont("DefaultLargeBold"));
	m_pType->SetFgColor(pScheme->GetColor("InventoryItemTextTypeColor", Color(255, 255, 255, 255)));
	m_pType->SetFont(pScheme->GetFont("DefaultBold"));
}

void InventoryItem::OnMouseReleased(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT)
		m_bWantsToRotate = false;
	else
		BaseClass::OnMouseReleased(code);
}

void InventoryItem::OnMousePressed(vgui::MouseCode code)
{
	if (m_bWantsToRotate)
	{
		BaseClass::OnMousePressed(code);
		return;
	}

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (code == MOUSE_LEFT)
	{
		m_bWantsToRotate = true;
		m_iOriginalCursorXPos = x;
	}
	else
		BaseClass::OnMousePressed(code);
}

void InventoryItem::OnMouseDoublePressed(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		char pszUseCommand[80];
		Q_snprintf(pszUseCommand, 80, "bb_inventory_item_use %u %i\n", m_iItemID, (m_bMapItem ? 1 : 0));
		engine->ClientCmd(pszUseCommand);
		engine->ClientCmd("player_tree 1\n");
	}
	else if (code == MOUSE_RIGHT)
	{
		KeyValues *pkvData = new KeyValues("DropItem");
		pkvData->SetInt("item", m_iItemID);
		pkvData->SetInt("mapitem", (m_bMapItem ? 1 : 0));
		PostActionSignal(pkvData);
	}
	else
		BaseClass::OnMouseDoublePressed(code);
}

void InventoryItem::OnMouseWheeled(int delta)
{
	bool bScrollingUp = (delta > 0);

	if (!m_bWantsToRotate && !bb2_inventory_item_debugging.GetBool())
	{
		m_flOriginX += (bScrollingUp ? -1 : 1);

		KeyValues *pkvChar = GameBaseShared()->GetInventoryItemDetails(m_iItemID, m_bMapItem);
		if (pkvChar)
		{
			KeyValues *pkvItem = GameBaseShared()->GetInventoryItemByID(pkvChar, m_iItemID);
			if (pkvItem)
			{
				KeyValues *pkvModel = pkvItem->FindKey("model");
				if (pkvModel)
				{
					pkvModel->SetFloat("origin_x", m_flOriginX);
					pkvModel->SetFloat("angles_y", m_flAngleY);
					m_pModelPreview->ParseModelInfo(pkvModel);
				}
			}

			pkvChar->deleteThis();
		}
	}

	BaseClass::OnMouseWheeled(delta);
}
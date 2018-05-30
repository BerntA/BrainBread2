//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Combo List - Similar to combo box but more visually appealing. 
//
//========================================================================================//

#include "cbase.h"
#include "ComboList.h"
#include <vgui_controls/MenuItem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ComboList::ComboList(vgui::Panel *parent, char const *panelName, const char *text, int items) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pComboBox = vgui::SETUP_PANEL(new vgui::ComboBox(this, "ComboBox", items, false));
	m_pComboBox->AddActionSignalTarget(this);
	m_pComboBox->SetZPos(20);

	m_pLabel = vgui::SETUP_PANEL(new vgui::Label(this, "Label", ""));
	m_pLabel->SetZPos(10);
	m_pLabel->SetText(text);
	m_pLabel->SetContentAlignment(Label::a_west);

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "Button", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetZPos(25);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Activate");

	InvalidateLayout();
	PerformLayout();
}

ComboList::~ComboList()
{
}

void ComboList::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h, difference;
	GetSize(w, h);
	difference = (GetWide() / 2);

	m_pComboBox->SetSize(difference, h);
	m_pButton->SetPos(0, 0);
	m_pButton->SetSize(w, h);

	m_pLabel->SetPos(0, 0);
	m_pLabel->SetSize(w - difference, h);
	m_pLabel->SetContentAlignment(Label::a_west);
	m_pComboBox->SetPos(w - difference, 0);
	m_pComboBox->PerformLayout();
}

void ComboList::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pComboBox->SetFont(pScheme->GetFont("OptionTextMedium"));
	m_pComboBox->SetFgColor(pScheme->GetColor("ComboListTextColor", Color(255, 255, 255, 255)));
	m_pComboBox->SetBgColor(Color(0, 0, 0, 0));
	m_pComboBox->SetSelectionBgColor(Color(0, 0, 0, 0));
	m_pComboBox->SetSelectionTextColor(pScheme->GetColor("ComboListTextColor", Color(255, 255, 255, 255)));
	m_pComboBox->SetDisabledBgColor(Color(0, 0, 0, 0));
	m_pComboBox->SetSelectionUnfocusedBgColor(Color(0, 0, 0, 0));
	if (m_pComboBox->GetMenu())
	{
		m_pComboBox->GetMenu()->SetFont(pScheme->GetFont("OptionTextSmall"));
		m_pComboBox->GetMenu()->SetFgColor(pScheme->GetColor("ComboListMenuItemFgColor", Color(23, 25, 24, 255)));
		m_pComboBox->GetMenu()->SetBgColor(pScheme->GetColor("ComboListMenuItemBgColor", Color(20, 21, 29, 255)));
		m_pComboBox->GetMenu()->SetBorder(NULL);
		m_pComboBox->GetMenu()->SetPaintBorderEnabled(false);
		for (int i = 0; i < m_pComboBox->GetMenu()->GetItemCount(); i++)
		{
			vgui::MenuItem *pItem = m_pComboBox->GetMenu()->GetMenuItem(i);
			if (pItem)
			{
				pItem->SetFont(pScheme->GetFont("OptionTextSmall"));
				pItem->SetFgColor(pScheme->GetColor("ComboListTextColor", Color(255, 255, 255, 255)));
				pItem->SetBgColor(Color(0, 0, 0, 0));
				pItem->SetBorder(NULL);
				pItem->SetPaintBorderEnabled(false);
			}
		}
	}

	m_pComboBox->SetBorder(NULL);
	m_pComboBox->SetPaintBorderEnabled(false);

	m_pLabel->SetFont(pScheme->GetFont("OptionTextMedium"));
	m_pLabel->SetFgColor(pScheme->GetColor("ComboListTextColor", Color(255, 255, 255, 255)));
}

void ComboList::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Activate"))
	{
		if (m_pComboBox->GetMenu())
		{
			if (!m_pComboBox->GetMenu()->IsVisible())
				m_pComboBox->ShowMenu();
			else
				m_pComboBox->HideMenu();
		}
	}

	BaseClass::OnCommand(pcCommand);
}

void ComboList::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);

	m_pComboBox->SetEnabled(state);
	m_pLabel->SetEnabled(state);
	m_pButton->SetEnabled(state);
}
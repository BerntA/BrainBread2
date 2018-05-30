//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Check Box List - A list of check box controls.
//
//========================================================================================//

#include "cbase.h"
#include "CheckBoxList.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CheckBoxList::CheckBoxList(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	SetZPos(5);
	InvalidateLayout();
	PerformLayout();
	m_iCurrentIndex = 0;
}

CheckBoxList::~CheckBoxList()
{
	Reset();
}

void CheckBoxList::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CheckBoxList::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetBorder(pScheme->GetBorder("FrameBorder"));
}

void CheckBoxList::Reset(void)
{
	m_iCurrentIndex = 0;
	for (int i = (checkBoxList.Count() - 1); i >= 0; i--)
		delete checkBoxList[i].boxGUI;

	checkBoxList.Purge();
}

void CheckBoxList::AddItemToList(const char *szText, int index)
{
	int w, h, iSize;
	GetSize(w, h);
	iSize = checkBoxList.Count();

	GraphicalCheckBox *pCheckBox = vgui::SETUP_PANEL(new GraphicalCheckBox(this, "CheckBox", szText, "Default"));
	pCheckBox->SetZPos(10);
	pCheckBox->SetSize(w - scheme()->GetProportionalScaledValue(11), scheme()->GetProportionalScaledValue(10));
	pCheckBox->SetPos(0, 1 + (iSize * scheme()->GetProportionalScaledValue(12)));
	pCheckBox->SetCheckedStatus(false);
	pCheckBox->MoveToFront();

	CheckBoxListItem pItem;
	pItem.boxGUI = pCheckBox;
	pItem.m_iPlayerIndex = index;

	checkBoxList.AddToTail(pItem);
}

void CheckBoxList::OnThink()
{
	if (HasItems())
		SetBgColor(Color(19, 19, 19, 170));
	else
		SetBgColor(Color(0, 0, 0, 0));

	BaseClass::OnThink();
}

void CheckBoxList::OnMouseWheeled(int delta)
{
	bool bCanScroll = (26 <= checkBoxList.Count());
	bool bIsScrollingUp = (delta > 0);

	if (bCanScroll)
	{
		if (bIsScrollingUp)
		{
			if (m_iCurrentIndex > 0)
			{
				for (int i = 0; i < checkBoxList.Count(); i++)
					checkBoxList[i].boxGUI->SetVisible(false);

				m_iCurrentIndex--;
				for (int i = m_iCurrentIndex; i < m_iCurrentIndex + 25; i++)
				{
					if ((checkBoxList.Count() - 1) < i)
						continue;

					checkBoxList[i].boxGUI->SetVisible(true);
					checkBoxList[i].boxGUI->SetPos(0, 1 + ((i - m_iCurrentIndex) * scheme()->GetProportionalScaledValue(12)));
				}
			}
		}
		else
		{
			if (m_iCurrentIndex < (checkBoxList.Count() - 25))
			{
				for (int i = 0; i < checkBoxList.Count(); i++)
					checkBoxList[i].boxGUI->SetVisible(false);

				m_iCurrentIndex++;
				for (int i = m_iCurrentIndex; i < m_iCurrentIndex + 25; i++)
				{
					if ((checkBoxList.Count() - 1) < i)
						continue;

					checkBoxList[i].boxGUI->SetVisible(true);
					checkBoxList[i].boxGUI->SetPos(0, 1 + ((i - m_iCurrentIndex) * scheme()->GetProportionalScaledValue(12)));
				}
			}
		}
	}

	BaseClass::OnMouseWheeled(delta);
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest List - Keeps track of all Quest Item objects. 
//
//========================================================================================//

#include "cbase.h"
#include "QuestList.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

QuestList::QuestList(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetProportional(true);
	SetScheme("BaseScheme");
	InvalidateLayout();
	PerformLayout();
}

QuestList::~QuestList()
{
	Cleanup();
}

void QuestList::Cleanup(void)
{
	for (int i = (pszQuestItems.Count() - 1); i >= 0; i--)
		delete pszQuestItems[i];

	pszQuestItems.Purge();
}

void QuestList::CreateList(void)
{
	Cleanup();

	int w, h;
	GetSize(w, h);
	m_iScrollOffset = 0;
	int iPositioning = 0;
	for (int i = 0; i < GameBaseShared()->GetSharedQuestData()->GetQuestList().Count(); i++)
	{
		const CQuestItem *data = GameBaseShared()->GetSharedQuestData()->GetQuestList()[i];
		if (!data->bIsActive)
			continue;

		QuestItem *pItem = vgui::SETUP_PANEL(new QuestItem(this, "QuestItem", data->szTitle, data->iQuestStatus, data->iQuestIndex));
		pItem->SetZPos(10);
		pItem->SetSize(w, scheme()->GetProportionalScaledValue(14));
		pItem->SetPos(0, (iPositioning * scheme()->GetProportionalScaledValue(14)));
		pItem->AddActionSignalTarget(this);
		pszQuestItems.AddToTail(pItem);
		iPositioning++;
	}
}

void QuestList::UpdateLayout(void)
{
	for (int i = 0; i < pszQuestItems.Count(); i++)
		pszQuestItems[i]->SetPos(0, (scheme()->GetProportionalScaledValue(m_iScrollOffset) + (i * scheme()->GetProportionalScaledValue(20))));
}

void QuestList::SetActiveIndex(int index)
{
	for (int i = 0; i < pszQuestItems.Count(); i++)
		pszQuestItems[i]->SetActive(pszQuestItems[i]->GetIndex() == index);
}

int QuestList::GetFirstItem(void)
{
	if (pszQuestItems.Count() <= 0)
		return 0;

	return pszQuestItems[0]->GetIndex();
}

void QuestList::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void QuestList::OnMouseWheeled(int delta)
{
	int w, h, x, y;
	GetSize(w, h);

	float flMaxItemsToShow = (h / scheme()->GetProportionalScaledValue(20));
	bool bCanScroll = ((pszQuestItems.Count() * scheme()->GetProportionalScaledValue(20)) > h);
	if (bCanScroll)
	{
		bool bScrollUp = (delta > 0);
		if (bScrollUp)
		{
			if (m_iScrollOffset < 0)
			{
				m_iScrollOffset += 1;
				UpdateLayout();
			}
		}
		else
		{
			int iIndexInList = (pszQuestItems.Count() - flMaxItemsToShow);
			if (iIndexInList < 0)
				iIndexInList = 0;

			pszQuestItems[iIndexInList]->GetPos(x, y);

			if (y > 0)
			{
				m_iScrollOffset -= 1;
				UpdateLayout();
			}
		}
	}

	BaseClass::OnMouseWheeled(delta);
}
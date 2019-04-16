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

QuestList::QuestList(vgui::Panel* parent, char const* panelName) : vgui::Panel(parent, panelName)
{
	SetProportional(true);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
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

	for (int i = 0; i < GameBaseShared()->GetSharedQuestData()->GetQuestList().Count(); i++)
	{
		const CQuestItem* data = GameBaseShared()->GetSharedQuestData()->GetQuestList()[i];
		if (!data->bIsActive)
			continue;

		QuestItem* pItem = vgui::SETUP_PANEL(new QuestItem(this, "QuestItem", data->szTitle, data->iQuestStatus, data->iQuestIndex));
		pItem->SetZPos(10);
		pItem->SetSize(w, scheme()->GetProportionalScaledValue(14));
		pszQuestItems.AddToTail(pItem);
	}

	UpdateLayout();
}

void QuestList::UpdateLayout(void)
{
	int offset = m_iScrollOffset * scheme()->GetProportionalScaledValue(20);
	for (int i = 0; i < pszQuestItems.Count(); i++)
	{
		pszQuestItems[i]->SetPos(0, (i * scheme()->GetProportionalScaledValue(15)) - (m_iScrollOffset ? offset : 0));
		offset -= scheme()->GetProportionalScaledValue(15);
	}
}

void QuestList::SetActiveIndex(int index)
{
	int item = 0;
	for (int i = 0; i < pszQuestItems.Count(); i++)
	{
		pszQuestItems[i]->SetActive(pszQuestItems[i]->GetIndex() == index);
		if (pszQuestItems[i]->GetIndex() == index)
			item = i + 1;
	}

	int tall = item * scheme()->GetProportionalScaledValue(15);
	m_iScrollOffset = (tall > GetTall()) ? item : 0;
	UpdateLayout();
}

int QuestList::GetFirstItem(void)
{
	return ((pszQuestItems.Count() > 0) ? pszQuestItems[0]->GetIndex() : 0);
}

int QuestList::GetLastItem(void)
{
	return ((pszQuestItems.Count() > 0) ? pszQuestItems[(pszQuestItems.Count() - 1)]->GetIndex() : 0);
}

int QuestList::GetNextItem(int ID, bool bUp)
{
	int count = pszQuestItems.Count();
	for (int i = 0; i < count; i++)
	{
		if (pszQuestItems[i]->GetIndex() == ID)
		{
			if (bUp)
			{
				if ((i - 1) >= 0)
					return pszQuestItems[(i - 1)]->GetIndex();

				return GetLastItem(); // No prev item, go back to end!
			}
			else
			{
				if ((i + 1) < count)
					return pszQuestItems[(i + 1)]->GetIndex();

				return GetFirstItem(); // If there is no next ID, go back to the start!
			}
		}
	}

	return 0;
}
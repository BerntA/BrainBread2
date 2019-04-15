//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Item - This controls indicates the progress of a quest (by color) and also sets the quest detail panel's active quest index when this item is clicked. 
// This item is linked to a quest index.
//
//========================================================================================//

#include "cbase.h"
#include "QuestItem.h"
#include <vgui/IInput.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Shared.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

QuestItem::QuestItem(vgui::Panel *parent, char const *panelName, const char *title, int iStatus, int iIndex) : vgui::Panel(parent, panelName)
{
	m_iIndex = iIndex;
	SetProportional(true);
	SetScheme("BaseScheme");

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetZPos(-1);
	m_pBackground->SetShouldScaleImage(true);

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "Title", ""));
	m_pLabelTitle->SetZPos(5);
	m_pLabelTitle->SetText(title);

	switch (iStatus)
	{
	
	case STATUS_FAILED:
	{
		m_pBackground->SetImage("base/quest/quest_failed");
		break;
	}

	case STATUS_SUCCESS:
	{
		m_pBackground->SetImage("base/quest/quest_completed");
		break;
	}

	}

	InvalidateLayout();
	PerformLayout();
}

QuestItem::~QuestItem()
{
}

void QuestItem::OnThink()
{
	BaseClass::OnThink();
	m_pLabelTitle->SetFgColor(m_bIsActive ? m_colActivated : m_colDeactivated);
}

void QuestItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelTitle->SetFont(pScheme->GetFont("DefaultBold"));
	
	m_colActivated = pScheme->GetColor("QuestItemActivated", Color(0, 0, 0, 255));
	m_colDeactivated = pScheme->GetColor("QuestItemDeactivated", Color(0, 0, 0, 255));
}

void QuestItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	m_pBackground->SetSize(wide, tall);
	m_pBackground->SetPos(0, 0);

	m_pLabelTitle->SetSize(wide, tall);
	m_pLabelTitle->SetPos(0, 0);
}
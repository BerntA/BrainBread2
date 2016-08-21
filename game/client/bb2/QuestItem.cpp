//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Item - This controls indicates the progress of a quest (by color) and also sets the quest detail panel's active quest index when this item is clicked. 
// This item is linked to a quest index.
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
#include "QuestItem.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "vgui/ILocalize.h"
#include "KeyValues.h"
#include "GameBase_Shared.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

QuestItem::QuestItem(vgui::Panel *parent, char const *panelName, const char *title, int iStatus, int iIndex) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_iIndex = iIndex;

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "Button", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/button_over.wav");
	m_pButton->SetZPos(10);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Open");

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetZPos(-1);
	m_pBackground->SetShouldScaleImage(true);

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "Title", ""));
	m_pLabelTitle->SetZPos(5);
	m_pLabelTitle->SetContentAlignment(Label::a_center);
	m_pLabelTitle->SetText(title);
	m_pLabelTitle->SetContentAlignment(Label::a_center);

	switch (iStatus)
	{
	case STATUS_ONGOING:
	{
		m_pBackground->SetImage("base/quest/quest_active");
		break;
	}
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
	default:
	{
		m_pBackground->SetImage("base/quest/quest_active");
		break;
	}
	}

	InvalidateLayout();

	PerformLayout();
}

QuestItem::~QuestItem()
{
}

void QuestItem::PerformLayout()
{
	BaseClass::PerformLayout();
}

void QuestItem::OnThink()
{
	BaseClass::OnThink();

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (!m_bIsActive)
	{
		if (m_pButton->IsWithin(x, y))
			m_pLabelTitle->SetFgColor(Color(255, 20, 20, 240));
		else
			m_pLabelTitle->SetFgColor(Color(255, 255, 255, 255));
	}
	else
		m_pLabelTitle->SetFgColor(Color(255, 20, 20, 240));
}

void QuestItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	m_pLabelTitle->SetFont(pScheme->GetFont("DefaultBold"));
}

void QuestItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	m_pBackground->SetSize(wide, tall);
	m_pBackground->SetPos(0, 0);

	m_pButton->SetSize(wide, tall);
	m_pButton->SetPos(0, 0);

	m_pLabelTitle->SetSize(wide, tall);
	m_pLabelTitle->SetPos(0, 0);
	m_pLabelTitle->SetContentAlignment(Label::a_center);
}

void QuestItem::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Open"))
	{
		if (!m_bIsActive)
		{
			m_bIsActive = true;
			GameBaseClient->SelectQuestPreview(GetIndex());
		}
	}

	BaseClass::OnCommand(pcCommand);
}
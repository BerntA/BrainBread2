//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Tool Tip GUI Control
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
#include "ToolTipItem.h"
#include <vgui/ILocalize.h>
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "KeyValues.h"
#include "clientmode_shared.h"
#include <steam/steam_api.h>
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ToolTipItem::ToolTipItem(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(false);

	SetScheme("BaseScheme");

	m_pInfoTitle = vgui::SETUP_PANEL(new vgui::Label(this, "LabelTitle", ""));
	m_pInfoTitle->SetContentAlignment(Label::a_west);
	m_pInfoTitle->SetZPos(20);

	m_pInfoDesc = vgui::SETUP_PANEL(new vgui::RichText(this, "LabelDesc"));
	m_pInfoDesc->SetZPos(20);

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "BG"));
	m_pBackground->SetZPos(10);
	m_pBackground->SetShouldScaleImage(true);
	m_pBackground->SetImage("shared/tooltip");

	InvalidateLayout();
	PerformLayout();
}

ToolTipItem::~ToolTipItem()
{
}

int ToolTipItem::GetSizeOfString(const char *szString)
{
	return (UTIL_ComputeStringWidth(defaultFont, szString) + 30);
}

void ToolTipItem::PerformLayout()
{
	BaseClass::PerformLayout();
}

void ToolTipItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	m_pBackground->SetSize(wide, tall);
	m_pBackground->SetPos(0, 0);

	m_pInfoTitle->SetSize(wide, 20);
	m_pInfoTitle->SetPos(14, 0);

	m_pInfoDesc->SetSize(wide, tall - 20);
	m_pInfoDesc->SetPos(14, 20);
}

void ToolTipItem::SetToolTip(const char *szTitle, const char *szDescription)
{
	m_pInfoTitle->SetText(szTitle);
	m_pInfoDesc->SetText(szDescription);

	m_pInfoDesc->GotoTextStart();
	m_pInfoDesc->SetVerticalScrollbar(false);
}

void ToolTipItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	defaultFont = pScheme->GetFont("DefaultSmall");

	m_pInfoTitle->SetFont(pScheme->GetFont("Default"));
	m_pInfoDesc->SetFont(pScheme->GetFont("DefaultSmall"));
	m_pInfoDesc->SetBgColor(Color(15, 15, 15, 0));
	m_pInfoDesc->SetFgColor(pScheme->GetColor("ToolTipDescriptionTextColor", Color(255, 255, 255, 255)));
	m_pInfoTitle->SetFgColor(pScheme->GetColor("ToolTipTitleTextColor", Color(255, 255, 255, 255)));
}
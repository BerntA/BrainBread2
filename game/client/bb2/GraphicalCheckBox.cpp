//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Image Based CheckBox.
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
#include "GraphicalCheckBox.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "KeyValues.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

GraphicalCheckBox::GraphicalCheckBox(vgui::Panel *parent, char const *panelName, const char *text, const char *fontName, bool bDisableOutput) : vgui::Panel(parent, panelName)
{
	Q_strncpy(szFont, fontName, 32);

	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "BaseText", ""));
	m_pLabelTitle->SetZPos(20);
	m_pLabelTitle->SetContentAlignment(Label::a_center);

	m_pCheckImg = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "CheckImg"));
	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "BaseButton", ""));

	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetArmedSound("ui/button_over.wav");
	m_pButton->SetZPos(40);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Activate");

	m_pCheckImg->SetZPos(30);
	m_pCheckImg->SetImage("mainmenu/checkboxno");
	m_pCheckImg->SetShouldScaleImage(true);
	m_pLabelTitle->SetContentAlignment(Label::a_center);
	m_pLabelTitle->SetText(text);

	m_bDisableInput = bDisableOutput;

	InvalidateLayout();

	PerformLayout();
}

GraphicalCheckBox::~GraphicalCheckBox()
{
}

void GraphicalCheckBox::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);

	m_pLabelTitle->SetEnabled(state);
	m_pButton->SetEnabled(state);
	m_pCheckImg->SetEnabled(state);
}

void GraphicalCheckBox::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pButton->SetVisible(!m_bDisableInput);
	m_pButton->SetEnabled(!m_bDisableInput);

	int w, h;
	GetSize(w, h);

	m_pCheckImg->SetPos(0, 0);
	m_pCheckImg->SetSize(h, h);

	m_pButton->SetSize(w, h);
	m_pButton->SetPos(0, 0);

	m_pLabelTitle->SetSize(w - h - scheme()->GetProportionalScaledValue(2), h);
	m_pLabelTitle->SetPos(h + scheme()->GetProportionalScaledValue(2), 0);
	m_pLabelTitle->SetContentAlignment(Label::a_west);
}

const char *GraphicalCheckBox::GetCheckImage(int iValue)
{
	if (iValue == 0)
		return "checkboxno";
	else
		return "checkboxyes";
}

void GraphicalCheckBox::SetCheckedStatus(bool bStatus)
{
	m_bIsChecked = bStatus;
	m_pCheckImg->SetImage(VarArgs("mainmenu/%s", GetCheckImage(IsChecked() ? 1 : 0)));
}

void GraphicalCheckBox::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelTitle->SetFgColor(pScheme->GetColor("GraphicalCheckBoxTextColor", Color(255, 255, 255, 255)));
	m_pLabelTitle->SetFont(pScheme->GetFont(szFont));
}

void GraphicalCheckBox::OnCommand(const char* pcCommand)
{
	if (m_pButton->IsEnabled())
	{
		if (!Q_stricmp(pcCommand, "Activate"))
		{
			SetCheckedStatus(!IsChecked());
			PostActionSignal(new KeyValues("CheckBoxCheckedChange"));
		}
	}

	BaseClass::OnCommand(pcCommand);
}
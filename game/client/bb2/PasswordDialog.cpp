//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A Password Dialog which pops up on writing connect <ip:port> .... But only if the server requires a password!
//
//========================================================================================//

#include "cbase.h"
#include "PasswordDialog.h"
#include "GameBase_Shared.h"
#include <vgui/IInput.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

CPasswordDialog::CPasswordDialog(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetImage("server/filterbg");
	m_pBackground->SetZPos(-1);
	m_pBackground->SetShouldScaleImage(true);

	m_pInfo = vgui::SETUP_PANEL(new vgui::Label(this, "Info", ""));
	m_pInfo->SetZPos(10);
	m_pInfo->SetContentAlignment(Label::a_center);
	m_pInfo->SetText("#GameUI_ServerBrowser_PasswordRequired");

	const char *szPasswordOptions[] =
	{
		"#GameUI_ServerBrowser_Ok",
		"#GameUI_ServerBrowser_Cancel",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButton[i] = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "PasswordBtn", 0, szPasswordOptions[i], "BB2_PANEL_SMALL", 0, true));
		m_pButton[i]->SetZPos(15);
		m_pButton[i]->SetVisible(true);
		m_pButton[i]->AddActionSignalTarget(this);
	}

	m_pPasswordText = vgui::SETUP_PANEL(new vgui::TextEntry(this, "PasswordEntry"));
	m_pPasswordText->AddActionSignalTarget(this);
	m_pPasswordText->SetZPos(20);
	m_pPasswordText->SetEditable(true);
	m_pPasswordText->SetTextHidden(true);
	m_pPasswordText->SetMaximumCharCount(-1);

	PerformLayout();
	InvalidateLayout();

	OnUpdate(false);
	SetAlpha(0);
}

CPasswordDialog::~CPasswordDialog()
{
}

void CPasswordDialog::ActivateUs(bool bActivate)
{
	SetVisible(bActivate);
}

void CPasswordDialog::OnUpdate(bool bInGame)
{
	if (!IsVisible())
		return;

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
		m_pButton[i]->OnUpdate();

	int w, h;
	GetSize(w, h);

	m_pBackground->SetSize(w, h);
	m_pBackground->SetPos(0, 0);

	m_pInfo->SetSize(w, scheme()->GetProportionalScaledValue(20));
	m_pInfo->SetPos(0, 0);

	m_pPasswordText->SetPos(0, scheme()->GetProportionalScaledValue(22));
	m_pPasswordText->SetSize(w, scheme()->GetProportionalScaledValue(14));

	m_pButton[0]->SetPos(scheme()->GetProportionalScaledValue(10), scheme()->GetProportionalScaledValue(38));
	m_pButton[1]->SetPos(w - scheme()->GetProportionalScaledValue(42), scheme()->GetProportionalScaledValue(38));
}

void CPasswordDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pInfo->SetFgColor(pScheme->GetColor("PasswordDialogTextColor", Color(255, 255, 255, 255)));
	m_pInfo->SetFont(pScheme->GetFont("OptionTextSmall"));

	m_pPasswordText->SetFgColor(pScheme->GetColor("PasswordDialogTextColor", Color(255, 255, 255, 255)));
	m_pPasswordText->SetFont(pScheme->GetFont("MainMenuTextSmall"));
}

void CPasswordDialog::OnCommand(const char *pcCommand)
{
	if (!Q_stricmp(pcCommand, "Activate"))
	{
		int zx, zy;
		vgui::input()->GetCursorPos(zx, zy);
		if (m_pButton[0]->IsWithin(zx, zy))
		{
			char szPassword[128];
			m_pPasswordText->GetText(szPassword, 128);
			GameBaseShared()->GetSteamServerManager()->DirectConnectWithPassword(szPassword);
		}
		else
			SetVisible(false);
	}

	BaseClass::OnCommand(pcCommand);
}

void CPasswordDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
		m_pButton[i]->SetSize(scheme()->GetProportionalScaledValue(32), scheme()->GetProportionalScaledValue(32));
}
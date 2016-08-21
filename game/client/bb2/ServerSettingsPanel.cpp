//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Setup Server Info - Custom BB2 cvars, hostname, password, maxplayers, etc...
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
#include "ServerSettingsPanel.h"
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

ServerSettingsPanel::ServerSettingsPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szTexts[] =
	{
		"#GameUI_ServerBrowser_Hostname",
		"#GameUI_CreateGame_Password",
		"#GameUI_CreateGame_MaxPlayers",
		"#GameUI_CreateGame_NPCScaleAmount",
		"#GameUI_CreateGame_SpawnProtection",
		"#GameUI_CreateGame_MercyValue",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pLabelInfo); i++)
	{
		m_pEditableField[i] = vgui::SETUP_PANEL(new vgui::TextEntry(this, ""));
		m_pEditableField[i]->SetZPos(35);
		m_pEditableField[i]->SetVisible(true);

		m_pLabelInfo[i] = vgui::SETUP_PANEL(new vgui::Label(this, "", ""));
		m_pLabelInfo[i]->SetZPos(30);
		m_pLabelInfo[i]->SetContentAlignment(Label::Alignment::a_center);
		m_pLabelInfo[i]->SetText(szTexts[i]);
	}

	const char *szCheckTexts[] =
	{
		"#GameUI_CreateGame_LateJoining",
		"#GameUI_CreateGame_NoTeamChangeClassic",
		"#GameUI_CreateGame_NPCAffectObjectives",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pCheckFields); i++)
	{
		m_pCheckFields[i] = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "", szCheckTexts[i], "OptionTextMedium"));
		m_pCheckFields[i]->SetZPos(40);
	}

	InvalidateLayout();
	PerformLayout();
}

ServerSettingsPanel::~ServerSettingsPanel()
{
}

void ServerSettingsPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	ConVarRef allow_latejoining("bb2_allow_latejoin");
	ConVarRef zombie_death_teamchange("bb2_classic_zombie_noteamchange");
	ConVarRef allow_npc_scoring("bb2_allow_npc_to_score");

	m_pCheckFields[0]->SetCheckedStatus(allow_latejoining.GetBool());
	m_pCheckFields[1]->SetCheckedStatus(zombie_death_teamchange.GetBool());
	m_pCheckFields[2]->SetCheckedStatus(allow_npc_scoring.GetBool());

	UpdateServerSettings();

	int w, h, x, y;
	GetSize(w, h);
	GetPos(x, y);

	for (int i = 0; i < _ARRAYSIZE(m_pLabelInfo); i++)
	{
		m_pLabelInfo[i]->SetPos(0, (scheme()->GetProportionalScaledValue(20) * i));
		m_pLabelInfo[i]->SetSize((w / 2), scheme()->GetProportionalScaledValue(18));

		m_pEditableField[i]->SetPos((w / 2) + scheme()->GetProportionalScaledValue(2), scheme()->GetProportionalScaledValue(4) + (scheme()->GetProportionalScaledValue(20) * i));
		m_pEditableField[i]->SetSize((w / 2) - scheme()->GetProportionalScaledValue(2), scheme()->GetProportionalScaledValue(12));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCheckFields); i++)
	{
		m_pCheckFields[i]->SetSize(w - scheme()->GetProportionalScaledValue(20), scheme()->GetProportionalScaledValue(12));
		m_pCheckFields[i]->SetPos(scheme()->GetProportionalScaledValue(25), scheme()->GetProportionalScaledValue(135) + (scheme()->GetProportionalScaledValue(15) * i));
	}
}

void ServerSettingsPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pLabelInfo); i++)
	{
		m_pLabelInfo[i]->SetFont(pScheme->GetFont("OptionTextMedium"));
		m_pLabelInfo[i]->SetFgColor(pScheme->GetColor("ServerSettingsLabelTextColor", Color(255, 255, 255, 255)));
	}
}

void ServerSettingsPanel::OnUpdate(bool bInGame)
{
}

void ServerSettingsPanel::ApplyServerSettings(void)
{
	char szHostname[64];
	char szPassword[64];

	m_pEditableField[0]->GetText(szHostname, 64);
	m_pEditableField[1]->GetText(szPassword, 64);

	engine->ClientCmd_Unrestricted(VarArgs("hostname %s\n", szHostname));
	engine->ClientCmd_Unrestricted(VarArgs("sv_password %s\n", szPassword));
	engine->ClientCmd_Unrestricted(VarArgs("maxplayers %i\n", m_pEditableField[2]->GetValueAsInt()));
	engine->ClientCmd_Unrestricted(VarArgs("bb2_npc_scaling %i\n", m_pEditableField[3]->GetValueAsInt()));
	engine->ClientCmd_Unrestricted(VarArgs("bb2_spawn_protection %f\n", m_pEditableField[4]->GetValueAsFloat()));
	engine->ClientCmd_Unrestricted(VarArgs("bb2_allow_mercy %i\n", m_pEditableField[5]->GetValueAsInt()));

	engine->ClientCmd_Unrestricted("sv_region 255\n");
	engine->ClientCmd_Unrestricted("mp_footsteps 1\n");
	engine->ClientCmd_Unrestricted("sv_footsteps 1\n");

	ConVar *pConvarList[] =
	{
		cvar->FindVar("bb2_allow_latejoin"),
		cvar->FindVar("bb2_classic_zombie_noteamchange"),
		cvar->FindVar("bb2_allow_npc_to_score"),
	};

	for (int i = 0; i < _ARRAYSIZE(m_pCheckFields); i++)
	{
		if (pConvarList[i])
		{
			if (m_pCheckFields[i]->IsChecked() && !pConvarList[i]->GetBool())
				pConvarList[i]->SetValue(1);
			else if (!m_pCheckFields[i]->IsChecked() && pConvarList[i]->GetBool())
				pConvarList[i]->SetValue(0);
		}
	}
}

void ServerSettingsPanel::UpdateServerSettings(void)
{
	ConVarRef hostname("hostname");
	ConVarRef sv_password("sv_password");
	ConVarRef npc_scaling("bb2_npc_scaling");
	ConVarRef bb2_spawn_protection("bb2_spawn_protection");
	ConVarRef bb2_allow_mercy("bb2_allow_mercy");

	m_pEditableField[0]->SetText(hostname.GetString());
	m_pEditableField[1]->SetText(sv_password.GetString());
	m_pEditableField[2]->SetText("12");
	m_pEditableField[3]->SetText(npc_scaling.GetString());
	m_pEditableField[4]->SetText(bb2_spawn_protection.GetString());
	m_pEditableField[5]->SetText(bb2_allow_mercy.GetString());
}
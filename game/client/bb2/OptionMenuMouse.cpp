//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Mouse Related Options
//
//========================================================================================//

#include "cbase.h"
#include "OptionMenuMouse.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

OptionMenuMouse::OptionMenuMouse(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szCheckOptions[] =
	{
		"#GameUI_MouseFilter",
		"#GameUI_ReverseMouse",
		"#GameUI_MouseRaw",
		"#GameUI_MouseAcceleration",
		"#GameUI_Joystick",
		"#GameUI_ReverseJoystick",
		"#GameUI_JoystickSouthpaw",
	};

	const char *szSliderOptions[] =
	{
		"#GameUI_MouseSensitivity",
		"#GameUI_MouseAcceleration",
		"#GameUI_JoystickLookSpeedYaw",
		"#GameUI_JoystickLookSpeedPitch",
	};

	const char *szCVARS[] =
	{
		"sensitivity",
		"m_customaccel_exponent",
		"joy_yawsensitivity",
		"joy_pitchsensitivity",
	};

	float flRangeMin[] =
	{
		1.0f,
		1.0f,
		0.5f,
		0.5f,
	};

	float flRangeMax[] =
	{
		6.0f,
		1.4f,
		7.0f,
		7.0f,
	};

	bool bNegatives[] =
	{
		false,
		false,
		true,
		false,
	};

	const char *szTitles[] =
	{
		"#GameUI_Mouse",
		"#GameUI_Joystick",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i] = vgui::SETUP_PANEL(new vgui::Label(this, "TitleLabel", szTitles[i]));
		m_pTextTitle[i]->SetZPos(70);
		m_pTextTitle[i]->SetContentAlignment(Label::a_center);

		m_pDivider[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Divider"));
		m_pDivider[i]->SetZPos(75);
		m_pDivider[i]->SetShouldScaleImage(true);
		m_pDivider[i]->SetImage("mainmenu/sub_divider");
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBoxVar); i++)
	{
		m_pCheckBoxVar[i] = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "CheckBox", szCheckOptions[i], "OptionTextMedium"));
		m_pCheckBoxVar[i]->AddActionSignalTarget(this);
		m_pCheckBoxVar[i]->SetZPos(50);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
	{
		m_pSlider[i] = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "GraphSlider", szSliderOptions[i], szCVARS[i], flRangeMin[i], flRangeMax[i], bNegatives[i], GraphicalOverlay::RawValueType::TYPE_FLOAT));
		m_pSlider[i]->SetZPos(60);
	}

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	InvalidateLayout();

	PerformLayout();
}

OptionMenuMouse::~OptionMenuMouse()
{
}

void OptionMenuMouse::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
			m_pSlider[i]->OnUpdate(bInGame);

		m_pCheckBoxVar[(_ARRAYSIZE(m_pCheckBoxVar) - 2)]->SetEnabled(m_pCheckBoxVar[4]->IsChecked());
		m_pCheckBoxVar[(_ARRAYSIZE(m_pCheckBoxVar) - 1)]->SetEnabled(m_pCheckBoxVar[4]->IsChecked());

		m_pSlider[1]->SetEnabled(m_pCheckBoxVar[3]->IsChecked());
		m_pSlider[2]->SetEnabled(m_pCheckBoxVar[(_ARRAYSIZE(m_pCheckBoxVar) - 3)]->IsChecked());
		m_pSlider[3]->SetEnabled(m_pCheckBoxVar[(_ARRAYSIZE(m_pCheckBoxVar) - 3)]->IsChecked());

		m_pApplyButton->OnUpdate();
	}
}

void OptionMenuMouse::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}
}

void OptionMenuMouse::ApplyChanges(void)
{
	static ConVarRef filter("m_filter");
	static ConVarRef pitch("m_pitch");
	static ConVarRef gamepad("joystick");
	static ConVarRef joy_inverty("joy_inverty");
	static ConVarRef joy_movement_stick("joy_movement_stick");
	static ConVarRef m_rawinput("m_rawinput");
	static ConVarRef m_customaccel("m_customaccel");

	filter.SetValue(m_pCheckBoxVar[0]->IsChecked());
	pitch.SetValue(m_pCheckBoxVar[1]->IsChecked() ? "-0.022000" : "0.022000");
	m_rawinput.SetValue(m_pCheckBoxVar[2]->IsChecked());
	m_customaccel.SetValue(m_pCheckBoxVar[3]->IsChecked() ? 3 : 0);
	gamepad.SetValue(m_pCheckBoxVar[4]->IsChecked());
	joy_inverty.SetValue(m_pCheckBoxVar[5]->IsChecked() && m_pCheckBoxVar[4]->IsChecked());
	joy_movement_stick.SetValue(m_pCheckBoxVar[6]->IsChecked() && m_pCheckBoxVar[4]->IsChecked());

	engine->ClientCmd_Unrestricted("host_writeconfig\n");
	engine->ClientCmd_Unrestricted("joyadvancedupdate\n");
}

void OptionMenuMouse::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h, wz, hz;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
	{
		m_pSlider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(20));
		m_pSlider[i]->GetSize(wz, hz);

		if (i < 2)
			m_pSlider[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(135) + (i * (hz + scheme()->GetProportionalScaledValue(4))));
		else
			m_pSlider[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(285) + ((i - 2) * (hz + scheme()->GetProportionalScaledValue(4))));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBoxVar); i++)
	{
		m_pCheckBoxVar[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(12));
		m_pCheckBoxVar[i]->GetSize(wz, hz);

		if (i < 4)
			m_pCheckBoxVar[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(55) + (i * (hz + scheme()->GetProportionalScaledValue(8))));
		else
			m_pCheckBoxVar[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(225) + ((i - 4) * (hz + scheme()->GetProportionalScaledValue(8))));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(2));
	}

	m_pTextTitle[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(12));
	m_pTextTitle[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(192));

	m_pDivider[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(37));
	m_pDivider[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(216));

	if (!IsVisible())
	{
		static ConVarRef filter("m_filter");
		static ConVarRef pitch("m_pitch");
		static ConVarRef gamepad("joystick");
		static ConVarRef joy_inverty("joy_inverty");
		static ConVarRef joy_movement_stick("joy_movement_stick");
		static ConVarRef m_rawinput("m_rawinput");
		static ConVarRef m_customaccel("m_customaccel");

		m_pCheckBoxVar[0]->SetCheckedStatus(filter.GetBool());
		m_pCheckBoxVar[1]->SetCheckedStatus((pitch.GetFloat() < 0) ? true : false);
		m_pCheckBoxVar[2]->SetCheckedStatus(m_rawinput.GetBool());
		m_pCheckBoxVar[3]->SetCheckedStatus(m_customaccel.GetBool());
		m_pCheckBoxVar[4]->SetCheckedStatus(gamepad.GetBool());
		m_pCheckBoxVar[5]->SetCheckedStatus(joy_inverty.GetBool());
		m_pCheckBoxVar[6]->SetCheckedStatus(joy_movement_stick.GetBool());
	}

	GetSize(w, h);
	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(52), h - scheme()->GetProportionalScaledValue(28));
}
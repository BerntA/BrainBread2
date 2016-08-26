//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Performance Options: Gibs, Particles, Misc stuff....
//
//========================================================================================//

#include "cbase.h"
#include <stdio.h>
#include "filesystem.h"
#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "OptionMenuPerformance.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include "vgui_controls/AnimationController.h"
#include <vgui_controls/SectionedListPanel.h>
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "inputsystem/iinputsystem.h"
#include "utlvector.h"
#include "KeyValues.h"
#include "filesystem.h"
#include <vgui_controls/TextImage.h>
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

OptionMenuPerformance::OptionMenuPerformance(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szOptions[] =
	{
		"#GameUI_Gore_Extreme",
		"#GameUI_Gore_BloodPuddles",
		"#GameUI_Gore_RagdollGibImpacts",
		"#GameUI_Gore_RagdollGibFade",
		"#GameUI_RenderMirror",
		"#GameUI_ParticleImpactFX",
		"#GameUI_RenderMuzzleDLight",
		"#GameUI_MulticoreRendering",
		"#GameUI_RenderWeaponAttachments",
		"#GameUI_GunEffectsFX",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBoxVar); i++)
	{
		m_pCheckBoxVar[i] = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "CheckBox", szOptions[i], "OptionTextMedium"));
		m_pCheckBoxVar[i]->AddActionSignalTarget(this);
		m_pCheckBoxVar[i]->SetZPos(50);
	}

	const char *szSliderOptions[] =
	{
		"#GameUI_Gore_GibLimit",
		"#GameUI_Gore_GibAndRagdollFadeTime",
		"#GameUI_Gore_BloodImpactChance",
	};

	const char *szCVARS[] =
	{
		"bb2_gibs_max",
		"bb2_gibs_fadeout_time",
		"bb2_gibs_blood_chance",
	};

	float flRangeMax[] =
	{
		128.0f,
		30.0f,
		100.0f,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
	{
		m_pSlider[i] = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "GraphSlider", szSliderOptions[i], szCVARS[i], 0.0f, flRangeMax[i], false, true));
		m_pSlider[i]->SetZPos(60);
	}

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	const char *szTitles[] =
	{
		"#GameUI_Gore",
		"#GameUI_Misc",
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

	InvalidateLayout();
	PerformLayout();
}

OptionMenuPerformance::~OptionMenuPerformance()
{
}

void OptionMenuPerformance::ApplyChanges(void)
{
	// Gore Options:
	ConVarRef bb2_extreme_gore("bb2_extreme_gore");
	ConVarRef bb2_gibs_spawn_blood_puddle("bb2_gibs_spawn_blood_puddle");
	ConVarRef bb2_gibs_spawn_blood("bb2_gibs_spawn_blood");
	ConVarRef bb2_gibs_enable_fade("bb2_gibs_enable_fade");

	// Misc Options:
	ConVarRef bb2_render_client_in_mirrors("bb2_render_client_in_mirrors");
	ConVarRef cl_new_impact_effects("cl_new_impact_effects");
	ConVarRef muzzleflash_light("muzzleflash_light");
	ConVarRef bb2_render_weapon_attachments("bb2_render_weapon_attachments");
	ConVarRef bb2_enable_particle_gunfx("bb2_enable_particle_gunfx");

	bb2_extreme_gore.SetValue(m_pCheckBoxVar[0]->IsChecked());
	bb2_gibs_spawn_blood_puddle.SetValue(m_pCheckBoxVar[1]->IsChecked());
	bb2_gibs_spawn_blood.SetValue(m_pCheckBoxVar[2]->IsChecked());
	bb2_gibs_enable_fade.SetValue(m_pCheckBoxVar[3]->IsChecked());

	bb2_render_client_in_mirrors.SetValue(m_pCheckBoxVar[4]->IsChecked());
	cl_new_impact_effects.SetValue(m_pCheckBoxVar[5]->IsChecked());
	muzzleflash_light.SetValue(m_pCheckBoxVar[6]->IsChecked());
	bb2_enable_multicore.SetValue(m_pCheckBoxVar[7]->IsChecked());
	bb2_render_weapon_attachments.SetValue(m_pCheckBoxVar[8]->IsChecked());
	bb2_enable_particle_gunfx.SetValue(m_pCheckBoxVar[9]->IsChecked());

	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}

void OptionMenuPerformance::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
			m_pSlider[i]->OnUpdate(bInGame);

		m_pApplyButton->OnUpdate();
	}
}

void OptionMenuPerformance::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h, wz, hz;
	GetSize(w, h);

	if (!IsVisible())
	{
		// Gore Options:
		ConVarRef bb2_extreme_gore("bb2_extreme_gore");
		ConVarRef bb2_gibs_spawn_blood_puddle("bb2_gibs_spawn_blood_puddle");
		ConVarRef bb2_gibs_spawn_blood("bb2_gibs_spawn_blood");
		ConVarRef bb2_gibs_enable_fade("bb2_gibs_enable_fade");

		// Misc Options:
		ConVarRef bb2_render_client_in_mirrors("bb2_render_client_in_mirrors");
		ConVarRef cl_new_impact_effects("cl_new_impact_effects");
		ConVarRef muzzleflash_light("muzzleflash_light");
		ConVarRef bb2_render_weapon_attachments("bb2_render_weapon_attachments");
		ConVarRef bb2_enable_particle_gunfx("bb2_enable_particle_gunfx");

		m_pCheckBoxVar[0]->SetCheckedStatus(bb2_extreme_gore.GetBool());
		m_pCheckBoxVar[1]->SetCheckedStatus(bb2_gibs_spawn_blood_puddle.GetBool());
		m_pCheckBoxVar[2]->SetCheckedStatus(bb2_gibs_spawn_blood.GetBool());
		m_pCheckBoxVar[3]->SetCheckedStatus(bb2_gibs_enable_fade.GetBool());
		m_pCheckBoxVar[4]->SetCheckedStatus(bb2_render_client_in_mirrors.GetBool());
		m_pCheckBoxVar[5]->SetCheckedStatus(cl_new_impact_effects.GetBool());
		m_pCheckBoxVar[6]->SetCheckedStatus(muzzleflash_light.GetBool());
		m_pCheckBoxVar[7]->SetCheckedStatus(bb2_enable_multicore.GetBool());
		m_pCheckBoxVar[8]->SetCheckedStatus(bb2_render_weapon_attachments.GetBool());
		m_pCheckBoxVar[9]->SetCheckedStatus(bb2_enable_particle_gunfx.GetBool());
	}

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(2));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBoxVar); i++)
	{
		m_pCheckBoxVar[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(12));
		m_pCheckBoxVar[i]->GetSize(wz, hz);

		if (i < 4)
			m_pCheckBoxVar[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(55) + (i * (hz + scheme()->GetProportionalScaledValue(8))));
		else
			m_pCheckBoxVar[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(257) + ((i - 4) * (hz + scheme()->GetProportionalScaledValue(8))));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
	{
		m_pSlider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(20));
		m_pSlider[i]->GetSize(wz, hz);
		m_pSlider[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(137) + (i * (hz + scheme()->GetProportionalScaledValue(4))));
	}

	m_pTextTitle[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(12));
	m_pTextTitle[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(214));

	m_pDivider[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(37));
	m_pDivider[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(239));

	GetSize(w, h);
	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(52), h - scheme()->GetProportionalScaledValue(28));
}

void OptionMenuPerformance::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}
}
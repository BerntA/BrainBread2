//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Custom Options - BB2 Custom Options & Other interesting tweak options for better performance / graphics.
//
//========================================================================================//

#include "cbase.h"
#include "OptionMenuOther.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Client.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define DIVIDER_START_XPOS 55

OptionMenuOther::OptionMenuOther(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szOptions[] =
	{
		"#GameUI_NPCHealthBarAll",
		"#GameUI_HUDNumericDetails",
		"#GameUI_FastSwitchCheck",
		"#GameUI_CombatText",
		"#GameUI_ScopeRefract",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBoxVar); i++)
	{
		m_pCheckBoxVar[i] = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "CheckBox", szOptions[i], "OptionTextMedium"));
		m_pCheckBoxVar[i]->AddActionSignalTarget(this);
		m_pCheckBoxVar[i]->SetZPos(50);
	}

	const char *szInfo[] =
	{
		"#GameUI_SOUNDSET_ZOMBIE",
		"#GameUI_SOUNDSET_FRED",
		"#GameUI_SOUNDSET_MILITARY",
		"#GameUI_SOUNDSET_BANDIT",
		"#GameUI_SOUNDSET_ANNOUNCER",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pComboSoundSet); i++)
	{
		m_pComboSoundSet[i] = vgui::SETUP_PANEL(new vgui::ComboList(this, "SoundComboList", szInfo[i], 6));
		m_pComboSoundSet[i]->SetZPos(50);
	}

	const char *szTitles[] =
	{
		"#GameUI_Misc",
		"#GameUI_NPC_SOUNDSETS",
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

	const char *szComboImg[] =
	{
		"#GameUI_CrosshairSelect",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pComboImgList); i++)
	{
		m_pComboImgList[i] = vgui::SETUP_PANEL(new vgui::ComboImageList(this, "ComboImgList", szComboImg[i], false));
		m_pComboImgList[i]->SetZPos(60);
		m_pComboImgList[i]->AddActionSignalTarget(this);
	}

	const char *szSliderOptions[] =
	{
		"#GameUI_CrosshairRed",
		"#GameUI_CrosshairGreen",
		"#GameUI_CrosshairBlue",
		"#GameUI_CrosshairAlpha",
	};

	const char *szCVARS[] =
	{
		"crosshair_color_red",
		"crosshair_color_green",
		"crosshair_color_blue",
		"crosshair_color_alpha",
	};

	m_pViewmodelFOVSlider = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "ViewmodelFOVSlider", "#GameUI_ViewmodelFOV", "viewmodel_fov", 30, 130, false, GraphicalOverlay::RawValueType::TYPE_INT));
	m_pViewmodelFOVSlider->SetZPos(60);

	for (int i = 0; i < _ARRAYSIZE(m_pCrosshairColorSlider); i++)
	{
		m_pCrosshairColorSlider[i] = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "CrosshairSlider", szSliderOptions[i], szCVARS[i], 0, 255, false, GraphicalOverlay::RawValueType::TYPE_INT));
		m_pCrosshairColorSlider[i]->SetZPos(60);
	}

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	InvalidateLayout();
	PerformLayout();
}

OptionMenuOther::~OptionMenuOther()
{
}

void OptionMenuOther::ApplyChanges(void)
{
	static ConVarRef crosshair("crosshair");

	static ConVarRef bb2_sound_zombie("bb2_sound_zombie");
	static ConVarRef bb2_sound_fred("bb2_sound_fred");
	static ConVarRef bb2_sound_military("bb2_sound_military");
	static ConVarRef bb2_sound_bandit("bb2_sound_bandit");
	static ConVarRef bb2_sound_announcer("bb2_sound_announcer");

	static ConVarRef bb2_enable_healthbar_for_all("bb2_enable_healthbar_for_all");
	static ConVarRef bb2_show_details("bb2_show_details");
	static ConVarRef hud_fastswitch("hud_fastswitch");
	static ConVarRef bb2_render_damage_text("bb2_render_damage_text");
	static ConVarRef bb2_scope_refraction("bb2_scope_refraction");

	bb2_sound_zombie.SetValue(m_pComboSoundSet[0]->GetComboBox()->GetActiveItem());
	bb2_sound_fred.SetValue(m_pComboSoundSet[1]->GetComboBox()->GetActiveItem());
	bb2_sound_military.SetValue(m_pComboSoundSet[2]->GetComboBox()->GetActiveItem());
	bb2_sound_bandit.SetValue(m_pComboSoundSet[3]->GetComboBox()->GetActiveItem());
	bb2_sound_announcer.SetValue(m_pComboSoundSet[4]->GetComboBox()->GetActiveItem());

	crosshair.SetValue((m_pComboImgList[0]->GetActiveItem() + 1));

	bb2_enable_healthbar_for_all.SetValue(m_pCheckBoxVar[0]->IsChecked());
	bb2_show_details.SetValue(m_pCheckBoxVar[1]->IsChecked());
	hud_fastswitch.SetValue(m_pCheckBoxVar[2]->IsChecked());
	bb2_render_damage_text.SetValue(m_pCheckBoxVar[3]->IsChecked());
	bb2_scope_refraction.SetValue(m_pCheckBoxVar[4]->IsChecked());

	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}

void OptionMenuOther::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pCrosshairColorSlider); i++)
			m_pCrosshairColorSlider[i]->OnUpdate(bInGame);

		m_pViewmodelFOVSlider->OnUpdate(bInGame);

		m_pApplyButton->OnUpdate();
	}
}

void OptionMenuOther::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h, wz, hz;
	GetSize(w, h);

	if (!IsVisible())
	{
		int types[] =
		{
			BB2_SoundTypes::TYPE_ZOMBIE,
			BB2_SoundTypes::TYPE_FRED,
			BB2_SoundTypes::TYPE_SOLDIER,
			BB2_SoundTypes::TYPE_BANDIT,
			BB2_SoundTypes::TYPE_ANNOUNCER,
		};

		for (int i = 0; i < _ARRAYSIZE(m_pComboSoundSet); i++)
		{
			m_pComboSoundSet[i]->GetComboBox()->RemoveAll();
			GameBaseShared()->GetSharedGameDetails()->AddSoundScriptItems(m_pComboSoundSet[i], types[i]);
		}

		static ConVarRef bb2_sound_zombie("bb2_sound_zombie");
		static ConVarRef bb2_sound_fred("bb2_sound_fred");
		static ConVarRef bb2_sound_military("bb2_sound_military");
		static ConVarRef bb2_sound_bandit("bb2_sound_bandit");
		static ConVarRef bb2_sound_announcer("bb2_sound_announcer");

		static ConVarRef bb2_enable_healthbar_for_all("bb2_enable_healthbar_for_all");
		static ConVarRef bb2_show_details("bb2_show_details");
		static ConVarRef hud_fastswitch("hud_fastswitch");
		static ConVarRef bb2_render_damage_text("bb2_render_damage_text");
		static ConVarRef bb2_scope_refraction("bb2_scope_refraction");

		m_pCheckBoxVar[0]->SetCheckedStatus(bb2_enable_healthbar_for_all.GetBool());
		m_pCheckBoxVar[1]->SetCheckedStatus(bb2_show_details.GetBool());
		m_pCheckBoxVar[2]->SetCheckedStatus(hud_fastswitch.GetBool());
		m_pCheckBoxVar[3]->SetCheckedStatus(bb2_render_damage_text.GetBool());
		m_pCheckBoxVar[4]->SetCheckedStatus(bb2_scope_refraction.GetBool());

		m_pComboSoundSet[0]->GetComboBox()->ActivateItem(clamp(bb2_sound_zombie.GetInt(), 0, (m_pComboSoundSet[0]->GetComboBox()->GetItemCount() - 1)));
		m_pComboSoundSet[1]->GetComboBox()->ActivateItem(clamp(bb2_sound_fred.GetInt(), 0, (m_pComboSoundSet[1]->GetComboBox()->GetItemCount() - 1)));
		m_pComboSoundSet[2]->GetComboBox()->ActivateItem(clamp(bb2_sound_military.GetInt(), 0, (m_pComboSoundSet[2]->GetComboBox()->GetItemCount() - 1)));
		m_pComboSoundSet[3]->GetComboBox()->ActivateItem(clamp(bb2_sound_bandit.GetInt(), 0, (m_pComboSoundSet[3]->GetComboBox()->GetItemCount() - 1)));
		m_pComboSoundSet[4]->GetComboBox()->ActivateItem(clamp(bb2_sound_announcer.GetInt(), 0, (m_pComboSoundSet[4]->GetComboBox()->GetItemCount() - 1)));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(2));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBoxVar); i++)
	{
		m_pCheckBoxVar[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(12));
		m_pCheckBoxVar[i]->GetSize(wz, hz);
		m_pCheckBoxVar[i]->SetPos(scheme()->GetProportionalScaledValue(DIVIDER_START_XPOS), scheme()->GetProportionalScaledValue(55) + (i * (hz + scheme()->GetProportionalScaledValue(8))));
	}

	m_pViewmodelFOVSlider->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(20));
	m_pViewmodelFOVSlider->SetPos(scheme()->GetProportionalScaledValue(DIVIDER_START_XPOS), scheme()->GetProportionalScaledValue(155));

	for (int i = 0; i < _ARRAYSIZE(m_pComboImgList); i++)
	{
		m_pComboImgList[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(60));
		m_pComboImgList[i]->SetPos(scheme()->GetProportionalScaledValue(DIVIDER_START_XPOS), scheme()->GetProportionalScaledValue(176));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCrosshairColorSlider); i++)
	{
		m_pCrosshairColorSlider[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(20));
		m_pCrosshairColorSlider[i]->GetSize(wz, hz);
		m_pCrosshairColorSlider[i]->SetPos(scheme()->GetProportionalScaledValue(DIVIDER_START_XPOS), scheme()->GetProportionalScaledValue(237) + (i * (hz + scheme()->GetProportionalScaledValue(4))));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pComboSoundSet); i++)
	{
		m_pComboSoundSet[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(24));
		m_pComboSoundSet[i]->GetSize(wz, hz);
		m_pComboSoundSet[i]->SetPos((w - scheme()->GetProportionalScaledValue(300 + DIVIDER_START_XPOS)), scheme()->GetProportionalScaledValue(50) + (i * (hz + scheme()->GetProportionalScaledValue(1))));
	}

	m_pTextTitle[0]->SetPos(scheme()->GetProportionalScaledValue(DIVIDER_START_XPOS), scheme()->GetProportionalScaledValue(12));
	m_pTextTitle[1]->SetPos((w - scheme()->GetProportionalScaledValue(300 + DIVIDER_START_XPOS)), scheme()->GetProportionalScaledValue(12));

	m_pDivider[0]->SetPos(scheme()->GetProportionalScaledValue(DIVIDER_START_XPOS), scheme()->GetProportionalScaledValue(37));
	m_pDivider[1]->SetPos((w - scheme()->GetProportionalScaledValue(300 + DIVIDER_START_XPOS)), scheme()->GetProportionalScaledValue(37));

	m_pComboImgList[0]->InitCrosshairList();
	m_pComboImgList[0]->SetActiveItem((crosshair.GetInt() - 1));

	GetSize(w, h);
	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(52), h - scheme()->GetProportionalScaledValue(28));
}

void OptionMenuOther::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}
}
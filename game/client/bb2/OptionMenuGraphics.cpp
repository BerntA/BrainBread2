//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Graphic Related Options
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
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "OptionMenuGraphics.h"
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
#include "IGameUIFuncs.h"
#include "modes.h"
#include "utlvector.h"
#include "materialsystem/materialsystem_config.h"
#include "inetchannelinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

OptionMenuGraphics::OptionMenuGraphics(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szName[] =
	{
		"#GameUI_Model_Detail",
		"#GameUI_Texture_Detail",
		"#GameUI_Antialiasing_Mode",
		"#GameUI_Filtering_Mode",
		"#GameUI_Shadow_Detail",
		"#GameUI_Water_Detail",
		"#GameUI_Wait_For_VSync",
		"#GameUI_HDR"
	};

	const char *szTitles[] =
	{
		"#GameUI_Graphics",
		"#GameUI_Experimental",
	};

	const char *szSliderOptions[] =
	{
		"#GameUI_RainParticlesLimit",
		"#GameUI_MaxDecals",
		"#GameUI_MaxModelDecals",
	};

	const char *szSliderCVARS[] =
	{
		"r_RainSplashPercentage",
		"r_decals",
		"r_maxmodeldecal",
	};

	float flSliderMins[] =
	{
		1,
		16,
		16,
	};

	float flSliderMax[] =
	{
		100,
		4096,
		256,
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

	for (int i = 0; i < _ARRAYSIZE(m_pGraphicsCombo); i++)
	{
		m_pGraphicsCombo[i] = vgui::SETUP_PANEL(new vgui::ComboList(this, "ComboList", szName[i], 6));
		m_pGraphicsCombo[i]->SetZPos(60);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
	{
		m_pSlider[i] = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "Slider", szSliderOptions[i], szSliderCVARS[i], flSliderMins[i], flSliderMax[i], false, true));
		m_pSlider[i]->SetZPos(60);
	}

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	// Add items
	// Model
	m_pGraphicsCombo[0]->GetComboBox()->AddItem("#gameui_low", NULL);
	m_pGraphicsCombo[0]->GetComboBox()->AddItem("#gameui_medium", NULL);
	m_pGraphicsCombo[0]->GetComboBox()->AddItem("#gameui_high", NULL);

	// Texture
	m_pGraphicsCombo[1]->GetComboBox()->AddItem("#gameui_low", NULL);
	m_pGraphicsCombo[1]->GetComboBox()->AddItem("#gameui_medium", NULL);
	m_pGraphicsCombo[1]->GetComboBox()->AddItem("#gameui_high", NULL);
	m_pGraphicsCombo[1]->GetComboBox()->AddItem("#gameui_ultra", NULL);

	// Antialias
	m_nNumAAModes = 0;
	m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_None", NULL);
	m_nAAModes[m_nNumAAModes].m_nNumSamples = 1;
	m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
	m_nNumAAModes++;

	if (materials->SupportsMSAAMode(2))
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_2X", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 2;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	if (materials->SupportsMSAAMode(4))
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_4X", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	if (materials->SupportsMSAAMode(6))
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_6X", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 6;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	if (materials->SupportsCSAAMode(4, 2))							// nVidia CSAA			"8x"
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_8X_CSAA", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 2;
		m_nNumAAModes++;
	}

	if (materials->SupportsCSAAMode(4, 4))							// nVidia CSAA			"16x"
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_16X_CSAA", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 4;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 4;
		m_nNumAAModes++;
	}

	if (materials->SupportsMSAAMode(8))
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_8X", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 8;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 0;
		m_nNumAAModes++;
	}

	if (materials->SupportsCSAAMode(8, 2))							// nVidia CSAA			"16xQ"
	{
		m_pGraphicsCombo[2]->GetComboBox()->AddItem("#GameUI_16XQ_CSAA", NULL);
		m_nAAModes[m_nNumAAModes].m_nNumSamples = 8;
		m_nAAModes[m_nNumAAModes].m_nQualityLevel = 2;
		m_nNumAAModes++;
	}

	// Filtering
	m_pGraphicsCombo[3]->GetComboBox()->AddItem("#GameUI_Bilinear", NULL);
	m_pGraphicsCombo[3]->GetComboBox()->AddItem("#GameUI_Trilinear", NULL);
	m_pGraphicsCombo[3]->GetComboBox()->AddItem("#GameUI_Anisotropic2X", NULL);
	m_pGraphicsCombo[3]->GetComboBox()->AddItem("#GameUI_Anisotropic4X", NULL);
	m_pGraphicsCombo[3]->GetComboBox()->AddItem("#GameUI_Anisotropic8X", NULL);
	m_pGraphicsCombo[3]->GetComboBox()->AddItem("#GameUI_Anisotropic16X", NULL);

	// Shadows
	m_pGraphicsCombo[4]->GetComboBox()->AddItem("#gameui_low", NULL);
	m_pGraphicsCombo[4]->GetComboBox()->AddItem("#gameui_medium", NULL);
	m_pGraphicsCombo[4]->GetComboBox()->AddItem("#gameui_high", NULL);

	// Reflections in water
	m_pGraphicsCombo[5]->GetComboBox()->AddItem("#gameui_noreflections", NULL);
	m_pGraphicsCombo[5]->GetComboBox()->AddItem("#gameui_reflectonlyworld", NULL);
	m_pGraphicsCombo[5]->GetComboBox()->AddItem("#gameui_reflectall", NULL);

	// VSync
	m_pGraphicsCombo[6]->GetComboBox()->AddItem("#gameui_disabled", NULL);
	m_pGraphicsCombo[6]->GetComboBox()->AddItem("#gameui_enabled", NULL);

	// HDR
	m_pGraphicsCombo[7]->GetComboBox()->AddItem("#GameUI_hdr_level0", NULL);
	m_pGraphicsCombo[7]->GetComboBox()->AddItem("#GameUI_hdr_level1", NULL);

	if (materials->SupportsHDRMode(HDR_TYPE_INTEGER))
		m_pGraphicsCombo[7]->GetComboBox()->AddItem("#GameUI_hdr_level2", NULL);

	MarkDefaultSettingsAsRecommended();

	InvalidateLayout();

	PerformLayout();
}

OptionMenuGraphics::~OptionMenuGraphics()
{
}

void OptionMenuGraphics::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
			m_pSlider[i]->OnUpdate(bInGame);

		m_pApplyButton->OnUpdate();
	}
}

void OptionMenuGraphics::ApplyChanges(void)
{
	ApplyChangesToConVar("r_rootlod", 2 - m_pGraphicsCombo[0]->GetComboBox()->GetActiveItem());
	ApplyChangesToConVar("mat_picmip", 2 - m_pGraphicsCombo[1]->GetComboBox()->GetActiveItem());

	// reset everything tied to the filtering mode, then the switch sets the appropriate one
	ApplyChangesToConVar("mat_trilinear", false);
	ApplyChangesToConVar("mat_forceaniso", 1);

	switch (m_pGraphicsCombo[3]->GetComboBox()->GetActiveItem())
	{
	case 0:
		break;
	case 1:
		ApplyChangesToConVar("mat_trilinear", true);
		break;
	case 2:
		ApplyChangesToConVar("mat_forceaniso", 2);
		break;
	case 3:
		ApplyChangesToConVar("mat_forceaniso", 4);
		break;
	case 4:
		ApplyChangesToConVar("mat_forceaniso", 8);
		break;
	case 5:
		ApplyChangesToConVar("mat_forceaniso", 16);
		break;
	}

	// Set the AA convars according to the menu item chosen
	int nActiveAAItem = m_pGraphicsCombo[2]->GetComboBox()->GetActiveItem();
	ApplyChangesToConVar("mat_antialias", m_nAAModes[nActiveAAItem].m_nNumSamples);
	ApplyChangesToConVar("mat_aaquality", m_nAAModes[nActiveAAItem].m_nQualityLevel);

	if (m_pGraphicsCombo[4]->GetComboBox()->GetActiveItem() == 0)						// Blobby shadows
	{
		ApplyChangesToConVar("r_shadowrendertotexture", 0);			// Turn off RTT shadows
		ApplyChangesToConVar("r_flashlightdepthtexture", 0);			// Turn off shadow depth textures
	}
	else if (m_pGraphicsCombo[4]->GetComboBox()->GetActiveItem() == 1)					// RTT shadows only
	{
		ApplyChangesToConVar("r_shadowrendertotexture", 1);			// Turn on RTT shadows
		ApplyChangesToConVar("r_flashlightdepthtexture", 0);			// Turn off shadow depth textures
	}
	else if (m_pGraphicsCombo[4]->GetComboBox()->GetActiveItem() == 2)					// Shadow depth textures
	{
		ApplyChangesToConVar("r_shadowrendertotexture", 1);			// Turn on RTT shadows
		ApplyChangesToConVar("r_flashlightdepthtexture", 1);			// Turn on shadow depth textures
	}

	switch (m_pGraphicsCombo[5]->GetComboBox()->GetActiveItem())
	{
	default:
	case 0:
		ApplyChangesToConVar("r_waterforceexpensive", false);
		ApplyChangesToConVar("r_waterforcereflectentities", false);
		break;
	case 1:
		ApplyChangesToConVar("r_waterforceexpensive", true);
		ApplyChangesToConVar("r_waterforcereflectentities", false);
		break;
	case 2:
		ApplyChangesToConVar("r_waterforceexpensive", true);
		ApplyChangesToConVar("r_waterforcereflectentities", true);
		break;
	}

	ApplyChangesToConVar("mat_vsync", m_pGraphicsCombo[6]->GetComboBox()->GetActiveItem());
	ApplyChangesToConVar("mat_hdr_level", m_pGraphicsCombo[7]->GetComboBox()->GetActiveItem());

	// Save
	engine->ClientCmd_Unrestricted("mat_savechanges\n");
	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}

void OptionMenuGraphics::SetupLayout(void)
{
	BaseClass::SetupLayout();

	if (!IsVisible())
	{
		ConVarRef r_rootlod("r_rootlod");
		ConVarRef mat_picmip("mat_picmip");
		ConVarRef mat_trilinear("mat_trilinear");
		ConVarRef mat_forceaniso("mat_forceaniso");
		ConVarRef mat_antialias("mat_antialias");
		ConVarRef mat_aaquality("mat_aaquality");
		ConVarRef mat_vsync("mat_vsync");
		ConVarRef mat_hdr_level("mat_hdr_level");
		ConVarRef r_flashlightdepthtexture("r_flashlightdepthtexture");
		ConVarRef r_waterforceexpensive("r_waterforceexpensive");
		ConVarRef r_waterforcereflectentities("r_waterforcereflectentities");
		ConVarRef r_shadowrendertotexture("r_shadowrendertotexture");

		m_pGraphicsCombo[0]->GetComboBox()->ActivateItem(2 - clamp(r_rootlod.GetInt(), 0, 2));
		m_pGraphicsCombo[1]->GetComboBox()->ActivateItem(2 - clamp(mat_picmip.GetInt(), -1, 2));

		if (r_flashlightdepthtexture.GetBool())		// If we're doing flashlight shadow depth texturing...
		{
			r_shadowrendertotexture.SetValue(1);		// ...be sure render to texture shadows are also on
			m_pGraphicsCombo[4]->GetComboBox()->ActivateItem(2);
		}
		else if (r_shadowrendertotexture.GetBool())	// RTT shadows, but not shadow depth texturing
			m_pGraphicsCombo[4]->GetComboBox()->ActivateItem(1);
		else	// Lowest shadow quality
			m_pGraphicsCombo[4]->GetComboBox()->ActivateItem(0);

		switch (mat_forceaniso.GetInt())
		{
		case 2:
			m_pGraphicsCombo[3]->GetComboBox()->ActivateItem(2);
			break;
		case 4:
			m_pGraphicsCombo[3]->GetComboBox()->ActivateItem(3);
			break;
		case 8:
			m_pGraphicsCombo[3]->GetComboBox()->ActivateItem(4);
			break;
		case 16:
			m_pGraphicsCombo[3]->GetComboBox()->ActivateItem(5);
			break;
		case 0:
		default:
			if (mat_trilinear.GetBool())
				m_pGraphicsCombo[3]->GetComboBox()->ActivateItem(1);
			else
				m_pGraphicsCombo[3]->GetComboBox()->ActivateItem(0);
			break;
		}

		// Map convar to item on AA drop-down
		int nAASamples = mat_antialias.GetInt();
		int nAAQuality = mat_aaquality.GetInt();
		int nMSAAMode = FindMSAAMode(nAASamples, nAAQuality);
		m_pGraphicsCombo[2]->GetComboBox()->ActivateItem(nMSAAMode);

		if (r_waterforceexpensive.GetBool())
		{
			if (r_waterforcereflectentities.GetBool())
				m_pGraphicsCombo[5]->GetComboBox()->ActivateItem(2);
			else
				m_pGraphicsCombo[5]->GetComboBox()->ActivateItem(1);
		}
		else
			m_pGraphicsCombo[5]->GetComboBox()->ActivateItem(0);

		m_pGraphicsCombo[6]->GetComboBox()->ActivateItem(mat_vsync.GetInt());
		m_pGraphicsCombo[7]->GetComboBox()->ActivateItem(clamp(mat_hdr_level.GetInt(), 0, 2));
	}

	// Set positions:
	int w, h, wz, hz;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(2));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pGraphicsCombo); i++)
	{
		m_pGraphicsCombo[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));
		m_pGraphicsCombo[i]->GetSize(wz, hz);
		m_pGraphicsCombo[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(55) + (i * (hz + scheme()->GetProportionalScaledValue(1))));
	}

	m_pTextTitle[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(12));
	m_pTextTitle[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(257));

	m_pDivider[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(37));
	m_pDivider[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(282));

	for (int i = 0; i < _ARRAYSIZE(m_pSlider); i++)
	{
		m_pSlider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(20));
		m_pSlider[i]->GetSize(wz, hz);
		m_pSlider[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(290) + (i * (hz + scheme()->GetProportionalScaledValue(4))));
	}

	GetSize(w, h);
	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(52), h - scheme()->GetProportionalScaledValue(28));
}

void OptionMenuGraphics::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}
}

void OptionMenuGraphics::ApplyChangesToConVar(const char *pConVarName, int value)
{
	Assert(cvar->FindVar(pConVarName));
	char szCmd[256];
	Q_snprintf(szCmd, sizeof(szCmd), "%s %d\n", pConVarName, value);
	engine->ClientCmd_Unrestricted(szCmd);
}

int OptionMenuGraphics::FindMSAAMode(int nAASamples, int nAAQuality)
{
	// Run through the AA Modes supported by the device
	for (int nAAMode = 0; nAAMode < m_nNumAAModes; nAAMode++)
	{
		// If we found the mode that matches what we're looking for, return the index
		if ((m_nAAModes[nAAMode].m_nNumSamples == nAASamples) && (m_nAAModes[nAAMode].m_nQualityLevel == nAAQuality))
			return nAAMode;
	}

	return 0;	// Didn't find what we're looking for, so no AA
}

void OptionMenuGraphics::SetComboItemAsRecommended(vgui::ComboBox *combo, int iItem)
{
	// get the item text
	wchar_t text[512];
	combo->GetItemText(iItem, text, sizeof(text));

	// append the recommended flag
	wchar_t newText[512];
	_snwprintf(newText, sizeof(newText) / sizeof(wchar_t), L"%s *", text);

	// reset
	combo->UpdateItem(iItem, newText, NULL);
}

void OptionMenuGraphics::MarkDefaultSettingsAsRecommended()
{
	// Pull in data from dxsupport.cfg database (includes fine-grained per-vendor/per-device config data)
	KeyValues *pKeyValues = new KeyValues("config");
	materials->GetRecommendedConfigurationInfo(0, pKeyValues);

	// Read individual values from keyvalues which came from dxsupport.cfg database
	int nSkipLevels = pKeyValues->GetInt("ConVar.mat_picmip", -1);
	int nAnisotropicLevel = pKeyValues->GetInt("ConVar.mat_forceaniso", 16);
	int nForceTrilinear = pKeyValues->GetInt("ConVar.mat_trilinear", 0);
	int nAASamples = pKeyValues->GetInt("ConVar.mat_antialias", 8);
	int nAAQuality = pKeyValues->GetInt("ConVar.mat_aaquality", 0);
	int nRenderToTextureShadows = pKeyValues->GetInt("ConVar.r_shadowrendertotexture", 1);
	int nShadowDepthTextureShadows = pKeyValues->GetInt("ConVar.r_flashlightdepthtexture", 1);
	int nWaterUseRealtimeReflection = pKeyValues->GetInt("ConVar.r_waterforceexpensive", 1);
	int nWaterUseEntityReflection = pKeyValues->GetInt("ConVar.r_waterforcereflectentities", 1);
	int nMatVSync = pKeyValues->GetInt("ConVar.mat_vsync", 1);
	int nRootLOD = pKeyValues->GetInt("ConVar.r_rootlod", 0);

	SetComboItemAsRecommended(m_pGraphicsCombo[0]->GetComboBox(), 2 - nRootLOD);
	SetComboItemAsRecommended(m_pGraphicsCombo[1]->GetComboBox(), 2 - nSkipLevels);

	switch (nAnisotropicLevel)
	{
	case 2:
		SetComboItemAsRecommended(m_pGraphicsCombo[3]->GetComboBox(), 2);
		break;
	case 4:
		SetComboItemAsRecommended(m_pGraphicsCombo[3]->GetComboBox(), 3);
		break;
	case 8:
		SetComboItemAsRecommended(m_pGraphicsCombo[3]->GetComboBox(), 4);
		break;
	case 16:
		SetComboItemAsRecommended(m_pGraphicsCombo[3]->GetComboBox(), 5);
		break;
	case 0:
	default:
		if (nForceTrilinear != 0)
			SetComboItemAsRecommended(m_pGraphicsCombo[3]->GetComboBox(), 1);
		else
			SetComboItemAsRecommended(m_pGraphicsCombo[3]->GetComboBox(), 0);
		break;
	}

	// Map desired mode to list item number
	int nMSAAMode = FindMSAAMode(nAASamples, nAAQuality);
	SetComboItemAsRecommended(m_pGraphicsCombo[2]->GetComboBox(), nMSAAMode);

	if (nShadowDepthTextureShadows)
		SetComboItemAsRecommended(m_pGraphicsCombo[4]->GetComboBox(), 2);	// Shadow depth mapping (in addition to RTT shadows)
	else if (nRenderToTextureShadows)
		SetComboItemAsRecommended(m_pGraphicsCombo[4]->GetComboBox(), 1);	// RTT shadows
	else
		SetComboItemAsRecommended(m_pGraphicsCombo[4]->GetComboBox(), 0);	// Blobbies

	if (nWaterUseRealtimeReflection)
	{
		if (nWaterUseEntityReflection)
			SetComboItemAsRecommended(m_pGraphicsCombo[5]->GetComboBox(), 2);
		else
			SetComboItemAsRecommended(m_pGraphicsCombo[5]->GetComboBox(), 1);
	}
	else
		SetComboItemAsRecommended(m_pGraphicsCombo[5]->GetComboBox(), 0);

	SetComboItemAsRecommended(m_pGraphicsCombo[6]->GetComboBox(), nMatVSync != 0);
	SetComboItemAsRecommended(m_pGraphicsCombo[7]->GetComboBox(), 2);

	pKeyValues->deleteThis();

	// Shader quality is maxed by default and color correction is always on!
	engine->ClientCmd_Unrestricted("mat_reducefillrate 0\n");
	engine->ClientCmd_Unrestricted("mat_colorcorrection 1\n");
	engine->ClientCmd_Unrestricted("mat_savechanges\n");
	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}
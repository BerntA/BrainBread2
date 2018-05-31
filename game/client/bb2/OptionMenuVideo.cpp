//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Video Related Options.
//
//========================================================================================//

#include "cbase.h"
#include "OptionMenuVideo.h"
#include <vgui/ILocalize.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Client.h"
#include "IGameUIFuncs.h"
#include "materialsystem/materialsystem_config.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

struct RatioToAspectMode_t
{
	int anamorphic;
	float aspectRatio;
};

RatioToAspectMode_t g_RatioToAspectModes[] =
{
	{ 0, 4.0f / 3.0f },
	{ 1, 16.0f / 9.0f },
	{ 2, 16.0f / 10.0f },
	{ 2, 1.0f },
};

//-----------------------------------------------------------------------------
// Purpose: returns the aspect ratio mode number for the given resolution
//-----------------------------------------------------------------------------
int GetScreenAspectMode(int width, int height)
{
	float aspectRatio = (float)width / (float)height;

	// just find the closest ratio
	float closestAspectRatioDist = 99999.0f;
	int closestAnamorphic = 0;
	for (int i = 0; i < ARRAYSIZE(g_RatioToAspectModes); i++)
	{
		float dist = fabs(g_RatioToAspectModes[i].aspectRatio - aspectRatio);
		if (dist < closestAspectRatioDist)
		{
			closestAspectRatioDist = dist;
			closestAnamorphic = g_RatioToAspectModes[i].anamorphic;
		}
	}

	return closestAnamorphic;
}

//-----------------------------------------------------------------------------
// Purpose: returns the string name of the specified resolution mode
//-----------------------------------------------------------------------------
void GetResolutionName(vmode_t *mode, char *sz, int sizeofsz)
{
	if (mode->width == 1280 && mode->height == 1024)
	{
		// LCD native monitor resolution gets special case
		Q_snprintf(sz, sizeofsz, "%i x %i (LCD)", mode->width, mode->height);
	}
	else
		Q_snprintf(sz, sizeofsz, "%i x %i", mode->width, mode->height);
}

OptionMenuVideo::OptionMenuVideo(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szSliderNames[] =
	{
		"#GameUI_Gamma",
		"#GameUI_GlowObjects",
	};

	const char *szSliderCVARS[] =
	{
		"mat_monitorgamma",
		"bb2_max_glow_effects",
	};

	float flSliderLimitMin[] =
	{
		1.6f,
		1.0f,
	};

	float flSliderLimitMax[] =
	{
		2.6f,
		20.0f,
	};

	int iSliderRawValues[] =
	{
		GraphicalOverlay::RawValueType::TYPE_PERCENT,
		GraphicalOverlay::RawValueType::TYPE_INT,
	};

	for (int i = 0; i < _ARRAYSIZE(m_pSensSlider); i++)
	{
		m_pSensSlider[i] = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "GraphSlider", szSliderNames[i], szSliderCVARS[i], flSliderLimitMin[i], flSliderLimitMax[i], false, iSliderRawValues[i]));
		m_pSensSlider[i]->SetZPos(60);
	}

	const char *szName[] =
	{
		"#GameUI_AspectRatio",
		"#GameUI_Resolution",
	};

	const char *szTitles[] =
	{
		"#GameUI_Video",
		"#GameUI_Shaders",
	};

	const char *szCheckBoxOptions[] =
	{
		"#GameUI_Windowed",
		"#GameUI_MotionBlur",
		"#GameUI_FilmGrain",
		"#GameUI_EnableGlow",
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

	for (int i = 0; i < _ARRAYSIZE(m_pVideoCombo); i++)
	{
		m_pVideoCombo[i] = vgui::SETUP_PANEL(new vgui::ComboList(this, "ComboList", szName[i], 6));
		m_pVideoCombo[i]->SetZPos(60);
		m_pVideoCombo[i]->AddActionSignalTarget(this);
		m_pVideoCombo[i]->GetComboBox()->AddActionSignalTarget(this);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBox); i++)
	{
		m_pCheckBox[i] = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "CheckBox", szCheckBoxOptions[i], "OptionTextMedium"));
		m_pCheckBox[i]->AddActionSignalTarget(this);
		m_pCheckBox[i]->SetZPos(50);
	}

	char pszAspectName[3][64];
	wchar_t *unicodeText = g_pVGuiLocalize->Find("#GameUI_AspectNormal");
	g_pVGuiLocalize->ConvertUnicodeToANSI(unicodeText, pszAspectName[0], 32);
	unicodeText = g_pVGuiLocalize->Find("#GameUI_AspectWide16x9");
	g_pVGuiLocalize->ConvertUnicodeToANSI(unicodeText, pszAspectName[1], 32);
	unicodeText = g_pVGuiLocalize->Find("#GameUI_AspectWide16x10");
	g_pVGuiLocalize->ConvertUnicodeToANSI(unicodeText, pszAspectName[2], 32);

	m_pVideoCombo[0]->GetComboBox()->AddItem(pszAspectName[0], NULL);
	m_pVideoCombo[0]->GetComboBox()->AddItem(pszAspectName[1], NULL);
	m_pVideoCombo[0]->GetComboBox()->AddItem(pszAspectName[2], NULL);

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	InvalidateLayout();
	PerformLayout();
}

OptionMenuVideo::~OptionMenuVideo()
{
}

void OptionMenuVideo::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pSensSlider); i++)
			m_pSensSlider[i]->OnUpdate(bInGame);

		m_pSensSlider[1]->SetEnabled(m_pCheckBox[3]->IsChecked());

		m_pApplyButton->OnUpdate();
	}
}

void OptionMenuVideo::ApplyChanges(void)
{
	static ConVarRef motionblur("mat_motion_blur_enabled");
	static ConVarRef motionblur_forward("mat_motion_blur_forward_enabled");
	static ConVarRef glow_outline_effect_enable("glow_outline_effect_enable");
	static ConVarRef bb2_fx_filmgrain("bb2_fx_filmgrain");

	motionblur.SetValue(m_pCheckBox[1]->IsChecked());
	motionblur_forward.SetValue(m_pCheckBox[1]->IsChecked());
	glow_outline_effect_enable.SetValue(m_pCheckBox[3]->IsChecked());
	bb2_fx_filmgrain.SetValue(m_pCheckBox[2]->IsChecked());

	char sz[256];
	if (m_nSelectedMode == -1)
		m_pVideoCombo[1]->GetComboBox()->GetText(sz, 256);
	else
		m_pVideoCombo[1]->GetComboBox()->GetItemText(m_nSelectedMode, sz, 256);

	int width = 0, height = 0;
	sscanf(sz, "%i x %i", &width, &height);

	// make sure there is a change
	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
	bool windowed = m_pCheckBox[0]->IsChecked();

	if (config.m_VideoMode.m_Width != width
		|| config.m_VideoMode.m_Height != height
		|| config.Windowed() != windowed)
	{
		// set mode
		char szCmd[256];
		Q_snprintf(szCmd, sizeof(szCmd), "mat_setvideomode %i %i %i\n", width, height, windowed ? 1 : 0);
		engine->ClientCmd_Unrestricted(szCmd);
	}

	m_pSensSlider[0]->SetEnabled(!windowed);

	// Save
	engine->ClientCmd_Unrestricted("mat_savechanges\n");
	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}

void OptionMenuVideo::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h, wz, hz;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(2));
		m_pTextTitle[i]->GetSize(wz, hz);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pVideoCombo); i++)
		m_pVideoCombo[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBox); i++)
		m_pCheckBox[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(12));

	for (int i = 0; i < _ARRAYSIZE(m_pSensSlider); i++)
		m_pSensSlider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(20));

	m_pTextTitle[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(12));
	m_pTextTitle[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(157));

	m_pDivider[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(37));
	m_pDivider[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(182));

	m_pSensSlider[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(139));
	m_pCheckBox[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(55));
	m_pCheckBox[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(200));
	m_pCheckBox[2]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(220));
	m_pCheckBox[3]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(240));
	m_pSensSlider[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(260));
	m_pVideoCombo[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(75));
	m_pVideoCombo[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(107));

	if (!IsVisible())
	{
		const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
		m_pSensSlider[0]->SetEnabled(!config.Windowed());
		m_pCheckBox[0]->SetCheckedStatus(config.Windowed());

		int iAspectMode = GetScreenAspectMode(config.m_VideoMode.m_Width, config.m_VideoMode.m_Height);
		switch (iAspectMode)
		{
		default:
		case 0:
			m_pVideoCombo[0]->GetComboBox()->ActivateItem(0);
			break;
		case 1:
			m_pVideoCombo[0]->GetComboBox()->ActivateItem(1);
			break;
		case 2:
			m_pVideoCombo[0]->GetComboBox()->ActivateItem(2);
			break;
		}

		SetCurrentResolutionComboItem();
		PrepareResolutionList();

		static ConVarRef motionblur("mat_motion_blur_enabled");
		static ConVarRef glow_outline_effect_enable("glow_outline_effect_enable");
		static ConVarRef bb2_fx_filmgrain("bb2_fx_filmgrain");

		m_pCheckBox[1]->SetCheckedStatus(motionblur.GetBool());
		m_pCheckBox[2]->SetCheckedStatus(bb2_fx_filmgrain.GetBool());
		m_pCheckBox[3]->SetCheckedStatus(glow_outline_effect_enable.GetBool());
	}

	GetSize(w, h);
	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(52), h - scheme()->GetProportionalScaledValue(28));
}

void OptionMenuVideo::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}
}

void OptionMenuVideo::SetCurrentResolutionComboItem()
{
	vmode_t *plist = NULL;
	int count = 0;
	gameuifuncs->GetVideoModes(&plist, &count);

	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();

	int resolution = -1;
	for (int i = 0; i < count; i++, plist++)
	{
		if (plist->width == config.m_VideoMode.m_Width &&
			plist->height == config.m_VideoMode.m_Height)
		{
			resolution = i;
			break;
		}
	}

	if (resolution != -1)
	{
		char sz[256];
		GetResolutionName(plist, sz, sizeof(sz));
		m_pVideoCombo[1]->GetComboBox()->SetText(sz);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Generates resolution list
//-----------------------------------------------------------------------------
void OptionMenuVideo::PrepareResolutionList()
{
	// get the currently selected resolution
	char sz[256];
	m_pVideoCombo[1]->GetComboBox()->GetText(sz, 256);
	int currentWidth = 0, currentHeight = 0;
	sscanf(sz, "%i x %i", &currentWidth, &currentHeight);

	// Clean up before filling the info again.
	m_pVideoCombo[1]->GetComboBox()->DeleteAllItems();
	m_pVideoCombo[0]->GetComboBox()->SetItemEnabled(1, false);
	m_pVideoCombo[0]->GetComboBox()->SetItemEnabled(2, false);

	// get full video mode list
	vmode_t *plist = NULL;
	int count = 0;
	gameuifuncs->GetVideoModes(&plist, &count);

	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();

	bool bWindowed = (config.Windowed());
	int desktopWidth, desktopHeight;
	gameuifuncs->GetDesktopResolution(desktopWidth, desktopHeight);

	// iterate all the video modes adding them to the dropdown
	bool bFoundWidescreen = false;
	int selectedItemID = -1;
	for (int i = 0; i < count; i++, plist++)
	{
		char sz[256];
		GetResolutionName(plist, sz, sizeof(sz));

		// don't show modes bigger than the desktop for windowed mode
		if (bWindowed && (plist->width > desktopWidth || plist->height > desktopHeight))
			continue;

		int itemID = -1;
		int iAspectMode = GetScreenAspectMode(plist->width, plist->height);
		if (iAspectMode > 0)
		{
			m_pVideoCombo[0]->GetComboBox()->SetItemEnabled(iAspectMode, true);
			bFoundWidescreen = true;
		}

		// filter the list for those matching the current aspect
		if (iAspectMode == m_pVideoCombo[0]->GetComboBox()->GetActiveItem())
			itemID = m_pVideoCombo[1]->GetComboBox()->AddItem(sz, NULL);

		// try and find the best match for the resolution to be selected
		if (plist->width == currentWidth && plist->height == currentHeight)
			selectedItemID = itemID;
		else if (selectedItemID == -1 && plist->width == config.m_VideoMode.m_Width && plist->height == config.m_VideoMode.m_Height)
			selectedItemID = itemID;
	}

	// disable ratio selection if we can't display widescreen.
	m_pVideoCombo[0]->SetEnabled(bFoundWidescreen);

	m_nSelectedMode = selectedItemID;

	if (selectedItemID != -1)
		m_pVideoCombo[1]->GetComboBox()->ActivateItem(selectedItemID);
	else
		m_pVideoCombo[1]->GetComboBox()->ActivateItem(0);
}

void OptionMenuVideo::OnTextChanged(Panel *pPanel, const char *pszText)
{
	if (pPanel == m_pVideoCombo[1]->GetComboBox())
	{
		m_nSelectedMode = m_pVideoCombo[1]->GetComboBox()->GetActiveItem();
	}
	else if (pPanel == m_pVideoCombo[0]->GetComboBox())
	{
		PrepareResolutionList();
	}
}
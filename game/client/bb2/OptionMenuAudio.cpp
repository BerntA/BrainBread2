//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Audio Related Options
//
//========================================================================================//

#include "cbase.h"
#include "OptionMenuAudio.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameBase_Client.h"
#include "fmod_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

OptionMenuAudio::OptionMenuAudio(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	const char *szOptions[] =
	{
		"#GameUI_SoundEffectVolume",
		"#GameUI_MusicVolume",
		"#GameUI_VoiceTransmitVolume",
		"#GameUI_VoiceReceiveVolume",
	};

	const char *szVars[] =
	{
		"volume",
		"snd_musicvolume",
		"voice_threshold",
		"voice_scale",
	};

	const char *szTitles[] =
	{
		"#GameUI_Audio",
		"#GameUI_Voice",
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

	const char *szCheckBoxTxt[] =
	{
		"#GameUI_EnableVoice",
		"#GameUI_BoostMicrophone",
		"#GameUI_SndMuteLoseFocus",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBox); i++)
	{
		m_pCheckBox[i] = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "CheckBox", szCheckBoxTxt[i], "OptionTextMedium"));
		m_pCheckBox[i]->SetZPos(60);
		m_pCheckBox[i]->AddActionSignalTarget(this);
	}

	for (int i = 0; i < _ARRAYSIZE(m_pSensSlider); i++)
	{
		m_pSensSlider[i] = vgui::SETUP_PANEL(new vgui::GraphicalOverlay(this, "Slider", szOptions[i], szVars[i], 0.0f, 1.0f));
		m_pSensSlider[i]->SetZPos(60);
	}

	const char *szComboTitles[] =
	{
		"#GameUI_SpeakerConfiguration",
		"#GameUI_CloseCaptions_Checkbox",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pComboList); i++)
	{
		m_pComboList[i] = vgui::SETUP_PANEL(new vgui::ComboList(this, "ComboList", szComboTitles[i], 6));
		m_pComboList[i]->SetZPos(60);
	}

	m_pComboList[0]->GetComboBox()->AddItem("#GameUI_Headphones", new KeyValues("SpeakerSetup", "speakers", 0));
	m_pComboList[0]->GetComboBox()->AddItem("#GameUI_2Speakers", new KeyValues("SpeakerSetup", "speakers", 2));
	m_pComboList[0]->GetComboBox()->AddItem("#GameUI_4Speakers", new KeyValues("SpeakerSetup", "speakers", 4));
	m_pComboList[0]->GetComboBox()->AddItem("#GameUI_5Speakers", new KeyValues("SpeakerSetup", "speakers", 5));
	m_pComboList[0]->GetComboBox()->AddItem("#GameUI_7Speakers", new KeyValues("SpeakerSetup", "speakers", 7));

	m_pComboList[1]->GetComboBox()->AddItem("#GameUI_NoClosedCaptions", NULL);
	m_pComboList[1]->GetComboBox()->AddItem("#GameUI_SubtitlesAndSoundEffects", NULL);
	m_pComboList[1]->GetComboBox()->AddItem("#GameUI_Subtitles", NULL);

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	InvalidateLayout();

	PerformLayout();
}

OptionMenuAudio::~OptionMenuAudio()
{
}

void OptionMenuAudio::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pSensSlider); i++)
			m_pSensSlider[i]->OnUpdate(bInGame);

		m_pApplyButton->OnUpdate();
	}
}

void OptionMenuAudio::ApplyChanges(void)
{
	static ConVarRef voice_enable("voice_modenable");
	static ConVarRef boost_mic_gain("voice_boost");
	static ConVarRef mute_losefocus("snd_mute_losefocus");

	static ConVarRef closed_captions("closecaption");
	static ConVarRef subtitles("cc_subtitles");
	static ConVarRef surround_speakers("snd_surround_speakers");

	voice_enable.SetValue(m_pCheckBox[0]->IsChecked());
	boost_mic_gain.SetValue(m_pCheckBox[1]->IsChecked());
	mute_losefocus.SetValue(m_pCheckBox[2]->IsChecked());

	int speakers = m_pComboList[0]->GetComboBox()->GetActiveItemUserData()->GetInt("speakers");
	if (speakers != surround_speakers.GetInt())
	{
		surround_speakers.SetValue(speakers);

		// headphones at high quality get enhanced stereo turned on
		static ConVar *dsp_enhance_stereo = cvar->FindVar("dsp_enhance_stereo");
		if (dsp_enhance_stereo)
		{
			if (speakers == 0)
				dsp_enhance_stereo->SetValue(1);
			else
				dsp_enhance_stereo->SetValue(0);
		}

		FMODManager()->RestartFMOD();
	}

	subtitles.SetValue(0);
	int iCloseCaptionValue = 0;
	switch (m_pComboList[1]->GetComboBox()->GetActiveItem())
	{
	default:
	case 0:
		break;
	case 1:
		iCloseCaptionValue = 1;
		break;
	case 2:
		iCloseCaptionValue = 1;
		subtitles.SetValue(1);
		break;
	}

	char cmd[64];
	Q_snprintf(cmd, sizeof(cmd), "closecaption %i\n", iCloseCaptionValue);
	engine->ClientCmd_Unrestricted(cmd);

	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}

void OptionMenuAudio::SetupLayout(void)
{
	BaseClass::SetupLayout();

	if (!IsVisible())
	{
		static ConVarRef voice_enable("voice_modenable");
		static ConVarRef boost_mic_gain("voice_boost");
		static ConVarRef mute_losefocus("snd_mute_losefocus");
		static ConVarRef closed_captions("closecaption");
		static ConVarRef subtitles("cc_subtitles");
		static ConVarRef surround_speakers("snd_surround_speakers");

		m_pCheckBox[0]->SetCheckedStatus(voice_enable.GetBool());
		m_pCheckBox[1]->SetCheckedStatus(boost_mic_gain.GetBool());
		m_pCheckBox[2]->SetCheckedStatus(mute_losefocus.GetBool());

		int speakers = surround_speakers.GetInt();
		for (int itemID = 0; itemID < m_pComboList[0]->GetComboBox()->GetItemCount(); itemID++)
		{
			KeyValues *kv = m_pComboList[0]->GetComboBox()->GetItemUserData(itemID);
			if (kv && kv->GetInt("speakers") == speakers)
				m_pComboList[0]->GetComboBox()->ActivateItem(itemID);
		}

		if (!closed_captions.GetBool())
			m_pComboList[1]->GetComboBox()->ActivateItem(0);
		else if (closed_captions.GetBool() && !subtitles.GetBool())
			m_pComboList[1]->GetComboBox()->ActivateItem(1);
		else
			m_pComboList[1]->GetComboBox()->ActivateItem(2);
	}

	int w, h, wz, hz;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pCheckBox); i++)
	{
		m_pCheckBox[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(12));
		m_pCheckBox[i]->GetSize(wz, hz);

		if (i < (_ARRAYSIZE(m_pCheckBox) - 1))
			m_pCheckBox[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(160) + (i * (hz + scheme()->GetProportionalScaledValue(8))));
		else
			m_pCheckBox[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(297));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pSensSlider); i++)
	{
		m_pSensSlider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(20));
		m_pSensSlider[i]->GetSize(wz, hz);

		if (i < 2)
			m_pSensSlider[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(55) + (i * (hz + scheme()->GetProportionalScaledValue(4))));
		else
			m_pSensSlider[i]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(200) + ((i - 2) * (hz + scheme()->GetProportionalScaledValue(4))));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pComboList); i++)
		m_pComboList[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(400), scheme()->GetProportionalScaledValue(2));
	}

	m_pComboList[0]->GetSize(wz, hz);
	m_pComboList[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(95));
	m_pComboList[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(317));

	m_pTextTitle[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(12));
	m_pTextTitle[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(117));
	m_pTextTitle[2]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(256));

	m_pDivider[0]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(37));
	m_pDivider[1]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(142));
	m_pDivider[2]->SetPos((w / 2) - (wz / 2), scheme()->GetProportionalScaledValue(281));

	GetSize(w, h);
	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(52), h - scheme()->GetProportionalScaledValue(28));
}

void OptionMenuAudio::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}
}
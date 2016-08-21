//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Allows the user to select & customize a character model to his/her liking.
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
#include "ProfileMenuCharacterPanel.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/MouseCode.h>
#include "vgui_controls/AnimationController.h"
#include <vgui_controls/SectionedListPanel.h>
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "GameBase_Shared.h"
#include "inputsystem/iinputsystem.h"
#include "utlvector.h"
#include "KeyValues.h"
#include "filesystem.h"
#include <vgui_controls/TextImage.h>
#include "GameDefinitions_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ProfileMenuCharacterPanel::ProfileMenuCharacterPanel(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	m_iCurrentSelectedItem = 0;

	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pSelectedModel = new vgui::CharacterPreviewPanel(this, "CharacterPreview");
	m_pSelectedModel->SetZPos(50);

	m_pBanner = vgui::SETUP_PANEL(new vgui::Divider(this, "BGBanner"));
	m_pBanner->SetZPos(60);

	m_pSurvivorCombo = vgui::SETUP_PANEL(new vgui::ComboList(this, "ComboListSurvivor", "", 6));
	m_pSurvivorCombo->AddActionSignalTarget(this);
	m_pSurvivorCombo->SetZPos(80);

	m_pSoundSetComboHuman = vgui::SETUP_PANEL(new vgui::ComboList(this, "ComboListSoundSetsHuman", "", 6));
	m_pSoundSetComboHuman->AddActionSignalTarget(this);
	m_pSoundSetComboHuman->SetZPos(80);

	m_pSoundSetComboZombie = vgui::SETUP_PANEL(new vgui::ComboList(this, "ComboListSoundSetsZombie", "", 6));
	m_pSoundSetComboZombie->AddActionSignalTarget(this);
	m_pSoundSetComboZombie->SetZPos(80);

	const char *szInfo[] =
	{
		"#GameUI_CharacterMenu_HumanSoundSet",
		"#GameUI_CharacterMenu_ZombieSoundSet",
		"#GameUI_CharacterMenu_Customization_Skin",
		"#GameUI_CharacterMenu_Customization_Head",
		"#GameUI_CharacterMenu_Customization_Body",
		"#GameUI_CharacterMenu_Customization_RightLeg",
		"#GameUI_CharacterMenu_Customization_LeftLeg",
		"",
	};

	for (int i = 0; i < _ARRAYSIZE(m_pInfo); i++)
	{
		m_pInfo[i] = vgui::SETUP_PANEL(new vgui::Label(this, "LabelInfo", szInfo[i]));
		m_pInfo[i]->SetZPos(75);
	}

	for (int i = 2; i < _ARRAYSIZE(m_pInfo); i++)
	{
		m_pInfo[i]->SetContentAlignment(Label::Alignment::a_center);
	}

	const char *szTitles[] =
	{
		"#GameUI_CharacterMenu_SoundSet",
		"#GameUI_CharacterMenu_Customization",
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

	for (int i = 0; i < MAX_CUSTOMIZABLE_ITEMS; i++)
	{
		m_pArrowLeft[i] = vgui::SETUP_PANEL(new vgui::ExoticImageButton(this, "ImageButton", "mainmenu/arrow_left", "mainmenu/arrow_left_over", VarArgs("ArrowLeft%i", i)));
		m_pArrowLeft[i]->AddActionSignalTarget(this);
		m_pArrowLeft[i]->SetZPos(80);

		m_pArrowRight[i] = vgui::SETUP_PANEL(new vgui::ExoticImageButton(this, "ImageButton", "mainmenu/arrow_right", "mainmenu/arrow_right_over", VarArgs("ArrowRight%i", i)));
		m_pArrowRight[i]->AddActionSignalTarget(this);
		m_pArrowRight[i]->SetZPos(80);
	}

	m_pApplyButton = vgui::SETUP_PANEL(new vgui::InlineMenuButton(this, "OptionButtons", COMMAND_OPTIONS_APPLY, "#GameUI_Apply", "BB2_PANEL_BIG"));
	m_pApplyButton->SetZPos(100);
	m_pApplyButton->AddActionSignalTarget(this);
	m_pApplyButton->SetIconImage("hud/vote_yes");

	InvalidateLayout();

	PerformLayout();
}

ProfileMenuCharacterPanel::~ProfileMenuCharacterPanel()
{
	Cleanup();
}

void ProfileMenuCharacterPanel::Cleanup(void)
{
	m_pSelectedModel->DeleteModelData();
	m_pSurvivorCombo->GetComboBox()->RemoveAll();
	m_pSoundSetComboHuman->GetComboBox()->RemoveAll();
	m_pSoundSetComboZombie->GetComboBox()->RemoveAll();
}

void ProfileMenuCharacterPanel::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h;
	GetSize(w, h);

	for (int i = 0; i < _ARRAYSIZE(m_pInfo); i++)
	{
		m_pInfo[i]->SetSize(scheme()->GetProportionalScaledValue(140), scheme()->GetProportionalScaledValue(14));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetSize(scheme()->GetProportionalScaledValue(140), scheme()->GetProportionalScaledValue(24));
		m_pDivider[i]->SetSize(scheme()->GetProportionalScaledValue(140), scheme()->GetProportionalScaledValue(2));
	}

	for (int i = 0; i < MAX_CUSTOMIZABLE_ITEMS; i++)
	{
		m_pArrowLeft[i]->SetSize(scheme()->GetProportionalScaledValue(12), scheme()->GetProportionalScaledValue(12));
		m_pArrowRight[i]->SetSize(scheme()->GetProportionalScaledValue(12), scheme()->GetProportionalScaledValue(12));
	}

	m_pBanner->SetPos(w - scheme()->GetProportionalScaledValue(140), 0);
	m_pBanner->SetSize(scheme()->GetProportionalScaledValue(140), h);

	m_pSelectedModel->SetPos(((w - scheme()->GetProportionalScaledValue(140)) / 2) - scheme()->GetProportionalScaledValue(150), 0);
	m_pSelectedModel->SetSize(scheme()->GetProportionalScaledValue(300), h);

	m_pInfo[7]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(24));
	m_pInfo[7]->SetPos(((w - scheme()->GetProportionalScaledValue(140)) / 2) - scheme()->GetProportionalScaledValue(150), h - scheme()->GetProportionalScaledValue(26));

	int x, y;
	m_pBanner->GetPos(x, y);

	m_pSurvivorCombo->SetPos(x - scheme()->GetProportionalScaledValue(117), scheme()->GetProportionalScaledValue(4));
	m_pSurvivorCombo->SetSize(scheme()->GetProportionalScaledValue(250), scheme()->GetProportionalScaledValue(12));

	m_pTextTitle[0]->SetPos(x, scheme()->GetProportionalScaledValue(18));
	m_pDivider[0]->SetPos(x, scheme()->GetProportionalScaledValue(43));

	m_pInfo[0]->SetPos(x, scheme()->GetProportionalScaledValue(47));
	m_pSoundSetComboHuman->SetPos(x - scheme()->GetProportionalScaledValue(117), scheme()->GetProportionalScaledValue(60));
	m_pSoundSetComboHuman->SetSize(scheme()->GetProportionalScaledValue(250), scheme()->GetProportionalScaledValue(12));

	m_pInfo[1]->SetPos(x, scheme()->GetProportionalScaledValue(74));
	m_pSoundSetComboZombie->SetPos(x - scheme()->GetProportionalScaledValue(117), scheme()->GetProportionalScaledValue(87));
	m_pSoundSetComboZombie->SetSize(scheme()->GetProportionalScaledValue(250), scheme()->GetProportionalScaledValue(12));

	m_pTextTitle[1]->SetPos(x, scheme()->GetProportionalScaledValue(102));
	m_pDivider[1]->SetPos(x, scheme()->GetProportionalScaledValue(127));

	m_pApplyButton->SetSize(scheme()->GetProportionalScaledValue(60), scheme()->GetProportionalScaledValue(24));
	m_pApplyButton->SetPos(w - scheme()->GetProportionalScaledValue(100), h - scheme()->GetProportionalScaledValue(28));
}

void ProfileMenuCharacterPanel::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		m_pApplyButton->OnUpdate();

		int selectedItem = m_pSurvivorCombo->GetComboBox()->GetActiveItem();
		if (selectedItem != m_iCurrentSelectedItem)
		{
			m_iCurrentSelectedItem = selectedItem;
			ShowInfoForCharacter(m_iCurrentSelectedItem);
		}
	}
}

void ProfileMenuCharacterPanel::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	if (bShow)
	{
		m_pSurvivorCombo->GetComboBox()->RemoveAll();

		ConVarRef survivor_choice("bb2_survivor_choice");
		const char *survivorName = survivor_choice.GetString();
		if (!survivorName)
			survivorName = "";

		m_iCurrentSelectedItem = 0;

		for (int i = 0; i < GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataList().Count(); i++)
		{
			if (!strcmp(survivorName, GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataList()[i].szSurvivorName))
				m_iCurrentSelectedItem = i;

			m_pSurvivorCombo->GetComboBox()->AddItem(GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataList()[i].szFriendlySurvivorName, NULL);
		}

		m_pSurvivorCombo->GetComboBox()->ActivateItem(m_iCurrentSelectedItem);
		ShowInfoForCharacter(m_iCurrentSelectedItem, true);
	}
	else
		Cleanup();
}

void ProfileMenuCharacterPanel::ShowInfoForCharacter(int index, bool bLoadSelf)
{
	if (bLoadSelf)
	{
		ConVarRef extra_skin("bb2_survivor_choice_skin");
		ConVarRef extra_head("bb2_survivor_choice_extra_head");
		ConVarRef extra_body("bb2_survivor_choice_extra_body");
		ConVarRef extra_rightleg("bb2_survivor_choice_extra_leg_right");
		ConVarRef extra_leftleg("bb2_survivor_choice_extra_leg_left");

		m_iCustomizationNum[0] = extra_skin.GetInt();
		m_iCustomizationNum[1] = extra_head.GetInt();
		m_iCustomizationNum[2] = extra_body.GetInt();
		m_iCustomizationNum[3] = extra_rightleg.GetInt();
		m_iCustomizationNum[4] = extra_leftleg.GetInt();
	}
	else
	{
		for (int i = 0; i < MAX_CUSTOMIZABLE_ITEMS; i++)
		{
			m_iCustomizationNum[i] = 0;
		}
	}

	DataPlayerItem_Survivor_Shared_t data = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(index);
	m_pSelectedModel->LoadModel(data.szSurvivorName, TEAM_HUMANS);
	m_pSelectedModel->SetProperties(m_iCustomizationNum[0], m_iCustomizationNum[1], m_iCustomizationNum[2], m_iCustomizationNum[3], m_iCustomizationNum[4]);

	ConVarRef human_voiceset("bb2_sound_player_human");
	ConVarRef zombie_voiceset("bb2_sound_player_deceased");

	// Load other stuff:
	m_pSoundSetComboHuman->GetComboBox()->RemoveAll();
	GameBaseShared()->GetSharedGameDetails()->AddSoundScriptItems(m_pSoundSetComboHuman, BB2_SoundTypes::TYPE_PLAYER, data.szSurvivorName);

	m_pSoundSetComboZombie->GetComboBox()->RemoveAll();
	GameBaseShared()->GetSharedGameDetails()->AddSoundScriptItems(m_pSoundSetComboZombie, BB2_SoundTypes::TYPE_DECEASED, data.szSurvivorName);

	for (int i = 2; i < (_ARRAYSIZE(m_pInfo) - 1); i++)
	{
		m_pInfo[i]->SetVisible(false);
	}

	for (int i = 0; i < MAX_CUSTOMIZABLE_ITEMS; i++)
	{
		m_pArrowRight[i]->SetVisible(false);
		m_pArrowLeft[i]->SetVisible(false);
	}

	m_pInfo[7]->SetText(data.szFriendlyDescription);

	m_pSoundSetComboHuman->SetVisible((m_pSoundSetComboHuman->GetComboBox()->GetItemCount() > 0));
	if (m_pSoundSetComboHuman->IsVisible())
		m_pSoundSetComboHuman->GetComboBox()->ActivateItem(GameBaseShared()->GetSharedGameDetails()->GetSelectedSoundsetItemID(m_pSoundSetComboHuman, BB2_SoundTypes::TYPE_PLAYER, data.szSurvivorName, human_voiceset.GetString()));

	m_pSoundSetComboZombie->SetVisible((m_pSoundSetComboZombie->GetComboBox()->GetItemCount() > 0));
	if (m_pSoundSetComboZombie->IsVisible())
		m_pSoundSetComboZombie->GetComboBox()->ActivateItem(GameBaseShared()->GetSharedGameDetails()->GetSelectedSoundsetItemID(m_pSoundSetComboZombie, BB2_SoundTypes::TYPE_DECEASED, data.szSurvivorName, zombie_voiceset.GetString()));

	int x, y;
	m_pDivider[1]->GetPos(x, y);
	y += scheme()->GetProportionalScaledValue(4);

	int customArray[MAX_CUSTOMIZABLE_ITEMS] =
	{
		data.iSkins,
		data.iSpecialHeadItems,
		data.iSpecialBodyItems,
		data.iSpecialRightLegItems,
		data.iSpecialLeftLegItems,
	};

	int w, h;
	GetSize(w, h);

	bool bShouldShowCustomizationPart = false;
	for (int i = 0; i < MAX_CUSTOMIZABLE_ITEMS; i++)
	{
		if (customArray[i] > 0)
		{
			m_pInfo[(i + 2)]->SetVisible(true);
			m_pInfo[(i + 2)]->SetPos(x, y);

			m_pArrowRight[i]->SetVisible(true);
			m_pArrowLeft[i]->SetVisible(true);

			m_pArrowRight[i]->SetPos(w - scheme()->GetProportionalScaledValue(12), y);
			m_pArrowLeft[i]->SetPos(x, y);

			y += scheme()->GetProportionalScaledValue(16);

			bShouldShowCustomizationPart = true;
		}
	}

	m_pTextTitle[1]->SetVisible(bShouldShowCustomizationPart);
	m_pDivider[1]->SetVisible(bShouldShowCustomizationPart);
}

void ProfileMenuCharacterPanel::ApplyChanges(void)
{
	DataPlayerItem_Survivor_Shared_t data = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(m_iCurrentSelectedItem);

	ConVarRef survivor_choice("bb2_survivor_choice");
	ConVarRef human_voiceset("bb2_sound_player_human");
	ConVarRef zombie_voiceset("bb2_sound_player_deceased");

	ConVarRef extra_skin("bb2_survivor_choice_skin");
	ConVarRef extra_head("bb2_survivor_choice_extra_head");
	ConVarRef extra_body("bb2_survivor_choice_extra_body");
	ConVarRef extra_rightleg("bb2_survivor_choice_extra_leg_right");
	ConVarRef extra_leftleg("bb2_survivor_choice_extra_leg_left");

	survivor_choice.SetValue(data.szSurvivorName);

	char pchFriendlyName[64];
	int activeSet = m_pSoundSetComboHuman->GetComboBox()->GetActiveItem();
	if (activeSet == -1)
		human_voiceset.SetValue("Pantsman");
	else
	{
		m_pSoundSetComboHuman->GetComboBox()->GetItemText(activeSet, pchFriendlyName, 64);
		human_voiceset.SetValue(GameBaseShared()->GetSharedGameDetails()->GetSoundPrefixForChoosenItem(BB2_SoundTypes::TYPE_PLAYER, data.szSurvivorName, pchFriendlyName));
	}

	activeSet = m_pSoundSetComboZombie->GetComboBox()->GetActiveItem();
	if (activeSet == -1)
		zombie_voiceset.SetValue("Default");
	else
	{
		m_pSoundSetComboZombie->GetComboBox()->GetItemText(activeSet, pchFriendlyName, 64);
		zombie_voiceset.SetValue(GameBaseShared()->GetSharedGameDetails()->GetSoundPrefixForChoosenItem(BB2_SoundTypes::TYPE_DECEASED, data.szSurvivorName, pchFriendlyName));
	}

	extra_skin.SetValue(m_iCustomizationNum[0]);
	extra_head.SetValue(m_iCustomizationNum[1]);
	extra_body.SetValue(m_iCustomizationNum[2]);
	extra_rightleg.SetValue(m_iCustomizationNum[3]);
	extra_leftleg.SetValue(m_iCustomizationNum[4]);
}

void ProfileMenuCharacterPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pBanner->SetBorder(NULL);
	m_pBanner->SetPaintBorderEnabled(false);
	m_pBanner->SetBgColor(pScheme->GetColor("CharacterMenuBannerBg", Color(24, 20, 28, 200)));
	m_pBanner->SetFgColor(pScheme->GetColor("CharacterMenuBannerFg", Color(20, 22, 27, 150)));

	for (int i = 0; i < _ARRAYSIZE(m_pInfo); i++)
	{
		m_pInfo[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pInfo[i]->SetFont(pScheme->GetFont("OptionTextSmall"));
	}

	for (int i = 0; i < _ARRAYSIZE(m_pTextTitle); i++)
	{
		m_pTextTitle[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTextTitle[i]->SetFont(pScheme->GetFont("OptionTextMedium"));
	}
}

void ProfileMenuCharacterPanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);

	DataPlayerItem_Survivor_Shared_t data = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(m_iCurrentSelectedItem);

	int customArray[MAX_CUSTOMIZABLE_ITEMS] =
	{
		data.iSkins,
		data.iSpecialHeadItems,
		data.iSpecialBodyItems,
		data.iSpecialRightLegItems,
		data.iSpecialLeftLegItems,
	};

	bool bUpdated = false;
	for (int i = 0; i < MAX_CUSTOMIZABLE_ITEMS; i++)
	{
		const char *pchItemName = VarArgs("ArrowRight%i", i);
		if (!strcmp(pchItemName, pcCommand))
		{
			if (m_iCustomizationNum[i] < customArray[i])
				m_iCustomizationNum[i]++;
			else
				m_iCustomizationNum[i] = 0;

			bUpdated = true;
		}

		pchItemName = VarArgs("ArrowLeft%i", i);
		if (!strcmp(pchItemName, pcCommand))
		{
			if (m_iCustomizationNum[i] > 0)
				m_iCustomizationNum[i]--;
			else
				m_iCustomizationNum[i] = customArray[i];

			bUpdated = true;
		}
	}

	if (bUpdated)
		m_pSelectedModel->SetProperties(m_iCustomizationNum[0], m_iCustomizationNum[1], m_iCustomizationNum[2], m_iCustomizationNum[3], m_iCustomizationNum[4]);
}
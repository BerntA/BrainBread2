//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Allows the user to select & customize a character model to his/her liking.
//
//========================================================================================//

#ifndef PROFILE_MENU_CHARACTER_PANEL_H
#define PROFILE_MENU_CHARACTER_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Divider.h>
#include "vgui_base_panel.h"
#include "CharacterPreviewPanel.h"
#include "InlineMenuButton.h"
#include "ComboList.h"
#include "ExoticImageButton.h"

#define MAX_CUSTOMIZABLE_ITEMS 5

namespace vgui
{
	class ProfileMenuCharacterPanel;
	class ProfileMenuCharacterPanel : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(ProfileMenuCharacterPanel, vgui::CVGUIBasePanel);

	public:
		ProfileMenuCharacterPanel(vgui::Panel *parent, char const *panelName);
		~ProfileMenuCharacterPanel();

		void SetupLayout(void);
		void OnUpdate(bool bInGame);
		void OnShowPanel(bool bShow);
		void ApplyChanges(void);

		void Cleanup(void);
		void ShowInfoForCharacter(int index, bool bLoadSelf = false);

	private:
		vgui::CharacterPreviewPanel *m_pSelectedModel;
		vgui::Divider *m_pBanner;

		vgui::ComboList *m_pSurvivorCombo;
		vgui::ComboList *m_pSoundSetComboHuman;
		vgui::ComboList *m_pSoundSetComboZombie; 

		vgui::Label *m_pInfo[8];

		vgui::ImagePanel *m_pDivider[2];
		vgui::Label *m_pTextTitle[2];

		vgui::ExoticImageButton *m_pArrowLeft[MAX_CUSTOMIZABLE_ITEMS];
		vgui::ExoticImageButton *m_pArrowRight[MAX_CUSTOMIZABLE_ITEMS];

		vgui::InlineMenuButton *m_pApplyButton;

		int m_iCurrentSelectedItem;
		int m_iCustomizationNum[MAX_CUSTOMIZABLE_ITEMS];

	protected:

		virtual void OnCommand(const char* pcCommand);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // PROFILE_MENU_CHARACTER_PANEL_H
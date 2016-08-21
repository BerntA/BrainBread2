//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Keyboard Related Options
//
//========================================================================================//

#ifndef OPTION_MENU_KEYBOARD_H
#define OPTION_MENU_KEYBOARD_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/SectionedListPanel.h>
#include "MouseInputPanel.h"
#include "vgui_base_panel.h"

namespace vgui
{
	class OptionMenuKeyboard;

	class OptionMenuKeyboard : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(OptionMenuKeyboard, vgui::CVGUIBasePanel);

	public:
		OptionMenuKeyboard(vgui::Panel *parent, char const *panelName);
		~OptionMenuKeyboard();

		void OnUpdate(bool bInGame);
		void SetupLayout(void);
		void FillKeyboardList(bool bfullUpdate = false);

	private:
		bool bInEditMode;
		int iCurrentModifiedSelectedID;
		bool m_bShouldUpdate;
		void UpdateKeyboardListData(ButtonCode_t code);

		vgui::SectionedListPanel *m_pKeyBoardList;
		vgui::Button *m_pEditPanel;
		vgui::MouseInputPanel *m_pMousePanel;

		CPanelAnimationVar(float, m_flUpdateTime, "KeyboardRefreshTimer", "0.0f");

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void OnKeyCodeTyped(KeyCode code);
	};
}

#endif // OPTION_MENU_KEYBOARD_H
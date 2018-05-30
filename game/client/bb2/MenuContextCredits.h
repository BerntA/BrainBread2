//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Credits Preview
//
//========================================================================================//

#ifndef MENU_CONTEXT_CREDITS_H
#define MENU_CONTEXT_CREDITS_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "InlineMenuButton.h"
#include "vgui_base_panel.h"

namespace vgui
{
	class MenuContextCredits;
	class MenuContextCredits : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(MenuContextCredits, vgui::CVGUIBasePanel);

	public:
		MenuContextCredits(vgui::Panel *parent, char const *panelName);
		~MenuContextCredits();

		void OnUpdate(bool bInGame);
		void OnShowPanel(bool bShow);
		void SetupLayout(void);

	private:

		char pchDeveloperList[2048];
		char pchContributorList[2048];
		char pchTesterList[2048];
		char pchSpecialThanksList[2048];

		vgui::Label *m_pLabelMessage[4];
		vgui::HFont m_DefaultFont;

		CPanelAnimationVar(float, m_flPosY, "LabelPosition", "0.0f");
		float m_flCreditsScrollTime;
		int m_iSizeH[4];

		int GetNumberOfLinesInString(const char *pszStr);

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
	};
}

#endif // MENU_CONTEXT_CREDITS_H
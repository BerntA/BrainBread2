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
	class CCreditsObject;
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

		vgui::HFont m_DefaultFont;
		Color m_DefaultColor;

		float m_flScrollRate;
		float m_flCurrentScrollRate;
		int m_iPosY;
		int m_iLastYPos;

		CUtlVector<CCreditsObject*> m_listDevelopers;
		CUtlVector<CCreditsObject*> m_listContributors;
		CUtlVector<CCreditsObject*> m_listTesters;
		CUtlVector<CCreditsObject*> m_listSpecialThanks;

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
		void OnMouseWheeled(int delta);
		void Paint();
		void PaintCreditObject(CCreditsObject *item);
		int GetYPos(int offset);
	};
}

#endif // MENU_CONTEXT_CREDITS_H
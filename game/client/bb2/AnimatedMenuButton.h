//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Animated Menu Button - Fades in an overlay behind the text.
//
//========================================================================================//

#ifndef ANIMATED_MENUBUTON_H
#define ANIMATED_MENUBUTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include "ImageProgressBar.h"

namespace vgui
{
	class AnimatedMenuButton;

	class AnimatedMenuButton : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(AnimatedMenuButton, vgui::Panel);

	public:
		AnimatedMenuButton(vgui::Panel *parent, char const *panelName, const char *text, int iCommand, bool bSendToParent = false);
		~AnimatedMenuButton();

		void OnUpdate();
		void SetLabelText(const char *szText) { m_pLabelTitle->SetText(szText); }

	private:

		vgui::Label *m_pLabelTitle;
		ImageProgressBar *m_pRolloverImage;
		vgui::Button *m_pButton;

		bool bGotActivated;
		bool m_bSendToParent;
		int m_iMenuCommand;

		CPanelAnimationVar(float, m_flFadeTime, "FadeTime", "0.0f");

	protected:

		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // ANIMATED_MENUBUTON_H
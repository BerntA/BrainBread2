//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A menu button which renders an icon (fades in) related to what it will do and smoothly interpolates a divider along the text.
//
//========================================================================================//

#ifndef INLINE_MENUBUTTON_H
#define INLINE_MENUBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/ImagePanel.h>

namespace vgui
{
	class InlineMenuButton;

	class InlineMenuButton : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(InlineMenuButton, vgui::Panel);

	public:
		InlineMenuButton(vgui::Panel *parent, char const *panelName, int iCommand, const char *text, const char *fontName = "MainMenuTextBig", int iconSize = 10, bool bSendToParent = false);
		~InlineMenuButton();

		void OnUpdate();
		void SetIconImage(const char *image) { m_pIcon->SetImage(image); }
		void SetCommandValue(int iValue) { m_iMenuCommand = iValue; }
		void SetText(const char *text) { m_pLabelTitle->SetText(text); }

	private:
		vgui::Label *m_pLabelTitle;
		vgui::Button *m_pButton;
		vgui::Divider *m_pOverlay;
		vgui::ImagePanel *m_pIcon;

		int m_iIconSize;
		int m_iLabelLength;
		char szFont[32];
		bool bGotActivated;
		bool m_bSendToParent;
		int m_iMenuCommand;

	protected:
		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // INLINE_MENUBUTTON_H
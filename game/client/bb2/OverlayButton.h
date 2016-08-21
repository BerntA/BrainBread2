//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A button which fades in an image within its bounds when hovering and stays that way until it is deselected if selected.
//
//========================================================================================//

#ifndef OVERLAY_BUTTON_H
#define OVERLAY_BUTTON_H

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

namespace vgui
{
	class OverlayButton;

	class OverlayButton : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(OverlayButton, vgui::Panel);

	public:
		OverlayButton(vgui::Panel *parent, char const *panelName, const char *text, const char *command, const char *font = "MainMenuTextBig");
		~OverlayButton();

		void OnUpdate();
		void SetText(const char *text) { m_pLabelTitle->SetText(text); }
		bool IsSelected() { return m_bIsSelected; }
		void SetSelection(bool bValue);
		void SetToggleMode(bool bValue) { m_bToggleMode = bValue; }
		void Reset(void);

	private:
		bool m_bToggleMode;
		bool m_bIsSelected;
		bool bGotActivated;

		vgui::Label *m_pLabelTitle;
		vgui::Button *m_pButton;
		vgui::ImagePanel *m_pOverlay;

		char pchFont[64];

	protected:
		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // OVERLAY_BUTTON_H
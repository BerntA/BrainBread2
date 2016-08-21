//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Circular Animated Button - Animates a circle overlay (images) when you hover over the control.
//
//========================================================================================//

#ifndef CIRCULARBUTTON_H
#define CIRCULARBUTTON_H

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
	class CircularButton;

	class CircularButton : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE( CircularButton, vgui::Panel );

	public:
		CircularButton( vgui::Panel *parent, char const *panelName );
		~CircularButton();

		vgui::Button *m_pButton;
		vgui::ImagePanel *m_pOverlay;

		void OnUpdate();
		void SetCMD( const char *szCommand );

	private:
		bool m_bMouseOver;
		CPanelAnimationVar(float, m_flFrameFrac, "PictureFrame", "0.0f");

	protected:
		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // CIRCULARBUTTON_H
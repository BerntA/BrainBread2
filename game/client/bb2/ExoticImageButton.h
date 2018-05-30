//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Image Button with basic hovering features.
//
//========================================================================================//

#ifndef EXOTIC_IMAGEBUTTON_H
#define EXOTIC_IMAGEBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>

namespace vgui
{
	class ExoticImageButton;
	class ExoticImageButton : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ExoticImageButton, vgui::Panel);

	public:
		ExoticImageButton(vgui::Panel *parent, char const *panelName, const char *image, const char *hoverImage, const char *command);
		~ExoticImageButton();

		void PerformLayout();
		void OnThink();

	private:
		vgui::ImagePanel *m_pBackground;
		vgui::Button *m_pButton;

		char pchImageDefault[80];
		char pchImageHover[80];

		bool m_bRollover;

	protected:
		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // EXOTIC_IMAGEBUTTON_H
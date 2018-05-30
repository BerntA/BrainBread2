//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Advertisement Panel - Links to our bb2 related pages.
//
//========================================================================================//

#ifndef ADVERTISEMENT_PANEL_H
#define ADVERTISEMENT_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>

namespace vgui
{
	class AdvertisementPanel;

	class AdvertisementPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(AdvertisementPanel, vgui::Panel);

	public:
		AdvertisementPanel(vgui::Panel *parent, char const *panelName);
		~AdvertisementPanel();

		void SetEnabled(bool state);

	private:
		const char *GetAdvertisementURL(int iIndex);

		vgui::Button *m_pButtonAdvert[3];
		vgui::ImagePanel *m_pImageAdvert[3];

	protected:

		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // ADVERTISEMENT_PANEL_H
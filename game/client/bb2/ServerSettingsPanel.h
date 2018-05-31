//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Setup Server Info - Custom BB2 cvars, hostname, password, maxplayers, etc...
//
//========================================================================================//

#ifndef SERVER_SETTINGS_PANEL_H
#define SERVER_SETTINGS_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include "GraphicalCheckBox.h"

namespace vgui
{
	class ServerSettingsPanel;
	class ServerSettingsPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ServerSettingsPanel, vgui::Panel);

	public:
		ServerSettingsPanel(vgui::Panel *parent, char const *panelName);
		~ServerSettingsPanel();

		vgui::Label *m_pLabelInfo[6];
		vgui::TextEntry *m_pEditableField[6];

		vgui::GraphicalCheckBox *m_pCheckFields[3];

		void OnUpdate(bool bInGame);
		void ApplyServerSettings(void);
		void UpdateServerSettings(void);

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // SERVER_SETTINGS_PANEL_H
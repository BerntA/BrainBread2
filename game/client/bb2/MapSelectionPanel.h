//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map Selection Control - Displays a map and allows you to browse to others by hitting the 'next'/'prev' button.
//
//========================================================================================//

#ifndef MAP_SELECTION_PANEL_H
#define MAP_SELECTION_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "MapSelectionItem.h"

namespace vgui
{
	class MapSelectionPanel;
	class MapSelectionPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(MapSelectionPanel, vgui::Panel);

	public:
		MapSelectionPanel(vgui::Panel *parent, char const *panelName);
		~MapSelectionPanel();

		void OnUpdate();
		void ShowMap(int iIndex);
		void Cleanup(void);
		void Redraw(int index = 0);
		const char *GetSelectedMap(void);
		int GetSelectedMapIndex();

		MESSAGE_FUNC_PARAMS(OnMapItemChange, "MapItemClicked", data);

	private:
		MapSelectionItem *m_pMapItems[6];

		vgui::ImagePanel *m_pImgArrowBox[2];
		vgui::Button *m_pBtnArrowBox[2];

		int m_iPageNum;
		int m_iCurrPage;

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
		void OnCommand(const char* pcCommand);
	};
}

#endif // MAP_SELECTION_PANEL_H
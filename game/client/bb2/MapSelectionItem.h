//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map Selection Item for the map selection panel.
//
//========================================================================================//

#ifndef MAP_SELECTION_ITEM_H
#define MAP_SELECTION_ITEM_H

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
	class MapSelectionItem;
	class MapSelectionItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(MapSelectionItem, vgui::Panel);

	public:
		MapSelectionItem(vgui::Panel *parent, char const *panelName, int mapIndex);
		~MapSelectionItem();

		int GetMapIndex(void) { return m_iMapItemIndexLink; }
		void SetupItem(void);
		void SetSelected(bool value) 
		{ 
			m_bSelected = value; 
			m_pFrame->SetImage(m_bSelected ? "mainmenu/preview_frame_over"  : "mainmenu/preview_frame");
		}
		bool IsSelected(void) { return m_bSelected; }

		void OnUpdate(void);

		vgui::Button *m_pButton;
		vgui::ImagePanel *m_pPreview;
		vgui::ImagePanel *m_pPreviewFade;
		vgui::ImagePanel *m_pFrame;

	private:
		int m_iMapItemIndexLink;
		int m_iAmountOfImages;
		bool m_bSelected;
		int m_iLastFaded;
		int m_iLastImage;

		void SelectNewImage(bool bPreview);

		float m_flFadeSpeed;
		float m_flFadeDelay;

	protected:
		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // MAP_SELECTION_ITEM_H
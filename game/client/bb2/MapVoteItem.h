//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map vote item, displays some map or option @ the end game menu.
//
//========================================================================================//

#ifndef MAP_VOTE_ITEM_H
#define MAP_VOTE_ITEM_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/ImagePanel.h>

namespace vgui
{
	class MapVoteItem;
	class MapVoteItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(MapVoteItem, vgui::Panel);

	public:
		MapVoteItem(vgui::Panel *parent, char const *panelName, int type, const char *mapName, bool bButtonOnly = false);
		~MapVoteItem();

		void PerformLayout();
		void OnThink();
		void SetMapLink(const char *map);
		void SetSelection(bool value) { m_bSelected = value; PerformLayout(); }
		int GetVoteType(void) { return m_iVoteType; }

	private:
		vgui::Label *m_pLabelTitle;
		vgui::Label *m_pVoteInfo;

		vgui::Button *m_pButton;
		vgui::Divider *m_pBackground;
		vgui::Divider *m_pVoteBG;
		vgui::ImagePanel *m_pMapImage;

		int m_iVoteType;
		char pchMapLink[MAX_MAP_NAME];
		bool m_bIsButtonOnly; // NO display image / map link.
		bool m_bSelected;
		float m_flUpdateTime;

	protected:
		void OnCommand(const char* pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // MAP_VOTE_ITEM_H
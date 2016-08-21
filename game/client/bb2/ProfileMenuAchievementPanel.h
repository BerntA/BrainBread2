//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the achievements...
//
//========================================================================================//

#ifndef PROFILE_MENU_ACHIEVEMENTS_H
#define PROFILE_MENU_ACHIEVEMENTS_H

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
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/SectionedListPanel.h>
#include "GraphicalCheckBox.h"
#include "GraphicalOverlayInset.h"
#include <vgui_controls/ListPanel.h>
#include "vgui_base_panel.h"
#include "ImageProgressBar.h"
#include <steam/steam_api.h>

namespace vgui
{
	class ProfileMenuAchievementPanel;
	class AchievementItem;

	class AchievementItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(AchievementItem, vgui::Panel);

	public:
		AchievementItem(vgui::Panel *parent, char const *panelName, const char *pszAchievementID);
		virtual ~AchievementItem();

		void SetSize(int wide, int tall);
		void OnUpdate(void);

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

		void SetAchievementIcon(void);

	private:
		vgui::Label *m_pLabelTitle;
		vgui::Label *m_pLabelDescription;
		vgui::Label *m_pLabelProgress;
		vgui::Label *m_pLabelDate;
		vgui::ImagePanel *m_pImageIcon;
		ImageProgressBar *m_pAchievementProgress;

		char pszAchievementStringID[80];
		const char *GetUnlockDate(void);

		bool m_bAchieved;
		float m_flAchievementCheckTime;
	};

	class ProfileMenuAchievementPanel : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(ProfileMenuAchievementPanel, vgui::CVGUIBasePanel);

	public:
		ProfileMenuAchievementPanel(vgui::Panel *parent, char const *panelName);
		~ProfileMenuAchievementPanel();

		void SetupLayout(void);
		void OnUpdate(bool bInGame);
		void Redraw(void);

	private:

		vgui::ScrollBar *m_pScrollBar;
		CUtlVector<AchievementItem*> pszAchievementList;

		MESSAGE_FUNC_INT(OnSliderMoved, "ScrollBarSliderMoved", position);

	protected:

		virtual void OnMouseWheeled(int delta);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // PROFILE_MENU_ACHIEVEMENTS_H
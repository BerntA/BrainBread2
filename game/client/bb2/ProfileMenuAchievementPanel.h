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

#include <steam/steam_api.h>
#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ScrollBar.h>
#include "vgui_base_panel.h"
#include "ImageProgressBar.h"

namespace vgui
{
	class ProfileMenuAchievementPanel;
	class AchievementItem;

	class AchievementItem : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(AchievementItem, vgui::Panel);

	public:
		AchievementItem(vgui::Panel *parent, char const *panelName, const char *pszAchievementID, int achIndex);
		virtual ~AchievementItem();

		void SetSize(int wide, int tall);
		void OnUpdate(void);
		int GetAchievementIndex(void) { return m_iAchievementIndex; }

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void SetAchievementIcon(void);

	private:
		vgui::Label *m_pLabelTitle;
		vgui::Label *m_pLabelDescription;
		vgui::Label *m_pLabelProgress;
		vgui::Label *m_pLabelDate;
		vgui::Label *m_pLabelEXP;
		vgui::ImagePanel *m_pImageIcon;
		ImageProgressBar *m_pAchievementProgress;

		char pszAchievementStringID[80];
		const char *GetUnlockDate(void);

		bool m_bAchieved;
		float m_flAchievementCheckTime;
		int m_iAchievementIndex;
	};

	class ProfileMenuAchievementPanel : public vgui::CVGUIBasePanel
	{
		DECLARE_CLASS_SIMPLE(ProfileMenuAchievementPanel, vgui::CVGUIBasePanel);

	public:
		ProfileMenuAchievementPanel(vgui::Panel *parent, char const *panelName);
		virtual ~ProfileMenuAchievementPanel();

		void CreateAchievementList(void);
		void SetupLayout(void);
		void OnUpdate(bool bInGame);
		void Redraw(void);
		void Cleanup(void);

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
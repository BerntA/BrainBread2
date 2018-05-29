//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Skill Icon - Draws the icon related to the skill, searches scripts/skills/*.txt for a relative file to the cc_language (translatable).
//
//========================================================================================//

#ifndef SKILLTREEICON_H
#define SKILLTREEICON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include "MouseInputPanel.h"

namespace vgui
{
	class SkillTreeIcon;
	class SkillTreeIcon : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(SkillTreeIcon, vgui::Panel);

	public:
		SkillTreeIcon(vgui::Panel *parent, char const *panelName, const char *name, const char *description, const char *command, const char *iconTexture);
		~SkillTreeIcon();

		void SetProgressValue(float value)
		{
			flProgress = (value / 10.0f);
		}

		char *GetSkillName() { return szName; }
		char *GetSkillDescription() { return szDesc; }
		virtual void PerformLayout();

	private:
		char szName[128];
		char szDesc[128];
		char szCommand[128];

		float flProgress;

		vgui::ImagePanel *m_pIcon;
		vgui::MouseInputPanel *m_pMousePanel;

	protected:
		virtual void Paint();
		virtual void OnMousePressed(vgui::MouseCode code);
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // SKILLTREEICON_H
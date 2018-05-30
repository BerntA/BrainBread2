//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays your personal stats in a vgui control.
//
//========================================================================================//

#ifndef CHARACTER_STAT_PREVIEW_H
#define CHARACTER_STAT_PREVIEW_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include "MultiLabel.h"

namespace vgui
{
	class CharacterStatPreview;
	class CharacterStatPreview : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CharacterStatPreview, vgui::Panel);

	public:
		CharacterStatPreview(vgui::Panel *parent, char const *panelName);
		virtual ~CharacterStatPreview();

		virtual void SetSize(int wide, int tall);
		virtual void SetVisible(bool state);
		virtual void ShowStatsForPlayer(int index);

	private:

		vgui::Label *m_pLabelHeader;
		vgui::ImagePanel *m_pImageDivider;

		vgui::MultiLabel *m_pTextDetails[4];

		int m_iActivePlayerIndex;

	protected:

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void OnThink();
	};
}

#endif // CHARACTER_STAT_PREVIEW_H
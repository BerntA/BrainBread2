//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Main Menu overlay screen : draws out main background image and renders film grain above it.
//
//========================================================================================//

#ifndef MENU_CONTEXT_OVERLAY_H
#define MENU_CONTEXT_OVERLAY_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Divider.h>
#include "InlineMenuButton.h"
#include "vgui_base_panel.h"

enum OverlayTypes
{
	VGUI_OVERLAY_MAINMENU = 0,
	VGUI_OVERLAY_INGAME_OPEN,
	VGUI_OVERLAY_INGAME_CLOSE,
	VGUI_OVERLAY_INGAME_FINISH_CLOSE,
	VGUI_OVERLAY_INGAME_FINISH,
};

namespace vgui
{
	class CMenuOverlay;
	class CMenuOverlay : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CMenuOverlay, vgui::Panel);

	public:

		CMenuOverlay(vgui::Panel *parent, char const *panelName);
		~CMenuOverlay();

		void OnUpdate();
		void OnSetupLayout(int value);

	protected:
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void PerformLayout();
		virtual void PaintBackground();

	private:
		int m_iAlpha;
		int m_iActiveCommand;
		bool InGame();

		vgui::ImagePanel *m_pImgBackground;
		vgui::ImagePanel *m_pImgCustomOverlay;
		vgui::Divider *m_pDividerBottom;
	};
}

#endif // MENU_CONTEXT_OVERLAY_H
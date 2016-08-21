//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Image List Panel - Holds a control of image panel for displaying in a list with scrolling of course.
//
//========================================================================================//

#ifndef IMAGE_LIST_PANEL_H
#define IMAGE_LIST_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/Divider.h>
#include "MouseInputPanel.h"
#include "hud_crosshair.h"

namespace vgui
{
	class ImageListPanel;

	class ImageListPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ImageListPanel, vgui::Panel);

	public:
		ImageListPanel(vgui::Panel *parent, char const *panelName);
		~ImageListPanel();

		void SetEnabled(bool state);
		void AddControl(vgui::Panel *pControl);
		void RemoveAll(void);
		void Redraw(bool bRedraw = true);
		void OnThink(void);
		void OnKillFocus();

		void SetActiveItem(const char *token);
		void SetActiveItem(int index);

		const char *GetTokenForIndex(int index);

	private:
		CUtlVector<vgui::Panel*> m_pControlList;
		vgui::ScrollBar *m_pScrollBar;
		vgui::Divider *m_pDivider;
		vgui::MouseInputPanel *m_pInputPanel;
		MESSAGE_FUNC_INT(OnSliderMoved, "ScrollBarSliderMoved", position);

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
		void OnMouseWheeled(int delta);
		void OnMousePressed(vgui::MouseCode code);

		Panel *GetImagePanelAtCursorPos(void);
	};
}

#endif // IMAGE_LIST_PANEL_H
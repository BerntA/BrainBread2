//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: This panel is only working as a hacky workaround for allowing mouse inputs! Put this panel in front of everything ( focus ) and it will record mouse inputs and redirect them to the parent's OnMousePressed Method and other keypress methods!
//
//========================================================================================//

#ifndef MOUSEINPUTPANEL_H
#define MOUSEINPUTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>

namespace vgui
{
	class MouseInputPanel;
	class MouseInputPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(MouseInputPanel, vgui::Panel);

	public:
		MouseInputPanel(vgui::Panel *parent, char const *panelName);
		~MouseInputPanel();

	protected:
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void OnMousePressed(MouseCode code);
		void OnMouseReleased(MouseCode code);
		void OnMouseDoublePressed(MouseCode code);
		void OnKeyCodePressed(KeyCode code);
		void OnMouseWheeled(int delta);
		void PaintBackground();
	};
}

#endif // MOUSEINPUTPANEL_H
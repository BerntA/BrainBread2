//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Slider VGUI Class - Handles the slider function (no gui - hidden) 
//
//========================================================================================//

#ifndef GRAPHICALSLIDER_H
#define GRAPHICALSLIDER_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Slider.h>

namespace vgui
{
	class GraphicalSlider;
	class GraphicalSlider : public vgui::Slider
	{
		DECLARE_CLASS_SIMPLE(GraphicalSlider, vgui::Slider);

	public:
		GraphicalSlider(vgui::Panel *parent, char const *panelName);
		virtual ~GraphicalSlider();

	protected:
		void PaintBackground();
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void DrawNob();
		void OnMousePressed(MouseCode code);
	};
}

#endif // GRAPHICALSLIDER_H
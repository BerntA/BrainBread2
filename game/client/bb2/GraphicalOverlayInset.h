//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Slider VGUI Class, overrides the GUI above the slider sort of.
//
//========================================================================================//

#ifndef GRPAHICALOVERLAYINSET_H
#define GRPAHICALOVERLAYINSET_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include <utlvector.h>
#include <utllinkedlist.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include "vgui_controls/CheckButton.h"
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Divider.h>
#include "GraphicalSlider.h"

namespace vgui
{
	class GraphicalOverlay;
	class GraphicalOverlay : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(GraphicalOverlay, vgui::Panel);

	public:

		enum RawValueType
		{
			TYPE_PERCENT = 0,
			TYPE_FLOAT,
			TYPE_FLOAT_SMALL,
			TYPE_INT,
		};

		GraphicalOverlay(vgui::Panel *parent, char const *panelName, const char *labelText, const char *conVarLink, float flRangeMin, float flRangeMax, bool bNegative = false, int iDisplayRawValue = TYPE_PERCENT);
		~GraphicalOverlay();

		void SetEnabled(bool state);
		void OnUpdate(bool bInGame);

	private:
		void UpdateNobPosition(void);
		char szConVar[64];
		ConVar *m_pCVARLink;

		int m_iRawValue;
		bool m_bNegative;
		float m_flMax;
		float m_flMin;

		vgui::ImagePanel *m_pImgBG;
		vgui::ImagePanel *m_pImgCircle;
		vgui::Divider *m_pFG;
		vgui::GraphicalSlider *m_pSlider;
		vgui::Label *m_pInfo;
		vgui::Label *m_pValue;

		float m_flOldVarVar;
		int m_iOldSliderVal;

	protected:
		void PerformLayout();
		void PaintBackground();
		void ApplySchemeSettings(vgui::IScheme *pScheme);
	};
}

#endif // GRPAHICALOVERLAYINSET_H
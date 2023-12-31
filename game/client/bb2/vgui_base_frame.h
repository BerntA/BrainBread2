//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Class for Frame panels, reimplements the Fade feature + setting escape to main menu fixes to prevent just that when you're in the actual panel.
//
//========================================================================================//

#ifndef VGUI_BASE_FRAME_H
#define VGUI_BASE_FRAME_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/RichText.h>
#include "KeyValues.h"
#include "filesystem.h"

namespace vgui
{
	class CVGUIBaseFrame : public vgui::Frame
	{
		DECLARE_CLASS_SIMPLE(CVGUIBaseFrame, vgui::Frame);

	public:
		CVGUIBaseFrame(vgui::VPANEL parent, const char* panelName, bool bOpenBuildMode = false, float flFadeTime = 0.5f, bool bDisableInput = false);
		virtual ~CVGUIBaseFrame();
		virtual void OnShowPanel(bool bShow);
		virtual void ForceClose(void);

	protected:

		virtual void UpdateLayout(void) {} // Late update: (every sec)
		virtual void OnThink();
		virtual void OnClose() {} // unused
		virtual void OnKeyCodeTyped(vgui::KeyCode code);
		virtual void PaintBackground();
		virtual bool RecordKeyPresses() { return true; }

		// Get our faded background.
		vgui::ImagePanel* GetBackground() {
			return m_pBackground;
		}

	private:

		vgui::ImagePanel* m_pBackground; // Fade out everything behind us?

		float m_flUpdateTime;
		float m_flFadeFrequency;
		bool m_bFadeOut;
		bool m_bOpenBuildMode;
		bool m_bDisableInput;
	};

} // namespace vgui

#endif // VGUI_BASE_FRAME_H
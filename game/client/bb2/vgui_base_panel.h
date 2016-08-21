//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Class for Panels that need a proper fade feature, instead of reinplementing this in every class that inherits Panel and needs a Fade func.
// Notice: Fading is implemented in Frame, however we've overriden some of those features in the new frame base class.
//
//========================================================================================//

#ifndef VGUI_BASE_PANEL_H
#define VGUI_BASE_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "vgui_controls/Frame.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/RichText.h>
#include "KeyValues.h"
#include "filesystem.h"
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

namespace vgui
{
	class CVGUIBasePanel;

	class CVGUIBasePanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CVGUIBasePanel, vgui::Panel);

	public:
		CVGUIBasePanel(vgui::Panel *parent, const char *panelName, float flFadeDuration = 1.0f);
		virtual ~CVGUIBasePanel();
		virtual void OnShowPanel(bool bShow);
		virtual void ForceClose(void);
		virtual bool IsBusy(void) { return m_bIsActive; }
		virtual void OnUpdate(bool bInGame) {} // Override me.
		virtual void SetupLayout(void); // Override me.

	protected:

		virtual void PerformLayout();
		virtual void PaintBackground();
		virtual void OnThink();
		virtual void FullyClosed(void) {} // Override me.
		void UpdateAllChildren(Panel *pChild);
		float GetFadeTime(void){ return m_flFadeDuration; }

	private:

		float m_flFadeDuration;
		bool m_bFadeOut;
		bool m_bIsActive;
	};
}

#endif // VGUI_BASE_PANEL_H
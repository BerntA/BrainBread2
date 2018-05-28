//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Dialogue Wheel / Voice Wheel - Voice Commands
//
//========================================================================================//

#ifndef VOICE_MENU_H
#define VOICE_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui/ILocalize.h>
#include <game/client/iviewport.h>
#include "MouseInputPanel.h"
#include "SkillTreeIcon.h"

namespace vgui
{
	class TextEntry;
}

class CVoiceMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CVoiceMenu, vgui::Frame);

public:
	CVoiceMenu(IViewPort *pViewPort);
	virtual ~CVoiceMenu();

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground();

	virtual void OnThink();
	const char *GetName(void) { return PANEL_VOICEWHEEL; }
	void SetData(KeyValues *data) {};
	void Update() {};
	void Reset();
	bool NeedsUpdate(void) { return false; }
	bool HasInputElements(void) { return true; }
	void ShowPanel(bool bShow);

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

protected:

	virtual vgui::Panel *CreateControlByName(const char *controlName);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	void OnCommand(const char *command);
	void PerformLayout();

	vgui::ImagePanel *m_pImgBackground;
	vgui::ImagePanel *m_pImgOverlay;
	vgui::Button *m_pButton[9];

	IViewPort	*m_pViewPort;
};

#endif // VOICE_MENU_H

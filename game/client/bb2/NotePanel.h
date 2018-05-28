//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A simple note system. Reads the text from the data/notes/<noteName>.txt and the header text sent in the usermessage from the item_note's field.
//
//========================================================================================//

#ifndef NOTE_PANEL_H
#define NOTE_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_base_frame.h"
#include "MouseInputPanel.h"

class CNotePanel : public vgui::CVGUIBaseFrame
{
	DECLARE_CLASS_SIMPLE(CNotePanel, vgui::CVGUIBaseFrame);

public:
	CNotePanel(vgui::VPANEL parent);
	~CNotePanel();

	void SetupNote(const char *szHeader, const char *szFile);

protected:

	virtual void OnShowPanel(bool bShow);
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseWheeled(int delta);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:

	vgui::ImagePanel *m_pBackground;
	vgui::Label *m_pNoteHeader;
	vgui::RichText *m_pNoteText;
	vgui::MouseInputPanel *m_pInputPanel;
};

#endif // NOTE_PANEL_H
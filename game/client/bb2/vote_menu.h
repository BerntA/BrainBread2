//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Vote Menu - Kick, Ban & Map voting.
//
//========================================================================================//

#ifndef VOTE_MENU_H
#define VOTE_MENU_H
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
#include "vgui_base_frame.h"
#include <vgui_controls/SectionedListPanel.h>
#include <networkstringtabledefs.h>

extern INetworkStringTable *g_pStringTableServerMapCycle;

class CVotePanel : public vgui::CVGUIBaseFrame
{
	DECLARE_CLASS_SIMPLE(CVotePanel, vgui::CVGUIBaseFrame);

public:
	CVotePanel(vgui::VPANEL parent);
	~CVotePanel();

	void OnShowPanel(bool bShow);
	void PerformLayout();

private:
	vgui::SectionedListPanel *m_pItemList;
	vgui::Button *m_pButton[5];
	vgui::Label *m_pTitle;
	int m_iSelectedOption;

	CUtlStringList m_VoteSetupMapCycle;

protected:
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char* pcCommand);
};

#endif // VOTE_MENU_H
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Scoreboard VGUI
//
//========================================================================================//

#ifndef SCOREBOARDPANEL_H
#define SCOREBOARDPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_base_frame.h"
#include <game/client/iviewport.h>
#include "GameEventListener.h"
#include "ScoreBoardSectionPanel.h"

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CScoreBoardPanel : public vgui::CVGUIBaseFrame, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE(CScoreBoardPanel, vgui::CVGUIBaseFrame);

public:
	CScoreBoardPanel(IViewPort *pViewPort);
	~CScoreBoardPanel();

	virtual const char *GetName(void) { return PANEL_SCOREBOARD; }
	virtual void SetData(KeyValues *data) { }
	virtual bool NeedsUpdate(void) { return false; }
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);
	virtual void Update() { }

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void Reset();
	virtual void PerformLayout();

protected:

	virtual void FireGameEvent(IGameEvent *event);
	virtual void OnThink();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:

	vgui::Label *serverLabel;
	vgui::Label *m_pInfoLabel[4];
	vgui::CScoreBoardSectionPanel *m_pScoreSections[3];

	IViewPort	*m_pViewPort;
};

#endif // SCOREBOARDPANEL_H
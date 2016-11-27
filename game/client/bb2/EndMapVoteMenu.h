//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map Vote Menu (game over voting)
//
//========================================================================================//

#ifndef MAP_VOTE_MENU_H
#define MAP_VOTE_MENU_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include <utlvector.h>
#include <vgui/ILocalize.h>
#include <vgui/KeyCode.h>
#include <game/client/iviewport.h>
#include "mouseoverpanelbutton.h"
#include "vgui/MouseCode.h"
#include "MapVoteItem.h"

namespace vgui
{
	class TextEntry;
}

class CEndMapVoteMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CEndMapVoteMenu, vgui::Frame);

public:

	CEndMapVoteMenu(IViewPort *pViewPort);
	virtual ~CEndMapVoteMenu();

	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void PaintBackground();
	void PerformLayout();
	void OnThink();
	const char *GetName(void) { return PANEL_ENDVOTE; }
	void SetData(KeyValues *data);
	void Update();
	void Reset();
	bool NeedsUpdate(void) { return false; }
	bool HasInputElements(void) { return true; }
	void ShowPanel(bool bShow);
	void Paint(void);

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

private:

	float m_flVoteTimeEnd;

protected:

	MESSAGE_FUNC_PARAMS(OnPlayerVote, "OnPlayerVote", data);

	virtual vgui::Panel *CreateControlByName(const char *controlName);

	vgui::ImagePanel *m_pBackgroundImg;
	vgui::Label *m_pLabelVoteAnnounce;
	vgui::MapVoteItem *m_pVoteOption[6];

	IViewPort	*m_pViewPort;
};

#endif // MAP_VOTE_MENU_H
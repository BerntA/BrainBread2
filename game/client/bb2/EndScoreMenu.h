//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Round End Score / End Game Score Preview, displaying best players, who won, etc in an animated and fancy manner.
//
//========================================================================================//

#ifndef ENDSCOREMENU_H
#define ENDSCOREMENU_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_base_frame.h"
#include <vgui_controls/Button.h>
#include <game/client/iviewport.h>
#include "MouseInputPanel.h"
#include "TopPlayersPanel.h"
#include "CharacterStatPreview.h"

namespace vgui
{
	class TextEntry;
}

class CEndScoreMenu : public vgui::CVGUIBaseFrame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CEndScoreMenu, vgui::CVGUIBaseFrame);

public:
	CEndScoreMenu(IViewPort *pViewPort);
	virtual ~CEndScoreMenu();

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground();

	virtual void OnThink();
	const char *GetName(void) { return PANEL_ENDSCORE; }
	void SetData(KeyValues *data);
	void Update();
	void Reset();
	bool NeedsUpdate(void) { return false; }
	bool HasInputElements(void) { return false; }
	void ShowPanel(bool bShow);

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	bool IsVisible() { return BaseClass::IsVisible(); }
	void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

private:
	float m_iLabelWinnerPositionY;

protected:

	bool RecordKeyPresses() { return false; }
	virtual vgui::Panel *CreateControlByName(const char *controlName);
	void PerformLayout();
	void SetupLayout(bool bReset = false, int iWinner = 0, bool bTimeRanOut = false);
	void ForceClose(void);

	vgui::Label *m_pLabelWinner;
	vgui::ImagePanel *m_pImgBannerWinner;

	vgui::TopPlayersPanel *m_pTopPlayerList;
	vgui::CharacterStatPreview *m_pCharacterStatsPreview;

	IViewPort	*m_pViewPort;
};

#endif // ENDSCOREMENU_H
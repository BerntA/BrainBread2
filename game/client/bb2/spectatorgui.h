//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SPECTATORGUI_H
#define SPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/ComboBox.h>
#include <igameevents.h>
#include "GameEventListener.h"

#include <game/client/iviewport.h>

class KeyValues;

namespace vgui
{
	class TextEntry;
	class Button;
	class Panel;
	class ImagePanel;
	class ComboBox;
}

#define BLACK_BAR_COLOR	Color(0, 0, 0, 196)

class IBaseFileSystem;

//-----------------------------------------------------------------------------
// Purpose: Spectator UI
//-----------------------------------------------------------------------------
class CSpectatorGUI : public vgui::EditablePanel, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE( CSpectatorGUI, vgui::EditablePanel );

public:
	CSpectatorGUI( IViewPort *pViewPort );
	virtual ~CSpectatorGUI();

	virtual const char *GetName( void ) { return PANEL_SPECGUI; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return false; }
	virtual void ShowPanel( bool bShow );
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();
	
	virtual bool ShouldShowPlayerLabel( int specmode );

	virtual Color GetBlackBarColor( void ) { return BLACK_BAR_COLOR; }
	
protected:

	void UpdateSpecInfo();

protected:	
	enum { INSET_OFFSET = 2 } ; 

	// vgui overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	vgui::Divider *m_pBackground;
	vgui::Label *m_pGameInfo;
	vgui::Label *m_pPlayerLabel;
	vgui::Label *m_pSharedInfo;

	char pszMapName[128];

	bool m_bShouldHide;

	IViewPort *m_pViewPort;
};

extern CSpectatorGUI * g_pSpectatorGUI;

#endif // SPECTATORGUI_H

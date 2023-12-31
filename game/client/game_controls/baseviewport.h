//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMFORTRESSVIEWPORT_H
#define TEAMFORTRESSVIEWPORT_H

// viewport interface for the rest of the dll
#include <game/client/iviewport.h>

#include <utlqueue.h> // a vector based queue template to manage our VGUI menu queue
#include <vgui_controls/Frame.h>
#include "vguitextwindow.h"
#include "vgui/ISurface.h"
#include <igameevents.h>

using namespace vgui;

class IBaseFileSystem;
class IGameUIFuncs;
class IGameEventManager;

//==============================================================================
class CBaseViewport : public vgui::EditablePanel, public IViewPort, public IGameEventListener2
{
	DECLARE_CLASS_SIMPLE( CBaseViewport, vgui::EditablePanel );

public: 
	CBaseViewport();
	virtual ~CBaseViewport();

	virtual IViewPortPanel* CreatePanelByName(const char *szPanelName);
	virtual IViewPortPanel* FindPanelByName(const char *szPanelName);
	virtual IViewPortPanel* GetActivePanel( void );
	virtual void RemoveAllPanels( void);

	virtual void ShowPanel( const char *pName, bool state );
	virtual void ShowPanel( IViewPortPanel* pPanel, bool state );
	virtual bool AddNewPanel( IViewPortPanel* pPanel, char const *pchDebugName );
	virtual void CreateDefaultPanels( void );
	virtual void UpdateAllPanels( void );
	virtual void PostMessageToPanel( const char *pName, KeyValues *pKeyValues );

	virtual void Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager2 *pGameEventManager );
	virtual void SetParent(vgui::VPANEL parent);

	virtual void ReloadScheme(const char *fromFile);
	virtual void ActivateClientUI();
	virtual void HideClientUI();
	virtual bool AllowedToPrintText( void );
	
	virtual AnimationController *GetAnimationController() { return m_pAnimController; }

	virtual void ShowBackGround(bool bShow) { }

	virtual int GetDeathMessageStartHeight( void );	

public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent * event);

protected:
	bool LoadHudAnimations( void );

protected:
	virtual void Paint();
	virtual void OnThink(); 
	virtual void OnClose() {} // unused
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	void PostMessageToPanel( IViewPortPanel* pPanel, KeyValues *pKeyValues );

protected:
	IGameUIFuncs*		m_GameuiFuncs; // for key binding details
	IGameEventManager2*	m_GameEventManager;
	CUtlVector<IViewPortPanel*> m_Panels;
	
	bool				m_bHasParent; // Used to track if child windows have parents or not.
	bool				m_bInitialized;

	IViewPortPanel		*m_pActivePanel;
	IViewPortPanel		*m_pLastActivePanel;

	vgui::HCursor		m_hCursorNone;
	vgui::AnimationController *m_pAnimController;
	int					m_OldSize[2];
};

#endif
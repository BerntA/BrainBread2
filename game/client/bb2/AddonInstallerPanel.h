//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Workshop client downloader and installer, triggered when you join a server hosting a workshop based map, auto install that workshop item and rejoin!
//
//========================================================================================//

#ifndef ADDON_INSTALLER_PANEL_H
#define ADDON_INSTALLER_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include <vgui/VGUI.h>
#include "vgui_controls/Frame.h"
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include "fmod_manager.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Divider.h>
#include "ImageProgressBar.h"
#include <steam/steam_api.h>

struct gameMapItem_t;
class CAddonInstallerPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CAddonInstallerPanel, vgui::Frame);

public:
	CAddonInstallerPanel(vgui::VPANEL parent);
	~CAddonInstallerPanel();

	virtual void ShowAddonPanel(bool visible, PublishedFileId_t addon);
	virtual void Cleanup(void);

protected:
	virtual void OnThink();
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual	void SetupLayout(void);

private:
	vgui::ImagePanel *m_pImgBackground;
	vgui::Label *m_pTextAddonInfo;
	vgui::Label *m_pTextProgress;
	vgui::Divider *m_pBlackDivider;
	ImageProgressBar *m_pProgressBar;

	float m_flLastTimeFetchedProgress;
	PublishedFileId_t m_ulWorkshopItemID;
	UGCQueryHandle_t addonQueryHandle;
	CUtlVector<gameMapItem_t> m_pTempMapItems;

	STEAM_CALLBACK(CAddonInstallerPanel, OnWorkshopItemInstalled, ItemInstalled_t, m_CallbackWorkshopItemInstalled);

	void OnReceiveUGCQueryResults(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure);
	CCallResult< CAddonInstallerPanel, SteamUGCQueryCompleted_t > m_SteamCallResultUGCQuery;
};

#endif // ADDON_INSTALLER_PANEL_H
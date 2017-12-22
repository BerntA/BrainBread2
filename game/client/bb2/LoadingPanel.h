//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Loading Dialog - Reports Loading Progress and the Loading String as well!
// Notice: We also deal with some client initialization on loading end.
//
//========================================================================================//

#ifndef CUSTOM_LOADING_PANEL_H
#define CUSTOM_LOADING_PANEL_H
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

namespace vgui
{
	class Panel;
}

class CLoadingPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CLoadingPanel, vgui::Frame);

public:
	CLoadingPanel(vgui::VPANEL parent);
	~CLoadingPanel();

	void SetRandomLoadingTip(void);

protected:
	virtual void OnThink();
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void OnTick();
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void FindVitalParentControls();
	virtual bool SafelyCloseLoadingScreen()
	{
		if (m_pParentCancelButton == NULL)
			return false;

		m_pParentCancelButton->FireActionSignal();
		return true;
	}
	virtual void ClearVitalParentControls()
	{
		m_pParentCancelButton = NULL;
		m_pParentProgressBar = NULL;
		m_pParentProgressText = NULL;
	}

	vgui::Button *m_pParentCancelButton;
	vgui::ProgressBar *m_pParentProgressBar;
	vgui::Label *m_pParentProgressText;

	bool m_bCanUpdateImage;
	bool m_bDisconnected;
	bool m_bLoadedMapData;
	float m_flLastTimeCheckedDownload;

private:
	void SetupLayout(void);

	vgui::ImagePanel *m_pImgTipIcon;
	vgui::ImagePanel *m_pImgLoadingBackground;
	vgui::Label *m_pTextLoadingTip;
	vgui::Label *m_pTextProgress;
	vgui::Divider *m_pTipBackground;
	ImageProgressBar *m_pProgressBar;

	// Map Details
	Color colMapString;
	Color colStatsStatus;
	vgui::Label *m_pTextMapDetail[4];
	ImageProgressBar *m_pMapRating;

	void SetLoadingAttributes(void);

	// Handle [] functions in tips.
	const char *GetLoadingTip(const char *text);
	char *ReplaceBracketsWithInfo(char *text);
};

#endif // CUSTOM_LOADING_PANEL_H
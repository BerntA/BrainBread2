//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Loading Dialog - Reports Loading Progress and the Loading String as well!
// Notice: We also deal with some client initialization on loading end.
//
//========================================================================================//

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
	bool m_bCanUpdateImage;
	bool m_bDisconnected;
	bool m_bLoadedMapData;

protected:
	virtual void OnThink();
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void OnTick();
	virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

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
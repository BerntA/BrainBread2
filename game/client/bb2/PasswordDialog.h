//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A Password Dialog which pops up on writing connect <ip:port> .... But only if the server requires a password!
//
//========================================================================================//

#include <vgui_controls/Panel.h>
#include <vgui/IVGui.h>
#include <vgui_controls/TextEntry.h>
#include "InlineMenuButton.h"

class CPasswordDialog : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CPasswordDialog, vgui::Panel);

public:
	CPasswordDialog(vgui::Panel *parent, char const *panelName);
	~CPasswordDialog();

	void ActivateUs(bool bActivate);
	void OnUpdate(bool bInGame);
	void PerformLayout();

protected:

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnCommand(const char *pcCommand);

private:

	vgui::ImagePanel *m_pBackground;
	vgui::Label *m_pInfo;
	vgui::TextEntry *m_pPasswordText;
	vgui::InlineMenuButton *m_pButton[2];
};
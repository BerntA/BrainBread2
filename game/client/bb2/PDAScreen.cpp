//=========       Copyright © Reperio Studios 2017 @ Bernt Andreas Eide!       ============//
//
// Purpose: PDA VGUI Screen.
//
//========================================================================================//

#include "cbase.h"
#include "PDAScreen.h"
#include "c_vguipdascreen.h"
#include "vgui/IVGui.h"
#include "vgui/IScheme.h"

using namespace vgui;

PDAScreen::PDAScreen(vgui::Panel *pParent, const char *pMetaClassName) : CVGuiScreenPanel(pParent, pMetaClassName)
{
	SetScheme("BaseScheme");
	m_pText = NULL;
}

PDAScreen::~PDAScreen()
{
	m_pText = NULL;
}

bool PDAScreen::Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData)
{
	if (!CVGuiScreenPanel::Init(pKeyValues, pInitData))
		return false;

	vgui::ivgui()->AddTickSignal(GetVPanel(), 1000);
	m_pText = dynamic_cast<vgui::Label*>(FindChildByName("TextLabel"));
	return true;
}

void PDAScreen::OnTick()
{
	CVGuiScreenPanel::OnTick();

	if (m_pText == NULL)
		return;

	C_VGuiPDAScreen *pPDABase = dynamic_cast<C_VGuiPDAScreen*> (GetEntity());
	if (!pPDABase)
		return;

	m_pText->SetText(pPDABase->GetKeyPadCode());
	m_pText->SetFgColor(Color(0, 0, 0, 255));
}

DECLARE_VGUI_SCREEN_FACTORY(PDAScreen, "PDAScreen");
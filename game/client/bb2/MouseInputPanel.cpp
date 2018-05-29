//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: This panel is only working as a hacky workaround for allowing mouse inputs! Put this panel in front of everything ( focus ) and it will record mouse inputs and redirect them to the parent's OnMousePressed Method and other keypress methods!
//
//========================================================================================//

#include "cbase.h"
#include "MouseInputPanel.h"
#include <vgui/MouseCode.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

MouseInputPanel::MouseInputPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetZPos(600);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	InvalidateLayout();
}

MouseInputPanel::~MouseInputPanel()
{
}

// Pass mouse inputs to the parent: Then check baseclass in the parent, etc...

void MouseInputPanel::OnMousePressed(MouseCode code)
{
	vgui::Panel *myParent = this->GetParent();
	if (myParent)
		myParent->OnMousePressed(code);
}

void MouseInputPanel::OnMouseReleased(MouseCode code)
{
	vgui::Panel *myParent = this->GetParent();
	if (myParent)
		myParent->OnMouseReleased(code);
}

void MouseInputPanel::OnMouseDoublePressed(MouseCode code)
{
	vgui::Panel *myParent = this->GetParent();
	if (myParent)
		myParent->OnMouseDoublePressed(code);
}

void MouseInputPanel::OnKeyCodePressed(KeyCode code)
{
	if (code == KEY_TAB)
	{
		vgui::Panel *myParent = this->GetParent();
		if (myParent)
			myParent->OnKeyCodeTyped(code);
	}
}

void MouseInputPanel::OnMouseWheeled(int delta)
{
	vgui::Panel *myParent = this->GetParent();
	if (myParent)
		myParent->OnMouseWheeled(delta);
}

void MouseInputPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
}

void MouseInputPanel::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}
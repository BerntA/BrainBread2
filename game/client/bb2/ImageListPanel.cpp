//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Image List Panel - Holds a control of image panel for displaying in a list with scrolling of course.
//
//========================================================================================//

#include "cbase.h"
#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include <vgui/IVGui.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "ImageListPanel.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "KeyValues.h"
#include "clientmode_shared.h"
#include <steam/steam_api.h>
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ImageListPanel::ImageListPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	m_pControlList.Purge();

	SetScheme("BaseScheme");

	m_pScrollBar = vgui::SETUP_PANEL(new vgui::ScrollBar(this, "ScrollBar", true));
	m_pScrollBar->AddActionSignalTarget(this);
	m_pScrollBar->SetZPos(35);
	m_pScrollBar->SetVisible(false);
	m_pScrollBar->InvalidateLayout();

	m_pInputPanel = vgui::SETUP_PANEL(new vgui::MouseInputPanel(this, "InputPanel"));
	m_pInputPanel->AddActionSignalTarget(this);
	m_pInputPanel->SetZPos(100);

	m_pDivider = vgui::SETUP_PANEL(new vgui::Divider(this, "Divider"));
	m_pDivider->SetVisible(false);
	m_pDivider->SetZPos(30);

	InvalidateLayout();
	PerformLayout();
}

ImageListPanel::~ImageListPanel()
{
	RemoveAll();
}

void ImageListPanel::AddControl(vgui::Panel *pControl)
{
	m_pControlList.AddToTail(pControl);
	Redraw();
}

void ImageListPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize(w, h);

	m_pScrollBar->SetPos(w - scheme()->GetProportionalScaledValue(10), 0);
	m_pScrollBar->SetSize(scheme()->GetProportionalScaledValue(10), h);
	Redraw();
}

void ImageListPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScrollBar->SetBorder(NULL);
	m_pScrollBar->SetBgColor(Color(20, 20, 20, 255));
	m_pScrollBar->SetFgColor(Color(32, 33, 35, 200));
	m_pScrollBar->SetPaintBorderEnabled(false);

	m_pDivider->SetBorder(NULL);
	m_pDivider->SetPaintBorderEnabled(false);
	m_pDivider->SetFgColor(Color(120, 15, 15, 80));
	m_pDivider->SetBgColor(Color(120, 15, 15, 80));

	SetBorder(NULL);
	SetPaintBorderEnabled(false);

	SetFgColor(Color(23, 25, 24, 180));
	SetBgColor(Color(20, 21, 29, 255));

	for (int i = 0; i < m_pControlList.Count(); i++)
	{
		vgui::Label *pLabel = dynamic_cast<vgui::Label*> (m_pControlList[i]);
		if (pLabel)
		{
			pLabel->SetFgColor(Color(crosshair_color_red.GetInt(), crosshair_color_green.GetInt(), crosshair_color_blue.GetInt(), 255));
			pLabel->SetFont(pScheme->GetFont("CrosshairBB2", false));
			pLabel->SetContentAlignment(Label::Alignment::a_center);
		}
	}
}

void ImageListPanel::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);
	m_pScrollBar->SetEnabled(state);
	m_pDivider->SetEnabled(state);
}

Panel *ImageListPanel::GetImagePanelAtCursorPos(void)
{
	int x, y;
	input()->GetCursorPosition(x, y);

	for (int i = 0; i < m_pControlList.Count(); ++i)
	{
		if (m_pControlList[i]->IsWithin(x, y))
			return m_pControlList[i];
	}

	return NULL;
}

void ImageListPanel::RemoveAll(void)
{
	for (int i = (m_pControlList.Count() - 1); i >= 0; i--)
		delete m_pControlList[i];

	m_pControlList.Purge();

	m_pScrollBar->SetValue(0);
	Redraw(false);
}

void ImageListPanel::OnKillFocus()
{
	BaseClass::OnKillFocus();
	SetVisible(false);
}

void ImageListPanel::OnThink(void)
{
	BaseClass::OnThink();

	if (IsVisible())
	{
		Panel *pPanel = GetImagePanelAtCursorPos();
		if (pPanel)
		{
			int x, y, w, h;
			pPanel->GetBounds(x, y, w, h);
			m_pDivider->SetVisible(true);
			m_pDivider->SetPos(x, y);
			m_pDivider->SetSize(w, h);
		}
		else
			m_pDivider->SetVisible(false);
	}
}

void ImageListPanel::Redraw(bool bRedraw)
{
	int w, h, wz, hz;
	m_pScrollBar->GetSize(wz, hz);
	GetSize(w, h);

	int size = w - (m_pScrollBar->IsVisible() ? wz : 0);
	int iHeight = 0;
	for (int i = 0; i < m_pControlList.Count(); ++i)
		iHeight += m_pControlList[i]->GetTall();

	// Do we need to scroll?
	if (iHeight > h)
	{
		m_pScrollBar->SetVisible(true);
		float rowsPerPage = (((float)GetTall()) / ((float)size));
		m_pScrollBar->SetRange(0, m_pControlList.Count());
		m_pScrollBar->SetButtonPressedScrollValue(1);
		m_pScrollBar->SetRangeWindow((int)rowsPerPage);
	}
	else
		m_pScrollBar->SetVisible(false);

	float flPercent = ((float)m_pScrollBar->GetValue() / (float)m_pControlList.Count());
	float flUnitsToMove = ((float)size * (float)m_pControlList.Count()) * flPercent;

	for (int i = 0; i < m_pControlList.Count(); ++i)
	{
		m_pControlList[i]->SetSize(size, size);
		m_pControlList[i]->SetPos(0, (size * i) - flUnitsToMove);
	}

	m_pInputPanel->SetPos(0, 0);
	m_pInputPanel->SetSize(size, h);
	m_pInputPanel->MoveToFront();

	if (bRedraw)
		InvalidateLayout(false, true);
}

void ImageListPanel::OnSliderMoved(int value)
{
	if (!m_pScrollBar->IsVisible() || !m_pScrollBar->IsEnabled())
		return;

	Redraw();
}

void ImageListPanel::OnMouseWheeled(int delta)
{
	int x, y;
	m_pScrollBar->GetRange(x, y);

	bool bUp = (delta < 0);
	if (bUp && (m_pScrollBar->GetValue() < y))
		m_pScrollBar->SetValue(m_pScrollBar->GetValue() + 1);
	else if (!bUp && (m_pScrollBar->GetValue() > 0))
		m_pScrollBar->SetValue(m_pScrollBar->GetValue() - 1);

	Redraw();

	BaseClass::OnMouseWheeled(delta);
}

void ImageListPanel::OnMousePressed(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		int x, y;
		input()->GetCursorPosition(x, y);

		for (int i = 0; i < m_pControlList.Count(); ++i)
		{
			if (m_pControlList[i]->IsWithin(x, y))
			{
				PostActionSignal(new KeyValues("ControlClicked", "index", VarArgs("%i", i)));
				SetVisible(false);
				break;
			}
		}

		if (!IsWithin(x, y))
			SetVisible(false);
	}

	BaseClass::OnMousePressed(code);
}

void ImageListPanel::SetActiveItem(const char *token)
{
	for (int i = 0; i < m_pControlList.Count(); ++i)
	{
		vgui::ImagePanel *pImage = dynamic_cast<vgui::ImagePanel*> (m_pControlList[i]);
		if (pImage)
		{
			if (!strcmp(pImage->GetImageName(), token))
			{
				PostActionSignal(new KeyValues("ControlClicked", "index", VarArgs("%i", i)));
				break;
			}
		}

		vgui::Label *pLabel = dynamic_cast<vgui::Label*> (m_pControlList[i]);
		if (pLabel)
		{
			char pszText[16];
			pLabel->GetText(pszText, 16);
			if (!strcmp(pszText, token))
			{
				PostActionSignal(new KeyValues("ControlClicked", "index", VarArgs("%i", i)));
				break;
			}
		}
	}
}

void ImageListPanel::SetActiveItem(int index)
{
	for (int i = 0; i < m_pControlList.Count(); ++i)
	{
		if (i == index)
		{
			PostActionSignal(new KeyValues("ControlClicked", "index", VarArgs("%i", i)));
			break;
		}
	}
}

const char *ImageListPanel::GetTokenForIndex(int index)
{
	const char *tokenInfo = NULL;
	for (int i = 0; i < m_pControlList.Count(); ++i)
	{
		if (i == index)
		{
			vgui::ImagePanel *pImage = dynamic_cast<vgui::ImagePanel*> (m_pControlList[i]);
			if (pImage)
			{
				tokenInfo = pImage->GetImageName();
				break;
			}

			vgui::Label *pLabel = dynamic_cast<vgui::Label*> (m_pControlList[i]);
			if (pLabel)
			{
				char pszText[16];
				pLabel->GetText(pszText, 16);
				tokenInfo = pszText;
				break;
			}
		}
	}

	return tokenInfo;
}
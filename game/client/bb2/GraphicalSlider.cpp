//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Slider VGUI Class - Handles the slider function (no gui - hidden) 
//
//========================================================================================//

#include "cbase.h"
#include "GraphicalSlider.h"
#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include <vgui/IBorder.h>
#include <vgui/ISystem.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#define NOB_HEIGHT 64

GraphicalSlider::GraphicalSlider(vgui::Panel *parent, char const *panelName) : vgui::Slider(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);
	m_nNumTicks = 20;
}

GraphicalSlider::~GraphicalSlider()
{
}

void GraphicalSlider::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

void GraphicalSlider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide, tall;
	GetTrackRect(x, y, wide, tall);
	Color col = GetFgColor();
#ifdef _X360
	if(HasFocus())
	{
		col = m_DepressedBgColor;
	}
#endif
	surface()->DrawSetColor(col);

	int nobheight = NOB_HEIGHT;

	surface()->DrawFilledRect(
		_nobPos[0],
		y + tall / 2 - nobheight / 2,
		_nobPos[1],
		y + tall / 2 + nobheight / 2);
	// border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0],
			y + tall / 2 - nobheight / 2,
			_nobPos[1],
			y + tall / 2 + nobheight / 2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Respond to mouse presses. Trigger Record staring positon.
//-----------------------------------------------------------------------------
void GraphicalSlider::OnMousePressed(MouseCode code)
{
	if (!IsEnabled())
		return;

	int x, y;
	input()->GetCursorPosition(x, y);

	ScreenToLocal(x, y);
	RequestFocus();

	bool startdragging = false, bPostDragStartSignal = false;

	if ((x >= _nobPos[0]) && (x < (_nobPos[1] + 15)))
	{
		startdragging = true;
		bPostDragStartSignal = true;
	}
	else
	{
		// we clicked elsewhere on the slider; move the nob to that position
		int min, max;
		GetRange(min, max);
		if (m_bUseSubRange)
		{
			min = _subrange[0];
			max = _subrange[1];
		}

		int _x, _y, wide, tall;
		GetTrackRect(_x, _y, wide, tall);
		if (wide > 0)
		{
			float frange = (float)(max - min);
			float clickFrac = clamp((float)(x - _x) / (float)(wide - 1), 0.0f, 1.0f);

			float value = (float)min + clickFrac * frange;

			startdragging = IsDragOnRepositionNob();

			if (startdragging)
			{
				_dragging = true; // Required when as
				SendSliderDragStartMessage();
			}

			SetValue((int)(value + 0.5f));
		}
	}

	if (startdragging)
	{
		// drag the nob
		_dragging = true;
		input()->SetMouseCapture(GetVPanel());
		_nobDragStartPos[0] = _nobPos[0];
		_nobDragStartPos[1] = _nobPos[1];
		_dragStartPos[0] = x;
		_dragStartPos[1] = y;
	}

	if (bPostDragStartSignal)
		SendSliderDragStartMessage();
}

void GraphicalSlider::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	SetFgColor(Color(0, 0, 0, 0));

	// this line is useful for debugging
	SetBgColor(Color(0, 0, 0, 0));

	m_TickColor = Color(0, 0, 0, 0);
	m_TrackColor = Color(0, 0, 0, 0);

#ifdef _X360
	m_DepressedBgColor = Color( 255, 255, 255, 0 );
#endif

	m_DisabledTextColor1 = Color(255, 255, 255, 0);
	m_DisabledTextColor2 = Color(255, 255, 255, 0);

	_sliderBorder = NULL;
	_insetBorder = NULL;

	if (_rightCaption)
	{
		_rightCaption->SetColor(Color(255, 255, 255, 0));
		_rightCaption->SetFont(pScheme->GetFont("MainMenuTextSmall", IsProportional()));
		_rightCaption->ResizeImageToContent();
	}

	if (_leftCaption)
	{
		_leftCaption->SetColor(Color(255, 255, 255, 0));
		_leftCaption->SetFont(pScheme->GetFont("MainMenuTextSmall", IsProportional()));
		_leftCaption->ResizeImageToContent();
	}
}
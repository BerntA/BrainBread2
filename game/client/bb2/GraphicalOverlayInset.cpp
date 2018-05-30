//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Slider VGUI Class, overrides the GUI above the slider sort of.
//
//========================================================================================//

#include "cbase.h"
#include "GraphicalOverlayInset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

#define SLIDER_GUI_TALL 4
#define SLIDER_XPOS_OFFSET scheme()->GetProportionalScaledValue(2)

GraphicalOverlay::GraphicalOverlay(vgui::Panel *parent, char const *panelName, const char *labelText, const char *conVarLink, float flRangeMin, float flRangeMax, bool bNegative, int iDisplayRawValue)
	: vgui::Panel(parent, panelName)
{
	m_pCVARLink = NULL;

	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pImgBG = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "BG"));
	m_pImgBG->SetShouldScaleImage(true);
	m_pImgBG->SetImage("mainmenu/sliderbg");
	m_pImgBG->SetZPos(5);

	m_pImgCircle = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Circle"));
	m_pImgCircle->SetShouldScaleImage(true);
	m_pImgCircle->SetImage("mainmenu/sliderknob");
	m_pImgCircle->SetZPos(15);

	m_pSlider = vgui::SETUP_PANEL(new vgui::GraphicalSlider(this, "Slider"));
	m_pSlider->SetZPos(20);

	m_pFG = vgui::SETUP_PANEL(new vgui::Divider(this, "DividerFG"));
	m_pFG->SetZPos(10);
	m_pFG->SetBorder(NULL);
	m_pFG->SetPaintBorderEnabled(false);

	m_pInfo = vgui::SETUP_PANEL(new vgui::Label(this, "InfoText", ""));
	m_pInfo->SetZPos(1);
	m_pInfo->SetText(labelText);

	m_pValue = vgui::SETUP_PANEL(new vgui::Label(this, "InfoText", ""));
	m_pValue->SetZPos(15);
	m_pValue->SetText("");

	m_iRawValue = iDisplayRawValue;
	m_bNegative = bNegative;
	m_flMin = flRangeMin;
	m_flMax = flRangeMax;

	m_pSlider->SetRange(0, 100);
	m_pSlider->SetTickCaptions("", "");

	m_pCVARLink = cvar->FindVar(conVarLink);

	PerformLayout();
}

GraphicalOverlay::~GraphicalOverlay()
{
}

void GraphicalOverlay::PerformLayout(void)
{
	BaseClass::PerformLayout();

	if (m_pCVARLink)
	{
		float percent = ((fabs(m_pCVARLink->GetFloat()) - m_flMin) * (100.0f / (m_flMax - m_flMin)));
		m_pSlider->SetValue(((int)percent));
		m_flOldVarVal = m_pCVARLink->GetFloat();
		m_iOldSliderVal = m_pSlider->GetValue();

		switch (m_iRawValue)
		{
		case RawValueType::TYPE_FLOAT:
			m_pValue->SetText(VarArgs("%.3f", m_pCVARLink->GetFloat()));
			break;
		case RawValueType::TYPE_FLOAT_SMALL:
			m_pValue->SetText(VarArgs("%.1f", m_pCVARLink->GetFloat()));
			break;
		case RawValueType::TYPE_INT:
			m_pValue->SetText(VarArgs("%i", m_pCVARLink->GetInt()));
			break;
		default: // Percent fallback.
			m_pValue->SetText(VarArgs("%i", m_pSlider->GetValue()));
			break;
		}
	}

	int w = 0,
		h = 0,
		sliderY = ((GetTall() / 2) + (scheme()->GetProportionalScaledValue(SLIDER_GUI_TALL) / 2)),
		sliderH = ((GetTall() / 2) - scheme()->GetProportionalScaledValue(SLIDER_GUI_TALL));

	GetSize(w, h);

	m_pInfo->SetSize(w, (h / 2));
	m_pInfo->SetPos(0, 0);

	m_pSlider->SetSize((w / 2), (h / 2));
	m_pSlider->SetPos(SLIDER_XPOS_OFFSET, (h / 2));

	m_pValue->SetPos(SLIDER_XPOS_OFFSET + (w / 2) + scheme()->GetProportionalScaledValue(2), (h / 2));
	m_pValue->SetSize((w / 2), (h / 2));

	m_pImgBG->SetSize((w / 2) - SLIDER_XPOS_OFFSET, sliderH);
	m_pImgBG->SetPos(SLIDER_XPOS_OFFSET, sliderY);

	m_pFG->SetPos(SLIDER_XPOS_OFFSET, sliderY);

	m_pImgCircle->SetSize((h / 2), (h / 2));

	UpdateNobPosition();
}

void GraphicalOverlay::UpdateNobPosition(void)
{
	int x, y, w, h, xz, yz;
	int sliderHeight = (GetTall() / 2) - scheme()->GetProportionalScaledValue(SLIDER_GUI_TALL);

	m_pSlider->GetSize(w, h);
	m_pSlider->GetNobPos(x, y);
	m_pSlider->GetPos(xz, yz);
	m_pImgCircle->SetPos(x + xz - scheme()->GetProportionalScaledValue(2), (GetTall() / 2));
	Repaint();

	m_pFG->SetSize(x + (y - x), sliderHeight);
}

void GraphicalOverlay::OnUpdate(bool bInGame)
{
	if (m_pCVARLink)
	{
		if (m_iOldSliderVal != m_pSlider->GetValue())
		{
			UpdateNobPosition();
			float percent = m_flMin + (((float)m_pSlider->GetValue() / 100.0f) * (m_flMax - m_flMin));
			m_pCVARLink->SetValue(m_bNegative ? -percent : percent);
			engine->ClientCmd_Unrestricted("host_writeconfig\n");
			m_iOldSliderVal = m_pSlider->GetValue();
			m_flOldVarVal = m_pCVARLink->GetFloat();
		}
		else if (m_flOldVarVal != m_pCVARLink->GetFloat())
		{
			float percent = ((fabs(m_pCVARLink->GetFloat()) - m_flMin) * (100.0f / (m_flMax - m_flMin)));
			m_pSlider->SetValue(((int)percent));
			m_iOldSliderVal = m_pSlider->GetValue();
			m_flOldVarVal = m_pCVARLink->GetFloat();
			UpdateNobPosition();
			engine->ClientCmd_Unrestricted("host_writeconfig\n");
		}

		switch (m_iRawValue)
		{
		case RawValueType::TYPE_FLOAT:
			m_pValue->SetText(VarArgs("%.3f", m_pCVARLink->GetFloat()));
			break;
		case RawValueType::TYPE_FLOAT_SMALL:
			m_pValue->SetText(VarArgs("%.1f", m_pCVARLink->GetFloat()));
			break;
		case RawValueType::TYPE_INT:
			m_pValue->SetText(VarArgs("%i", m_pCVARLink->GetInt()));
			break;
		default: // Percent fallback.
			m_pValue->SetText(VarArgs("%i", m_pSlider->GetValue()));
			break;
		}
	}
}

void GraphicalOverlay::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

void GraphicalOverlay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	SetFgColor(Color(0, 0, 0, 0));

	m_pFG->SetFgColor(pScheme->GetColor("CustomSliderValueSlideFgCol", Color(255, 0, 0, 255)));
	m_pFG->SetBgColor(pScheme->GetColor("CustomSliderValueSlideBgCol", Color(255, 0, 0, 75)));
	m_pFG->SetAlpha(75);

	m_pInfo->SetFgColor(pScheme->GetColor("CustomSliderTextColor", Color(255, 255, 255, 255)));
	m_pInfo->SetFont(pScheme->GetFont("OptionTextMedium"));
	m_pSlider->SetFgColor(Color(0, 0, 0, 0));
	m_pSlider->SetBgColor(Color(0, 0, 0, 0));

	m_pValue->SetFgColor(pScheme->GetColor("CustomSliderTextColor", Color(255, 255, 255, 255)));
	m_pValue->SetFont(pScheme->GetFont("OptionTextSmall"));
}

void GraphicalOverlay::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);

	m_pFG->SetEnabled(state);
	m_pInfo->SetEnabled(state);
	m_pSlider->SetEnabled(state);
	m_pValue->SetEnabled(state);
	m_pImgCircle->SetEnabled(state);
}
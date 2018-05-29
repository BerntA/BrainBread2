//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Draw multiple labels in one control, properly positioning and so forth. Allowing multiple colors.
//
//========================================================================================//

#include "cbase.h"
#include "MultiLabel.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MultiLabel::MultiLabel(vgui::Panel *parent, char const *panelName, const char *fontName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);
	SetScheme("BaseScheme");
	InvalidateLayout();
	PerformLayout();

	Q_strncpy(szFont, fontName, 32);
}

MultiLabel::~MultiLabel()
{
	DeleteAll();
}

void MultiLabel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	//BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont(szFont, false); // IsProportional()

	SetBgColor(Color(255, 255, 255, 0));
	SetFgColor(Color(255, 255, 255, 255));

	for (int i = 0; i < pszTextSplits.Count(); i++)
	{
		pszTextSplits[i].m_pLabel->SetFont(m_hFont);
		pszTextSplits[i].m_pLabel->SetFgColor(pszTextSplits[i].fgColor);
	}
}

void MultiLabel::PerformLayout()
{
	BaseClass::PerformLayout();
}

// Remove all labels / splits...
void MultiLabel::DeleteAll()
{
	for (int i = (pszTextSplits.Count() - 1); i >= 0; i--)
		delete pszTextSplits[i].m_pLabel;

	pszTextSplits.Purge();
}

// We can split as many as we wish...
void MultiLabel::SetTextColorSegmented(const char *szText[], Color textColor[], int iSize)
{
	int arraySize = iSize;

	// Never add duplicates.
	for (int i = 0; i < pszTextSplits.Count(); i++)
	{
		char szLabel[80];
		pszTextSplits[i].m_pLabel->GetText(szLabel, 80);

		for (int items = 0; items <= arraySize; items++)
		{
			if (!strcmp(szLabel, szText[items]))
				return;
		}
	}

	int x_pos = 0;
	for (int items = 0; items <= arraySize; items++)
	{
		MultiLabel_Item_t labelItem;

		vgui::Label *m_pLabel = vgui::SETUP_PANEL(new vgui::Label(this, "Label", szText[items]));
		m_pLabel->SetVisible(true);
		m_pLabel->SetText(szText[items]);

		// If we use localization we need to use the localized text, not the actual token... (for size computing)
		char pchText[256];
		m_pLabel->GetText(pchText, 256);

		int iLen = UTIL_ComputeStringWidth(m_hFont, pchText);

		m_pLabel->SetSize(scheme()->GetProportionalScaledValue(iLen), scheme()->GetProportionalScaledValue(20));
		m_pLabel->SetPos(x_pos, 0);

		labelItem.m_pLabel = m_pLabel;
		labelItem.fgColor = textColor[items];

		pszTextSplits.AddToTail(labelItem);

		x_pos += (iLen + scheme()->GetProportionalScaledValue(2));
	}
}

// Modify text...
void MultiLabel::UpdateText(const char *szText[])
{
	int x_pos = 0;
	for (int i = 0; i < pszTextSplits.Count(); i++)
	{
		pszTextSplits[i].m_pLabel->SetText(szText[i]);

		int iLen = UTIL_ComputeStringWidth(m_hFont, szText[i]);

		pszTextSplits[i].m_pLabel->SetSize(scheme()->GetProportionalScaledValue(iLen), scheme()->GetProportionalScaledValue(20));
		pszTextSplits[i].m_pLabel->SetPos(x_pos, 0);

		x_pos += (iLen + scheme()->GetProportionalScaledValue(2));
	}
}
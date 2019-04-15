//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Detail Panel - Draws the information for a quest in the quest list, such as objectives, finished objectives, etc...
//
//========================================================================================//

#include "cbase.h"
#include "QuestDetailPanel.h"
#include <vgui/IInput.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

QuestDetailPanel::QuestDetailPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);
	SetScheme("BaseScheme");

	m_pDividerTitle = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Divider"));
	m_pDividerTitle->SetShouldScaleImage(true);
	m_pDividerTitle->SetZPos(10);
	m_pDividerTitle->SetImage("base/quest/sub_divider");

	m_pDividerObjectives = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Divider"));
	m_pDividerObjectives->SetShouldScaleImage(true);
	m_pDividerObjectives->SetZPos(10);
	m_pDividerObjectives->SetImage("base/quest/sub_divider");

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "Title", ""));
	m_pLabelTitle->SetZPos(20);

	m_pTextDescription = vgui::SETUP_PANEL(new vgui::RichText(this, "Description"));
	m_pTextDescription->SetZPos(20);

	m_pLabelObjectives = vgui::SETUP_PANEL(new vgui::Label(this, "Objectives", ""));
	m_pLabelObjectives->SetZPos(20);
	m_pLabelObjectives->SetText("Objectives");

	InvalidateLayout();
	PerformLayout();
}

QuestDetailPanel::~QuestDetailPanel()
{
	Cleanup();
}

void QuestDetailPanel::Cleanup(void)
{
	for (int i = (pszObjectives.Count() - 1); i >= 0; i--)
	{
		if (pszObjectives[i])
			delete pszObjectives[i];
	}

	pszObjectives.Purge();
}

void QuestDetailPanel::SetupLayout(void)
{
	int wide, tall;
	GetSize(wide, tall);

	m_pLabelTitle->SetSize(wide, scheme()->GetProportionalScaledValue(15));
	m_pLabelTitle->SetPos(0, 0);

	m_pLabelObjectives->SetSize(wide, scheme()->GetProportionalScaledValue(15));
	m_pLabelObjectives->SetPos(0, tall - scheme()->GetProportionalScaledValue(120));

	m_pTextDescription->SetSize(wide, scheme()->GetProportionalScaledValue(100));
	m_pTextDescription->SetPos(0, scheme()->GetProportionalScaledValue(18));

	m_pDividerTitle->SetSize(wide / 2, scheme()->GetProportionalScaledValue(3));
	m_pDividerTitle->SetPos(0, scheme()->GetProportionalScaledValue(14));

	m_pDividerObjectives->SetSize(wide / 2, scheme()->GetProportionalScaledValue(3));
	m_pDividerObjectives->SetPos(0, tall - scheme()->GetProportionalScaledValue(107));

	int startY = tall - scheme()->GetProportionalScaledValue(100);

	for (int i = 0; i < pszObjectives.Count(); i++)
	{
		pszObjectives[i]->SetSize(wide, scheme()->GetProportionalScaledValue(10));
		pszObjectives[i]->SetPos(0, startY + (i * scheme()->GetProportionalScaledValue(12)));
	}
}

void QuestDetailPanel::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);
	SetupLayout();
}

void QuestDetailPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	SetupLayout();
}

void QuestDetailPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	m_pLabelTitle->SetFont(pScheme->GetFont("DefaultLargeBold"));
	m_pLabelTitle->SetFgColor(pScheme->GetColor("QuestPanelTitleTextColor", Color(0, 0, 0, 255)));

	m_pTextDescription->SetFgColor(pScheme->GetColor("QuestPanelDescriptionTextColor", Color(0, 0, 0, 255)));
	m_pTextDescription->SetBgColor(Color(0, 0, 0, 0));
	m_pTextDescription->SetBorder(NULL);	
	m_pTextDescription->SetFont(pScheme->GetFont("DefaultBold"));

	m_pLabelObjectives->SetFont(pScheme->GetFont("DefaultBold"));
	m_pLabelObjectives->SetFgColor(pScheme->GetColor("QuestPanelObjectivesTextColor", Color(0, 0, 0, 255)));	
}

bool QuestDetailPanel::SelectID(int index)
{
	const CQuestItem *pQuestItem = GameBaseShared()->GetSharedQuestData()->GetQuestDataForIndex(index);
	if (!pQuestItem || !pQuestItem->bIsActive)
		return false;

	Cleanup();

	m_pLabelTitle->SetText(pQuestItem->szTitle);
	m_pTextDescription->SetText(pQuestItem->szDescription);
	m_pTextDescription->GotoTextStart();

	for (int i = 0; i < pQuestItem->iObjectivesCount; i++)
	{
		char szOutput[256];
		wchar_t wszArg[32];
		wchar_t wszObjective[256];
		wchar_t sIDString[256];
		sIDString[0] = 0;

		_snwprintf(wszArg, ARRAYSIZE(wszArg) - 1, L"%i / %i", pQuestItem->pObjectives[i].iCurrentKills, pQuestItem->pObjectives[i].iKillsNeeded);
		wszArg[ARRAYSIZE(wszArg) - 1] = '\0';

		const char *objectiveString = pQuestItem->pObjectives[i].szObjective;
		wchar_t *wszToken = g_pVGuiLocalize->Find(objectiveString);

		if (wszToken != NULL)
			g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), wszToken, 1, wszArg);
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(objectiveString, wszObjective, sizeof(wszObjective));
			g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), wszObjective, 1, wszArg);
		}

		g_pVGuiLocalize->ConvertUnicodeToANSI(sIDString, szOutput, 256);

		bool bShouldShow = true;

		// If we have a specified quest we draw the objectives in a different order:
		if (i > 0 && pQuestItem->bShowInOrder && (!pQuestItem->pObjectives[(i - 1)].bObjectiveCompleted))
			bShouldShow = false;

		if (bShouldShow)
		{
			GraphicalCheckBox *pCheckBox = vgui::SETUP_PANEL(new vgui::GraphicalCheckBox(this, "CheckBox", szOutput, "DefaultBold", true));
			pCheckBox->SetCheckedStatus(pQuestItem->pObjectives[i].bObjectiveCompleted);
			pCheckBox->SetZPos(50);
			pszObjectives.AddToTail(pCheckBox);
		}
	}

	SetupLayout();
	return true;
}
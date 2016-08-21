//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Quest Detail Panel - Draws the information for a quest in the quest list, such as objectives, finished objectives, etc...
// We also draw the quests available in a list and side quests.
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
#include <vgui_controls/Frame.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/Controls.h"
#include "QuestDetailPanel.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/AnimationController.h"
#include "KeyValues.h"
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

	m_pImagePreview = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Preview"));
	m_pImagePreview->SetShouldScaleImage(true);
	m_pImagePreview->SetImage("transparency");
	m_pImagePreview->SetZPos(5);
	m_pImagePreview->SetVisible(false);

	for (int i = 0; i < _ARRAYSIZE(m_pImageDivider); i++)
	{
		m_pImageDivider[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Divider"));
		m_pImageDivider[i]->SetShouldScaleImage(true);
		m_pImageDivider[i]->SetZPos(10);
		m_pImageDivider[i]->SetImage("base/quest/sub_divider");

		m_pQuestDivider[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "QuestDivider"));
		m_pQuestDivider[i]->SetShouldScaleImage(true);
		m_pQuestDivider[i]->SetZPos(10);
		m_pQuestDivider[i]->SetImage("base/quest/header_divider");

		m_pQuestListLabel[i] = vgui::SETUP_PANEL(new vgui::Label(this, "QuestLabel", ""));
		m_pQuestListLabel[i]->SetZPos(12);
	}

	m_pQuestLists[0] = vgui::SETUP_PANEL(new vgui::QuestList(this, "QuestList", false));
	m_pQuestLists[1] = vgui::SETUP_PANEL(new vgui::QuestList(this, "QuestList", true));
	for (int i = 0; i < _ARRAYSIZE(m_pQuestLists); i++)
	{
		m_pQuestLists[i]->SetZPos(30);
		m_pQuestLists[i]->AddActionSignalTarget(this);
	}

	m_pDivider = vgui::SETUP_PANEL(new vgui::Divider(this, "MainDivider"));
	m_pDivider->SetZPos(10);

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "Title", ""));
	m_pLabelTitle->SetZPos(20);

	m_pLabelHeader = vgui::SETUP_PANEL(new vgui::Label(this, "Header", ""));
	m_pLabelHeader->SetZPos(20);

	m_pTextDescription = vgui::SETUP_PANEL(new vgui::RichText(this, "Description"));
	m_pTextDescription->SetZPos(20);

	m_pLabelObjectives = vgui::SETUP_PANEL(new vgui::Label(this, "Objectives", ""));
	m_pLabelObjectives->SetZPos(20);
	m_pLabelObjectives->SetText("Objectives");

	m_pQuestListLabel[0]->SetText("Main Quests");
	m_pQuestListLabel[1]->SetText("Side Quests");

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
	m_pImagePreview->SetVisible(false);

	int w, h;
	GetSize(w, h);
	SetSize(w, h); // update size.

	m_pQuestLists[0]->CreateList();
	m_pQuestLists[1]->CreateList();

	ChooseQuestItem(m_pQuestLists[0]->GetFirstItem());
}

void QuestDetailPanel::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	bool bImageLayout = m_pImagePreview->IsVisible();
	int iHeight = (tall / 2) - scheme()->GetProportionalScaledValue(46);
	int w, h, w2, h2, x, y, z, g;
	m_pDivider->SetSize(scheme()->GetProportionalScaledValue(4), tall);
	m_pDivider->SetPos(scheme()->GetProportionalScaledValue(115), 0);
	m_pDivider->GetSize(x, y);
	x += scheme()->GetProportionalScaledValue(115);

	m_pImagePreview->SetSize(wide - x, scheme()->GetProportionalScaledValue(128));
	m_pImagePreview->SetPos(x, 0);

	m_pQuestDivider[0]->SetPos(scheme()->GetProportionalScaledValue(17), scheme()->GetProportionalScaledValue(10));
	m_pQuestDivider[0]->SetSize(scheme()->GetProportionalScaledValue(80), scheme()->GetProportionalScaledValue(13));

	m_pQuestListLabel[0]->SetPos(scheme()->GetProportionalScaledValue(18), scheme()->GetProportionalScaledValue(10));
	m_pQuestListLabel[0]->SetSize(scheme()->GetProportionalScaledValue(80), scheme()->GetProportionalScaledValue(13));

	m_pQuestLists[0]->SetPos(scheme()->GetProportionalScaledValue(17), scheme()->GetProportionalScaledValue(23));
	m_pQuestLists[0]->SetSize(scheme()->GetProportionalScaledValue(80), iHeight);
	m_pQuestLists[0]->GetPos(z, g);

	m_pQuestDivider[1]->SetPos(scheme()->GetProportionalScaledValue(17), g + iHeight + scheme()->GetProportionalScaledValue(31));
	m_pQuestDivider[1]->SetSize(scheme()->GetProportionalScaledValue(80), scheme()->GetProportionalScaledValue(13));

	m_pQuestListLabel[1]->SetPos(scheme()->GetProportionalScaledValue(18), g + iHeight + scheme()->GetProportionalScaledValue(31));
	m_pQuestListLabel[1]->SetSize(scheme()->GetProportionalScaledValue(80), scheme()->GetProportionalScaledValue(13));

	m_pQuestLists[1]->SetPos(scheme()->GetProportionalScaledValue(17), g + iHeight + scheme()->GetProportionalScaledValue(44));
	m_pQuestLists[1]->SetSize(scheme()->GetProportionalScaledValue(80), iHeight);

	m_pImagePreview->GetSize(w, h);

	m_pLabelTitle->SetSize(wide - x, scheme()->GetProportionalScaledValue(20));
	m_pLabelTitle->SetPos(x + scheme()->GetProportionalScaledValue(2), (h - scheme()->GetProportionalScaledValue(17)));
	m_pLabelTitle->SetVisible(bImageLayout);

	m_pImageDivider[0]->SetSize((wide / 2) - x, scheme()->GetProportionalScaledValue(4));
	m_pImageDivider[0]->SetPos(x, bImageLayout ? (h + scheme()->GetProportionalScaledValue(19)) : scheme()->GetProportionalScaledValue(19));

	m_pLabelHeader->SetPos(x + scheme()->GetProportionalScaledValue(2), bImageLayout ? (h + scheme()->GetProportionalScaledValue(1)) : 0);
	m_pLabelHeader->SetSize(wide - x, scheme()->GetProportionalScaledValue(20));

	m_pTextDescription->SetPos(x, bImageLayout ? (h + scheme()->GetProportionalScaledValue(24)) : scheme()->GetProportionalScaledValue(24));
	m_pTextDescription->SetSize(wide - x, scheme()->GetProportionalScaledValue(100));
	m_pTextDescription->GetSize(w2, h2);
	m_pTextDescription->GetPos(z, g);

	m_pLabelObjectives->SetPos(x + scheme()->GetProportionalScaledValue(2), ((h2 + g) + scheme()->GetProportionalScaledValue(3)));
	m_pLabelObjectives->SetSize(wide - x, scheme()->GetProportionalScaledValue(20));
	m_pLabelObjectives->GetPos(z, g);

	m_pImageDivider[1]->SetSize((wide / 2) - x, scheme()->GetProportionalScaledValue(4));
	m_pImageDivider[1]->SetPos(x, (g + scheme()->GetProportionalScaledValue(18)));
}

void QuestDetailPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void QuestDetailPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pDivider->SetFgColor(pScheme->GetColor("QuestPanelDividerColor", Color(25, 25, 25, 255)));
	m_pDivider->SetBgColor(pScheme->GetColor("QuestPanelDividerColor", Color(25, 25, 25, 255)));
	m_pDivider->SetBorder(NULL);

	for (int i = 0; i < _ARRAYSIZE(m_pQuestListLabel); i++)
	{
		m_pQuestListLabel[i]->SetFgColor(pScheme->GetColor("QuestPanelTextColor", Color(255, 255, 255, 255)));
		m_pQuestListLabel[i]->SetFont(pScheme->GetFont("DefaultLargeBold"));
	}

	m_pLabelTitle->SetFgColor(pScheme->GetColor("QuestPanelTitleTextColor", Color(255, 255, 255, 255)));
	m_pLabelHeader->SetFgColor(pScheme->GetColor("QuestPanelHeaderTextColor", Color(255, 255, 255, 255)));
	m_pTextDescription->SetFgColor(pScheme->GetColor("QuestPanelDescriptionTextColor", Color(255, 255, 255, 255)));
	m_pTextDescription->SetBgColor(Color(255, 255, 255, 0));
	m_pTextDescription->SetBorder(NULL);
	m_pLabelObjectives->SetFgColor(pScheme->GetColor("QuestPanelObjectivesTextColor", Color(255, 255, 255, 255)));

	m_pLabelTitle->SetFont(pScheme->GetFont("DefaultLargeBold"));
	m_pLabelHeader->SetFont(pScheme->GetFont("DefaultMediumBold"));
	m_pTextDescription->SetFont(pScheme->GetFont("DefaultBold"));
	m_pLabelObjectives->SetFont(pScheme->GetFont("DefaultBold"));
}

void QuestDetailPanel::ChooseQuestItem(int index)
{
	CQuestItem *pQuestItem = GameBaseShared()->GetSharedQuestData()->GetQuestDataForIndex(index);
	if (!pQuestItem)
		return;

	if (!pQuestItem->bIsActive)
		return;

	Cleanup();

	int w, h, x, y;

	GetSize(w, h);

	m_pQuestLists[1]->SetActiveIndex(index);
	m_pQuestLists[0]->SetActiveIndex(index);

	char szFullPathToIMG[MAX_WEAPON_STRING];
	Q_snprintf(szFullPathToIMG, MAX_WEAPON_STRING, "materials/vgui/%s.vmt", pQuestItem->szImage);
	m_pImagePreview->SetVisible(filesystem->FileExists(szFullPathToIMG, "MOD"));
	if (m_pImagePreview->IsVisible())
		m_pImagePreview->SetImage(pQuestItem->szImage);

	SetSize(w, h); // update size.

	m_pImageDivider[1]->GetPos(x, y);
	m_pImageDivider[1]->GetSize(w, h);

	m_pLabelTitle->SetText(pQuestItem->szTitle);
	m_pLabelHeader->SetText(pQuestItem->szHeader);
	m_pTextDescription->SetText(pQuestItem->szDescription);
	m_pTextDescription->GotoTextStart();

	for (int i = 0; i < pQuestItem->m_iObjectivesCount; i++)
	{
		char szOutput[256];
		wchar_t wszArg[32];
		wchar_t wszObjective[256];
		wchar_t sIDString[256];
		sIDString[0] = 0;

		_snwprintf(wszArg, ARRAYSIZE(wszArg) - 1, L"%i / %i", pQuestItem->pObjectives[i].m_iCurrentKills, pQuestItem->pObjectives[i].m_iKillsNeeded);
		wszArg[ARRAYSIZE(wszArg) - 1] = '\0';

		const char *objectiveString = pQuestItem->pObjectives[i].szObjective;
		wchar_t *wszToken = g_pVGuiLocalize->Find(objectiveString);

		if (wszToken != NULL)
		{
			g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), wszToken, 1, wszArg);
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(objectiveString, wszObjective, sizeof(wszObjective));
			g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), wszObjective, 1, wszArg);
		}

		g_pVGuiLocalize->ConvertUnicodeToANSI(sIDString, szOutput, 256);

		bool bShouldShow = true;

		// If we have a specified quest we draw the objectives in a different order:
		if (i > 0 && pQuestItem->bShowInOrder && (!pQuestItem->pObjectives[(i - 1)].m_bObjectiveCompleted))
			bShouldShow = false;

		if (bShouldShow)
		{
			GraphicalCheckBox *pCheckBox = new vgui::GraphicalCheckBox(this, "CheckBox", szOutput, "DefaultBold", true);
			pCheckBox->SetSize(w, scheme()->GetProportionalScaledValue(10));
			pCheckBox->SetPos(x + scheme()->GetProportionalScaledValue(4), (y + scheme()->GetProportionalScaledValue(5)) + (i * scheme()->GetProportionalScaledValue(13)));
			pCheckBox->SetCheckedStatus(pQuestItem->pObjectives[i].m_bObjectiveCompleted);
			pCheckBox->SetZPos(50);
			pszObjectives.AddToTail(pCheckBox);
		}
	}
}
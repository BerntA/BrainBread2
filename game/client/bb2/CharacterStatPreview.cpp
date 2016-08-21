//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays your personal stats in a vgui control.
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
#include "CharacterStatPreview.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "vgui/ILocalize.h"
#include "inputsystem/iinputsystem.h"
#include "KeyValues.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CharacterStatPreview::CharacterStatPreview(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	m_iActivePlayerIndex = 0;

	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pLabelHeader = new Label(this, "Header", "");
	m_pLabelHeader->SetZPos(20);

	m_pImageDivider = new ImagePanel(this, "HeaderDivider");
	m_pImageDivider->SetShouldScaleImage(true);
	m_pImageDivider->SetZPos(15);
	m_pImageDivider->SetImage("base/quest/sub_divider");

	m_pLabelHeader->SetText("#VGUI_CharacterSkillMenu_Header");

	for (int i = 0; i < _ARRAYSIZE(m_pTextDetails); i++)
	{
		m_pTextDetails[i] = new MultiLabel(this, "MultiText", "DefaultBold");
		m_pTextDetails[i]->SetZPos(30);
	}

	InvalidateLayout();

	PerformLayout();
}

CharacterStatPreview::~CharacterStatPreview()
{
}

void CharacterStatPreview::SetVisible(bool state)
{
	BaseClass::SetVisible(state);
}

void CharacterStatPreview::ShowStatsForPlayer(int index)
{
	C_HL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(index));
	if (!pPlayer || !g_PR)
		return;

	m_iActivePlayerIndex = index;

	for (int i = 0; i < _ARRAYSIZE(m_pTextDetails); i++)
		m_pTextDetails[i]->DeleteAll();

	char szValue[80];

	Q_snprintf(szValue, 80, "%i", g_PR->GetLevel(index));
	const char *szLevel[] = { "#VGUI_CharacterSkillMenu_Level", szValue, };
	Color colLevel[] = { Color(255, 255, 255, 255), Color(0, 255, 0, 255), };
	m_pTextDetails[0]->SetTextColorSegmented(szLevel, colLevel, 1);
	m_pTextDetails[0]->SetVisible(HL2MPRules()->CanUseSkills());

	Q_snprintf(szValue, 80, "%i", g_PR->GetTotalScore(index));
	const char *szTotalScore[] = { "#VGUI_CharacterSkillMenu_TotalScore", szValue, };
	Color colTotalScore[] = { Color(255, 255, 255, 255), Color(0, 255, 0, 255), };
	m_pTextDetails[1]->SetTextColorSegmented(szTotalScore, colTotalScore, 1);

	Q_snprintf(szValue, 80, "%i", g_PR->GetRoundScore(index));
	const char *szRoundScore[] = { "#VGUI_CharacterSkillMenu_RoundScore", szValue, };
	Color colRoundScore[] = { Color(255, 255, 255, 255), Color(0, 255, 0, 255), };
	m_pTextDetails[2]->SetTextColorSegmented(szRoundScore, colRoundScore, 1);

	Q_snprintf(szValue, 80, "%i", g_PR->GetTotalDeaths(index));
	const char *szDeaths[] = { "#VGUI_CharacterSkillMenu_Deaths", szValue, };
	Color colDeaths[] = { Color(255, 255, 255, 255), Color(255, 40, 40, 255), };
	m_pTextDetails[3]->SetTextColorSegmented(szDeaths, colDeaths, 1);
}

void CharacterStatPreview::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	int wM, hM;
	float scaleOfSize = (((float)wide) * 0.5f);

	wM = scaleOfSize;
	hM = tall;

	wM += scheme()->GetProportionalScaledValue(1);

	m_pLabelHeader->SetPos(wM, 0);
	m_pLabelHeader->SetSize(scaleOfSize, scheme()->GetProportionalScaledValue(20));
	m_pImageDivider->SetPos(wM, scheme()->GetProportionalScaledValue(18));
	m_pImageDivider->SetSize((scaleOfSize / 2.0f), scheme()->GetProportionalScaledValue(3));

	int x1, y1;

	m_pImageDivider->GetPos(x1, y1);
	y1 += scheme()->GetProportionalScaledValue(4);
	int yOffset = 0;
	for (int i = 0; i < _ARRAYSIZE(m_pTextDetails); i++)
	{
		if (!m_pTextDetails[i]->IsVisible())
			continue;

		m_pTextDetails[i]->SetPos(x1, y1 + (yOffset * scheme()->GetProportionalScaledValue(14)));
		m_pTextDetails[i]->SetSize(scaleOfSize, scheme()->GetProportionalScaledValue(14));
		yOffset++;
	}
}

void CharacterStatPreview::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CharacterStatPreview::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabelHeader->SetFgColor(Color(255, 255, 255, 255));
	m_pLabelHeader->SetFont(pScheme->GetFont("DefaultLargeBold"));
}

void CharacterStatPreview::OnThink()
{
	BaseClass::OnThink();
}
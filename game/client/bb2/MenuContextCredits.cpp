//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Credits Preview
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
#include "MenuContextCredits.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "GameBase_Shared.h"
#include "fmod_manager.h"
#include "KeyValues.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MenuContextCredits::MenuContextCredits(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	m_DefaultFont = NULL;
	SetScheme("BaseScheme");

	for (int i = 0; i < _ARRAYSIZE(m_pLabelMessage); i++)
	{
		m_pLabelMessage[i] = vgui::SETUP_PANEL(new vgui::Label(this, "", ""));
		m_pLabelMessage[i]->SetZPos(210);
		m_pLabelMessage[i]->SetText("");
	}

	pchDeveloperList[0] = 0;
	pchContributorList[0] = 0;
	pchTesterList[0] = 0;
	pchSpecialThanksList[0] = 0;

	GameBaseShared()->GetFileContent("data/settings/credits_developers.txt", pchDeveloperList, 2048);
	GameBaseShared()->GetFileContent("data/settings/credits_contributors.txt", pchContributorList, 2048);
	GameBaseShared()->GetFileContent("data/settings/credits_testers.txt", pchTesterList, 2048);
	GameBaseShared()->GetFileContent("data/settings/credits_specialthanks.txt", pchSpecialThanksList, 2048);

	InvalidateLayout();
	PerformLayout();
}

MenuContextCredits::~MenuContextCredits()
{
}

int MenuContextCredits::GetNumberOfLinesInString(const char *pszStr)
{
	int stringLines = 0;
	int strSize = strlen(pszStr);
	for (int i = 0; i < strSize; i++)
	{
		if (pszStr[i] == '\n')
			stringLines++;
	}

	return stringLines;
}

void MenuContextCredits::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h;
	GetSize(w, h);

	if (!m_DefaultFont)
		return;

	int iSizeOffset = 12;
	int iLabelHeight = (surface()->GetFontTall(m_DefaultFont) * GetNumberOfLinesInString(pchDeveloperList)) + scheme()->GetProportionalScaledValue(iSizeOffset);
	m_pLabelMessage[0]->SetSize(w, iLabelHeight);
	m_pLabelMessage[0]->SetText(pchDeveloperList);
	m_iSizeH[0] = iLabelHeight;

	iLabelHeight = (surface()->GetFontTall(m_DefaultFont) * GetNumberOfLinesInString(pchContributorList)) + scheme()->GetProportionalScaledValue(iSizeOffset);
	m_pLabelMessage[1]->SetSize(w, iLabelHeight);
	m_pLabelMessage[1]->SetText(pchContributorList);
	m_iSizeH[1] = iLabelHeight;

	iLabelHeight = (surface()->GetFontTall(m_DefaultFont) * GetNumberOfLinesInString(pchTesterList)) + scheme()->GetProportionalScaledValue(iSizeOffset);
	m_pLabelMessage[2]->SetSize(w, iLabelHeight);
	m_pLabelMessage[2]->SetText(pchTesterList);
	m_iSizeH[2] = iLabelHeight;

	iLabelHeight = (surface()->GetFontTall(m_DefaultFont) * GetNumberOfLinesInString(pchSpecialThanksList)) + scheme()->GetProportionalScaledValue(iSizeOffset);
	m_pLabelMessage[3]->SetSize(w, iLabelHeight);
	m_pLabelMessage[3]->SetText(pchSpecialThanksList);
	m_iSizeH[3] = iLabelHeight;

	for (int i = 0; i < _ARRAYSIZE(m_pLabelMessage); i++)
		m_pLabelMessage[i]->SetContentAlignment(vgui::Label::a_center);
}

void MenuContextCredits::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		if (m_flPosY >= 1)
		{
			m_flPosY = 0.0f;
			GetAnimationController()->RunAnimationCommand(this, "LabelPosition", 1.0f, GetFadeTime(), 60.0f, AnimationController::INTERPOLATOR_LINEAR);
			GameBaseClient->RunCommand(COMMAND_RETURN);
		}

		int iPosYMax = -(ScreenHeight() + m_iSizeH[0] + m_iSizeH[1] + m_iSizeH[2] + m_iSizeH[3]);

		int iPosOffset[] =
		{
			0,
			m_iSizeH[0],
			(m_iSizeH[0] + m_iSizeH[1]),
			(m_iSizeH[0] + m_iSizeH[1] + m_iSizeH[2]),
		};

		for (int i = 0; i < _ARRAYSIZE(m_pLabelMessage); i++)
			m_pLabelMessage[i]->SetPos(0, (ScreenHeight() + iPosOffset[i]) + (m_flPosY * iPosYMax));
	}
}

void MenuContextCredits::PerformLayout()
{
	BaseClass::PerformLayout();
	m_flPosY = 0.0f;
}

void MenuContextCredits::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	if (bShow)
		m_flPosY = 0.0f;

	if (!engine->IsInGame())
	{
		FMODManager()->SetSoundVolume(1.0f);

		if (bShow)
			FMODManager()->TransitionAmbientSound("ui/credits_theme.mp3");
		else
			FMODManager()->TransitionAmbientSound("ui/mainmenu_theme.mp3");
	}

	GetAnimationController()->RunAnimationCommand(this, "LabelPosition", 1.0f, GetFadeTime(), m_flCreditsScrollTime, AnimationController::INTERPOLATOR_LINEAR);
}

void MenuContextCredits::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Get the time we'll use to move the credits list from down to up.
	m_flCreditsScrollTime = atof(pScheme->GetResourceString("CreditsScrollTime"));
	if (!m_flCreditsScrollTime)
		m_flCreditsScrollTime = 40.0f;

	m_DefaultFont = pScheme->GetFont("OptionTextMedium");

	for (int i = 0; i < _ARRAYSIZE(m_pLabelMessage); i++)
	{
		m_pLabelMessage[i]->SetFgColor(pScheme->GetColor("CreditsLabelColor", Color(255, 255, 255, 255)));
		m_pLabelMessage[i]->SetFont(m_DefaultFont);
	}
}
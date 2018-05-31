//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles displaying available maps to play, showing info about them and so forth.
//
//========================================================================================//

#include "cbase.h"
#include "PlayMenuCreateGame.h"
#include <vgui/IInput.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Client.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define LOAD_MAP_WAIT_TIME 0.25f

PlayMenuCreateGame::PlayMenuCreateGame(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	for (int i = 0; i < ARRAYSIZE(m_pDividers); i++)
	{
		m_pDividers[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgDivider"));
		m_pDividers[i]->SetShouldScaleImage(true);
		m_pDividers[i]->SetZPos(300);
		m_pDividers[i]->SetImage("mainmenu/sub_divider");
	}

	for (int i = 0; i < ARRAYSIZE(m_pTitles); i++)
	{
		m_pTitles[i] = vgui::SETUP_PANEL(new vgui::Label(this, "", ""));
		m_pTitles[i]->SetContentAlignment(Label::Alignment::a_center);
		m_pTitles[i]->SetZPos(305);
	}

	m_pTitles[1]->SetText("#GameUI_CreateGame_ServerInfo");

	m_pGamemodeToolTip = new vgui::TextTooltip(this, "");
	m_pGamemodeToolTip->SetTooltipDelay(200);
	m_pGamemodeToolTip->SetEnabled(true);
	m_pGamemodeToolTip->SetTooltipFormatToMultiLine();

	m_pMapDescription = vgui::SETUP_PANEL(new vgui::RichText(this, "TextMapDesc"));
	m_pMapDescription->SetZPos(310);
	m_pMapDescription->SetText("");
	m_pMapDescription->SetScrollbarAlpha(0);

	m_pExtraInfo = vgui::SETUP_PANEL(new vgui::Label(this, "InfoLabel", ""));
	m_pExtraInfo->SetZPos(315);
	m_pExtraInfo->SetTooltip(m_pGamemodeToolTip, "#GameUI_ServerBrowser_InfoGamemode");

	m_pMapPanel = vgui::SETUP_PANEL(new vgui::MapSelectionPanel(this, "MapPanel"));
	m_pMapPanel->SetZPos(250);
	m_pMapPanel->AddActionSignalTarget(this);

	m_pServerPanel = vgui::SETUP_PANEL(new vgui::ServerSettingsPanel(this, "ServerPanel"));
	m_pServerPanel->SetZPos(250);
	m_pServerPanel->AddActionSignalTarget(this);

	m_pPlayButton = vgui::SETUP_PANEL(new vgui::AnimatedMenuButton(this, "PlayButton", "#GameUI_CreateGame_StartGame", 0, true));
	m_pPlayButton->SetZPos(330);
	m_pPlayButton->AddActionSignalTarget(this);

	InvalidateLayout();

	PerformLayout();
}

PlayMenuCreateGame::~PlayMenuCreateGame()
{
}

void PlayMenuCreateGame::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	if (bShow)
		m_pMapPanel->Redraw();
}

void PlayMenuCreateGame::SetupLayout(void)
{
	BaseClass::SetupLayout();

	int w, h;
	GetSize(w, h);

	m_pMapPanel->SetSize(scheme()->GetProportionalScaledValue(690), scheme()->GetProportionalScaledValue(100));
	m_pMapPanel->SetPos((w / 2) - (scheme()->GetProportionalScaledValue(690) / 2), scheme()->GetProportionalScaledValue(6));

	int wz, hz;
	for (int i = 0; i < _ARRAYSIZE(m_pTitles); i++)
	{
		m_pTitles[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(24));
		m_pDividers[i]->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(2));
		m_pTitles[i]->GetSize(wz, hz);
	}

	m_pTitles[0]->SetPos(scheme()->GetProportionalScaledValue(15), scheme()->GetProportionalScaledValue(120));
	m_pTitles[1]->SetPos(w - scheme()->GetProportionalScaledValue(315), scheme()->GetProportionalScaledValue(120));

	m_pDividers[0]->SetPos(scheme()->GetProportionalScaledValue(15), scheme()->GetProportionalScaledValue(143));
	m_pDividers[1]->SetPos(w - scheme()->GetProportionalScaledValue(315), scheme()->GetProportionalScaledValue(143));

	m_pMapDescription->SetSize(scheme()->GetProportionalScaledValue(300), h - scheme()->GetProportionalScaledValue(150));
	m_pMapDescription->SetPos(scheme()->GetProportionalScaledValue(15), scheme()->GetProportionalScaledValue(148));

	m_pExtraInfo->SetSize(scheme()->GetProportionalScaledValue(300), scheme()->GetProportionalScaledValue(90));
	m_pExtraInfo->SetPos(scheme()->GetProportionalScaledValue(15), h - scheme()->GetProportionalScaledValue(92));

	m_pServerPanel->SetPos(w - scheme()->GetProportionalScaledValue(315), scheme()->GetProportionalScaledValue(148));
	m_pServerPanel->SetSize(scheme()->GetProportionalScaledValue(300), h - scheme()->GetProportionalScaledValue(150));

	m_pPlayButton->SetPos((w / 2) - (scheme()->GetProportionalScaledValue(110) / 2), h - scheme()->GetProportionalScaledValue(26));
	m_pPlayButton->SetSize(scheme()->GetProportionalScaledValue(110), scheme()->GetProportionalScaledValue(23));
}

void PlayMenuCreateGame::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		m_pMapPanel->OnUpdate();
		m_pServerPanel->OnUpdate(bInGame);

		if (m_flLoadMapTimer >= 1.0f)
		{
			m_flLoadMapTimer = 0.0f;
			GameBaseClient->RunMap(m_pMapPanel->GetSelectedMap());
		}

		m_pPlayButton->OnUpdate();
	}
}

void PlayMenuCreateGame::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	for (int i = 0; i < _ARRAYSIZE(m_pTitles); i++)
	{
		m_pTitles[i]->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
		m_pTitles[i]->SetFont(pScheme->GetFont("OptionTextLarge"));
	}

	m_pExtraInfo->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
	m_pExtraInfo->SetFont(pScheme->GetFont("OptionTextSmall"));

	m_pMapDescription->SetFgColor(pScheme->GetColor("OptionTitleTextColor", Color(255, 255, 255, 255)));
	m_pMapDescription->SetBgColor(Color(0, 0, 0, 0));
	m_pMapDescription->SetPaintBorderEnabled(false);
	m_pMapDescription->SetPaintBackgroundEnabled(false);
	m_pMapDescription->SetBorder(NULL);
	m_pMapDescription->SetUnusedScrollbarInvisible(true);
	m_pMapDescription->SetFont(pScheme->GetFont("OptionTextMedium"));
}

void PlayMenuCreateGame::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "MapSelect"))
	{
		int mapIndex = m_pMapPanel->GetSelectedMapIndex();
		if (mapIndex != -1)
		{
			gameMapItem_t *mapItem = &GameBaseShared()->GetSharedMapData()->pszGameMaps[mapIndex];
			m_pTitles[0]->SetText(mapItem->pszMapTitle);
			m_pMapDescription->SetText(mapItem->pszMapDescription);
			m_pExtraInfo->SetText(mapItem->pszMapExtraInfo);
		}
	}
	else if (!Q_stricmp(pcCommand, "Activate"))
	{
		m_pServerPanel->ApplyServerSettings();
		GetAnimationController()->RunAnimationCommand(this, "LoadMapTimer", 1.0f, 0.0f, 1.0f, AnimationController::INTERPOLATOR_LINEAR);
	}

	BaseClass::OnCommand(pcCommand);
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom Context Handler - This class handles the actual options, plays and profile panels... Like video options for example.
//
//========================================================================================//

#include "cbase.h"
#include "MainMenu.h"
#include "GameBase_Client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CMenuContextHandler::CMenuContextHandler()
{
	m_bInitialized = false;
}

CMenuContextHandler::~CMenuContextHandler()
{
	m_bInitialized = false;
}

void CMenuContextHandler::CreateMenuContext(CMainMenu *pParent, int zpos)
{
	if (!pParent || m_bInitialized)
		return;

	m_pKeyboardOptions = vgui::SETUP_PANEL(new vgui::OptionMenuKeyboard(pParent, "OptionsKeyboard"));
	m_pKeyboardOptions->SetZPos(zpos);
	m_pKeyboardOptions->ForceClose();

	m_pMouseOptions = vgui::SETUP_PANEL(new vgui::OptionMenuMouse(pParent, "OptionsMouse"));
	m_pMouseOptions->SetZPos(zpos);
	m_pMouseOptions->ForceClose();

	m_pAudioOptions = vgui::SETUP_PANEL(new vgui::OptionMenuAudio(pParent, "OptionsAudio"));
	m_pAudioOptions->SetZPos(zpos);
	m_pAudioOptions->ForceClose();

	m_pVideoOptions = vgui::SETUP_PANEL(new vgui::OptionMenuVideo(pParent, "OptionsVideo"));
	m_pVideoOptions->SetZPos(zpos);
	m_pVideoOptions->ForceClose();

	m_pGraphicOptions = vgui::SETUP_PANEL(new vgui::OptionMenuGraphics(pParent, "OptionsGraphics"));
	m_pGraphicOptions->SetZPos(zpos);
	m_pGraphicOptions->ForceClose();

	m_pPerformanceOptions = vgui::SETUP_PANEL(new vgui::OptionMenuPerformance(pParent, "OptionsPerformance"));
	m_pPerformanceOptions->SetZPos(zpos);
	m_pPerformanceOptions->ForceClose();

	m_pOtherOptions = vgui::SETUP_PANEL(new vgui::OptionMenuOther(pParent, "OptionsOther"));
	m_pOtherOptions->SetZPos(zpos);
	m_pOtherOptions->ForceClose();

	m_pCreateGameMenu = vgui::SETUP_PANEL(new vgui::PlayMenuCreateGame(pParent, "CreateGame"));
	m_pCreateGameMenu->SetZPos(zpos);
	m_pCreateGameMenu->ForceClose();

	m_pServerMenu = vgui::SETUP_PANEL(new vgui::PlayMenuServerBrowser(pParent, "ServerMenu"));
	m_pServerMenu->SetZPos(zpos);
	m_pServerMenu->ForceClose();

	m_pScoreboard = vgui::SETUP_PANEL(new vgui::PlayMenuScoreboard(pParent, "ScoreMenu"));
	m_pScoreboard->SetZPos(zpos);
	m_pScoreboard->ForceClose();

	m_pAchievementPanel = vgui::SETUP_PANEL(new vgui::ProfileMenuAchievementPanel(pParent, "AchievementPanel"));
	m_pAchievementPanel->SetZPos(zpos);
	m_pAchievementPanel->ForceClose();

	m_pCharacterPanel = vgui::SETUP_PANEL(new vgui::ProfileMenuCharacterPanel(pParent, "CharacterPanel"));
	m_pCharacterPanel->SetZPos(zpos);
	m_pCharacterPanel->ForceClose();

	m_bInitialized = true;
}

void CMenuContextHandler::SetupLayout(int w, int h)
{
	if (!m_bInitialized)
		return;

	int sizeH = scheme()->GetProportionalScaledValue(376);
	int keyboardW = (w/2);

	m_pCreateGameMenu->SetPos(0, 0);
	m_pCreateGameMenu->SetSize(w, sizeH);

	m_pServerMenu->SetPos(0, 0);
	m_pServerMenu->SetSize(w, sizeH);

	m_pScoreboard->SetPos(0, 0);
	m_pScoreboard->SetSize(w, sizeH);

	m_pAchievementPanel->SetPos(0, scheme()->GetProportionalScaledValue(4));
	m_pAchievementPanel->SetSize(w, sizeH - scheme()->GetProportionalScaledValue(8));

	m_pCharacterPanel->SetPos(0, 0);
	m_pCharacterPanel->SetSize(w, sizeH);

	m_pKeyboardOptions->SetSize(keyboardW, sizeH - scheme()->GetProportionalScaledValue(8));
	m_pMouseOptions->SetSize(w, sizeH);
	m_pAudioOptions->SetSize(w, sizeH);
	m_pVideoOptions->SetSize(w, sizeH);
	m_pGraphicOptions->SetSize(w, sizeH);
	m_pPerformanceOptions->SetSize(w, sizeH);
	m_pOtherOptions->SetSize(w, sizeH);

	m_pKeyboardOptions->SetPos((w / 2) - (keyboardW / 2), scheme()->GetProportionalScaledValue(4));
	m_pMouseOptions->SetPos(0, 0);
	m_pAudioOptions->SetPos(0, 0);
	m_pVideoOptions->SetPos(0, 0);
	m_pGraphicOptions->SetPos(0, 0);
	m_pPerformanceOptions->SetPos(0, 0);
	m_pOtherOptions->SetPos(0, 0);
}

void CMenuContextHandler::ResetOptions(void)
{
	if (!m_bInitialized)
		return;

	m_pKeyboardOptions->FillKeyboardList(true);

	m_pMouseOptions->SetupLayout();
	m_pAudioOptions->SetupLayout();
	m_pVideoOptions->SetupLayout();
	m_pGraphicOptions->SetupLayout();
	m_pPerformanceOptions->SetupLayout();
	m_pOtherOptions->SetupLayout();

	engine->ClientCmd_Unrestricted("host_writeconfig\n");
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Dialogue Wheel / Voice Wheel - Voice Commands
//
//========================================================================================//

#include "cbase.h"
#include "voice_menu.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include <filesystem.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/IInput.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IVGui.h>
#include "c_hl2mp_player.h"
#include "GameBase_Client.h"
#include "IGameUIFuncs.h"
#include <game/client/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Console Helpers:
extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

const char *pszOverlayPaths[] =
{
	"voicewheel/agree_hover",
	"voicewheel/disagree_hover",
	"voicewheel/follow_hover",
	"voicewheel/take_point_hover",
	"voicewheel/noweapon_hover",
	"voicewheel/noammo_hover",
	"voicewheel/ready_hover",
	"voicewheel/look_hover",
	"voicewheel/exit_hover",
};

CVoiceMenu::CVoiceMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_VOICEWHEEL)
{
	m_pViewPort = pViewPort;

	SetZPos(10);

	// initialize dialog
	SetTitle("", false);

	// load the new scheme early!!
	SetScheme("BaseScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetCloseButtonVisible(false);
	SetProportional(true);

	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);

	DisableAllFadeEffects();

	// Init Default 
	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pImgBackground->SetImage("voicewheel/base");
	m_pImgBackground->SetZPos(10);
	m_pImgBackground->SetShouldScaleImage(true);

	m_pImgOverlay = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Overlay"));
	m_pImgOverlay->SetImage("");
	m_pImgOverlay->SetZPos(20);
	m_pImgOverlay->SetShouldScaleImage(true);
	m_pImgOverlay->SetVisible(false);

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButton[i] = vgui::SETUP_PANEL(new vgui::Button(this, VarArgs("btn%i", (i + 1)), ""));
		m_pButton[i]->AddActionSignalTarget(this);
		m_pButton[i]->SetPaintBorderEnabled(false);
		m_pButton[i]->SetPaintEnabled(false);
		m_pButton[i]->SetZPos(30);
		//m_pButton[i]->SetArmedSound("ui/button_over.wav");
		//m_pButton[i]->SetReleasedSound("ui/buttonclick.wav");
		m_pButton[i]->SetCommand(VarArgs("%i", (i + 1)));

		m_pButton[i]->SetKeyBoardInputEnabled(true);
		m_pButton[i]->SetMouseInputEnabled(true);
	}

	m_pButton[(_ARRAYSIZE(m_pButton) - 1)]->SetZPos(40); // The exit button should be above the other ones.

	PerformLayout();

	LoadControlSettings("resource/ui/voicemenu.res");

	InvalidateLayout();
}

CVoiceMenu::~CVoiceMenu()
{
}

void CVoiceMenu::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CVoiceMenu::OnThink()
{
	C_HL2MP_Player *pPlayer = (C_HL2MP_Player *)C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return;

	MoveToCenterOfScreen();

	m_pImgBackground->SetSize(GetWide(), GetTall());
	m_pImgBackground->SetPos(0, 0);

	m_pImgOverlay->SetSize(GetWide(), GetTall());
	m_pImgOverlay->SetPos(0, 0);

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	bool bShouldBeVis = false;
	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		if (m_pButton[i]->IsWithin(x, y))
		{
			m_pImgOverlay->SetImage(pszOverlayPaths[i]);
			bShouldBeVis = true;
		}
	}

	m_pImgOverlay->SetVisible(bShouldBeVis);
}

void CVoiceMenu::OnCommand(const char *command)
{
	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		if (!Q_stricmp(command, VarArgs("%i", (i + 1))))
		{
			if (i < (_ARRAYSIZE(m_pButton) - 1))
			{
				char pszCommand[64];
				Q_snprintf(pszCommand, 64, "bb_voice_command %i\n", i);
				engine->ClientCmd(pszCommand);

				PerformLayout();
				ShowPanel(false);
				break;
			}
			else
			{
				PerformLayout();
				ShowPanel(false);
			}
		}
	}

	BaseClass::OnCommand(command);
}

void CVoiceMenu::ShowPanel(bool bShow)
{
	if (bShow && GameBaseClient->IsViewPortPanelVisible(PANEL_SCOREBOARD))
		return;

	PerformLayout();
	SetMouseInputEnabled(bShow);
	SetKeyBoardInputEnabled(bShow);

	if (bShow)
	{
		Activate();
		engine->ClientCmd_Unrestricted("gameui_preventescapetoshow\n");
	}
	else
		engine->ClientCmd_Unrestricted("gameui_allowescapetoshow\n");

	SetVisible(bShow);
}

void CVoiceMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void CVoiceMenu::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

Panel *CVoiceMenu::CreateControlByName(const char *controlName)
{
	return BaseClass::CreateControlByName(controlName);
}

void CVoiceMenu::Reset()
{
}

void CVoiceMenu::OnKeyCodeTyped(vgui::KeyCode code)
{
	if ((code == KEY_ESCAPE) || (gameuifuncs->GetButtonCodeForBind("voice_menu") == code))
	{
		PerformLayout();
		ShowPanel(false);
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}
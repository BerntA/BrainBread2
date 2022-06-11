//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Skill Icon - Draws the icon related to the skill, searches scripts/skills/*.txt for a relative file to the cc_language (translatable).
//
//========================================================================================//

#include "cbase.h"
#include "SkillTreeIcon.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IInput.h>
#include <inputsystem/iinputsystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

SkillTreeIcon::SkillTreeIcon(vgui::Panel* parent, char const* panelName, const char* name, const char* description, const char* command, const char* iconTexture) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);
	SetScheme("BaseScheme");

	m_pIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "IconImage"));
	m_pIcon->SetZPos(12);
	m_pIcon->SetImage("transparency");
	m_pIcon->SetShouldScaleImage(true);
	m_pIcon->SetImage(iconTexture);

	Q_strncpy(szName, name, 128);
	Q_strncpy(szDesc, description, 128);
	Q_strncpy(szCommand, command, 128);
	flCommandTime = 0.0f;

	SetProgressValue(0.0f);
	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);
	InvalidateLayout();
	PerformLayout();
}

SkillTreeIcon::~SkillTreeIcon()
{
}

void SkillTreeIcon::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, w, h;
	GetSize(w, h);
	GetPos(x, y);

	m_pIcon->SetSize(w - scheme()->GetProportionalScaledValue(8), h - scheme()->GetProportionalScaledValue(8));
	m_pIcon->SetPos(scheme()->GetProportionalScaledValue(4), scheme()->GetProportionalScaledValue(4));

	m_pLevelIconBG = gHUD.GetIcon("level_min");
	m_pLevelIconFG = gHUD.GetIcon("level_max");
}

void SkillTreeIcon::Paint()
{
	BaseClass::Paint();

	Color col = Color(255, 255, 255, 255);

	if (m_pLevelIconBG)
	{
		surface()->DrawSetColor(col);
		surface()->DrawSetTexture(m_pLevelIconBG->textureId);
		surface()->DrawTexturedRect(0, 0, GetWide(), GetTall());
	}

	if (m_pLevelIconFG)
		m_pLevelIconFG->DrawCircularProgression(col, 0, 0, GetWide(), GetTall(), flProgress);

	const bool bM1Down = g_pInputSystem->IsButtonDown(MOUSE_LEFT);
	const bool bM2Down = g_pInputSystem->IsButtonDown(MOUSE_RIGHT);
	const float flTime = engine->Time();

	if ((flTime >= flCommandTime) && (bM1Down || bM2Down))
	{
		surface()->PlaySound(bM1Down ? "common/wpn_hudoff.wav" : "common/wpn_moveselect.wav");
		engine->ClientCmd_Unrestricted(VarArgs("%s %i\n", szCommand, (bM1Down ? 1 : 0)));
		flCommandTime = (flTime + 0.2f);
	}
}
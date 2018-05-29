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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static CHudTexture *m_pLevelIconBG = NULL;
static CHudTexture *m_pLevelIconFG = NULL;

SkillTreeIcon::SkillTreeIcon(vgui::Panel *parent, char const *panelName, const char *name, const char *description, const char *command, const char *iconTexture) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);
	SetScheme("BaseScheme");

	// Icon
	m_pIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "IconImage"));
	m_pIcon->SetZPos(12);
	m_pIcon->SetImage("transparency");
	m_pIcon->SetShouldScaleImage(true);

	m_pMousePanel = vgui::SETUP_PANEL(new vgui::MouseInputPanel(this, "MouseInputs"));
	m_pMousePanel->SetZPos(90);
	m_pMousePanel->SetVisible(true);
	m_pMousePanel->SetEnabled(true);

	Q_strncpy(szName, name, 128);
	Q_strncpy(szDesc, description, 128);
	Q_strncpy(szCommand, command, 128);
	m_pIcon->SetImage(iconTexture);

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

	m_pMousePanel->SetSize(w, h);
	m_pMousePanel->SetPos(0, 0);

	m_pLevelIconBG = gHUD.GetIcon("level_min");
	m_pLevelIconFG = gHUD.GetIcon("level_max");
}

void SkillTreeIcon::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
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
}

void SkillTreeIcon::OnMousePressed(vgui::MouseCode code)
{
	if (!IsVisible())
	{
		BaseClass::OnMousePressed(code);
		return;
	}

	if (code == MOUSE_LEFT)
		engine->ClientCmd_Unrestricted(VarArgs("%s 1\n", szCommand));
	else if (code == MOUSE_RIGHT)
		engine->ClientCmd_Unrestricted(VarArgs("%s 0\n", szCommand));
	else
		BaseClass::OnMousePressed(code);
}
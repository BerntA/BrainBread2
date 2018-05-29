//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map Selection Item for the map selection panel.
//
//========================================================================================//

#include "cbase.h"
#include "MapSelectionItem.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/AnimationController.h>
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MapSelectionItem::MapSelectionItem(vgui::Panel *parent, char const *panelName, int mapIndex) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	m_iMapItemIndexLink = mapIndex;
	m_bSelected = false;

	m_flFadeSpeed = 1.5f;
	m_flFadeDelay = 3.0f;

	SetScheme("BaseScheme");

	m_pButton = vgui::SETUP_PANEL(new vgui::Button(this, "Button", ""));
	m_pButton->SetPaintBorderEnabled(false);
	m_pButton->SetPaintEnabled(false);
	m_pButton->SetReleasedSound("ui/button_click.wav");
	m_pButton->SetZPos(140);
	m_pButton->AddActionSignalTarget(this);
	m_pButton->SetCommand("Activate");

	m_pPreview = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Preview"));
	m_pPreview->SetShouldScaleImage(true);
	m_pPreview->SetZPos(130);

	m_pPreviewFade = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "PreviewFaded"));
	m_pPreviewFade->SetShouldScaleImage(true);
	m_pPreviewFade->SetZPos(130);

	m_pFrame = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Frame"));
	m_pFrame->SetShouldScaleImage(true);
	m_pFrame->SetZPos(135);
	m_pFrame->SetImage("mainmenu/preview_frame");

	m_iAmountOfImages = -1;
	m_iLastFaded = 0;
	m_iLastImage = 0;

	InvalidateLayout();
	PerformLayout();
}

MapSelectionItem::~MapSelectionItem()
{
}

void MapSelectionItem::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize(w, h);

	m_pPreview->SetSize(w, (h - scheme()->GetProportionalScaledValue(20)));
	m_pPreview->SetPos(0, 0);

	m_pPreviewFade->SetSize(w, (h - scheme()->GetProportionalScaledValue(20)));
	m_pPreviewFade->SetPos(0, 0);

	m_pFrame->SetSize(w, (h - scheme()->GetProportionalScaledValue(20)));
	m_pFrame->SetPos(0, 0);

	m_pButton->SetSize(w, h);
	m_pButton->SetPos(0, 0);

	SetupItem();
}

void MapSelectionItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	float flFadeSpeed = atof(pScheme->GetResourceString("MapItemFadeSpeed"));
	float flFadeDelay = atof(pScheme->GetResourceString("MapItemFadeDelay"));

	if (flFadeSpeed > 0.0f)
		m_flFadeSpeed = flFadeSpeed;

	if (flFadeDelay > 0.0f)
		m_flFadeDelay = flFadeDelay;
}

void MapSelectionItem::OnCommand(const char* pcCommand)
{
	KeyValues *pkvMsg = new KeyValues("MapItemClicked");
	pkvMsg->SetInt("index", m_iMapItemIndexLink);
	PostActionSignal(pkvMsg);
}

void MapSelectionItem::SetupItem(void)
{
	m_iAmountOfImages = -1;
	m_iLastFaded = 0;
	m_pPreviewFade->SetAlpha(0);
	m_pPreview->SetAlpha(255);

	if (GameBaseShared()->GetSharedMapData() && (m_iMapItemIndexLink >= 0) && (m_iMapItemIndexLink < GameBaseShared()->GetSharedMapData()->pszGameMaps.Count()))
	{
		gameMapItem_t *mapItem = &GameBaseShared()->GetSharedMapData()->pszGameMaps[m_iMapItemIndexLink];

		m_iAmountOfImages = mapItem->numImagePreviews;
		if (m_iAmountOfImages > 1)
		{
			m_iLastImage = random->RandomInt(0, (m_iAmountOfImages - 1));
			m_pPreview->SetImage(mapItem->GetImagePreview(m_iLastImage));
		}
		else if (m_iAmountOfImages == 1)
			m_pPreview->SetImage(mapItem->GetImagePreview(0));
		else
			m_pPreview->SetImage("steam_default_avatar");
	}
	else
		m_pPreview->SetImage("steam_default_avatar");
}

void MapSelectionItem::OnUpdate(void)
{
	if (m_iAmountOfImages > 1)
	{
		if (m_iLastFaded == 0)
		{
			SelectNewImage(false);

			GetAnimationController()->RunAnimationCommand(m_pPreviewFade, "alpha", 256.0f, m_flFadeDelay, m_flFadeSpeed, AnimationController::INTERPOLATOR_LINEAR);
			GetAnimationController()->RunAnimationCommand(m_pPreview, "alpha", 0.0f, m_flFadeDelay, m_flFadeSpeed, AnimationController::INTERPOLATOR_LINEAR);
			m_iLastFaded = 2;
		}
		else if (m_iLastFaded == 1)
		{
			SelectNewImage(true);

			GetAnimationController()->RunAnimationCommand(m_pPreview, "alpha", 256.0f, m_flFadeDelay, m_flFadeSpeed, AnimationController::INTERPOLATOR_LINEAR);
			GetAnimationController()->RunAnimationCommand(m_pPreviewFade, "alpha", 0.0f, m_flFadeDelay, m_flFadeSpeed, AnimationController::INTERPOLATOR_LINEAR);
			m_iLastFaded = 3;
		}
		else if ((m_iLastFaded == 2) && (m_pPreviewFade->GetAlpha() >= 255))
			m_iLastFaded = 1;
		else if ((m_iLastFaded == 3) && (m_pPreview->GetAlpha() >= 255))
			m_iLastFaded = 0;
	}
}

void MapSelectionItem::SelectNewImage(bool bPreview)
{
	int iNewImage = random->RandomInt(0, (m_iAmountOfImages - 1));
	while (iNewImage == m_iLastImage)
		iNewImage = random->RandomInt(0, (m_iAmountOfImages - 1));

	m_iLastImage = iNewImage;

	if (GameBaseShared()->GetSharedMapData() && (m_iMapItemIndexLink >= 0) && (m_iMapItemIndexLink < GameBaseShared()->GetSharedMapData()->pszGameMaps.Count()))
	{
		gameMapItem_t *mapItem = &GameBaseShared()->GetSharedMapData()->pszGameMaps[m_iMapItemIndexLink];
		if (bPreview)
			m_pPreview->SetImage(mapItem->GetImagePreview(m_iLastImage));
		else
			m_pPreviewFade->SetImage(mapItem->GetImagePreview(m_iLastImage));
	}
}
//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Map Selection Control - Displays a map and allows you to browse to others by hitting the 'next'/'prev' button.
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
#include "MapSelectionPanel.h"
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include <igameresources.h>
#include "cdll_util.h"
#include "GameBase_Client.h"
#include "IGameUIFuncs.h"
#include "modes.h"
#include "utlvector.h"
#include "materialsystem/materialsystem_config.h"
#include "inetchannelinfo.h"
#include "KeyValues.h"
#include "GameBase_Shared.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

MapSelectionPanel::MapSelectionPanel(vgui::Panel *parent, char const *panelName) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	for (int i = 0; i < ARRAYSIZE(m_pBtnArrowBox); i++)
	{
		m_pImgArrowBox[i] = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "imgNavigation"));
		m_pImgArrowBox[i]->SetShouldScaleImage(true);
		m_pImgArrowBox[i]->SetZPos(40);

		m_pBtnArrowBox[i] = vgui::SETUP_PANEL(new vgui::Button(this, "btnNavigation", ""));
		m_pBtnArrowBox[i]->SetPaintBorderEnabled(false);
		m_pBtnArrowBox[i]->SetPaintEnabled(false);
		m_pBtnArrowBox[i]->SetReleasedSound("ui/button_click.wav");
		m_pBtnArrowBox[i]->SetArmedSound("ui/button_over.wav");
		m_pBtnArrowBox[i]->SetZPos(50);
		m_pBtnArrowBox[i]->AddActionSignalTarget(this);
	}

	m_pBtnArrowBox[0]->SetCommand("Left");
	m_pBtnArrowBox[1]->SetCommand("Right");

	for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
		m_pMapItems[i] = NULL;

	InvalidateLayout();
	PerformLayout();
}

MapSelectionPanel::~MapSelectionPanel()
{
	Cleanup();
}

void MapSelectionPanel::Cleanup(void)
{
	for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
	{
		if (m_pMapItems[i] != NULL)
		{
			delete m_pMapItems[i];
			m_pMapItems[i] = NULL;
		}
	}
}

void MapSelectionPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if (GameBaseShared()->GetSharedMapData())
	{
		m_iCurrPage = 0;
		m_iPageNum = 0;
		int iNeededItems = 6;
		int iItem = 0;
		int itemCount = GameBaseShared()->GetSharedMapData()->pszGameMaps.Count();
		for (int i = 0; i <= itemCount; i++)
		{
			if (i < itemCount)
			{
				if (GameBaseShared()->GetSharedMapData()->pszGameMaps[i].bExclude)
					continue;
			}

			if (iItem > iNeededItems)
			{
				m_iPageNum++;
				iNeededItems += 6;
			}

			iItem++;
		}
	}

	int w, h, x, y;
	GetSize(w, h);
	GetPos(x, y);

	for (int i = 0; i < ARRAYSIZE(m_pBtnArrowBox); i++)
	{
		m_pBtnArrowBox[i]->SetSize(scheme()->GetProportionalScaledValue(30), scheme()->GetProportionalScaledValue(30));
		m_pImgArrowBox[i]->SetSize(scheme()->GetProportionalScaledValue(30), scheme()->GetProportionalScaledValue(30));
	}

	int ypos = ((h / 2) - (scheme()->GetProportionalScaledValue(30) / 2)) - scheme()->GetProportionalScaledValue(10);

	m_pBtnArrowBox[0]->SetPos(0, ypos);
	m_pBtnArrowBox[1]->SetPos(w - scheme()->GetProportionalScaledValue(30), ypos);

	m_pImgArrowBox[0]->SetPos(0, ypos);
	m_pImgArrowBox[1]->SetPos(w - scheme()->GetProportionalScaledValue(30), ypos);
}

void MapSelectionPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void MapSelectionPanel::OnMapItemChange(KeyValues *data)
{
	int index = data->GetInt("index");
	ShowMap(index);
}

void MapSelectionPanel::OnUpdate()
{
	if (IsVisible())
	{
		for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
		{
			if (m_pMapItems[i] != NULL)
				m_pMapItems[i]->OnUpdate();
		}

		int x, y;
		vgui::input()->GetCursorPos(x, y);

		if (m_pBtnArrowBox[0]->IsWithin(x, y))
			m_pImgArrowBox[0]->SetImage("mainmenu/arrow_left_over");
		else
			m_pImgArrowBox[0]->SetImage("mainmenu/arrow_left");

		if (m_pBtnArrowBox[1]->IsWithin(x, y))
			m_pImgArrowBox[1]->SetImage("mainmenu/arrow_right_over");
		else
			m_pImgArrowBox[1]->SetImage("mainmenu/arrow_right");

		m_pBtnArrowBox[0]->SetVisible(m_iCurrPage > 0);
		m_pBtnArrowBox[1]->SetVisible(m_iCurrPage < m_iPageNum);

		m_pImgArrowBox[0]->SetVisible(m_iCurrPage > 0);
		m_pImgArrowBox[1]->SetVisible(m_iCurrPage < m_iPageNum);
	}
}

void MapSelectionPanel::ShowMap(int iIndex)
{
	for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
	{
		if (m_pMapItems[i] != NULL)
		{
			if (m_pMapItems[i]->GetMapIndex() != iIndex)
				m_pMapItems[i]->SetSelected(false);
			else
				m_pMapItems[i]->SetSelected(true);
		}
	}

	if (GetParent())
		GetParent()->OnCommand("MapSelect");
}

const char *MapSelectionPanel::GetSelectedMap(void)
{
	for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
	{
		if (m_pMapItems[i] != NULL)
		{
			if (m_pMapItems[i]->IsSelected())
				return (GameBaseShared()->GetSharedMapData()->pszGameMaps[m_pMapItems[i]->GetMapIndex()].pszMapName);
		}
	}

	return "";
}

int MapSelectionPanel::GetSelectedMapIndex()
{
	for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
	{
		if (m_pMapItems[i] != NULL)
		{
			if (m_pMapItems[i]->IsSelected())
				return m_pMapItems[i]->GetMapIndex();
		}
	}

	return -1;
}

void MapSelectionPanel::Redraw(int index)
{
	if (GameBaseShared()->GetSharedMapData())
	{
		Cleanup();

		int mapCount = GameBaseShared()->GetSharedMapData()->pszGameMaps.Count();
		int startIndex = index;
		int offset = index;

		for (int i = 0; i < _ARRAYSIZE(m_pMapItems); i++)
		{
			if (offset >= mapCount)
				break;

			bool bFoundMap = false;
			for (int mapIndex = offset; mapIndex < mapCount; mapIndex++)
			{
				if (!GameBaseShared()->GetSharedMapData()->pszGameMaps[mapIndex].bExclude)
				{
					offset = mapIndex;
					bFoundMap = true;
					break;
				}
			}

			if (!bFoundMap)
				break;

			m_pMapItems[i] = new MapSelectionItem(this, "MapItem", offset);
			m_pMapItems[i]->SetZPos(10);
			m_pMapItems[i]->SetPos(scheme()->GetProportionalScaledValue(32) + (scheme()->GetProportionalScaledValue(104) * i), 0);
			m_pMapItems[i]->SetSize(scheme()->GetProportionalScaledValue(100), scheme()->GetProportionalScaledValue(100));
			m_pMapItems[i]->AddActionSignalTarget(this);
			offset++;
		}

		ShowMap(startIndex);
	}
}

void MapSelectionPanel::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Left"))
	{
		if (m_iCurrPage > 0)
		{
			m_iCurrPage--;
			Redraw((m_iCurrPage * _ARRAYSIZE(m_pMapItems)));
		}
	}
	else if (!Q_stricmp(pcCommand, "Right"))
	{
		if (m_iCurrPage < m_iPageNum)
		{
			m_iCurrPage++;
			Redraw((m_iCurrPage * _ARRAYSIZE(m_pMapItems)));
		}
	}

	BaseClass::OnCommand(pcCommand);
}
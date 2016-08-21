//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Combo Image List - An image selection combo box. (includes tga to vtf conversions and such)
//
//========================================================================================//

#include "cbase.h"
#include "ComboImageList.h"
#include "vgui/MouseCode.h"
#include <vgui_controls/ImagePanel.h>
#include "KeyValues.h"
#include "filesystem.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ComboImageList::ComboImageList(vgui::Panel *parent, char const *panelName, const char *text, bool bFileDialog) : vgui::Panel(parent, panelName)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_hImportSprayDialog = NULL;

	m_pComboBox = vgui::SETUP_PANEL(new vgui::ImageListPanel(parent, "ComboBox"));
	m_pComboBox->AddActionSignalTarget(this);
	m_pComboBox->SetZPos(100);
	m_pComboBox->SetVisible(false);

	m_pLabel = vgui::SETUP_PANEL(new vgui::Label(this, "Label", ""));
	m_pLabel->SetZPos(10);
	m_pLabel->SetText(text);
	m_pLabel->SetContentAlignment(Label::a_west);

	m_pActiveImage = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "ImagePreview"));
	m_pActiveImage->SetZPos(10);
	m_pActiveImage->SetShouldScaleImage(true);

	m_pActiveLabel = vgui::SETUP_PANEL(new vgui::Label(this, "LabelPreview", ""));
	m_pActiveLabel->SetZPos(10);
	m_pActiveLabel->SetContentAlignment(vgui::Label::Alignment::a_center);

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
	{
		m_pButton[i] = vgui::SETUP_PANEL(new vgui::Button(this, "Button", ""));
		m_pButton[i]->SetPaintBorderEnabled(false);
		m_pButton[i]->SetPaintEnabled(false);
		m_pButton[i]->SetReleasedSound("ui/button_click.wav");
		m_pButton[i]->SetZPos(25);
		m_pButton[i]->AddActionSignalTarget(this);
	}

	m_pButton[1]->SetCommand("Activate");

#ifdef _WIN32
	m_bFileDialogEnabled = bFileDialog;
	if (bFileDialog)
		m_pButton[0]->SetCommand("ActivateList");
	else
		m_pButton[0]->SetCommand("Activate");
#else
	m_bFileDialogEnabled = false;
	m_pButton[0]->SetCommand("Activate");
#endif

	InvalidateLayout();
	PerformLayout();
}

ComboImageList::~ComboImageList()
{
	m_pComboBox->RemoveAll();
}

void ComboImageList::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h, x, y;
	GetSize(w, h);
	GetPos(x, y);

	m_pComboBox->SetSize(h, h);

	m_pButton[0]->SetPos(0, 0);
	m_pButton[0]->SetSize((w / 2), h);

	m_pButton[1]->SetPos((w / 2), 0);
	m_pButton[1]->SetSize((w / 2), h);

	m_pLabel->SetPos(0, 0);
	m_pLabel->SetSize(w - h, h);
	m_pLabel->SetContentAlignment(Label::a_west);
	m_pComboBox->SetPos(x + (w - h), y + h);

	m_pActiveImage->SetPos(w - h, 0);
	m_pActiveImage->SetSize(h, h);

	m_pActiveLabel->SetPos(w - h, 0);
	m_pActiveLabel->SetSize(h, h);
}

void ComboImageList::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pLabel->SetFont(pScheme->GetFont("OptionTextMedium"));
	m_pLabel->SetFgColor(pScheme->GetColor("ComboListTextColor", Color(255, 255, 255, 255)));

	m_pActiveLabel->SetFont(pScheme->GetFont("CrosshairBB2", false));
}

void ComboImageList::OnCommand(const char* pcCommand)
{
	if (!Q_stricmp(pcCommand, "Activate"))
	{
#ifdef _WIN32
		if (m_bFileDialogEnabled)
			InitSprayList();
#endif

		m_pComboBox->SetVisible(!m_pComboBox->IsVisible());
		if (m_pComboBox->IsVisible())
		{
			m_pComboBox->MoveToFront();
			m_pComboBox->RequestFocus();
			PerformLayout();
		}
	}
	else if (!Q_stricmp(pcCommand, "ActivateList"))
	{
		if (m_hImportSprayDialog == NULL)
		{
			m_hImportSprayDialog = new FileOpenDialog(NULL, "#GameUI_ImportSprayImage", true);
			m_hImportSprayDialog->AddFilter("*.tga,*.jpg,*.bmp,*.png", "#GameUI_All_Images", true);
			m_hImportSprayDialog->AddFilter("*.tga", "#GameUI_TGA_Images", false);
			m_hImportSprayDialog->AddFilter("*.jpg", "#GameUI_JPEG_Images", false);
			m_hImportSprayDialog->AddFilter("*.bmp", "#GameUI_BMP_Images", false);
			m_hImportSprayDialog->AddFilter("*.png", "#GameUI_PNG_Images", false);
			m_hImportSprayDialog->AddActionSignalTarget(this);
		}

		m_hImportSprayDialog->DoModal(false);
		m_hImportSprayDialog->Activate();
	}

	BaseClass::OnCommand(pcCommand);
}

void ComboImageList::SetEnabled(bool state)
{
	BaseClass::SetEnabled(state);

	m_pComboBox->SetEnabled(state);
	m_pLabel->SetEnabled(state);

	for (int i = 0; i < _ARRAYSIZE(m_pButton); i++)
		m_pButton[i]->SetEnabled(state);
}

void ComboImageList::InitCrosshairList(void)
{
	m_pComboBox->RemoveAll();

	KeyValues *pkvCrosshairList = new KeyValues("CrosshairInfo");
	if (pkvCrosshairList->LoadFromFile(filesystem, "scripts/hud_crosshairs.txt", "MOD"))
	{
		KeyValues *pkvTextureData = pkvCrosshairList->FindKey("TextureData");
		for (KeyValues *sub = pkvTextureData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			const char *character = ReadAndAllocStringValue(sub, "character");
			if (character && strlen(character) > 0)
			{
				Label *panel = new Label(m_pComboBox, "Label", character);
				panel->SetZPos(25);
				panel->SetContentAlignment(Label::Alignment::a_center);
				panel->SetText(character);
				m_pComboBox->AddControl(panel);
			}
		}
	}

	pkvCrosshairList->deleteThis();
}

void ComboImageList::InitSprayList(void)
{
	m_pComboBox->RemoveAll();

	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx("materials/vgui/logos/*.vmt", "MOD", &findHandle);
	while (pFilename)
	{
		char fileNameWithoutExtension[32];
		Q_strncpy(fileNameWithoutExtension, pFilename, strlen(pFilename) - 3);

		ImagePanel *panel = new ImagePanel(m_pComboBox, "Image");
		panel->SetZPos(25);
		panel->SetShouldScaleImage(true);
		panel->SetImage(VarArgs("logos/%s", fileNameWithoutExtension));
		m_pComboBox->AddControl(panel);

		pFilename = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
}

void ComboImageList::OnThink(void)
{
	BaseClass::OnThink();
	m_pActiveLabel->SetFgColor(Color(crosshair_color_red.GetInt(), crosshair_color_green.GetInt(), crosshair_color_blue.GetInt(), crosshair_color_alpha.GetInt()));
}

void ComboImageList::OnImageListClick(int index)
{
	m_iSelectedIndex = index;
	PostActionSignal(new KeyValues("ComboImageListClicked"));

	const char *szToken = GetTokenForActiveItem();
	if (szToken)
	{
		if ((strlen(szToken) > 1))
			m_pActiveImage->SetImage(szToken);
		else
			m_pActiveLabel->SetText(szToken);
	}
}

void ComboImageList::OnFileSelected(const char *fullpath)
{
#ifdef _WIN32
	if ((fullpath == NULL) || (fullpath[0] == 0))
		return;

	char szPath[512];
	char szUTILPath[512];

	char szExe[512];
	char szParams[1024];
	Q_strncpy(szPath, fullpath, 512);
	Q_strncpy(szUTILPath, engine->GetGameDirectory(), (strlen(engine->GetGameDirectory()) - 11)); // strip away brainbread2, so we're now in common/brainbread2 instead of common/brainbread2/brainbread2

	Q_snprintf(szExe, 512, "%s\\bin\\utils\\VTFCmd.exe", szUTILPath);
	Q_snprintf(szParams, 1024, "-file \"%s\" -output \"%s\\materials\\vgui\\logos\" -format \"dxt5\" -resize -shader \"UnlitGeneric\" -param \"$translucent\" \"1\" -param \"$vertexcolor\" \"1\" -param \"$vertexalpha\" \"1\" -param \"$no_fullbright\" \"1\" -param \"ignorez\" \"1\" -silent", szPath, engine->GetGameDirectory());

	system()->ShellExecuteEx("open", szExe, szParams);
#endif
}
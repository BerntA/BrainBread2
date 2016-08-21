//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: A simple note system. Reads the text from the data/notes/<noteName>.txt and the header text sent in the usermessage from the item_note's field.
//
//========================================================================================//

#include "cbase.h"
#include "NotePanel.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include "ienginevgui.h"
#include "c_baseplayer.h" 
#include "hud_numericdisplay.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/RichText.h>

using namespace vgui;

void CNotePanel::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);
	LoadControlSettings("resource/ui/notepanel.res");
}

void CNotePanel::OnShowPanel(bool bShow)
{
	vgui::surface()->PlaySound(bShow ? "ui/document_read.wav" : "ui/document_close.wav");
	BaseClass::OnShowPanel(bShow);

	if (GetBackground())
		GetBackground()->SetVisible(false);

	int w, h;
	GetSize(w, h);
	m_pInputPanel->SetPos(0, 0);
	m_pInputPanel->SetSize(w, h);
}

CNotePanel::CNotePanel(vgui::VPANEL parent) : BaseClass(NULL, "NotePanel", false, 0.25f)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(true);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetVisible(false);

	m_pBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Background"));
	m_pBackground->SetImage("notebg");
	m_pBackground->SetZPos(10);
	m_pBackground->SetShouldScaleImage(true);

	m_pNoteText = vgui::SETUP_PANEL(new vgui::RichText(this, "NoteText"));
	m_pNoteText->AddActionSignalTarget(this);
	m_pNoteText->SetZPos(20);
	m_pNoteText->SetText("");
	m_pNoteText->SetUnusedScrollbarInvisible(false);
	m_pNoteText->SetVerticalScrollbar(true);

	m_pNoteHeader = vgui::SETUP_PANEL(new vgui::Label(this, "NoteHeader", ""));
	m_pNoteHeader->SetZPos(30);
	m_pNoteHeader->SetContentAlignment(Label::a_center);

	m_pInputPanel = vgui::SETUP_PANEL(new vgui::MouseInputPanel(this, "InputPanel"));
	m_pInputPanel->AddActionSignalTarget(this);
	m_pInputPanel->SetZPos(50);

	SetScheme("BaseScheme");

	LoadControlSettings("resource/ui/notepanel.res");

	PerformLayout();

	InvalidateLayout();
}

CNotePanel::~CNotePanel()
{
}

void CNotePanel::SetupNote(const char *szHeader, const char *szFile)
{
	m_pNoteHeader->SetText(szHeader);
	m_pNoteText->SetScrollbarAlpha(0);

	bool bCouldRead = false;
	FileHandle_t f = filesystem->Open(VarArgs("data/notes/%s.txt", szFile), "rb", "MOD");
	if (f)
	{
		int fileSize = filesystem->Size(f);
		unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 2);

		char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
		Assert(buffer);

		// read into local buffer
		bool bRetOK = (((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f) != 0);

		filesystem->Close(f);	// close file after reading

		if (bRetOK)
		{
			buffer[fileSize] = 0; // null terminate file as EOF
			buffer[fileSize + 1] = 0; // double NULL terminating in case this is a unicode file

			m_pNoteText->SetText(buffer);
			bCouldRead = true;
		}

		((IFileSystem *)filesystem)->FreeOptimalReadBuffer(buffer);
	}
	else
	{
		Warning("The note you tried to read doesn't exist in your game folder!\n");
		return;
	}

	if (!bCouldRead)
	{
		Warning("Unable to read note!\n");
		return;
	}

	m_pNoteHeader->SetContentAlignment(vgui::Label::a_center);
	m_pNoteText->GotoTextStart();

	OnShowPanel(true);
}

void CNotePanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pNoteHeader->SetFgColor(pScheme->GetColor("NotePanelTitleTextColor", Color(5, 5, 5, 255)));
	m_pNoteHeader->SetFont(pScheme->GetFont("DefaultLarge"));

	m_pNoteText->SetBgColor(Color(0, 0, 0, 0));
	m_pNoteText->SetBorder(NULL);
	m_pNoteText->SetFgColor(pScheme->GetColor("NotePanelDescriptionTextColor", Color(5, 5, 5, 255)));
	m_pNoteText->SetFont(pScheme->GetFont("Default"));
}

void CNotePanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	OnShowPanel(false);
	BaseClass::OnKeyCodeTyped(code);
}

void CNotePanel::OnMousePressed(vgui::MouseCode code)
{
	OnShowPanel(false);
	BaseClass::OnMousePressed(code);
}

void CNotePanel::OnMouseWheeled(int delta)
{
	BaseClass::OnMouseWheeled(delta);
	m_pNoteText->ForceOnMouseWheeled(delta);
}
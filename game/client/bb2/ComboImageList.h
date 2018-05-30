//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Combo Image List - An image selection combo box. (includes tga to vtf conversions and such)
//
//========================================================================================//

#ifndef COMBO_IMAGE_LIST_H
#define COMBO_IMAGE_LIST_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/FileOpenDialog.h>
#include "ImageListPanel.h"

namespace vgui
{
	class ComboImageList;

	class ComboImageList : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(ComboImageList, vgui::Panel);

	public:
		ComboImageList(vgui::Panel *parent, char const *panelName, const char *text, bool bFileDialog = false);
		~ComboImageList();

		void SetEnabled(bool state);
		int GetActiveItem(void) { return m_iSelectedIndex; }

		void InitCrosshairList(void);
		void InitSprayList(void);

		void OnThink();

		void SetActiveItem(int index) { m_pComboBox->SetActiveItem(index); }
		void SetActiveItem(const char *token) { m_pComboBox->SetActiveItem(token); }
		const char *GetTokenForActiveItem() { return m_pComboBox->GetTokenForIndex(m_iSelectedIndex); }

	private:
		bool m_bFileDialogEnabled;
		int  m_iSelectedIndex;
		vgui::Button *m_pButton[2];
		vgui::Label *m_pLabel;
		vgui::ImagePanel *m_pActiveImage;
		vgui::Label *m_pActiveLabel;
		vgui::ImageListPanel *m_pComboBox;
		vgui::FileOpenDialog *m_hImportSprayDialog;

		MESSAGE_FUNC_CHARPTR(OnFileSelected, "FileSelected", fullpath);
		MESSAGE_FUNC_INT(OnImageListClick, "ControlClicked", index);

	protected:
		void OnCommand(const char *pcCommand);
		void ApplySchemeSettings(vgui::IScheme *pScheme);
		void PerformLayout();
	};
}

#endif // COMBO_IMAGE_LIST_H
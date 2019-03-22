//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handles Keyboard Related Options
//
//========================================================================================//

#include "cbase.h"
#include "OptionMenuKeyboard.h"
#include <vgui_controls/AnimationController.h>
#include <inputsystem/iinputsystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define KEYBOARD_REFRESH_TIME 0.25f

const char *szSectionNames[] =
{
	"#Valve_Movement_Title",
	"#Valve_Combat_Title",
	"#Valve_Miscellaneous_Title",
};

const char *szCollumData[] =
{
	"binding",
	"button",
};

OptionMenuKeyboard::OptionMenuKeyboard(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/BaseKeyboardScheme.res", "BaseKeyboardScheme"));

	m_pEditPanel = vgui::SETUP_PANEL(new vgui::Button(this, "btnEdit", "#GameUI_SetNewKey", this));
	m_pEditPanel->SetZPos(100);
	m_pEditPanel->SetVisible(false);
	m_pEditPanel->SetText("#GameUI_SetNewKey");
	m_pEditPanel->SetContentAlignment(Label::a_center);

	m_pMousePanel = vgui::SETUP_PANEL(new vgui::MouseInputPanel(this, "MouseInputs"));
	m_pMousePanel->SetZPos(500);
	m_pMousePanel->SetVisible(false);
	m_pMousePanel->SetEnabled(false);

	m_pKeyBoardList = vgui::SETUP_PANEL(new vgui::SectionedListPanel(this, "KeyboardList"));
	m_pKeyBoardList->SetBgColor(Color(0, 0, 0, 0));
	m_pKeyBoardList->SetBorder(NULL);
	m_pKeyBoardList->SetFgColor(Color(255, 255, 255, 255));

	for (int i = 0; i <= 2; i++)
	{
		m_pKeyBoardList->AddSection(i, "");
		m_pKeyBoardList->SetSectionAlwaysVisible(i);
		m_pKeyBoardList->SetSectionFgColor(i, Color(255, 255, 255, 255));

		if (i == 0)
		{
			m_pKeyBoardList->AddColumnToSection(i, szCollumData[0], szSectionNames[0], 0, (GetWide() / 2));
			m_pKeyBoardList->AddColumnToSection(i, szCollumData[1], "#GameUI_KeyButton", 0, (GetWide() / 2));
		}
		else if (i == 1)
		{
			m_pKeyBoardList->AddColumnToSection(i, szCollumData[0], szSectionNames[1], 0, (GetWide() / 2));
			m_pKeyBoardList->AddColumnToSection(i, szCollumData[1], "#GameUI_KeyButton", 0, (GetWide() / 2));
		}
		else
		{
			m_pKeyBoardList->AddColumnToSection(i, szCollumData[0], szSectionNames[2], 0, (GetWide() / 2));
			m_pKeyBoardList->AddColumnToSection(i, szCollumData[1], "#GameUI_KeyButton", 0, (GetWide() / 2));
		}
	}

	FillKeyboardList();

	InvalidateLayout();

	PerformLayout();

	bInEditMode = false;
	iCurrentModifiedSelectedID = -1;
}

OptionMenuKeyboard::~OptionMenuKeyboard()
{
}

void OptionMenuKeyboard::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		if (bInEditMode)
		{
			// Force this ID until we go out of edit mode!
			m_pKeyBoardList->SetSelectedItem(iCurrentModifiedSelectedID);

			// Force Mouse Panel to be infront! If it ain't then we can't record mouse pressing!
			m_pMousePanel->SetVisible(true);
			m_pMousePanel->SetEnabled(true);
			m_pMousePanel->RequestFocus();
			m_pMousePanel->MoveToFront();

			ButtonCode_t code = BUTTON_CODE_INVALID;
			if (engine->CheckDoneKeyTrapping(code))
			{
				bInEditMode = false;
				m_pKeyBoardList->LeaveEditMode();
				m_pKeyBoardList->ClearSelection();
				UpdateKeyboardListData(code);
			}
		}
		else
		{
			m_pMousePanel->SetVisible(false);
			m_pMousePanel->SetEnabled(false);

			// Refresh the keyboard list:
			if (m_bShouldUpdate && m_flUpdateTime >= 1.0f)
			{
				m_bShouldUpdate = false;
				m_flUpdateTime = 0.0f;
				FillKeyboardList();
			}
		}
	}
}

void OptionMenuKeyboard::SetupLayout(void)
{
	BaseClass::SetupLayout();

	InvalidateLayout(false, true);

	if (!IsVisible())
		FillKeyboardList();

	m_pMousePanel->SetSize(GetWide(), GetTall());
	m_pMousePanel->SetPos(0, 0);
	m_pKeyBoardList->SetSize(GetWide(), GetTall());
	m_pKeyBoardList->SetPos(0, 0);

	m_pKeyBoardList->SetColumnWidthAtSection(0, 0, (GetWide() / 2));
	m_pKeyBoardList->SetColumnWidthAtSection(0, 1, (GetWide() / 2));
	m_pKeyBoardList->SetColumnWidthAtSection(1, 0, (GetWide() / 2));
	m_pKeyBoardList->SetColumnWidthAtSection(1, 1, (GetWide() / 2));
	m_pKeyBoardList->SetColumnWidthAtSection(2, 0, (GetWide() / 2));
	m_pKeyBoardList->SetColumnWidthAtSection(2, 1, (GetWide() / 2));
}

void OptionMenuKeyboard::OnKeyCodeTyped(KeyCode code)
{
	if (bInEditMode)
		return;

	if (code == KEY_ENTER)
	{
		if (!bInEditMode && IsVisible() && (m_pKeyBoardList->GetSelectedItem() != -1))
		{
			iCurrentModifiedSelectedID = m_pKeyBoardList->GetSelectedItem();
			bInEditMode = true;
			m_pKeyBoardList->EnterEditMode(iCurrentModifiedSelectedID, 1, m_pEditPanel);
			engine->StartKeyTrapMode();
		}
	}
	else
		BaseClass::OnKeyCodeTyped(code);
}

void OptionMenuKeyboard::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	m_pKeyBoardList->SetBorder(NULL);

	for (int i = 0; i <= m_pKeyBoardList->GetHighestItemID(); i++)
	{
		m_pKeyBoardList->SetItemFgColor(i, pScheme->GetColor("KeyboardItemTextColor", Color(255, 255, 255, 255)));
		m_pKeyBoardList->SetItemFont(i, pScheme->GetFont("KeyboardTextSmall"));
	}

	for (int i = 0; i <= 2; i++)
		m_pKeyBoardList->SetFontSection(i, pScheme->GetFont("KeyboardListSection"));

	m_pEditPanel->SetSize(scheme()->GetProportionalScaledValue(80), scheme()->GetProportionalScaledValue(10));
	m_pEditPanel->SetFgColor(pScheme->GetColor("EditPanelFgColor", Color(122, 73, 57, 255)));
	m_pEditPanel->SetBgColor(pScheme->GetColor("EditPanelBgColor", Color(255, 155, 0, 255)));
	m_pEditPanel->SetFont(pScheme->GetFont("KeyboardTextSmall"));

	m_pKeyBoardList->SetFgColor(pScheme->GetColor("KeyboardListFgColor", Color(255, 255, 255, 255)));
	m_pKeyBoardList->SetBgColor(pScheme->GetColor("KeyboardListBgColor", Color(0, 0, 0, 0)));
}

// Load all available key binds from a script file in the data folder! 
void OptionMenuKeyboard::FillKeyboardList(bool bfullUpdate)
{
	if (bfullUpdate)
	{
		m_bShouldUpdate = true;
		m_flUpdateTime = 0.0f;
		GetAnimationController()->RunAnimationCommand(this, "KeyboardRefreshTimer", 1.0f, 0.0f, KEYBOARD_REFRESH_TIME, AnimationController::INTERPOLATOR_LINEAR);
		return;
	}

	// RemoveItem
	m_pKeyBoardList->DeleteAllItems(); // Reset...

	// Create a new keyvalue, load our keyboard button def file. THIS IS THE NEW KB_ACT.LST FILE ( SEE MOD/SCRIPTS FOR EXAMPLE OF OLD STUFF ).
	// NEW KEYS GOES IN KeyboardButtons.TXT!
	KeyValues *kvGetData = new KeyValues("KeyboardData");
	if (kvGetData->LoadFromFile(filesystem, "data/settings/KeyboardButtons.txt", "MOD"))
	{
		KeyValues *pkvCombat = kvGetData->FindKey("Combat");
		KeyValues *pkvMovement = kvGetData->FindKey("Movement");
		KeyValues *pkvMisc = kvGetData->FindKey("Misc");

		for (KeyValues *sub = pkvCombat->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			char szBinding[256];

			const char *szFindBinding = engine->Key_LookupBinding(sub->GetName());

			if (szFindBinding)
				Q_strncpy(szBinding, szFindBinding, 256);
			else
				Q_strncpy(szBinding, "NONE", 256);

			// Force UPPERCASE!
			if (Q_strcmp(szBinding, "SEMICOLON") == 0)
				Q_strncpy(szBinding, ";", 256);
			else if (strlen(szBinding) == 1 && szBinding[0] >= 'a' && szBinding[0] <= 'z')
				szBinding[0] += ('A' - 'a');

			KeyValues *pkvDataItem = new KeyValues("KeyboardButton");
			pkvDataItem->SetString(szCollumData[0], sub->GetString());
			pkvDataItem->SetString(szCollumData[1], szBinding);
			m_pKeyBoardList->AddItem(1, pkvDataItem);
			pkvDataItem->deleteThis();
		}

		for (KeyValues *sub = pkvMovement->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			char szBinding[256];

			const char *szFindBinding = engine->Key_LookupBinding(sub->GetName());

			if (szFindBinding)
				Q_strncpy(szBinding, szFindBinding, 256);
			else
				Q_strncpy(szBinding, "NONE", 256);

			// Force UPPERCASE!
			if (Q_strcmp(szBinding, "SEMICOLON") == 0)
				Q_strncpy(szBinding, ";", 256);
			else if (strlen(szBinding) == 1 && szBinding[0] >= 'a' && szBinding[0] <= 'z')
				szBinding[0] += ('A' - 'a');

			KeyValues *pkvDataItem = new KeyValues("KeyboardButton");
			pkvDataItem->SetString(szCollumData[0], sub->GetString());
			pkvDataItem->SetString(szCollumData[1], szBinding);
			m_pKeyBoardList->AddItem(0, pkvDataItem);
			pkvDataItem->deleteThis();
		}

		for (KeyValues *sub = pkvMisc->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			char szBinding[256];

			const char *szFindBinding = engine->Key_LookupBinding(sub->GetName());

			if (szFindBinding)
				Q_strncpy(szBinding, szFindBinding, 256);
			else
				Q_strncpy(szBinding, "NONE", 256);

			// Force UPPERCASE!
			if (Q_strcmp(szBinding, "SEMICOLON") == 0)
				Q_strncpy(szBinding, ";", 256);
			else if (strlen(szBinding) == 1 && szBinding[0] >= 'a' && szBinding[0] <= 'z')
				szBinding[0] += ('A' - 'a');

			KeyValues *pkvDataItem = new KeyValues("KeyboardButton");
			pkvDataItem->SetString(szCollumData[0], sub->GetString());
			pkvDataItem->SetString(szCollumData[1], szBinding);
			m_pKeyBoardList->AddItem(2, pkvDataItem);
			pkvDataItem->deleteThis();
		}
	}

	kvGetData->deleteThis();

	bInEditMode = false;
	iCurrentModifiedSelectedID = -1;

	m_bShouldUpdate = false;
	m_flUpdateTime = 0.0f;

	m_pMousePanel->SetVisible(false);
	m_pMousePanel->SetEnabled(false);

	m_pKeyBoardList->SetBorder(NULL);
}

void OptionMenuKeyboard::UpdateKeyboardListData(ButtonCode_t code)
{
	if (code != KEY_ESCAPE)
	{
		char szCodeToBind[256];
		Q_strncpy(szCodeToBind, g_pInputSystem->ButtonCodeToString(code), 256);

		char szOldKey[256];
		char szNewKey[256];

		engine->ClientCmd_Unrestricted(VarArgs("unbind %s", szCodeToBind));

		KeyValues *pkvKeyData = m_pKeyBoardList->GetItemData(iCurrentModifiedSelectedID);
		pkvKeyData = pkvKeyData->FindKey("binding");
		if (pkvKeyData)
			Q_strncpy(szOldKey, pkvKeyData->GetString(), 256);

		// We load the keyboard file to find the binding related to the text in szOldKey.
		KeyValues *kvGetData = new KeyValues("KeyboardData");
		if (kvGetData->LoadFromFile(filesystem, "data/settings/KeyboardButtons.txt", "MOD"))
		{
			KeyValues *pkvCombat = kvGetData->FindKey("Combat");
			KeyValues *pkvMovement = kvGetData->FindKey("Movement");
			KeyValues *pkvMisc = kvGetData->FindKey("Misc");

			for (KeyValues *sub = pkvCombat->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				if (!strcmp(sub->GetString(), szOldKey))
				{
					engine->ClientCmd_Unrestricted(VarArgs("unbind %s", engine->Key_LookupBinding(sub->GetName())));
					Q_strncpy(szNewKey, sub->GetName(), 256);
					break;
				}
			}

			for (KeyValues *sub = pkvMovement->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				if (!strcmp(sub->GetString(), szOldKey))
				{
					engine->ClientCmd_Unrestricted(VarArgs("unbind %s", engine->Key_LookupBinding(sub->GetName())));
					Q_strncpy(szNewKey, sub->GetName(), 256);
					break;
				}
			}

			for (KeyValues *sub = pkvMisc->GetFirstSubKey(); sub; sub = sub->GetNextKey())
			{
				if (!strcmp(sub->GetString(), szOldKey))
				{
					engine->ClientCmd_Unrestricted(VarArgs("unbind %s", engine->Key_LookupBinding(sub->GetName())));
					Q_strncpy(szNewKey, sub->GetName(), 256);
					break;
				}
			}
		}
		kvGetData->deleteThis();

		if (szNewKey && szNewKey[0])
		{
			const char *szBinding = VarArgs("bind %s \"%s\"", szCodeToBind, szNewKey);
			engine->ClientCmd_Unrestricted(szBinding);
		}

		engine->ClientCmd_Unrestricted("host_writeconfig"); // update config.cfg

		m_bShouldUpdate = true;
		m_flUpdateTime = 0.0f;
		GetAnimationController()->RunAnimationCommand(this, "KeyboardRefreshTimer", 1.0f, 0.0f, KEYBOARD_REFRESH_TIME, AnimationController::INTERPOLATOR_LINEAR);
	}
	else
		iCurrentModifiedSelectedID = -1;
}
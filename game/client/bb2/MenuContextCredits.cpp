//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Credits Preview
//
//========================================================================================//

#include "cbase.h"
#include "MenuContextCredits.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "GameBase_Client.h"
#include "GameBase_Shared.h"
#include "fmod_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

namespace vgui
{
	class CCreditsObject
	{
	public:
		CCreditsObject()
		{
			name[0] = 0;
		}

		CCreditsObject(const char *inName, const char *inDesc)
		{
			Q_strncpy(this->name, inName, sizeof(this->name));
			V_SplitString(inDesc, ",", this->desc);
		}

		~CCreditsObject()
		{
			ClearCharVectorList(this->desc);
		}

		char name[64];
		CUtlVector<char*> desc;

	private:
		CCreditsObject(const CCreditsObject &);
	};

	struct SectionItem
	{
		const char *name;
		CUtlVector<CCreditsObject*> *pList;
	};

	void ClearSectionList(CUtlVector<CCreditsObject*> &list)
	{
		for (int i = 0; i < list.Count(); i++)
			delete list[i];
		list.RemoveAll();
	}
}

MenuContextCredits::MenuContextCredits(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	m_DefaultFont = NULL;
	m_DefaultColor = COLOR_WHITE;
	m_flScrollRate = 200.0f;
	m_flCurrentScrollRate = 0.0f;
	m_iPosY = 0;
	m_iLastYPos = 1;

	SetScheme("BaseScheme");

	{
		KeyValues *pkvCreditsFile = new KeyValues("Credits");
		KeyValues *pkvSection = NULL;
		SectionItem sections[] = { { "developers", &m_listDevelopers }, { "contributors", &m_listContributors }, { "testers", &m_listTesters }, { "special", &m_listSpecialThanks } };
		if (pkvCreditsFile->LoadFromFile(filesystem, "data/settings/credits.txt", "MOD"))
		{
			for (int s = 0; s < _ARRAYSIZE(sections); s++)
			{
				SectionItem &item = sections[s];
				pkvSection = pkvCreditsFile->FindKey(item.name);
				if (pkvSection)
				{
					for (KeyValues *sub = pkvSection->GetFirstSubKey(); sub; sub = sub->GetNextKey())
						item.pList->AddToTail(new CCreditsObject(sub->GetName(), sub->GetString()));
				}
			}
		}
		pkvCreditsFile->deleteThis();
	}

	InvalidateLayout();
	PerformLayout();
}

MenuContextCredits::~MenuContextCredits()
{
	ClearSectionList(m_listDevelopers);
	ClearSectionList(m_listContributors);
	ClearSectionList(m_listTesters);
	ClearSectionList(m_listSpecialThanks);
}

void MenuContextCredits::SetupLayout(void)
{
	BaseClass::SetupLayout();
}

void MenuContextCredits::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		if (m_iLastYPos < 0)
		{
			GameBaseClient->RunCommand(COMMAND_RETURN);
			m_iLastYPos = 1;
		}
	}
}

void MenuContextCredits::PerformLayout()
{
	BaseClass::PerformLayout();
}

void MenuContextCredits::OnShowPanel(bool bShow)
{
	BaseClass::OnShowPanel(bShow);

	if (bShow)
	{
		m_iLastYPos = 1;
		m_flCurrentScrollRate = 0.0f;
		m_flScrollRate = 200.0f;
	}

	if (!engine->IsInGame())
	{
		FMODManager()->SetSoundVolume(1.0f);

		if (bShow)
			FMODManager()->TransitionAmbientSound("ui/credits_theme.mp3");
		else
			FMODManager()->TransitionAmbientSound("ui/mainmenu_theme.mp3");
	}
}

void MenuContextCredits::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// Get the time we'll use to move the credits list from down to up.
	//m_flCreditsScrollTime = atof(pScheme->GetResourceString("CreditsScrollTime"));

	m_DefaultFont = pScheme->GetFont("OptionTextMedium");
	m_DefaultColor = pScheme->GetColor("CreditsLabelColor", Color(255, 255, 255, 255));
}

void MenuContextCredits::OnMouseWheeled(int delta)
{
	BaseClass::OnMouseWheeled(delta);
	float flDelta = ((float)delta) * 20.0f;
	m_flScrollRate += flDelta;
	m_flScrollRate = clamp(m_flScrollRate, 50.0f, 100000.0f);
}

#define XPOS_START scheme()->GetProportionalScaledValue(210)

void MenuContextCredits::Paint()
{
	BaseClass::Paint();
	if (m_DefaultFont == NULL)
		return;

	m_iPosY = 0;
	wchar_t unicode[128];
	const int fontTall = surface()->GetFontTall(m_DefaultFont);
	surface()->DrawSetColor(m_DefaultColor);
	surface()->DrawSetTextColor(m_DefaultColor);
	surface()->DrawSetTextFont(m_DefaultFont);

	SectionItem sections[] = { { "DEVELOPERS", &m_listDevelopers }, { "CONTRIBUTORS", &m_listContributors }, { "TESTERS", &m_listTesters }, { "SPECIAL THANKS", &m_listSpecialThanks } };
	for (int s = 0; s < _ARRAYSIZE(sections); s++)
	{
		const SectionItem &item = sections[s];

		g_pVGuiLocalize->ConvertANSIToUnicode(item.name, unicode, sizeof(unicode));
		surface()->DrawSetTextPos(XPOS_START, GetYPos(m_iPosY));
		surface()->DrawUnicodeString(unicode);
		m_iPosY += fontTall + scheme()->GetProportionalScaledValue(8);

		for (int i = 0; i < item.pList->Count(); i++)
			PaintCreditObject(item.pList->Element(i));

		m_iPosY += fontTall;
	}

	m_flCurrentScrollRate += (gpGlobals->frametime * m_flScrollRate);
}

void MenuContextCredits::PaintCreditObject(CCreditsObject *item)
{
	wchar_t unicode[128];
	const int fontTall = surface()->GetFontTall(m_DefaultFont);

	surface()->DrawSetColor(m_DefaultColor);
	surface()->DrawSetTextColor(m_DefaultColor);
	surface()->DrawSetTextFont(m_DefaultFont);

	g_pVGuiLocalize->ConvertANSIToUnicode(item->name, unicode, sizeof(unicode));
	surface()->DrawSetTextPos(XPOS_START + scheme()->GetProportionalScaledValue(2), GetYPos(m_iPosY));
	surface()->DrawUnicodeString(unicode);
	m_iPosY += fontTall;

	for (int i = 0; i < item->desc.Count(); i++)
	{
		const char *desc = item->desc[i];
		g_pVGuiLocalize->ConvertANSIToUnicode(desc, unicode, sizeof(unicode));
		surface()->DrawSetTextPos(XPOS_START + scheme()->GetProportionalScaledValue(4), GetYPos(m_iPosY));
		surface()->DrawUnicodeString(unicode);
		m_iPosY += fontTall;
	}

	m_iLastYPos = GetYPos(m_iPosY);
	m_iPosY += (fontTall / 2);
}

int MenuContextCredits::GetYPos(int offset)
{
	return ((ScreenHeight() + offset) - ceil(m_flCurrentScrollRate));
}
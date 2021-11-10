//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Displays the achievements...
//
//========================================================================================//

#include "cbase.h"
#include "ProfileMenuAchievementPanel.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "GameDefinitions_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

bool HasAchievementInList(CUtlVector<AchievementItem*> &list, int index)
{
	for (int i = 0; i < list.Count(); i++)
	{
		if (list[i]->GetAchievementIndex() == index)
			return true;
	}

	return false;
}

AchievementItem::AchievementItem(vgui::Panel *parent, char const *panelName, const char *pszAchievementID, int achIndex)
	: vgui::Panel(parent, panelName)
{
	m_iAchievementIndex = achIndex;

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);

	SetScheme("BaseScheme");

	m_pImageIcon = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "Icon"));
	m_pImageIcon->SetShouldScaleImage(true);
	m_pImageIcon->SetZPos(-1);
	m_pImageIcon->SetImage("steam_default_avatar");

	m_pLabelTitle = vgui::SETUP_PANEL(new vgui::Label(this, "Title", ""));
	m_pLabelTitle->SetZPos(5);
	m_pLabelTitle->SetText("");

	m_pLabelDescription = vgui::SETUP_PANEL(new vgui::Label(this, "Description", ""));
	m_pLabelDescription->SetZPos(5);
	m_pLabelDescription->SetText("");

	m_pAchievementProgress = vgui::SETUP_PANEL(new ImageProgressBar(this, "ImageProgress", "vgui/achievements/achievement_progress_full", "vgui/achievements/achievement_progress_empty"));
	m_pAchievementProgress->SetZPos(10);

	m_pLabelProgress = vgui::SETUP_PANEL(new vgui::Label(this, "Progress", ""));
	m_pLabelProgress->SetZPos(12);
	m_pLabelProgress->SetText("");
	m_pLabelProgress->SetContentAlignment(Label::Alignment::a_center);

	m_pLabelDate = vgui::SETUP_PANEL(new vgui::Label(this, "Date", ""));
	m_pLabelDate->SetZPos(12);
	m_pLabelDate->SetText("");
	m_pLabelDate->SetContentAlignment(Label::Alignment::a_east);

	m_pLabelEXP = vgui::SETUP_PANEL(new vgui::Label(this, "Experience", ""));
	m_pLabelEXP->SetZPos(12);
	m_pLabelEXP->SetText("");
	m_pLabelEXP->SetContentAlignment(Label::Alignment::a_east);

	Q_strncpy(pszAchievementStringID, pszAchievementID, 80);

	InvalidateLayout();

	PerformLayout();

	m_flAchievementCheckTime = engine->Time() + 1.0f;
	m_bAchieved = false;

	if (steamapicontext && steamapicontext->SteamUserStats())
	{
		steamapicontext->SteamUserStats()->GetAchievement(pszAchievementStringID, &m_bAchieved);
		SetAchievementIcon();

		m_pLabelTitle->SetText(steamapicontext->SteamUserStats()->GetAchievementDisplayAttribute(pszAchievementStringID, "name"));
		m_pLabelDescription->SetText(steamapicontext->SteamUserStats()->GetAchievementDisplayAttribute(pszAchievementStringID, "desc"));
	}
}

AchievementItem::~AchievementItem()
{
}

void AchievementItem::OnUpdate(void)
{
	if (m_flAchievementCheckTime < engine->Time())
	{
		m_flAchievementCheckTime = engine->Time() + 1.0f;

		if (steamapicontext && steamapicontext->SteamUserStats())
		{
			bool bAchieved = false;
			steamapicontext->SteamUserStats()->GetAchievement(pszAchievementStringID, &bAchieved);
			if (bAchieved != m_bAchieved)
			{
				m_bAchieved = bAchieved;
				SetAchievementIcon();
			}

			const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(m_iAchievementIndex);
			if (pAchiev)
			{
				bool bBoolean = (!(pAchiev->szStat && pAchiev->szStat[0]));
				float flProgress = 0.0f;
				if (!bBoolean)
				{
					int iValue = 0;
					if (steamapicontext->SteamUserStats()->GetStat(pAchiev->szStat, &iValue))
					{
						iValue = clamp(iValue, 0, pAchiev->value);
						flProgress = ((float)iValue / (float)pAchiev->value);
					}

					m_pLabelProgress->SetText(VarArgs("%i / %i", iValue, pAchiev->value));
				}
				else if (m_bAchieved)
				{
					flProgress = 1.0f;
					m_pLabelProgress->SetText("1 / 1");
				}
				else
					m_pLabelProgress->SetText("0 / 1");

				m_pAchievementProgress->SetVisible(!bBoolean);
				m_pLabelProgress->SetVisible(!bBoolean);

				m_pAchievementProgress->SetProgress(flProgress);
				m_pLabelDate->SetText(GetUnlockDate());

				char pchXPLabel[16];
				Q_snprintf(pchXPLabel, 16, "%i XP", pAchiev->reward);
				m_pLabelEXP->SetText(pchXPLabel);
			}
		}
	}
}

const char *AchievementItem::GetUnlockDate(void)
{
	bool bAchieved = false;
	uint32 iActualTime = 0;
	steamapicontext->SteamUserStats()->GetAchievementAndUnlockTime(pszAchievementStringID, &bAchieved, &iActualTime);
	float flUnlockedTime = ((float)iActualTime);

	if ((iActualTime <= 0) || (bAchieved == false))
		return "";

	uint32 iYears = 1970;
	while (flUnlockedTime >= 31556926.0F)
	{
		flUnlockedTime -= 31556926.0F;
		iYears++;
	}

	uint32 iMonths = 1;
	while (flUnlockedTime >= 2629743.83F)
	{
		flUnlockedTime -= 2629743.83F;
		iMonths++;
	}

	uint32 iDays = 1;
	while (flUnlockedTime >= 86400.0F)
	{
		flUnlockedTime -= 86400.0F;
		iDays++;
	}

	if (iMonths < 10 && iDays < 10)
		return VarArgs("%lu.0%lu.0%lu", iYears, iMonths, iDays);
	else if (iMonths < 10)
		return VarArgs("%lu.0%lu.%lu", iYears, iMonths, iDays);
	else if (iDays < 10)
		return VarArgs("%lu.%lu.0%lu", iYears, iMonths, iDays);
	else
		return VarArgs("%lu.%lu.%lu", iYears, iMonths, iDays);
}

void AchievementItem::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	Color achTextColor = pScheme->GetColor("AchievementItemTextColor", Color(255, 255, 255, 255));
	Color achBgColor = pScheme->GetColor("AchievementItemBgColor", Color(35, 35, 35, 255));

	m_pLabelTitle->SetFont(pScheme->GetFont("OptionTextMedium"));
	m_pLabelTitle->SetFgColor(achTextColor);

	m_pLabelProgress->SetFont(pScheme->GetFont("OptionTextSmall"));
	m_pLabelProgress->SetFgColor(achTextColor);

	m_pLabelDescription->SetFont(pScheme->GetFont("OptionTextSmall"));
	m_pLabelDescription->SetFgColor(achTextColor);

	m_pLabelDate->SetFont(pScheme->GetFont("SpecText"));
	m_pLabelDate->SetFgColor(achTextColor);

	m_pLabelEXP->SetFont(pScheme->GetFont("SpecText"));
	m_pLabelEXP->SetFgColor(achTextColor);

	SetBgColor(achBgColor);
	SetFgColor(Color(0, 0, 0, 0));
}

void AchievementItem::SetSize(int wide, int tall)
{
	BaseClass::SetSize(wide, tall);

	int iconSize = GetTall() - scheme()->GetProportionalScaledValue(2);
	int iconPos = scheme()->GetProportionalScaledValue(1);
	m_pImageIcon->SetSize(iconSize, iconSize);
	m_pImageIcon->SetPos(iconPos, iconPos);

	int iOffset = scheme()->GetProportionalScaledValue(2);
	float flSize = ((float)tall);

	m_pLabelTitle->SetSize((wide - tall), (flSize * 0.35f));
	m_pLabelTitle->SetPos(tall + iOffset, 0);

	m_pLabelDescription->SetSize((wide - tall), (flSize * 0.20f));
	m_pLabelDescription->SetPos(tall + iOffset, (flSize * 0.35f));

	m_pLabelProgress->SetSize((wide - tall) - (iOffset * 3), (flSize * 0.25f));
	m_pLabelProgress->SetPos(tall + iOffset, (flSize * 0.65f));
	m_pLabelProgress->SetContentAlignment(Label::Alignment::a_center);

	m_pAchievementProgress->SetSize((wide - tall) - (iOffset * 3), (flSize * 0.25f));
	m_pAchievementProgress->SetPos(tall + iOffset, (flSize * 0.65f));

	m_pLabelDate->SetSize(((float)wide) * 40.0f, (flSize * 0.20f));
	m_pLabelDate->SetPos((wide - (((float)wide) * 40.0f)) - iOffset, iOffset);
	m_pLabelDate->SetContentAlignment(Label::Alignment::a_east);

	m_pLabelEXP->SetSize(((float)wide) * 40.0f, (flSize * 0.20f));
	m_pLabelEXP->SetPos((wide - (((float)wide) * 40.0f)) - iOffset, iOffset + scheme()->GetProportionalScaledValue(10));
	m_pLabelEXP->SetContentAlignment(Label::Alignment::a_east);
}

void AchievementItem::SetAchievementIcon(void)
{
	char pszAchievementImage[80], pszAchievementPath[128];
	Q_snprintf(pszAchievementImage, 80, "achievements/%s_%i", pszAchievementStringID, (m_bAchieved ? 1 : 0));
	Q_snprintf(pszAchievementPath, 128, "materials/vgui/%s.vmt", pszAchievementImage);

	Q_strlower(pszAchievementImage);
	Q_strlower(pszAchievementPath);

	if (filesystem->FileExists(pszAchievementPath, "MOD"))
		m_pImageIcon->SetImage(pszAchievementImage);
	else
		m_pImageIcon->SetImage("steam_default_avatar");
}

ProfileMenuAchievementPanel::ProfileMenuAchievementPanel(vgui::Panel *parent, char const *panelName) : BaseClass(parent, panelName, 0.5f)
{
	SetParent(parent);
	SetName(panelName);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetProportional(true);
	SetScheme("BaseScheme");

	m_pScrollBar = vgui::SETUP_PANEL(new vgui::ScrollBar(this, "ScrollBar", true));
	m_pScrollBar->SetZPos(20);
	m_pScrollBar->MoveToFront();
	m_pScrollBar->AddActionSignalTarget(this);

	CreateAchievementList();

	InvalidateLayout();
	PerformLayout();
}

ProfileMenuAchievementPanel::~ProfileMenuAchievementPanel()
{
	Cleanup();
}

void ProfileMenuAchievementPanel::CreateAchievementList(void)
{
	bool bOnlyAddHiddenAchievements = (pszAchievementList.Count() > 0);
	for (int i = 0; i < ACHIEVEMENTS::GetNumAchievements(); i++)
	{
		const achievementStatItem_t *pAchiev = ACHIEVEMENTS::GetAchievementItem(i);
		if (pAchiev == NULL)
			continue;

		if (pAchiev->hidden)
		{
			if (!(pAchiev->szAchievement && pAchiev->szAchievement[0]))
				continue;

			if (!steamapicontext || !steamapicontext->SteamUserStats())
				continue;

			bool bAchieved = false;
			steamapicontext->SteamUserStats()->GetAchievement(pAchiev->szAchievement, &bAchieved);
			if (!bAchieved)
				continue;
		}

		if (bOnlyAddHiddenAchievements)
		{
			if (!pAchiev->hidden)
				continue;

			if (HasAchievementInList(pszAchievementList, i))
				continue;
		}

		AchievementItem *pAchItem = vgui::SETUP_PANEL(new AchievementItem(this, "AchievementItem", pAchiev->szAchievement, i));
		pAchItem->SetZPos(10);
		pAchItem->MoveToFront();
		pszAchievementList.AddToTail(pAchItem);
	}
}

void ProfileMenuAchievementPanel::SetupLayout(void)
{
	BaseClass::SetupLayout();

	CreateAchievementList();

	int w, h;
	GetSize(w, h);

	float flWide = ((float)w) * 0.4f;
	for (int i = 0; i < pszAchievementList.Count(); i++)
		pszAchievementList[i]->SetSize(flWide, scheme()->GetProportionalScaledValue(42));

	int scrollSizeW = scheme()->GetProportionalScaledValue(8);
	m_pScrollBar->SetSize(scrollSizeW, h);
	m_pScrollBar->SetPos((w / 2) + (flWide / 2) + scheme()->GetProportionalScaledValue(1), 0);

	float rowsPerPage = (((float)GetTall()) / ((float)scheme()->GetProportionalScaledValue(42)));
	m_pScrollBar->SetRange(0, pszAchievementList.Count());
	m_pScrollBar->SetButtonPressedScrollValue(1);

	m_pScrollBar->SetRangeWindow((int)rowsPerPage);
	m_pScrollBar->SetValue(0);

	m_pScrollBar->InvalidateLayout();
	m_pScrollBar->Repaint();

	Redraw();
}

void ProfileMenuAchievementPanel::OnUpdate(bool bInGame)
{
	if (IsVisible())
	{
		for (int i = 0; i < pszAchievementList.Count(); i++)
			pszAchievementList[i]->OnUpdate();
	}
}

void ProfileMenuAchievementPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pScrollBar->SetBorder(NULL);
	m_pScrollBar->SetPaintBorderEnabled(false);
}

void ProfileMenuAchievementPanel::OnMouseWheeled(int delta)
{
	if (!m_pScrollBar->IsVisible() || !m_pScrollBar->IsEnabled())
		return;

	bool bIsScrollingUp = (delta > 0);

	int val = m_pScrollBar->GetValue();
	if (bIsScrollingUp)
	{
		if (val > 0)
			val -= 1;
	}
	else if (val < pszAchievementList.Count())
		val += 1;

	m_pScrollBar->SetValue(val);

	Redraw();
}

void ProfileMenuAchievementPanel::Redraw(void)
{
	float flPercent = ((float)m_pScrollBar->GetValue() / (float)pszAchievementList.Count());
	float flUnitsToMove = ((float)scheme()->GetProportionalScaledValue(42) * (float)pszAchievementList.Count()) * flPercent;

	int w, h;
	GetSize(w, h);

	float flWide = ((float)w) * 0.4f;
	for (int i = 0; i < pszAchievementList.Count(); i++)
		pszAchievementList[i]->SetPos((w / 2) - (flWide / 2), (scheme()->GetProportionalScaledValue(42) * i) - flUnitsToMove);
}

void ProfileMenuAchievementPanel::OnSliderMoved(int value)
{
	if (!m_pScrollBar->IsVisible() || !m_pScrollBar->IsEnabled())
		return;

	Redraw();
}

void ProfileMenuAchievementPanel::Cleanup(void)
{
	for (int i = 0; i < pszAchievementList.Count(); i++)
		delete pszAchievementList[i];

	pszAchievementList.Purge();
}
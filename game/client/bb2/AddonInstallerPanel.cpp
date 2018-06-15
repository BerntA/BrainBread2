//=========       Copyright © Reperio Studios 2013-2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Workshop client downloader and installer, triggered when you join a server hosting a workshop based map, auto install that workshop item and rejoin!
//
//========================================================================================//

#include "cbase.h"
#include "AddonInstallerPanel.h"
#include <vgui/ILocalize.h>
#include "GameBase_Client.h"
#include "GameBase_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CAddonInstallerPanel *m_pWorkshopInstaller = NULL;

CAddonInstallerPanel::CAddonInstallerPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "AddonInstaller"), m_CallbackWorkshopItemInstalled(this, &CAddonInstallerPanel::OnWorkshopItemInstalled)
{
	SetParent(parent);
	SetTitleBarVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(false);
	SetSizeable(false);
	SetMoveable(false);
	SetProportional(true);
	SetVisible(false);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetScheme("ClientScheme");
	SetZPos(100);

	m_pImgBackground = vgui::SETUP_PANEL(new vgui::ImagePanel(this, "BGImage"));
	m_pImgBackground->SetImage("mainmenu/backgroundart");
	m_pImgBackground->SetShouldScaleImage(true);
	m_pImgBackground->SetZPos(110);

	m_pTextAddonInfo = vgui::SETUP_PANEL(new vgui::Label(this, "TextTip", ""));
	m_pTextAddonInfo->SetZPos(130);
	m_pTextAddonInfo->SetText("");
	m_pTextAddonInfo->SetContentAlignment(Label::Alignment::a_center);

	m_pTextProgress = vgui::SETUP_PANEL(new vgui::Label(this, "TextProgress", ""));
	m_pTextProgress->SetZPos(150);
	m_pTextProgress->SetText("");

	m_pBlackDivider = vgui::SETUP_PANEL(new vgui::Divider(this, "Divider"));
	m_pBlackDivider->SetZPos(120);
	m_pBlackDivider->SetBorder(NULL);

	m_pProgressBar = vgui::SETUP_PANEL(new ImageProgressBar(this, "ProgressBar", "vgui/loading/progress_bar", "vgui/loading/progress_bg"));
	m_pProgressBar->SetProgressDirection(ProgressBar::PROGRESS_EAST);
	m_pProgressBar->SetZPos(135);

	InvalidateLayout();
	PerformLayout();

	m_pWorkshopInstaller = this;
	addonQueryHandle = NULL;
}

CAddonInstallerPanel::~CAddonInstallerPanel()
{
	m_pWorkshopInstaller = NULL;
	m_pTempMapItems.Purge();
}

void CAddonInstallerPanel::ShowAddonPanel(bool visible, PublishedFileId_t addon)
{
	Cleanup();
	m_flLastTimeFetchedProgress = 0.0f;

	if (visible && steamapicontext && steamapicontext->SteamUGC())
	{
		SetVisible(true);
		SetupLayout();
		Activate();
		RequestFocus();
		MoveToFront();

		m_pProgressBar->SetProgress(0.0f);
		m_pTextProgress->SetText("");
		m_pTextAddonInfo->SetText("#GameUI_AddonInstaller_Waiting");

		m_ulWorkshopItemID = addon;
		addonQueryHandle = steamapicontext->SteamUGC()->CreateQueryUGCDetailsRequest(&m_ulWorkshopItemID, 1);
		steamapicontext->SteamUGC()->SetReturnKeyValueTags(addonQueryHandle, true);
		SteamAPICall_t apiCallback = steamapicontext->SteamUGC()->SendQueryUGCRequest(addonQueryHandle);
		m_SteamCallResultUGCQuery.Set(apiCallback, this, &CAddonInstallerPanel::OnReceiveUGCQueryResults);
		return;
	}

	m_ulWorkshopItemID = 0;
	SetVisible(false);
}

void CAddonInstallerPanel::Cleanup(void)
{
	m_SteamCallResultUGCQuery.Cancel();
	m_pTempMapItems.Purge();
	if (addonQueryHandle != NULL)
	{
		steamapicontext->SteamUGC()->ReleaseQueryUGCRequest(addonQueryHandle);
		addonQueryHandle = NULL;
	}

	if (m_ulWorkshopItemID)
		steamapicontext->SteamUGC()->UnsubscribeItem(m_ulWorkshopItemID);
}

void CAddonInstallerPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_pTextAddonInfo->SetFgColor(pScheme->GetColor("LoadingTipTextColor", Color(255, 255, 255, 255)));
	m_pTextAddonInfo->SetFont(pScheme->GetFont("LoadingTip", true));

	m_pTextProgress->SetFgColor(pScheme->GetColor("LoadingProgressTextColor", Color(255, 255, 255, 255)));
	m_pTextProgress->SetFont(pScheme->GetFont("BB2_PANEL", true));

	m_pBlackDivider->SetBorder(NULL);
	m_pBlackDivider->SetFgColor(pScheme->GetColor("LoadingTipBackgroundFg", Color(0, 0, 0, 255)));
	m_pBlackDivider->SetBgColor(pScheme->GetColor("LoadingTipBackgroundBg", Color(0, 0, 0, 255)));
}

void CAddonInstallerPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	if (code == KEY_ESCAPE)
		ShowAddonPanel(false, 0);
	else
		BaseClass::OnKeyCodeTyped(code);
}

void CAddonInstallerPanel::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBorderEnabled(false);
	BaseClass::PaintBackground();
}

void CAddonInstallerPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	SetupLayout();
}

void CAddonInstallerPanel::SetupLayout(void)
{
	SetPos(0, 0);
	SetSize(ScreenWidth(), ScreenHeight());

	float flScale = (float)((float)ScreenWidth() * 0.1);
	m_pProgressBar->SetSize((ScreenWidth() - flScale), scheme()->GetProportionalScaledValue(16));
	m_pTextProgress->SetSize((ScreenWidth() - flScale), scheme()->GetProportionalScaledValue(16));
	m_pTextProgress->SetContentAlignment(Label::a_center);

	flScale = (float)((float)ScreenHeight() * 0.1);
	m_pBlackDivider->SetSize(ScreenWidth(), flScale);

	m_pImgBackground->SetSize(ScreenWidth(), (ScreenHeight() - flScale));
	m_pImgBackground->SetPos(0, 0);

	m_pTextAddonInfo->SetSize(ScreenWidth(), flScale);
	m_pTextAddonInfo->SetPos(0, (ScreenHeight() - flScale));
	m_pBlackDivider->SetPos(0, (ScreenHeight() - flScale));

	float flX = (float)((float)ScreenWidth() * 0.05);
	m_pProgressBar->SetPos(flX, (ScreenHeight() - flScale - scheme()->GetProportionalScaledValue(16)));
	m_pTextProgress->SetPos(flX, (ScreenHeight() - flScale - scheme()->GetProportionalScaledValue(34)));
}

void CAddonInstallerPanel::OnThink()
{
	BaseClass::OnThink();

	if (m_flLastTimeFetchedProgress < engine->Time())
	{
		m_flLastTimeFetchedProgress = engine->Time() + 0.1f;

		if (m_ulWorkshopItemID)
		{
			uint64 bytesReceived = 0, totalBytesToReceive = 0;
			if (steamapicontext->SteamUGC()->GetItemDownloadInfo(m_ulWorkshopItemID, &bytesReceived, &totalBytesToReceive))
			{
				if (totalBytesToReceive)
				{
					wchar_t wszBytesReceived[80], wszBytesToReceive[80], wszProgress[512];
					V_swprintf_safe(wszBytesReceived, L"%llu", bytesReceived);
					V_swprintf_safe(wszBytesToReceive, L"%llu", totalBytesToReceive);

					g_pVGuiLocalize->ConstructString(wszProgress, sizeof(wszProgress), g_pVGuiLocalize->Find("#GameUI_AddonInstaller_Progress"), 2, wszBytesReceived, wszBytesToReceive);
					m_pTextProgress->SetText(wszProgress);

					float percent = ((double)bytesReceived) / ((double)totalBytesToReceive);
					percent = clamp(percent, 0.0f, 1.0f);
					m_pProgressBar->SetProgress(percent);
				}
			}
		}
	}
}

void CAddonInstallerPanel::OnWorkshopItemInstalled(ItemInstalled_t *pParam)
{
	if (pParam->m_unAppID != steamapicontext->SteamUtils()->GetAppID())
		return;

	// Load the addon:

	char pszFullPath[1024], pszMapToEnter[MAX_MAP_NAME]; pszMapToEnter[0] = 0;
	Q_snprintf(pszFullPath, 1024, "../../workshop/content/346330/%llu/", m_ulWorkshopItemID);

	filesystem->AddSearchPath(pszFullPath, "MOD", PATH_ADD_TO_HEAD);
	filesystem->AddSearchPath(pszFullPath, "GAME", PATH_ADD_TO_HEAD);

	if (GameBaseShared() && GameBaseShared()->GetSharedMapData())
	{
		int mapCount = m_pTempMapItems.Count();
		if (mapCount)
		{
			Q_strncpy(pszMapToEnter, m_pTempMapItems[0].pszMapName, MAX_MAP_NAME);
			for (int i = 0; i < mapCount; i++)
			{
				GameBaseShared()->GetSharedMapData()->GetMapImageData(m_pTempMapItems[i].pszMapName, m_pTempMapItems[i]);
				GameBaseShared()->GetSharedMapData()->pszGameMaps.AddToTail(m_pTempMapItems[i]);
			}
		}

		m_pTempMapItems.Purge();
	}

	m_ulWorkshopItemID = 0;
	ShowAddonPanel(false, 0);
	GameBaseClient->Changelevel(pszMapToEnter);
	engine->ClientCmd_Unrestricted("retry\n");
}

void CAddonInstallerPanel::OnReceiveUGCQueryResults(SteamUGCQueryCompleted_t *pCallback, bool bIOFailure)
{
	if ((pCallback->m_eResult == k_EResultOK) && !bIOFailure && GameBaseShared() && GameBaseShared()->GetSharedMapData())
	{
		for (uint32 i = 0; i < pCallback->m_unNumResultsReturned; ++i)
		{
			SteamUGCDetails_t WorkshopItem;
			if (steamapicontext->SteamUGC()->GetQueryUGCResult(pCallback->m_handle, i, &WorkshopItem))
			{
				// This is not a map item therefore we skip it...
				if (!Q_strstr(WorkshopItem.m_rgchTags, "Elimination") &&
					!Q_strstr(WorkshopItem.m_rgchTags, "Arena") &&
					!Q_strstr(WorkshopItem.m_rgchTags, "Classic") &&
					!Q_strstr(WorkshopItem.m_rgchTags, "Objective") &&
					!Q_strstr(WorkshopItem.m_rgchTags, "Story Mode") &&
					!Q_strstr(WorkshopItem.m_rgchTags, "Deathmatch") &&
					!Q_strstr(WorkshopItem.m_rgchTags, "Custom"))
					continue;

				wchar_t addonName[64], downloadInfo[512];
				g_pVGuiLocalize->ConvertANSIToUnicode(WorkshopItem.m_rgchTitle, addonName, sizeof(addonName));
				g_pVGuiLocalize->ConstructString(downloadInfo, sizeof(downloadInfo), g_pVGuiLocalize->Find("#GameUI_AddonInstaller_Info"), 1, addonName);
				m_pTextAddonInfo->SetText(downloadInfo);

				uint32 uAmountOfKeys = steamapicontext->SteamUGC()->GetQueryUGCNumKeyValueTags(pCallback->m_handle, i);
				for (uint32 items = 0; items < uAmountOfKeys; items += 2)
				{
					int iVerification = MAP_VERIFIED_UNKNOWN;
					if (Q_strstr(WorkshopItem.m_rgchTags, "Whitelisted"))
						iVerification = MAP_VERIFIED_WHITELISTED;

					char mapNameKey[32], mapNameValue[32], mapSizeKey[32], mapSizeValue[128];
					steamapicontext->SteamUGC()->GetQueryUGCKeyValueTag(pCallback->m_handle, i, items, mapNameKey, 32, mapNameValue, 32);
					steamapicontext->SteamUGC()->GetQueryUGCKeyValueTag(pCallback->m_handle, i, (items + 1), mapSizeKey, 32, mapSizeValue, 128);

					if (strlen(mapNameValue) <= 0)
						continue;

					int iExistingMapIndex = GameBaseShared()->GetSharedMapData()->GetMapIndex(mapNameValue);
					if (iExistingMapIndex == -1)
					{
						gameMapItem_t itemMapDataItem;
						Q_strncpy(itemMapDataItem.pszMapTitle, ((strlen(WorkshopItem.m_rgchTitle) <= 0) ? mapNameValue : WorkshopItem.m_rgchTitle), 32);
						Q_strncpy(itemMapDataItem.pszMapDescription, WorkshopItem.m_rgchDescription, 256);
						Q_strncpy(itemMapDataItem.pszMapExtraInfo, "", 256);
						itemMapDataItem.iMapVerification = iVerification;
						itemMapDataItem.workshopID = WorkshopItem.m_nPublishedFileId;
						itemMapDataItem.bExclude = false;
						itemMapDataItem.flScore = WorkshopItem.m_flScore;
						Q_strncpy(itemMapDataItem.pszMapName, mapNameValue, 32);
						m_pTempMapItems.AddToTail(itemMapDataItem);
					}
				}
			}
		}

		steamapicontext->SteamUGC()->SubscribeItem(m_ulWorkshopItemID);
	}
	else
		ShowAddonPanel(false, 0);

	if (addonQueryHandle)
	{
		steamapicontext->SteamUGC()->ReleaseQueryUGCRequest(addonQueryHandle);
		addonQueryHandle = NULL;
	}
}

CON_COMMAND_F(workshop_client_install, "Install the desired workshop item for the client.", FCVAR_HIDDEN)
{
	if (args.ArgC() != 2 || engine->IsInGame() || (m_pWorkshopInstaller == NULL) || !steamapicontext || !steamapicontext->SteamUGC())
		return;

	unsigned long long workshopID = ((unsigned long long)atoll(args[1]));
	if (workshopID <= 0)
		return;

	int itemState = steamapicontext->SteamUGC()->GetItemState((PublishedFileId_t)workshopID);
	if (itemState & (k_EItemStateSubscribed | k_EItemStateInstalled))
	{
		Warning("You're already subscribed to this item!\n");
		return;
	}

	GameBaseClient->CloseConsole();
	m_pWorkshopInstaller->ShowAddonPanel(true, (PublishedFileId_t)workshopID);
}
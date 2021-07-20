//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 HTML Handler.
//
//========================================================================================//

#include "cbase.h"
#include "html_data_handler.h"
#include "convar.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "GameChecksumManager.h"

#ifndef OSX
#include "../../../thirdparty/curl/curl.h"
#endif

ConVar bb2_enable_ban_list("bb2_enable_ban_list", "1", FCVAR_GAMEDLL, "Enable or Disable the official ban list?", true, 0.0f, true, 1.0f);
static ConVar bb2_debug_libcurl("bb2_debug_libcurl", "0", FCVAR_GAMEDLL | FCVAR_HIDDEN, "Enable Verboseness for LIBCURL.", true, 0.0f, true, 1.0f);
static ConVar bb2_libcurl_timeout("bb2_libcurl_timeout", "0", FCVAR_GAMEDLL | FCVAR_HIDDEN, "Set LIBCURL timeout time.");

void EnumerateSoundScriptFolder(FileHandle_t fileInfo, const char *path, const char *targetFolder)
{
	if (fileInfo == FILESYSTEM_INVALID_HANDLE)
		return;

	char pszNextTargetFolder[64];
	pszNextTargetFolder[0] = 0;

	char pszFileOrFolder[128];
	pszFileOrFolder[0] = 0;

	FileFindHandle_t findHandle;
	const char *pFileName = NULL;
	pFileName = filesystem->FindFirst(path, &findHandle);
	while (pFileName != NULL)
	{
		if (strlen(pFileName) > 2)
		{
			if (filesystem->FindIsDirectory(findHandle))
			{
				Q_snprintf(pszNextTargetFolder, 64, "%s/%s", targetFolder, pFileName);
				Q_snprintf(pszFileOrFolder, 128, "data/soundscripts/%s/%s/*.*", targetFolder, pFileName);
				EnumerateSoundScriptFolder(fileInfo, pszFileOrFolder, pszNextTargetFolder);
			}
			else
			{
				char pszFile[128];
				Q_snprintf(pszFile, 128, "data/soundscripts/%s/%s", targetFolder, pFileName);

				char pszContent[256];
				Q_snprintf(pszContent, 256, "    \"precache_file\" \"%s\"\n", pszFile);
				g_pFullFileSystem->Write(&pszContent, strlen(pszContent), fileInfo);
			}
		}

		pFileName = filesystem->FindNext(findHandle);
	}
	filesystem->FindClose(findHandle);
}

void RecreateSoundScriptsManifest(void)
{
	// Initialize sound script manifest:
	FileHandle_t soundManifest = g_pFullFileSystem->Open("scripts/game_sounds_manifest.txt", "w");
	if (soundManifest != FILESYSTEM_INVALID_HANDLE)
	{
		char pszFileBase[32];
		Q_snprintf(pszFileBase, 32,
			"game_sounds_manifest\n"
			"{\n"
			);

		g_pFullFileSystem->Write(&pszFileBase, strlen(pszFileBase), soundManifest);

		FileFindHandle_t findHandle;
		const char *pFileName = NULL;
		pFileName = filesystem->FindFirst("data/soundscripts/*.*", &findHandle);
		while (pFileName != NULL)
		{
			if (strlen(pFileName) > 2)
			{
				if (filesystem->FindIsDirectory(findHandle))
				{
					char pszFullPath[128], pszFolder[64];
					Q_snprintf(pszFullPath, 128, "data/soundscripts/%s/*.*", pFileName);
					Q_strncpy(pszFolder, pFileName, 64);
					EnumerateSoundScriptFolder(soundManifest, pszFullPath, pszFolder);
				}
				else
				{
					char pszFile[128];
					Q_snprintf(pszFile, 128, "data/soundscripts/%s", pFileName);

					char pszContent[256];
					Q_snprintf(pszContent, 256, "    \"precache_file\" \"%s\"\n", pszFile);
					g_pFullFileSystem->Write(&pszContent, strlen(pszContent), soundManifest);
				}
			}

			pFileName = filesystem->FindNext(findHandle);
		}
		filesystem->FindClose(findHandle);

		char pszFileEnd[16];
		Q_snprintf(pszFileEnd, 16, "}");
		g_pFullFileSystem->Write(&pszFileEnd, strlen(pszFileEnd), soundManifest);

		g_pFullFileSystem->Close(soundManifest);
	}
}

#ifndef OSX
static int g_iActiveItemType = 0;
static CUtlStringList htmlDataList;

size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{
	htmlDataList.AddToTail(buf);
	return size * nmemb;
}

bool ParseHTML(const char *url)
{
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);

	if (bb2_debug_libcurl.GetBool())
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	if (bb2_libcurl_timeout.GetInt()) // Not advisable ? ? ?	
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, ((long)bb2_libcurl_timeout.GetInt()));

	CURLcode result = curl_easy_perform(curl);
	if (result == CURLE_OK)
	{
		char pchTemp[4096];
		pchTemp[0] = 0;

		for (int i = 0; i < htmlDataList.Count(); i++)
			Q_strncat(pchTemp, htmlDataList[i], sizeof(pchTemp));

		KeyValues *pkvData = new KeyValues("HTMLData");
		if (pkvData->LoadFromBuffer("ReperioData", pchTemp, filesystem, "MOD"))
		{
			for (KeyValues *sub = pkvData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				GameBaseServer()->AddItemToSharedList(sub->GetString(), g_iActiveItemType);
		}
		pkvData->deleteThis();
	}
	else
		Warning("CURL Error: %i\n", result);

	htmlDataList.RemoveAll();
	curl_easy_cleanup(curl);

	return (result == CURLE_OK);
}
#endif

void LoadSharedData(void)
{
	GameBaseServer()->LoadSharedInfo();

#ifndef OSX		
	if (GameBaseServer()->IsUsingDBSystem())
	{
		KeyValues *pkvMiscData = GetChecksumKeyValue("Tags");
		GameBaseServer()->SetServerBlacklisted(!pkvMiscData);
		if (!pkvMiscData)
		{
			Msg("Unable to locate/read game data tags, server will be blacklisted until this issue has been resolved.\n");
			return;
		}

		if (pkvMiscData->GetInt("disabled", 0) >= 1)
		{
			Msg("Game tags and such has been disabled!\n");
			return;
		}

		g_iActiveItemType = 0;
		for (KeyValues *sub = pkvMiscData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			if (!ParseHTML(sub->GetString()))
				Msg("Unable to parse the %s!\n", sub->GetName());

			g_iActiveItemType++;
		}
	}
#else
	Warning("OSX doesn't support LIBCURL at this time...\n");
#endif
}
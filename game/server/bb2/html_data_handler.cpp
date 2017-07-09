//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 HTML Handler.
//
//========================================================================================//

#include "cbase.h"
#include "html_data_handler.h"
#include "convar.h"
#include "GameBase_Server.h"
#include "filesystem.h"
#include "KeyValues.h"

#ifndef OSX
#include "../../../thirdparty/curl/curl.h"
#include <string>
#endif

ConVar bb2_enable_ban_list("bb2_enable_ban_list", "1", FCVAR_REPLICATED, "Enable or Disable the official ban list?", true, 0.0f, true, 1.0f);

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
int g_iActiveItemType = 0;
std::string htmlData;

const char *pchDataURLs[5] =
{
	"http://reperio-studios.eu/gamedata/brainbread2/developers.txt",
	"http://reperio-studios.eu/gamedata/brainbread2/donators.txt",
	"http://reperio-studios.eu/gamedata/brainbread2/testers.txt",
	"http://reperio-studios.eu/gamedata/brainbread2/bans.txt",
	"http://reperio-studios.eu/gamedata/brainbread2/blacklistedservers.txt",
};

size_t writeCallback(char* buf, size_t size, size_t nmemb, void* up)
{
	for (size_t c = 0; c < size*nmemb; c++)
		htmlData.push_back(buf[c]);

	return size * nmemb;
}

void ParseHTML(const char *url)
{
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	if (curl_easy_perform(curl) != CURLE_OK)
		Msg("Unable to parse the url: '%s'!\n", url);
	else
	{
		KeyValues *pkvData = new KeyValues("HTMLData");
		if (pkvData->LoadFromBuffer("ReperioData", htmlData.c_str(), filesystem, "MOD"))
		{
			for (KeyValues *sub = pkvData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				GameBaseServer()->AddItemToSharedList(sub->GetString(), g_iActiveItemType);
		}
		pkvData->deleteThis();
	}

	curl_easy_cleanup(curl);
	htmlData.clear();
}
#endif

void LoadSharedData(void)
{
	GameBaseServer()->LoadSharedInfo();

#ifndef OSX
	g_iActiveItemType = 0;
	if (GameBaseServer()->IsUsingDBSystem())
	{
		for (int i = 0; i < _ARRAYSIZE(pchDataURLs); i++)
		{
			g_iActiveItemType = i;
			ParseHTML(pchDataURLs[i]);
		}
	}
#else
	Warning("OSX doesn't support LIBCURL at this time...\n");
#endif
}
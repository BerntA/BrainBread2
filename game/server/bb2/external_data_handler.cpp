//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 External Data Handler - Uses libcurl to request data from various sources, such as dev tags for ex.
// Does not work fully on OSX due to libcurl compile issues!
//
//========================================================================================//

#include "cbase.h"
#include "external_data_handler.h"
#include "convar.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "GameChecksumManager.h"

#ifndef OSX

#if defined(_WIN32) && defined(USE_VS2022)
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE* __cdecl __iob_func(void) { return _iob; }
#endif

#include "curl/curl.h"

#endif

ConVar bb2_enable_ban_list("bb2_enable_ban_list", "1", FCVAR_GAMEDLL, "Enable or Disable the official ban list?", true, 0.0f, true, 1.0f);
static ConVar bb2_debug_libcurl("bb2_debug_libcurl", "0", FCVAR_GAMEDLL | FCVAR_HIDDEN, "Enable Verboseness for LIBCURL.", true, 0.0f, true, 1.0f);
static ConVar bb2_libcurl_timeout("bb2_libcurl_timeout", "0", FCVAR_GAMEDLL | FCVAR_HIDDEN, "Set LIBCURL timeout time.");

#ifndef OSX
#define MAX_BUFFER_SIZE 8192
static char g_pDataBuffer[MAX_BUFFER_SIZE];

static size_t DataWriteCallback(char* buf, size_t size, size_t nmemb, void* up)
{
	Q_strncat(g_pDataBuffer, buf, MAX_BUFFER_SIZE - strlen(g_pDataBuffer) - 1);
	return size * nmemb;
}

static bool CurlGetRequest(const char* url)
{
	if (!(url && url[0]))
		return false;

	g_pDataBuffer[0] = 0;

	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &DataWriteCallback);

	if (bb2_debug_libcurl.GetBool())
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	if (bb2_libcurl_timeout.GetInt()) // Not advisable ? ? ?	
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, ((long)bb2_libcurl_timeout.GetInt()));

	CURLcode result = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (result != CURLE_OK)
		Warning("CURL Error: %i\n", result);

	return (result == CURLE_OK);
}
#endif

void LoadSharedData(void)
{
	GameBaseServer()->LoadSharedInfo();

	if (HL2MPRules())
		HL2MPRules()->SetXPRate((GameBaseShared()->GetFestiveEvent() > FESTIVE_EVENT_NONE) ? 2.5f : 1.0f);

#ifndef OSX
	if (!engine->IsDedicatedServer())
		return;

	// Load reperio studios data.
	KeyValues* pkvMiscData = GetChecksumKeyValue("Tags");
	if (pkvMiscData)
	{
		int iActiveItemType = 0;
		for (KeyValues* sub = pkvMiscData->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			if (CurlGetRequest(sub->GetString()))
			{
				KeyValues* pkvData = new KeyValues("ExternalData");
				if (pkvData->LoadFromBuffer("ReperioData", g_pDataBuffer, filesystem, "MOD"))
				{
					for (KeyValues* pkvDataSub = pkvData->GetFirstSubKey(); pkvDataSub; pkvDataSub = pkvDataSub->GetNextKey())
						GameBaseServer()->AddItemToSharedList(pkvDataSub->GetString(), iActiveItemType);
				}
				pkvData->deleteThis();
			}
			else
				Msg("Unable to parse the %s!\n", sub->GetName());

			iActiveItemType++;
		}
	}
#endif
}
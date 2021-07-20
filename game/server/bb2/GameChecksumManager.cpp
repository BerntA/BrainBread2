//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: Game Checksum Manager - Load checksums from an encrypted file.
//
//========================================================================================//

#include "cbase.h"
#include "GameChecksumManager.h"
#include "GameBase_Shared.h"
#include "hl2mp_gamerules.h"

static KeyValues *g_pKVChecksums = NULL;
static const char *g_pFilesToCheck[] = {
	// Game Data
	"data/game/game_base_player_arena.txt",
	"data/game/game_base_player_deathmatch.txt",
	"data/game/game_base_player_elimination.txt",
	"data/game/game_base_player_objective.txt",
	"data/game/game_base_player_powerups.txt",
	"data/game/game_base_player_shared.txt",
	"data/game/game_base_shared.txt",
	"data/game/gamemode_shared.txt",

	// NPCs
	"data/npc/bandit.txt",
	"data/npc/bandit_leader.txt",
	"data/npc/bandit_johnson.txt",
	"data/npc/fred.txt",
	"data/npc/runner.txt",
	"data/npc/walker.txt",
	"data/npc/police.txt",
	"data/npc/swat.txt",
	"data/npc/riot.txt",
	"data/npc/military.txt",

	// Weapons
	"scripts/weapon_beretta.txt",
	"scripts/weapon_deagle.txt",
	"scripts/weapon_akimbo_beretta.txt",
	"scripts/weapon_glock17.txt",
	"scripts/weapon_akimbo_glock17.txt",
	"scripts/weapon_ak47.txt",
	"scripts/weapon_g36c.txt",
	"scripts/weapon_remington700.txt",
	"scripts/weapon_remington.txt",
	"scripts/weapon_benelli_m4.txt",
	"scripts/weapon_winchester1894.txt",
	"scripts/weapon_sawedoff.txt",
	"scripts/weapon_akimbo_sawedoff.txt",
	"scripts/weapon_mac11.txt",
	"scripts/weapon_mp7.txt",
	"scripts/weapon_mp5.txt",
	"scripts/weapon_microuzi.txt",
	"scripts/weapon_famas.txt",
	"scripts/weapon_minigun.txt",
	"scripts/weapon_flamethrower.txt",
	"scripts/weapon_rex.txt",
	"scripts/weapon_akimbo_rex.txt",
	"scripts/weapon_zombhands.txt",
	"scripts/weapon_hands.txt",
	"scripts/weapon_machete.txt",
	"scripts/weapon_baseballbat.txt",
	"scripts/weapon_brick.txt",
	"scripts/weapon_fireaxe.txt",
	"scripts/weapon_sledgehammer.txt",
	"scripts/weapon_frag.txt",
	"scripts/weapon_propane.txt",
	"scripts/weapon_hatchet.txt",
	"scripts/weapon_m9_bayonet.txt",
};

bool LoadChecksums()
{
	DeleteChecksums();

	if (GameBaseShared() == NULL)
		return false;

	g_pKVChecksums = GameBaseShared()->ReadEncryptedKeyValueFile(filesystem, "data/checksums", true);
	if (!g_pKVChecksums)
	{
		DeleteChecksums();
		Warning("Unable to read checksums! Cannot verify scripts, global stats and achievs will be disabled!\n");
		return false;
	}

	KeyValues *pkvScripts = g_pKVChecksums->FindKey("Scripts");
	if (!pkvScripts)
	{
		DeleteChecksums();
		Warning("Unable to read checksums! Cannot verify scripts, global stats and achievs will be disabled!\n");
		return false;
	}

	// Verify
	unsigned int validatedFiles = 0, i = 0;
	for (KeyValues *sub = pkvScripts->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		if (i >= _ARRAYSIZE(g_pFilesToCheck))
			break;

		const char *file = g_pFilesToCheck[i];
		const char *current_checksum = GameBaseShared()->GetFileChecksum(file);
		const char *wanted_checksum = sub->GetString();

		if (Q_strcmp(current_checksum, wanted_checksum) == 0)
			validatedFiles++;
		else
			Warning("Unable to verify checksum for %s!\n", file);

		i++;
	}

	if (validatedFiles != _ARRAYSIZE(g_pFilesToCheck))
	{
		DeleteChecksums();
		Warning("Could only verify checksum for %i of %i files, global stats and achievs will be disabled!\n", validatedFiles, _ARRAYSIZE(g_pFilesToCheck));
		return false;
	}

	Warning("Successfully verified the checksum of %i files!\n", _ARRAYSIZE(g_pFilesToCheck));
	return true; // Everything seems to be good!
}

bool IsChecksumsValid()
{
	return (g_pKVChecksums != NULL);
}

void DeleteChecksums()
{
	if (g_pKVChecksums)
		g_pKVChecksums->deleteThis();
	g_pKVChecksums = NULL;
}

KeyValues *GetChecksumKeyValue(const char *key)
{
	if (g_pKVChecksums && key && key[0])
		return g_pKVChecksums->FindKey(key);
	return NULL;
}
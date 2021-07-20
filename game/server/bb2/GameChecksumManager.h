//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: Game Checksum Manager - Load checksums from an encrypted file.
//
//========================================================================================//

#ifndef GAME_CHECKSUM_MGR_H
#define GAME_CHECKSUM_MGR_H

#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "filesystem.h"

bool LoadChecksums();
bool IsChecksumsValid();
void DeleteChecksums();
KeyValues *GetChecksumKeyValue(const char *key);

#endif // GAME_CHECKSUM_MGR_H
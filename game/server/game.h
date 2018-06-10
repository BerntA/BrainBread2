//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef GAME_H
#define GAME_H


#include "globals.h"

extern void GameDLLInit( void );

extern ConVar	displaysoundlist;
extern ConVar	mapcyclefile;
extern ConVar	servercfgfile;
extern ConVar	lservercfgfile;

// multiplayer server rules
extern ConVar bb2_spawn_protection;
extern ConVar	teamplay;
extern ConVar	fraglimit;
extern ConVar	falldamage;
extern ConVar	weaponstay;
extern ConVar	footsteps;
extern ConVar	decalfrequency;

// Engine Cvars
extern const ConVar *g_pDeveloper;
#endif		// GAME_H

//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 External Data Handler - Uses libcurl to request data from various sources, such as dev tags for ex.
//
//========================================================================================//

#ifndef EXTERNAL_DATA_HANDLER_H
#define EXTERNAL_DATA_HANDLER_H

#ifdef _WIN32
#pragma once
#endif

extern void LoadSharedData(void);
extern ConVar bb2_enable_ban_list;

#endif // EXTERNAL_DATA_HANDLER_H
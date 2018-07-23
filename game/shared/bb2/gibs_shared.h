//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Shared Gib Details!
// Setting the bodygroups with the value 1 means gibbed, 0 is non gibbed.
// Ragdoll info will be found here as well!
//
//========================================================================================//

#ifndef GIBS_SHARED_H
#define GIBS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
class C_ClientRagdollGib;

#include "c_baseanimating.h"
#include "c_basetempentity.h"
#else
#include "baseanimating.h"
#include "props.h"
#include "basetempentity.h"
#endif

#define GIB_BODYGROUP_BASE_HEAD "head"

#define GIB_BODYGROUP_BASE_ARM_LEFT "arms_left"
#define GIB_BODYGROUP_BASE_ARM_RIGHT "arms_right"

#define GIB_BODYGROUP_BASE_LEG_LEFT "legs_left"
#define GIB_BODYGROUP_BASE_LEG_RIGHT "legs_right"

#define GIB_EXTRA_PUSH_FORCE 2.0f
#define GIB_FADE_OUT_DELAY 4.0f

#define PLAYER_ACCESSORY_MAX 4

// basecombatcharacter needs to remember which parts we've gibbed.
enum
{
	GIB_NO_HEAD = 0x001,

	GIB_NO_ARM_LEFT = 0x002,
	GIB_NO_ARM_RIGHT = 0x004,

	GIB_NO_LEG_LEFT = 0x008,
	GIB_NO_LEG_RIGHT = 0x010,

	GIB_FULL_EXPLODE = 0x020,

	MAX_GIB_BITS = 6
};

enum
{
	GIB_NO_GIBS = 0,
	GIB_FULL_GIBS,
	GIB_EXPLODE_ONLY,
};

enum ClientSideGibTypes
{
	CLIENT_GIB_PROP = 0, // A prop physics like a helmet, door gibs, etc...
	CLIENT_GIB_RAGDOLL_NORMAL_PHYSICS, // A prop physics gib like a head (must be human parts)
	CLIENT_GIB_RAGDOLL, // A prop physics ragdoll gib, for example an arm, leg, etc...
	CLIENT_RAGDOLL, // Ragdoll - Can still be gibbed!
};

struct gibDataInfo
{
	const char *bodygroup;
	const char *attachmentName;
	int flag;
	int hitgroup;
};

struct gibSharedDataItem
{
	const char *soundscript;
	const char *attachmentName;
	const char *bodygroup;
	const char *limbName;
	bool bCanExplode;
	int gibFlag;
};

extern const char *PLAYER_BODY_ACCESSORY_BODYGROUPS[PLAYER_ACCESSORY_MAX];

#ifdef CLIENT_DLL
extern void OnClientPlayerRagdollSpawned(C_ClientRagdollGib *ragdoll, int gibFlags);
#endif

#endif // GIBS_SHARED_H
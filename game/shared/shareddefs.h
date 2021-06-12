//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Definitions that are shared by the game DLL and the client DLL.
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHAREDDEFS_H
#define SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif

#define TICK_INTERVAL			(gpGlobals->interval_per_tick)

#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

#define ANIMATION_CYCLE_BITS		15
#define ANIMATION_CYCLE_MINFRAC		(1.0f / (1<<ANIMATION_CYCLE_BITS))

// Each mod defines these for itself.
class CViewVectors
{
public:
	CViewVectors() {}

	CViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vSlideHullMin,
		Vector vSlideHullMax,
		Vector vSlideView
		)
	{
		m_vView = vView;
		m_vHullMin = vHullMin;
		m_vHullMax = vHullMax;
		m_vDuckHullMin = vDuckHullMin;
		m_vDuckHullMax = vDuckHullMax;
		m_vDuckView = vDuckView;
		m_vObsHullMin = vObsHullMin;
		m_vObsHullMax = vObsHullMax;
		m_vDeadViewHeight = vDeadViewHeight;
		m_vSlideHullMin = vSlideHullMin;
		m_vSlideHullMax = vSlideHullMax;
		m_vSlideView = vSlideView;
	}

	// Height above entity position where the viewer's eye is.
	Vector m_vView;
	
	Vector m_vHullMin;
	Vector m_vHullMax;
	
	Vector m_vDuckHullMin;
	Vector m_vDuckHullMax;
	Vector m_vDuckView;
	
	Vector m_vObsHullMin;
	Vector m_vObsHullMax;
	
	Vector m_vDeadViewHeight;

	Vector m_vSlideHullMin;
	Vector m_vSlideHullMax;
	Vector m_vSlideView;
};

// Height above entity position where the viewer's eye is.
#define VEC_VIEW			g_pGameRules->GetViewVectors()->m_vView
#define VEC_HULL_MIN		g_pGameRules->GetViewVectors()->m_vHullMin
#define VEC_HULL_MAX		g_pGameRules->GetViewVectors()->m_vHullMax

#define VEC_DUCK_HULL_MIN	g_pGameRules->GetViewVectors()->m_vDuckHullMin
#define VEC_DUCK_HULL_MAX	g_pGameRules->GetViewVectors()->m_vDuckHullMax
#define VEC_DUCK_VIEW		g_pGameRules->GetViewVectors()->m_vDuckView

#define VEC_SLIDE_HULL_MIN	g_pGameRules->GetViewVectors()->m_vSlideHullMin
#define VEC_SLIDE_HULL_MAX	g_pGameRules->GetViewVectors()->m_vSlideHullMax
#define VEC_SLIDE_VIEW		g_pGameRules->GetViewVectors()->m_vSlideView

#define VEC_OBS_HULL_MIN	g_pGameRules->GetViewVectors()->m_vObsHullMin
#define VEC_OBS_HULL_MAX	g_pGameRules->GetViewVectors()->m_vObsHullMax

#define VEC_DEAD_VIEWHEIGHT	g_pGameRules->GetViewVectors()->m_vDeadViewHeight

// If the player (enemy bots) are scaled, adjust the hull
#define VEC_VIEW_SCALED( player )				( g_pGameRules->GetViewVectors()->m_vView * player->GetModelScale() )
#define VEC_HULL_MIN_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vHullMin * player->GetModelScale() )
#define VEC_HULL_MAX_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vHullMax * player->GetModelScale() )

#define VEC_DUCK_HULL_MIN_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vDuckHullMin * player->GetModelScale() )
#define VEC_DUCK_HULL_MAX_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vDuckHullMax * player->GetModelScale() )
#define VEC_DUCK_VIEW_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vDuckView * player->GetModelScale() )

#define VEC_SLIDE_HULL_MIN_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vSlideHullMin * player->GetModelScale() )
#define VEC_SLIDE_HULL_MAX_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vSlideHullMax * player->GetModelScale() )
#define VEC_SLIDE_VIEW_SCALED( player )			( g_pGameRules->GetViewVectors()->m_vSlideView * player->GetModelScale() )

#define VEC_OBS_HULL_MIN_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vObsHullMin * player->GetModelScale() )
#define VEC_OBS_HULL_MAX_SCALED( player )		( g_pGameRules->GetViewVectors()->m_vObsHullMax * player->GetModelScale() )

#define VEC_DEAD_VIEWHEIGHT_SCALED( player )	( g_pGameRules->GetViewVectors()->m_vDeadViewHeight * player->GetModelScale() )

#define WATERJUMP_HEIGHT			8

#define MAX_CLIMB_SPEED		200

#define TIME_TO_DUCK		0.4
#define TIME_TO_DUCK_MS		400.0f

#define TIME_TO_UNDUCK		0.2
#define TIME_TO_UNDUCK_MS	200.0f

#define MAX_WEAPON_SLOTS		4	// hud item selection slots
#define MAX_WEAPONS				10	// Max number of weapons available

#define WEAPON_NOCLIP			-1	// clip sizes set to this tell the weapon it doesn't use a clip

#define	MAX_AMMO_TYPES	20		// ???
#define MAX_AMMO_SLOTS  20		// not really slots

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

//===================================================================================================================
// Close caption flags
#define CLOSE_CAPTION_WARNIFMISSING	( 1<<0 )
#define CLOSE_CAPTION_FROMPLAYER	( 1<<1 )
#define CLOSE_CAPTION_GENDER_MALE	( 1<<2 )
#define CLOSE_CAPTION_GENDER_FEMALE	( 1<<3 )

//===================================================================================================================
// Hud Element hiding flags
#define	HIDEHUD_WEAPONSELECTION		( 1<<0 )	// Hide ammo count & weapon selection
#define	HIDEHUD_FLASHLIGHT			( 1<<1 )
#define	HIDEHUD_ALL					( 1<<2 )
#define HIDEHUD_HEALTH				( 1<<3 )	// Hide health & armor / suit battery
#define HIDEHUD_PLAYERDEAD			( 1<<4 )	// Hide when local player's dead
#define HIDEHUD_MISCSTATUS			( 1<<5 )	// Hide miscellaneous status elements (trains, pickup history, death notices, etc)
#define HIDEHUD_CHAT				( 1<<6 )	// Hide all communication elements (saytext, voice icon, etc)
#define	HIDEHUD_CROSSHAIR			( 1<<7 )	// Hide crosshairs
#define HIDEHUD_SCOREBOARD		    ( 1<<8 )	// Hide HUD when the scoreboard is visible.
#define HIDEHUD_ZOMBIEMODE			( 1<<9 ) // Hide when we're a zombie...
#define HIDEHUD_ROUNDSTARTING		( 1<<10 ) // Hide when we're waiting for round start / displaying scores.
#define HIDEHUD_INWEPSELECTION		( 1<<11 ) // Hide when we're selecting weapons.

#define HIDEHUD_BITCOUNT			12

// BB2 
#define MAX_TEAMMATE_DISTANCE 500.0f
#define MAX_GLOW_RADIUS_DIST 100.0f

//===================================================================================================================
// Player Defines

// Max number of players in a game ( see const.h for ABSOLUTE_PLAYER_LIMIT (256 ) )
// The Source engine is really designed for 32 or less players.  If you raise this number above 32, you better know what you are doing
//  and have a good answer for a bunch of perf question related to player simulation, thinking logic, tracelines, networking overhead, etc.
// But if you are brave or are doing something interesting, go for it...   ywb 9/22/03

//You might be wondering why these aren't multiple of 2. Well the reason is that if servers decide to have HLTV or Replay enabled we need the extra slot.
//This is ok since MAX_PLAYERS is used for code specific things like arrays and loops, but it doesn't really means that this is the max number of players allowed
//Since this is decided by the gamerules (and it can be whatever number as long as its less than MAX_PLAYERS).

#define MAX_PLAYERS				13  // Absolute max players supported

#define MAX_FOV						90

//===================================================================================================================
// Team Defines
#define TEAM_ANY				-2
#define	TEAM_INVALID			-1
#define TEAM_UNASSIGNED			0	// not assigned to a team
#define TEAM_SPECTATOR			1	// spectator team
// Start your team numbers after this
#define LAST_SHARED_TEAM		TEAM_SPECTATOR

// The first team that's game specific (i.e. not unassigned / spectator)
#define FIRST_GAME_TEAM			(LAST_SHARED_TEAM+1)

#define MAX_TEAMS				5	// Max number of teams in a game
#define MAX_TEAM_NAME_LENGTH	32	// Max length of a team's name

#define WEAPON_NOT_CARRIED				0	// Weapon is on the ground
#define WEAPON_IS_CARRIED_BY_PLAYER		1	// This client is carrying this weapon.
#define WEAPON_IS_ACTIVE				2	// This client is carrying this weapon and it's the currently held weapon

// Weapon flags
// -----------------------------------------
//	Flags - NOTE: KEEP g_ItemFlags IN WEAPON_PARSE.CPP UPDATED WITH THESE
// -----------------------------------------
#define ITEM_FLAG_SELECTONEMPTY		(1<<0)
#define ITEM_FLAG_NOAUTORELOAD		(1<<1)
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	(1<<2)
#define ITEM_FLAG_LIMITINWORLD		(1<<3)
#define ITEM_FLAG_EXHAUSTIBLE		(1<<4)	// A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_DOHITLOCATIONDMG	(1<<5)	// This weapon take hit location into account when applying damage
#define ITEM_FLAG_NOAMMOPICKUPS		(1<<6)	// Don't draw ammo pickup sprites/sounds when ammo is received
#define ITEM_FLAG_NOITEMPICKUP		(1<<7)	// Don't draw weapon pickup when this weapon is picked up by the player
// NOTE: KEEP g_ItemFlags IN WEAPON_PARSE.CPP UPDATED WITH THESE

#define MAX_BEAM_ENTS			10

#define TRACER_TYPE_DEFAULT		0x00000001
#define TRACER_TYPE_GUNSHIP		0x00000002
#define TRACER_TYPE_STRIDER		0x00000004 // Here ya go, Jay!
#define TRACER_TYPE_GAUSS		0x00000008
#define TRACER_TYPE_WATERBULLET	0x00000010

#define MUZZLEFLASH_TYPE_DEFAULT	0x00000001
#define MUZZLEFLASH_TYPE_GUNSHIP	0x00000002
#define MUZZLEFLASH_TYPE_STRIDER	0x00000004

// Muzzle flash definitions (for the flags field of the "MuzzleFlash" DispatchEffect)
enum
{
	MUZZLEFLASH_AR2				= 0,
	MUZZLEFLASH_SHOTGUN,
	MUZZLEFLASH_SMG1,
	MUZZLEFLASH_SMG2,
	MUZZLEFLASH_PISTOL,
	MUZZLEFLASH_COMBINE,
	MUZZLEFLASH_357,
	MUZZLEFLASH_RPG,
	MUZZLEFLASH_COMBINE_TURRET,

	MUZZLEFLASH_FIRSTPERSON		= 0x100,
};

// Tracer Flags
#define TRACER_FLAG_WHIZ			0x0001
#define TRACER_FLAG_USEATTACHMENT	0x0002

#define TRACER_DONT_USE_ATTACHMENT	-1

// Entity Dissolve types
enum
{
	ENTITY_DISSOLVE_NORMAL = 0,
	ENTITY_DISSOLVE_ELECTRICAL,
	ENTITY_DISSOLVE_ELECTRICAL_LIGHT,
	ENTITY_DISSOLVE_CORE,

	// NOTE: Be sure to up the bits if you make more dissolve types
	ENTITY_DISSOLVE_BITS = 3
};

// ---------------------------
//  Hit Group standards
// ---------------------------
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)

// HL2 has 600 gravity by default
// NOTE: The discrete ticks can have quantization error, so these numbers are biased a little to
// make the heights more exact
#define PLAYER_FATAL_FALL_SPEED		922.5f // approx 60 feet sqrt( 2 * gravity * 60 * 12 )
#define PLAYER_MAX_SAFE_FALL_SPEED	385.0f // approx 20 feet sqrt( 2 * gravity * 20 * 12 )
#define PLAYER_LAND_ON_FLOATING_OBJECT	173 // Can fall another 173 in/sec without getting hurt
#define PLAYER_MIN_BOUNCE_SPEED		173
#define PLAYER_FALL_PUNCH_THRESHOLD 303.0f // won't punch player's screen/make scrape noise unless player falling at least this fast - at least a 76" fall (sqrt( 2 * g * 76))
#define DAMAGE_FOR_FALL_SPEED		100.0f / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED ) // damage per unit per second.

// instant damage

// For a means of resolving these consts into debug string text, see function
// CTakeDamageInfo::DebugGetDamageTypeString(unsigned int DamageType, char *outbuf, unsigned int outbuflength )
#define DMG_GENERIC			0			// generic damage -- do not use if you want players to flinch and bleed!
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object. 
										// NOTE: It's assumed crush damage is occurring as a result of physics collision, so no extra physics force is generated by crush damage.
										// DON'T use DMG_CRUSH when damaging entities unless it's the result of a physics collision. You probably want DMG_CLUB instead.
#define DMG_BULLET			(1 << 1)	// shot
#define DMG_SLASH			(1 << 2)	// cut, clawed, stabbed
#define DMG_BURN			(1 << 3)	// heat burned
//#define 			(1 << 4)	// FREE
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_BLAST			(1 << 6)	// explosive blast damage
#define DMG_CLUB			(1 << 7)	// crowbar, punch, headbutt
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_SONIC			(1 << 9)	// sound pulse shockwave
#define DMG_ENERGYBEAM		(1 << 10)	// laser or other high energy beam 
#define DMG_PREVENT_PHYSICS_FORCE		(1 << 11)	// Prevent a physics force 
#define DMG_NEVERGIB		(1 << 12)	// with this bit OR'd in, no damage type will be able to gib victims upon death
#define DMG_ALWAYSGIB		(1 << 13)	// with this bit OR'd in, any damage type can be made to gib victims upon death.
#define DMG_DROWN			(1 << 14)	// Drowning


#define DMG_PARALYZE		(1 << 15)	// slows affected creature down
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_POISON			(1 << 17)	// blood poisoning - heals over time like drowning damage
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven

#define DMG_REMOVENORAGDOLL	(1<<22)		// with this bit OR'd in, no ragdoll will be created, and the target will be quietly removed.
										// use this to kill an entity that you've already got a server-side ragdoll for

#define DMG_PHYSGUN			(1<<23)		// Hit by manipulator. Usually doesn't do any damage.
#define DMG_PLASMA			(1<<24)		// Shot by Cremator
#define DMG_AIRBOAT			(1<<25)		// Hit by the airboat's gun

#define DMG_DISSOLVE		(1<<26)		// Dissolving!
#define DMG_BLAST_SURFACE	(1<<27)		// A blast on the surface of water that cannot harm things underwater
#define DMG_DIRECT			(1<<28)
#define DMG_BUCKSHOT		(1<<29)		// not quite a bullet. Little, rounder, different.
#define DMG_ZOMBIE          (1<<30)     // Scratches / BIG Slash

// NOTE: DO NOT ADD ANY MORE CUSTOM DMG_ TYPES. MODS USE THE DMG_LASTGENERICFLAG BELOW, AND
//		 IF YOU ADD NEW DMG_ TYPES, THEIR TYPES WILL BE HOSED. WE NEED A BETTER SOLUTION.

// TODO: keep this up to date so all the mod-specific flags don't overlap anything.
#define DMG_LASTGENERICFLAG	DMG_ZOMBIE



// settings for m_takedamage
#define	DAMAGE_NO				0
#define DAMAGE_EVENTS_ONLY		1		// Call damage functions, but don't modify health
#define	DAMAGE_YES				2
#define	DAMAGE_AIM				3

// Spectator Movement modes
enum {
	OBS_MODE_NONE = 0,	// not in spectator mode
	OBS_MODE_DEATHCAM,	// special mode for death cam animation
	OBS_MODE_FREEZECAM,	// zooms to a target, and freeze-frames on them
	OBS_MODE_FIXED,		// view from a fixed camera position
	OBS_MODE_IN_EYE,	// follow a player in first person view
	OBS_MODE_CHASE,		// follow a player in third person view
	OBS_MODE_ROAMING,	// free roaming

	NUM_OBSERVER_MODES,
};

#define LAST_PLAYER_OBSERVERMODE	OBS_MODE_ROAMING

// Force Camera Restrictions with mp_forcecamera
enum {
	OBS_ALLOW_ALL = 0,	// allow all modes, all targets
	OBS_ALLOW_TEAM,		// allow only own team & first person, no PIP
	OBS_ALLOW_NONE,		// don't allow any spectating after death (fixed & fade to black)

	OBS_ALLOW_NUM_MODES,
};

enum
{
	TYPE_TEXT = 0,	// just display this plain text
	TYPE_INDEX,		// lookup text & title in stringtable
	TYPE_URL,		// show this URL
	TYPE_FILE,		// show this local file
} ;

//=============================================================================
// HPE_BEGIN:
// [Forrest] Replaced text window command string with TEXTWINDOW_CMD enumeration
// of options.  Passing a command string is dangerous and allowed a server network
// message to run arbitrary commands on the client.
//=============================================================================
enum
{
	TEXTWINDOW_CMD_NONE = 0,
	TEXTWINDOW_CMD_JOINGAME,
	TEXTWINDOW_CMD_CHANGETEAM,
	TEXTWINDOW_CMD_IMPULSE101,
	TEXTWINDOW_CMD_MAPINFO,
	TEXTWINDOW_CMD_CLOSED_HTMLPAGE,
	TEXTWINDOW_CMD_CHOOSETEAM,
};
//=============================================================================
// HPE_END
//=============================================================================

// VGui Screen Flags
enum
{
	VGUI_SCREEN_ACTIVE = 0x1,
	VGUI_SCREEN_VISIBLE_TO_TEAMMATES = 0x2,
	VGUI_SCREEN_ATTACHED_TO_VIEWMODEL = 0x4,
	VGUI_SCREEN_TRANSPARENT = 0x8,
	VGUI_SCREEN_ONLY_USABLE_BY_OWNER = 0x10,

	VGUI_SCREEN_MAX_BITS = 5
};

typedef enum
{
	USE_OFF = 0, 
	USE_ON = 1, 
	USE_SET = 2, 
	USE_TOGGLE = 3
} USE_TYPE;

// basic team colors
#define COLOR_RED		Color(255, 64, 64, 255)
#define COLOR_BLUE		Color(153, 204, 255, 255)
#define COLOR_YELLOW	Color(255, 178, 0, 255)
#define COLOR_GREEN		Color(153, 255, 153, 255)
#define COLOR_GREY		Color(204, 204, 204, 255)
#define COLOR_WHITE		Color(255, 255, 255, 255)
#define COLOR_BLACK		Color(0, 0, 0, 255)

// All NPCs need this data
enum
{
	DONT_BLEED = -1,
	BLOOD_COLOR_RED = 0,
	BLOOD_COLOR_YELLOW,
	BLOOD_COLOR_GREEN,
	BLOOD_COLOR_MECH,
};

//-----------------------------------------------------------------------------
// Water splash effect flags
//-----------------------------------------------------------------------------
enum
{
	FX_WATER_IN_SLIME = 0x1,
};

// Shared think context stuff
#define	MAX_CONTEXT_LENGTH		32
#define NO_THINK_CONTEXT	-1

// entity flags, CBaseEntity::m_iEFlags
enum
{
	EFL_KILLME	=				(1<<0),	// This entity is marked for death -- This allows the game to actually delete ents at a safe time
	EFL_DORMANT	=				(1<<1),	// Entity is dormant, no updates to client
	EFL_NOCLIP_ACTIVE =			(1<<2),	// Lets us know when the noclip command is active.
	EFL_SETTING_UP_BONES =		(1<<3),	// Set while a model is setting up its bones.
	EFL_KEEP_ON_RECREATE_ENTITIES = (1<<4), // This is a special entity that should not be deleted when we restart entities only

	EFL_HAS_PLAYER_CHILD=		(1<<4),	// One of the child entities is a player.

	EFL_DIRTY_SHADOWUPDATE =	(1<<5),	// Client only- need shadow manager to update the shadow...
	EFL_NOTIFY =				(1<<6),	// Another entity is watching events on this entity (used by teleport)

	// The default behavior in ShouldTransmit is to not send an entity if it doesn't
	// have a model. Certain entities want to be sent anyway because all the drawing logic
	// is in the client DLL. They can set this flag and the engine will transmit them even
	// if they don't have a model.
	EFL_FORCE_CHECK_TRANSMIT =	(1<<7),

	EFL_BOT_FROZEN =			(1<<8),	// This is set on bots that are frozen.
	EFL_SERVER_ONLY =			(1<<9),	// Non-networked entity.
	EFL_NO_AUTO_EDICT_ATTACH =	(1<<10), // Don't attach the edict; we're doing it explicitly
	
	// Some dirty bits with respect to abs computations
	EFL_DIRTY_ABSTRANSFORM =	(1<<11),
	EFL_DIRTY_ABSVELOCITY =		(1<<12),
	EFL_DIRTY_ABSANGVELOCITY =	(1<<13),
	EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS	= (1<<14),
	EFL_DIRTY_SPATIAL_PARTITION = (1<<15),
//	UNUSED						= (1<<16),

	EFL_IN_SKYBOX =				(1<<17),	// This is set if the entity detects that it's in the skybox.
											// This forces it to pass the "in PVS" for transmission.
	EFL_USE_PARTITION_WHEN_NOT_SOLID = (1<<18),	// Entities with this flag set show up in the partition even when not solid
	EFL_TOUCHING_FLUID =		(1<<19),	// Used to determine if an entity is floating

	// FIXME: Not really sure where I should add this...
	EFL_IS_BEING_LIFTED_BY_BARNACLE = (1<<20),
	EFL_NO_ROTORWASH_PUSH =		(1<<21),		// I shouldn't be pushed by the rotorwash
	EFL_NO_THINK_FUNCTION =		(1<<22),
	EFL_NO_GAME_PHYSICS_SIMULATION = (1<<23),

	EFL_CHECK_UNTOUCH =			(1<<24),
	EFL_DONTBLOCKLOS =			(1<<25),		// I shouldn't block NPC line-of-sight
	EFL_DONTWALKON =			(1<<26),		// NPC;s should not walk on this entity
	EFL_NO_DISSOLVE =			(1<<27),		// These guys shouldn't dissolve
	EFL_NO_MEGAPHYSCANNON_RAGDOLL = (1<<28),	// Mega physcannon can't ragdoll these guys.
	EFL_NO_WATER_VELOCITY_CHANGE  =	(1<<29),	// Don't adjust this entity's velocity when transitioning into water
	EFL_NO_PHYSCANNON_INTERACTION =	(1<<30),	// Physcannon can't pick these up or punt them
	EFL_NO_DAMAGE_FORCES =		(1<<31),	// Doesn't accept forces from physics damage
};

//-----------------------------------------------------------------------------
// EFFECTS
//-----------------------------------------------------------------------------
const int FX_BLOODSPRAY_DROPS	= 0x01;
const int FX_BLOODSPRAY_GORE	= 0x02;
const int FX_BLOODSPRAY_CLOUD	= 0x04;
const int FX_BLOODSPRAY_ALL		= 0xFF;

//-----------------------------------------------------------------------------
#define MAX_SCREEN_OVERLAYS		10

// These are the types of data that hang off of CBaseEntities and the flag bits used to mark their presence
enum
{
	GROUNDLINK = 0,
	TOUCHLINK,
	STEPSIMULATION,
	MODELSCALE,
	POSITIONWATCHER,
	PHYSICSPUSHLIST,
	VPHYSICSUPDATEAI,
	VPHYSICSWATCHER,

	// Must be last and <= 32
	NUM_DATAOBJECT_TYPES,
};

class CBaseEntity;

//-----------------------------------------------------------------------------
// Bullet firing information
//-----------------------------------------------------------------------------
class CBaseEntity;

enum FireBulletsFlags_t
{
	FIRE_BULLETS_FIRST_SHOT_ACCURATE = 0x1,		// Pop the first shot with perfect accuracy
	FIRE_BULLETS_DONT_HIT_UNDERWATER = 0x2,		// If the shot hits its target underwater, don't damage it
	FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS = 0x4,	// If the shot hits water surface, still call DoImpactEffect
	FIRE_BULLETS_TEMPORARY_DANGER_SOUND = 0x8,		// Danger sounds added from this impact can be stomped immediately if another is queued
};


struct FireBulletsInfo_t
{
	FireBulletsInfo_t()
	{
		m_iShots = 1;
		m_vecSpread.Init( 0, 0, 0 );
		m_vecFirstStartPos.Init(0, 0, 0);
		m_flDistance = 8192;
		m_iTracerFreq = 4;
		m_flDamage = 0;
		m_iPlayerDamage = 0;
		m_pAttacker = NULL;
		m_nFlags = 0;
		m_nPlayerSkillFlags = 0;
		m_pAdditionalIgnoreEnt = NULL;
		m_flDamageForceScale = 1.0f;

#ifdef _DEBUG
		m_iAmmoType = -1;
		m_vecSrc.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
		m_vecDirShooting.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
#endif
		m_bPrimaryAttack = true;
		m_bIgnoreSkills = false;
		m_flDropOffDist = 0.0f;
		m_bUseServerRandomSeed = false;
	}

	FireBulletsInfo_t( int nShots, const Vector &vecSrc, const Vector &vecDir, const Vector &vecSpread, float flDistance, int nAmmoType, bool bPrimaryAttack = true )
	{
		m_iShots = nShots;
		m_vecSrc = vecSrc;
		m_vecDirShooting = vecDir;
		m_vecSpread = vecSpread;
		m_vecFirstStartPos.Init(0, 0, 0);
		m_flDistance = flDistance;
		m_iAmmoType = nAmmoType;
		m_iTracerFreq = 4;
		m_flDamage = 0;
		m_iPlayerDamage = 0;
		m_pAttacker = NULL;
		m_nFlags = 0;
		m_nPlayerSkillFlags = 0;
		m_pAdditionalIgnoreEnt = NULL;
		m_flDamageForceScale = 1.0f;
		m_bPrimaryAttack = bPrimaryAttack;
		m_bIgnoreSkills = false;
		m_flDropOffDist = 0.0f;
		m_bUseServerRandomSeed = false;
	}

	int m_iShots;
	Vector m_vecSrc;
	Vector m_vecDirShooting;
	Vector m_vecSpread;
	Vector m_vecFirstStartPos;
	float m_flDropOffDist;
	float m_flDistance;
	int m_iAmmoType;
	int m_iTracerFreq;
	float m_flDamage;
	int m_iPlayerDamage;	// Damage to be used instead of m_flDamage if we hit a player
	int m_nFlags;			// See FireBulletsFlags_t
	int m_nPlayerSkillFlags;
	float m_flDamageForceScale;
	CBaseEntity *m_pAttacker;
	CBaseEntity *m_pAdditionalIgnoreEnt;
	bool m_bPrimaryAttack;
	bool m_bUseServerRandomSeed;
	bool m_bIgnoreSkills;
};

//-----------------------------------------------------------------------------
// Purpose: Data for making the MOVETYPE_STEP entities appear to simulate every frame
//  We precompute the simulation and then meter it out each tick during networking of the 
//  entities origin and orientation.  Uses a bit more bandwidth, but it solves the NPCs interacting
//  with elevators/lifts bugs.
//-----------------------------------------------------------------------------
struct StepSimulationStep
{
	int			nTickCount;
	Vector		vecOrigin;
	Quaternion	qRotation;
};

struct StepSimulationData
{
	// Are we using the Step Simulation Data
	bool		m_bOriginActive;
	bool		m_bAnglesActive;

	// This is the pre-pre-Think position, orientation (Quaternion) and tick count
	StepSimulationStep	m_Previous2;

	// This is the pre-Think position, orientation (Quaternion) and tick count
	StepSimulationStep	m_Previous;

	// This is a potential mid-think position, orientation (Quaternion) and tick count
	// Used to mark motion discontinuities that happen between thinks
	StepSimulationStep	m_Discontinuity;

	// This is the goal or post-Think position and orientation (and Quaternion for blending) and next think time tick
	StepSimulationStep	m_Next;
	QAngle		m_angNextRotation;

	// This variable is used so that we only compute networked origin/angles once per tick
	int			m_nLastProcessTickCount;
	// The computed/interpolated network origin/angles to use
	Vector		m_vecNetworkOrigin;
	QAngle		m_angNetworkAngles;
};

//-----------------------------------------------------------------------------
// Purpose: Simple state tracking for changing model sideways shrinkage during barnacle swallow
//-----------------------------------------------------------------------------
struct ModelScale
{
	float		m_flModelScaleStart;
	float		m_flModelScaleGoal;
	float		m_flModelScaleFinishTime;
	float		m_flModelScaleStartTime;
};

#include "soundflags.h"

struct CSoundParameters;
typedef short HSOUNDSCRIPTHANDLE;
//-----------------------------------------------------------------------------
// Purpose: Aggregates and sets default parameters for EmitSound function calls
//-----------------------------------------------------------------------------
struct EmitSound_t
{
	EmitSound_t() :
		m_nChannel( 0 ),
		m_pSoundName( 0 ),
		m_flVolume( VOL_NORM ),
		m_SoundLevel( SNDLVL_NONE ),
		m_nFlags( 0 ),
		m_nPitch( PITCH_NORM ),
		m_nSpecialDSP( 0 ),
		m_pOrigin( 0 ),
		m_flSoundTime( 0.0f ),
		m_pflSoundDuration( 0 ),
		m_bEmitCloseCaption( true ),
		m_bWarnOnMissingCloseCaption( false ),
		m_bWarnOnDirectWaveReference( false ),
		m_nSpeakerEntity( -1 ),
		m_UtlVecSoundOrigin(),
		m_hSoundScriptHandle( -1 )
	{
	}

	EmitSound_t( const CSoundParameters &src );

	int							m_nChannel;
	char const					*m_pSoundName;
	float						m_flVolume;
	soundlevel_t				m_SoundLevel;
	int							m_nFlags;
	int							m_nPitch;
	int							m_nSpecialDSP;
	const Vector				*m_pOrigin;
	float						m_flSoundTime; ///< NOT DURATION, but rather, some absolute time in the future until which this sound should be delayed
	float						*m_pflSoundDuration;
	bool						m_bEmitCloseCaption;
	bool						m_bWarnOnMissingCloseCaption;
	bool						m_bWarnOnDirectWaveReference;
	int							m_nSpeakerEntity;
	mutable CUtlVector< Vector >	m_UtlVecSoundOrigin;  ///< Actual sound origin(s) (can be multiple if sound routed through speaker entity(ies) )
	mutable HSOUNDSCRIPTHANDLE		m_hSoundScriptHandle;
};

#define MAX_ACTORS_IN_SCENE 16

// YWB:  3/12/2007
// Changing the following #define for Prediction Error checking (See gamemovement.cpp for overview) will to 1 or 2 enables the system, 0 turns it off
// Level 1 enables it, but doesn't force "full precision" networking, so you can still get lots of errors in position/velocity/etc.
// Level 2 enables it but also forces origins/angles to be sent full precision, so other fields can be error / tolerance checked
// NOTE:  This stuff only works on a listen server since it punches a hole from the client .dll to server .dll!!!
#define PREDICTION_ERROR_CHECK_LEVEL 0

enum
{
	SIMULATION_TIME_WINDOW_BITS = 8,
};

enum
{
	HILL_TYPE_NONE = 0,
	HILL_TYPE_UPHILL,
	HILL_TYPE_DOWNHILL,
};

#define NOINTERP_PARITY_MAX			4
#define NOINTERP_PARITY_MAX_BITS	2

//-----------------------------------------------------------------------------
// Generic activity lookup support
//-----------------------------------------------------------------------------
enum
{
	kActivityLookup_Unknown = -2,			// hasn't been searched for
	kActivityLookup_Missing = -1,			// has been searched for but wasn't found
};

// BB2
#define BB2_AI
#define BB2_MISC_FIXES
#define GLOWS_ENABLE
#define BB2_GLOWS
#define BB2_USE_NAVMESH
#define BB2_LAGCOMP_HIT_MAX 4 // When figuring out which entities to hit, this is the max amount for now! During 1 comp. tick.

#define BB2_NPC_FADE_TIME 1.25f
#define BB2_NPC_FADE_FREQ ((1.0f / BB2_NPC_FADE_TIME) * 255.0f)

enum
{
	GLOW_MODE_NONE = 0,
	GLOW_MODE_RADIUS, // Glow a normal item which can be picked up / can be consumed. Like weapons, ammo, etc... You can't force glow items using this type / change'em. (it is client side)
	GLOW_MODE_GLOBAL, // Forces Glowing on any entity.
	// PLAYER STUFF:
	GLOW_MODE_TEAMMATE, // Others can glow me if I'm X amount away from them, even if I'm not visible.
};

enum WeaponUniqueIDs
{
	WEAPON_ID_NONE = -1,
	WEAPON_ID_BERETTA = 0,
	WEAPON_ID_BERETTA_AKIMBO,
	WEAPON_ID_GLOCK17,
	WEAPON_ID_GLOCK17_AKIMBO,
	WEAPON_ID_DEAGLE,
	WEAPON_ID_REXMP412,
	WEAPON_ID_REXMP412_AKIMBO,

	WEAPON_ID_AK74,
	WEAPON_ID_FAMAS,
	WEAPON_ID_G36C,
	WEAPON_ID_WINCHESTER1894,

	WEAPON_ID_REMINGTON700,

	WEAPON_ID_REMINGTON870,
	WEAPON_ID_BENELLIM4,
	WEAPON_ID_SAWEDOFF,
	WEAPON_ID_SAWEDOFF_AKIMBO,

	WEAPON_ID_MINIGUN,
	WEAPON_ID_FLAMETHROWER,

	WEAPON_ID_MAC11,
	WEAPON_ID_MP7,
	WEAPON_ID_MP5,
	WEAPON_ID_MICROUZI,

	WEAPON_ID_HANDS,
	WEAPON_ID_ZOMBHANDS,
	WEAPON_ID_BRICK,
	WEAPON_ID_KICK,
	WEAPON_ID_M9BAYONET,
	WEAPON_ID_FIREAXE,
	WEAPON_ID_MACHETE,
	WEAPON_ID_HATCHET,
	WEAPON_ID_SLEDGEHAMMER,
	WEAPON_ID_BASEBALLBAT,

	WEAPON_ID_PROPANE,
	WEAPON_ID_FRAG,

	WEAPON_ID_COUNT
};

#define DEFAULT_PLAYER_MODEL(team) ( ( team == TEAM_DECEASED ) ? ( "models/characters/player/marine_zombie.mdl" ) : ( "models/characters/player/marine.mdl" ) )

#endif // SHAREDDEFS_H

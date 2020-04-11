//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_PARSE_H
#define WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class IFileSystem;

typedef unsigned short WEAPON_FILE_INFO_HANDLE;

// -----------------------------------------------------------
// Weapon sound types
// Used to play sounds defined in the weapon's classname.txt file
// This needs to match pWeaponSoundCategories in weapon_parse.cpp
// ------------------------------------------------------------
typedef enum {
	EMPTY,
	SINGLE,
	SINGLE_NPC,
	WPN_DOUBLE, // Can't be "DOUBLE" because windows.h uses it.
	DOUBLE_NPC,
	BURST,
	RELOAD,
	RELOAD_NPC,
	MELEE_MISS,
	MELEE_HIT,
	MELEE_HIT_WORLD,
	SPECIAL1,
	SPECIAL2,
	SPECIAL3,
	TAUNT,
	DEPLOY,

	// Add new shoot sound types here

	NUM_SHOOT_SOUND_TYPES,
} WeaponSound_t;

struct particleItem_t
{
	char szFirstpersonParticle[MAX_MAP_NAME];
	char szThirdpersonParticle[MAX_MAP_NAME];
};

enum WeaponParticleTypes
{
	PARTICLE_TYPE_MUZZLE = 0,
	PARTICLE_TYPE_SMOKE,
	PARTICLE_TYPE_TRACER,
};

int GetWeaponSoundFromString( const char *pszString );

#define MAX_SHOOT_SOUNDS	16			// Maximum number of shoot sounds per shoot type

#define MAX_WEAPON_STRING	80
#define MAX_WEAPON_ZOOM_MODES 3

#define WEAPON_PRINTNAME_MISSING "!!! Missing printname on weapon"

class CHudTexture;
class KeyValues;

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the weapon's script file. 
// It's cached so we only read each weapon's script file once.
// Each game provides a CreateWeaponInfo function so it can have game-specific
// data (like CS move speeds) in the weapon script.
//-----------------------------------------------------------------------------
class FileWeaponInfo_t
{
public:

	FileWeaponInfo_t();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse( KeyValues *pKeyValuesData, const char *szWeaponName );
	
public:	

	bool					bParsedScript;
	bool					bLoadedHudElements;

// SHARED
	char					szClassName[MAX_WEAPON_STRING];
	char					szPrintName[MAX_WEAPON_STRING];			// Name for showing in HUD, etc.

	char					szViewModel[MAX_WEAPON_STRING];			// View model of this weapon
	char					szWorldModel[MAX_WEAPON_STRING];		// Model of this weapon seen carried by the player
	int						iSlot;									// inventory slot.
	int						iMaxClip;								// max clip size (-1 if no clip)
	int						iDefaultClip;							// amount of ammo in the gun when it's created
	int						iWeight;								// this value used to determine this weapon's importance in autoselection.
	int						iRumbleEffect;							// Which rumble effect to use when fired? (xbox)
	int						iFlags;									// miscellaneous weapon flags

	char szAttachmentLink[MAX_WEAPON_STRING];
	Vector vecAttachmentPosOffset;
	QAngle angAttachmentAngOffset;

	float m_flSpecialDamage;
	float m_flSpecialDamage2;

	// Weapon Detailed Properties:
	int m_iLevelReq;
	int m_iPellets;
	int m_iRangeMax;
	int m_iRangeMin;
	int m_iAccuracy;
	int m_iAccuracyPvP;
	int m_iZoomModeFOV[MAX_WEAPON_ZOOM_MODES];
	float m_flPhysicalWeight;
	float m_flFireRate;
	float m_flFireRate2;
	float m_flAccuracyFactor;
	float m_flPickupPenalty;
	float m_flBashRange;
	float m_flBashForce;
	float m_flSecondaryAttackCooldown;
	float m_flDropOffDistance;
	float m_flBurstFireRate;
	float m_flWeaponChargeTime;
	float m_flDepletionFactor;

	// Client Ammo Properties:
	int m_iAmmoHUDIndex;
	bool m_bShowAsMagsLeft;

	// Weapon Skill Stuff:
	float m_flSkillDamageFactor;
	float m_flSkillFireRateFactor;
	float m_flSkillBleedFactor;
	float m_flSkillCrippleFactor;

	CUtlVector<particleItem_t> pszMuzzleParticles;
	CUtlVector<particleItem_t> pszSmokeParticles;
	CUtlVector<particleItem_t> pszTracerParticles;

	// Sound blocks
	char					aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];	

	bool					m_bMeleeWeapon;		// Melee weapons can always "fire" regardless of ammo.

// CLIENT DLL
	// Sprite data, read from the data file
	CHudTexture						*iconActive;
	CHudTexture	 					*iconInactive;
	CHudTexture 					*iconAmmo;
};

// The weapon parse function
bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, 
	WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey = NULL );

// If weapon info has been loaded for the specified class name, this returns it.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name );

FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle );
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void );
void PrecacheFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey );

// 
// Read a possibly-encrypted KeyValues file in. 
// If pICEKey is NULL, then it appends .txt to the filename and loads it as an unencrypted file.
// If pICEKey is non-NULL, then it appends .ctx to the filename and loads it as an encrypted file.
//
// (This should be moved into a more appropriate place).
//
KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey, bool bForceReadEncryptedFile = false );

// Each game implements this. It can return a derived class and override Parse() if it wants.
extern FileWeaponInfo_t* CreateWeaponInfo();

#endif // WEAPON_PARSE_H
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The sound categories found in the weapon classname.txt files
// This needs to match the WeaponSound_t enum in weapon_parse.h
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ] = 
{
	"empty",
	"single_shot",
	"single_shot_npc",
	"double_shot",
	"double_shot_npc",
	"burst",
	"reload",
	"reload_npc",
	"melee_miss",
	"melee_hit",
	"melee_hit_world",
	"special1",
	"special2",
	"special3",
	"taunt",
	"deploy"
};
#else
extern const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ];
#endif

int GetWeaponSoundFromString( const char *pszString )
{
	for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
	{
		if ( !Q_stricmp(pszString,pWeaponSoundCategories[i]) )
			return (WeaponSound_t)i;
	}
	return -1;
}


// Item flags that we parse out of the file.
typedef struct
{
	const char *m_pFlagName;
	int m_iFlagValue;
} itemFlags_t;
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
itemFlags_t g_ItemFlags[8] =
{
	{ "ITEM_FLAG_SELECTONEMPTY",	ITEM_FLAG_SELECTONEMPTY },
	{ "ITEM_FLAG_NOAUTORELOAD",		ITEM_FLAG_NOAUTORELOAD },
	{ "ITEM_FLAG_NOAUTOSWITCHEMPTY", ITEM_FLAG_NOAUTOSWITCHEMPTY },
	{ "ITEM_FLAG_LIMITINWORLD",		ITEM_FLAG_LIMITINWORLD },
	{ "ITEM_FLAG_EXHAUSTIBLE",		ITEM_FLAG_EXHAUSTIBLE },
	{ "ITEM_FLAG_DOHITLOCATIONDMG", ITEM_FLAG_DOHITLOCATIONDMG },
	{ "ITEM_FLAG_NOAMMOPICKUPS",	ITEM_FLAG_NOAMMOPICKUPS },
	{ "ITEM_FLAG_NOITEMPICKUP",		ITEM_FLAG_NOITEMPICKUP }
};
#else
extern itemFlags_t g_ItemFlags[8];
#endif

static CUtlDict< FileWeaponInfo_t*, unsigned short > m_WeaponInfoDatabase;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : FileWeaponInfo_t
//-----------------------------------------------------------------------------
static WEAPON_FILE_INFO_HANDLE FindWeaponInfoSlot( const char *name )
{
	// Complain about duplicately defined metaclass names...
	unsigned short lookup = m_WeaponInfoDatabase.Find( name );
	if ( lookup != m_WeaponInfoDatabase.InvalidIndex() )
	{
		return lookup;
	}

	FileWeaponInfo_t *insert = CreateWeaponInfo();

	lookup = m_WeaponInfoDatabase.Insert( name, insert );
	Assert( lookup != m_WeaponInfoDatabase.InvalidIndex() );
	return lookup;
}

// Find a weapon slot, assuming the weapon's data has already been loaded.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name )
{
	return m_WeaponInfoDatabase.Find( name );
}

// FIXME, handle differently?
static FileWeaponInfo_t gNullWeaponInfo;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : FileWeaponInfo_t
//-----------------------------------------------------------------------------
FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle )
{
	if ( handle < 0 || handle >= m_WeaponInfoDatabase.Count() )
	{
		return &gNullWeaponInfo;
	}

	if ( handle == m_WeaponInfoDatabase.InvalidIndex() )
	{
		return &gNullWeaponInfo;
	}

	return m_WeaponInfoDatabase[ handle ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : WEAPON_FILE_INFO_HANDLE
//-----------------------------------------------------------------------------
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void )
{
	return (WEAPON_FILE_INFO_HANDLE)m_WeaponInfoDatabase.InvalidIndex();
}

#if 0
void ResetFileWeaponInfoDatabase( void )
{
	int c = m_WeaponInfoDatabase.Count(); 
	for ( int i = 0; i < c; ++i )
	{
		delete m_WeaponInfoDatabase[ i ];
	}
	m_WeaponInfoDatabase.RemoveAll();
}
#endif

void PrecacheFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey )
{
	if ( m_WeaponInfoDatabase.Count() )
		return;

	KeyValues *manifest = new KeyValues( "weaponscripts" );
	if ( manifest->LoadFromFile( filesystem, "scripts/weapon_manifest.txt", "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL ; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				char fileBase[512];
				Q_FileBase( sub->GetString(), fileBase, sizeof(fileBase) );
				WEAPON_FILE_INFO_HANDLE tmp;
#ifdef CLIENT_DLL
				if ( ReadWeaponDataFromFileForSlot( filesystem, fileBase, &tmp, pICEKey ) )
				{
					gWR.LoadWeaponSprites( tmp );
				}
#else
				ReadWeaponDataFromFileForSlot( filesystem, fileBase, &tmp, pICEKey );
#endif
			}
			else
			{
				Error( "Expecting 'file', got %s\n", sub->GetName() );
			}
		}
	}
	manifest->deleteThis();
}

KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey, bool bForceReadEncryptedFile /*= false*/ )
{
	Assert( strchr( szFilenameWithoutExtension, '.' ) == NULL );
	char szFullName[512];

	const char *pSearchPath = "MOD";

	if ( pICEKey == NULL )
	{
		pSearchPath = "GAME";
	}

	// Open the weapon data file, and abort if we can't
	KeyValues *pKV = new KeyValues( "WeaponDatafile" );

	Q_snprintf(szFullName,sizeof(szFullName), "%s.txt", szFilenameWithoutExtension);

	if ( bForceReadEncryptedFile || !pKV->LoadFromFile( filesystem, szFullName, pSearchPath ) ) // try to load the normal .txt file first
	{
#ifndef _XBOX
		if ( pICEKey )
		{
			Q_snprintf(szFullName,sizeof(szFullName), "%s.bbd", szFilenameWithoutExtension); // fall back to the .ctx file

			FileHandle_t f = filesystem->Open( szFullName, "rb", pSearchPath );

			if (!f)
			{
				pKV->deleteThis();
				return NULL;
			}
			// load file into a null-terminated buffer
			int fileSize = filesystem->Size(f);
			char *buffer = (char*)MemAllocScratch(fileSize + 1);
		
			Assert(buffer);
		
			filesystem->Read(buffer, fileSize, f); // read into local buffer
			buffer[fileSize] = 0; // null terminate file as EOF
			filesystem->Close( f );	// close file after reading

			UTIL_DecodeICE( (unsigned char*)buffer, fileSize, pICEKey );

			bool retOK = pKV->LoadFromBuffer( szFullName, buffer, filesystem );

			MemFreeScratch();

			if ( !retOK )
			{
				pKV->deleteThis();
				return NULL;
			}
		}
		else
		{
			pKV->deleteThis();
			return NULL;
		}
#else
		pKV->deleteThis();
		return NULL;
#endif
	}

	return pKV;
}

//-----------------------------------------------------------------------------
// Purpose: Read data on weapon from script file
// Output:  true  - if data2 successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------

bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey )
{
	if ( !phandle )
	{
		Assert( 0 );
		return false;
	}
	
	*phandle = FindWeaponInfoSlot( szWeaponName );
	FileWeaponInfo_t *pFileInfo = GetFileWeaponInfoFromHandle( *phandle );
	Assert( pFileInfo );

	if ( pFileInfo->bParsedScript )
		return true;

	char sz[128];
	Q_snprintf( sz, sizeof( sz ), "scripts/%s", szWeaponName );

	KeyValues *pKV = ReadEncryptedKVFile( filesystem, sz, pICEKey, false);
	if ( !pKV )
		return false;

	pFileInfo->Parse( pKV, szWeaponName );

	pKV->deleteThis();

	return true;
}


//-----------------------------------------------------------------------------
// FileWeaponInfo_t implementation.
//-----------------------------------------------------------------------------

FileWeaponInfo_t::FileWeaponInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;
	szClassName[0] = 0;
	szPrintName[0] = 0;
	szAttachmentLink[0] = 0;

	pszMuzzleParticles.Purge();
	pszSmokeParticles.Purge();
	pszTracerParticles.Purge();

	szViewModel[0] = 0;
	szWorldModel[0] = 0;
	iSlot = 0;
	iMaxClip = 0;
	iDefaultClip = 0;
	iWeight = 0;
	iRumbleEffect = -1;
	iFlags = 0;
	memset(aShootSounds, 0, sizeof(aShootSounds));
	m_bMeleeWeapon = false;
	iconActive = 0;
	iconInactive = 0;
	iconAmmo = 0;

	// BB2 Skills:
	m_flSpecialDamage = 0.0f;
	m_flSpecialDamage2 = 0.0f;
	m_iLevelReq = 0;
	m_iPellets = 1;
	m_iAccuracy = 0;
	m_iAccuracyPvP = 0;
	m_iRangeMin = 64;
	m_iRangeMax = 1024;

	for (int i = 0; i < MAX_WEAPON_ZOOM_MODES; i++)
		m_iZoomModeFOV[i] = 0;

	m_flFireRate = 1;
	m_flAccuracyFactor = 0.2f;
	m_flPhysicalWeight = 1; // Default 1kg.
	m_flPickupPenalty = 60.0f;
	m_flBashRange = 33.0f;
	m_flBashForce = 500.0f;
	m_flSecondaryAttackCooldown = 0.0f;
	m_flDropOffDistance = 0.0f;
	m_flBurstFireRate = 0.2f;
	m_flWeaponChargeTime = 3.0f;
	m_flDepletionFactor = 5.0f;

	m_iAmmoHUDIndex = 0;
	m_bShowAsMagsLeft = true;

	m_flSkillDamageFactor = 6.0f;
	m_flSkillFireRateFactor = 3.0f;
	m_flSkillBleedFactor = 10.0f;
	m_flSkillCrippleFactor = 1.0f;
}

void FileWeaponInfo_t::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	// Classname
	Q_strncpy( szClassName, szWeaponName, MAX_WEAPON_STRING );
	// Printable name
	Q_strncpy( szPrintName, pKeyValuesData->GetString( "printname", WEAPON_PRINTNAME_MISSING ), MAX_WEAPON_STRING );
	// View model & world model
	Q_strncpy( szViewModel, pKeyValuesData->GetString( "viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szWorldModel, pKeyValuesData->GetString( "playermodel" ), MAX_WEAPON_STRING );
	iSlot = pKeyValuesData->GetInt( "bucket", 0 );
	
	iMaxClip = pKeyValuesData->GetInt("clip_size", WEAPON_NOCLIP);					// Max clips gun can hold (assume they don't use clips by default)
	iDefaultClip = pKeyValuesData->GetInt("default_clip", iMaxClip);		// amount of ammo placed in the clip when it's picked up
	iWeight = pKeyValuesData->GetInt( "weight", 0 );

	iRumbleEffect = pKeyValuesData->GetInt( "rumble", -1 );
	
	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt("item_flags");

	for ( int i=0; i < ARRAYSIZE( g_ItemFlags ); i++ )
	{
		int iVal = pKeyValuesData->GetInt( g_ItemFlags[i].m_pFlagName, -1 );
		if ( iVal == 0 )
		{
			iFlags &= ~g_ItemFlags[i].m_iFlagValue;
		}
		else if ( iVal == 1 )
		{
			iFlags |= g_ItemFlags[i].m_iFlagValue;
		}
	}

	m_bMeleeWeapon = ( pKeyValuesData->GetInt( "MeleeWeapon", 0 ) != 0 ) ? true : false;

	m_flSpecialDamage = pKeyValuesData->GetFloat("damage_special", 0.0f);
	m_flSpecialDamage2 = pKeyValuesData->GetFloat("damage_special2", 0.0f);

	// BB2 Wep Properties:
	KeyValues *pPropertiesField = pKeyValuesData->FindKey( "Properties" );
	m_iLevelReq = pPropertiesField ? pPropertiesField->GetInt("level_required", 0) : 0;
	m_iPellets = pPropertiesField ? pPropertiesField->GetInt("pellets", 1) : 1;
	m_iAccuracy = pPropertiesField ? pPropertiesField->GetInt("accuracy", 0) : 0;
	m_iAccuracyPvP = pPropertiesField ? pPropertiesField->GetInt("accuracy_pvp", 0) : 0;
	m_iRangeMin = pPropertiesField ? pPropertiesField->GetInt("range_min", 64) : 64;
	m_iRangeMax = pPropertiesField ? pPropertiesField->GetInt("range_max", 1024) : 1024;
	m_flFireRate = pPropertiesField ? pPropertiesField->GetFloat("fire_rate", 1.0f) : 1.0f;
	m_flAccuracyFactor = pPropertiesField ? pPropertiesField->GetFloat("accuracy_factor", 0.2f) : 0.2f;
	m_flPhysicalWeight = pPropertiesField ? pPropertiesField->GetFloat("weight", 1.0f) : 1.0f;
	m_flPickupPenalty = pPropertiesField ? pPropertiesField->GetFloat("pickup_penalty", 60.0f) : 60.0f; // Prevent the user from picking up this same weapon for X sec. (only counts for respawnable items (not dropped))
	m_flBashRange = pPropertiesField ? pPropertiesField->GetFloat("range_bash", 33.0f) : 33.0f;
	m_flBashForce = pPropertiesField ? pPropertiesField->GetFloat("bash_force", 500.0f) : 500.0f;
	m_flSecondaryAttackCooldown = pPropertiesField ? pPropertiesField->GetFloat("secondary_cooldown", 0.0f) : 0.0f;
	m_flDropOffDistance = pPropertiesField ? pPropertiesField->GetFloat("dropoff_distance", 0.0f) : 0.0f;
	m_flBurstFireRate = pPropertiesField ? pPropertiesField->GetFloat("fire_rate_burst", 0.2f) : 0.2f;
	m_flWeaponChargeTime = pPropertiesField ? pPropertiesField->GetFloat("charge_time", 3.0f) : 3.0f;
	m_flDepletionFactor = pPropertiesField ? pPropertiesField->GetFloat("depletion_factor", 5.0f) : 5.0f;

	m_iZoomModeFOV[0] = pPropertiesField ? pPropertiesField->GetInt("zoom_mode_1") : 0;
	m_iZoomModeFOV[1] = pPropertiesField ? pPropertiesField->GetInt("zoom_mode_2") : 0;
	m_iZoomModeFOV[2] = pPropertiesField ? pPropertiesField->GetInt("zoom_mode_3") : 0;

	KeyValues *pClientAmmoHUDField = pKeyValuesData->FindKey("AmmoData");
	m_iAmmoHUDIndex = pClientAmmoHUDField ? pClientAmmoHUDField->GetInt("index") : 0;
	m_bShowAsMagsLeft = pClientAmmoHUDField ? pClientAmmoHUDField->GetBool("magazine_style", true) : true;

	KeyValues *pSkillField = pKeyValuesData->FindKey("Skills");
	m_flSkillDamageFactor = pSkillField ? pSkillField->GetFloat("DamageFactor", 6.0f) : 6.0f;
	m_flSkillFireRateFactor = pSkillField ? pSkillField->GetFloat("FireRateFactor", 3.0f) : 3.0f;
	m_flSkillBleedFactor = pSkillField ? pSkillField->GetFloat("BleedFactor", 10.0f) : 10.0f;
	m_flSkillCrippleFactor = pSkillField ? pSkillField->GetFloat("CrippleFactor", 1.0f) : 1.0f;

	KeyValues *pParticleField = pKeyValuesData->FindKey( "MuzzleParticles" );
	if (pParticleField)
	{
		for (KeyValues *sub = pParticleField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			particleItem_t pItem;
			Q_strncpy(pItem.szFirstpersonParticle, sub->GetName(), MAX_MAP_NAME);
			Q_strncpy(pItem.szThirdpersonParticle, sub->GetString(), MAX_MAP_NAME);
			pszMuzzleParticles.AddToTail(pItem);
		}
	}

	pParticleField = pKeyValuesData->FindKey("SmokeParticles");
	if (pParticleField)
	{
		for (KeyValues *sub = pParticleField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			particleItem_t pItem;
			Q_strncpy(pItem.szFirstpersonParticle, sub->GetName(), MAX_MAP_NAME);
			Q_strncpy(pItem.szThirdpersonParticle, sub->GetString(), MAX_MAP_NAME);
			pszSmokeParticles.AddToTail(pItem);
		}
	}

	pParticleField = pKeyValuesData->FindKey("TracerParticles");
	if (pParticleField)
	{
		for (KeyValues *sub = pParticleField->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			particleItem_t pItem;
			Q_strncpy(pItem.szFirstpersonParticle, sub->GetName(), MAX_MAP_NAME);
			Q_strncpy(pItem.szThirdpersonParticle, sub->GetString(), MAX_MAP_NAME);
			pszTracerParticles.AddToTail(pItem);
	    }
	}

	KeyValues *pAttachments = pKeyValuesData->FindKey( "Attachment" );
	if (pAttachments)
	{
		Q_strncpy( szAttachmentLink, pAttachments->GetString( "attachment", "primary" ), MAX_WEAPON_STRING );
		vecAttachmentPosOffset.x		= pAttachments->GetFloat( "forward", 0.0f );
		vecAttachmentPosOffset.y		= pAttachments->GetFloat( "right", 0.0f );
		vecAttachmentPosOffset.z		= pAttachments->GetFloat( "up", 0.0f );
 
		angAttachmentAngOffset[PITCH]	= pAttachments->GetFloat( "pitch", 0.0f );
		angAttachmentAngOffset[YAW]		= pAttachments->GetFloat( "yaw", 0.0f );
		angAttachmentAngOffset[ROLL]	= pAttachments->GetFloat( "roll", 0.0f );
	}
	else
	{
		Q_strncpy( szAttachmentLink, "primary", MAX_WEAPON_STRING );
		vecAttachmentPosOffset = vec3_origin;
		angAttachmentAngOffset.Init();
	}

	// Now read the weapon sounds
	memset( aShootSounds, 0, sizeof( aShootSounds ) );
	KeyValues *pSoundData = pKeyValuesData->FindKey( "SoundData" );
	if ( pSoundData )
	{
		for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
		{
			const char *soundname = pSoundData->GetString( pWeaponSoundCategories[i] );
			if ( soundname && soundname[0] )
			{
				Q_strncpy( aShootSounds[i], soundname, MAX_WEAPON_STRING );
			}
		}
	}
}


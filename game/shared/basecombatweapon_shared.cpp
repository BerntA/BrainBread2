//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Base Weapon Handling - Handles FX, Bash, Special stuff...
//
//========================================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_saverestore.h"
#include "datacache/imdlcache.h"
#include "activitylist.h"
#include "GameBase_Shared.h"
#include "random_extended.h"

#ifdef CLIENT_DLL
#include "GameBase_Client.h"
#include "prediction.h"
#include "c_baseplayer.h"
#include "c_hl2mp_player.h"
#include "c_te_effect_dispatch.h"
#else

// Game DLL Headers
#include "soundent.h"
#include "eventqueue.h"
#include "fmtstr.h"
#include "particle_parse.h"
#include "te_effect_dispatch.h"
#include "ilagcompensationmanager.h"
#include "ai_basenpc.h"

#ifdef HL2MP
#include "hl2mp_player.h"
#endif

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HIDEWEAPON_THINK_CONTEXT			"BaseCombatWeapon_HideThink"

// BB2
// Viewkick ConVar Defines
ConVar viewpunch_x_min("viewpunch_x_min", "-1", FCVAR_REPLICATED);
ConVar viewpunch_y_min("viewpunch_y_min", "-0.5", FCVAR_REPLICATED);
ConVar viewpunch_z_min("viewpunch_z_min", "-0.5", FCVAR_REPLICATED);

ConVar viewpunch_x_max("viewpunch_x_max", "0", FCVAR_REPLICATED);
ConVar viewpunch_y_max("viewpunch_y_max", "0.5", FCVAR_REPLICATED);
ConVar viewpunch_z_max("viewpunch_z_max", "0.5", FCVAR_REPLICATED);

CBaseCombatWeapon::CBaseCombatWeapon()
{
	// Constructor must call this
	// CONSTRUCT_PREDICTABLE( CBaseCombatWeapon );

	// Some default values.  There should be set in the particular weapon classes
	m_fMinRange1		= 65;
	m_fMinRange2		= 65;
	m_fMaxRange1		= 1024;
	m_fMaxRange2		= 1024;

	m_bReloadsSingly	= false;

	// Defaults to zero
	m_nViewModelIndex	= 0;

	// BB2
	m_flHolsteredTime = 0.0f;

	// Holster Sequence
	m_bWantsHolster = false;
	m_bIsBloody = false;

	// Combat Related
	m_flViewKickPenalty = 1.0f;
	m_flViewKickTime = 0.0f;
	m_iShotsFired = 0;
	m_iMeleeAttackType = 0;

	// Default Glow Color
	color32 col32 = { 70, 130, 180, 255 };
	m_GlowColor = col32;

#if defined( CLIENT_DLL )
	m_iState = m_iOldState = WEAPON_NOT_CARRIED;
	m_iClip = -1;
	m_iAmmoType = -1;
#else
	OnBaseCombatWeaponCreated( this );
	m_iDefaultAmmoCount = -1;
	m_bCanRemoveWeapon = false;
	SetGlowMode(GLOW_MODE_RADIUS);
	m_bSuppressRespawn = false;

	m_pEnemiesStruck.Purge();
	m_flLastTraceTime = 0.0f;
#endif

	m_hWeaponFileInfo = GetInvalidWeaponInfoHandle();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CBaseCombatWeapon::~CBaseCombatWeapon( void )
{
#if !defined( CLIENT_DLL )
	OnBaseCombatWeaponDestroyed( this );
	m_pEnemiesStruck.Purge();
#endif
}

void CBaseCombatWeapon::Activate( void )
{
	BaseClass::Activate();

#ifndef CLIENT_DLL
	if ( GetOwnerEntity() )
		return;

	if ( g_pGameRules->IsAllowedToSpawn( this ) == false )
	{
		UTIL_Remove( this );
		return;
	}
#endif

}

void CBaseCombatWeapon::GiveDefaultAmmo(void)
{
	m_iClip = UsesClipsForAmmo() ? (AutoFiresFullClip() ? 0 : GetDefaultClip()) : WEAPON_NOCLIP;

#ifndef CLIENT_DLL
	if (m_iClip == WEAPON_NOCLIP)
	{
		SetAmmoCount(GetDefaultClip());
		return;
	}

	if (GetAmmoTypeID() == -1)
		return;

	// Give the actual ammo:
	int iMaxCarry = GetAmmoMaxCarry();
	if (m_iDefaultAmmoCount == -1 || (m_iDefaultAmmoCount > iMaxCarry))
		SetAmmoCount(iMaxCarry);
	else
		SetAmmoCount(m_iDefaultAmmoCount);
#endif
}

void CBaseCombatWeapon::RemoveAmmo(int count)
{
	if ((count <= 0) || (GetAmmoTypeID() == -1))
		return;
	SetAmmoCount(MAX(GetAmmoCount() - count, 0));
}

int CBaseCombatWeapon::GiveAmmo(int count, bool bSuppressSound)
{
	if ((GetAmmoTypeID() == -1) || (count <= 0))
		return 0;

	int iAdd = 0;

#ifndef CLIENT_DLL
	int iMaxCarry = GetAmmoMaxCarry();
	int iCurrent = GetAmmoCount();

	if (iCurrent < iMaxCarry)
	{
		iAdd = MIN(count, iMaxCarry - iCurrent);
		if (iAdd > 0)
			SetAmmoCount(iCurrent + iAdd);
	}

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (pPlayer)
	{
		if (iAdd <= 0)
		{
			// we've been denied the pickup, display a hud icon to show that
			CSingleUserRecipientFilter user(pPlayer);
			user.MakeReliable();
			UserMessageBegin(user, "AmmoDenied");
			WRITE_SHORT(GetAmmoTypeID());
			MessageEnd();
		}
		else
		{
			// Ammo pickup sound
			if (!bSuppressSound)
				pPlayer->EmitSound("BaseCombatCharacter.AmmoPickup");

			// Show the pickup icon:
			CSingleUserRecipientFilter user(pPlayer);
			user.MakeReliable();
			UserMessageBegin(user, "ItemPickup");
			WRITE_STRING("AMMO");
			WRITE_SHORT(GetAmmoTypeID());
			MessageEnd();
		}
	}
#endif

	return iAdd;
}

//-----------------------------------------------------------------------------
// Purpose: Set mode to world model and start falling to the ground
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	SetSolid(SOLID_BBOX);
	m_flNextEmptySoundTime = 0.0f;

	// Weapons won't show up in trace calls if they are being carried...
	RemoveEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	m_iState = WEAPON_NOT_CARRIED;
	// Assume 
	m_nViewModelIndex = 0;

	GiveDefaultAmmo();

	if ( GetWorldModel() )
	{
		SetModel( GetWorldModel() );
	}

#if !defined( CLIENT_DLL )
	FallInit();
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetBlocksLOS( false );
	m_bCanRemoveWeapon = false;
#endif

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds(true, 10.0f);

	// Use more efficient bbox culling on the client. Otherwise, it'll setup bones for most
	// characters even when they're not in the frustum.
	AddEffects( EF_BONEMERGE_FASTCULL );

	SetupWeaponRanges();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Precache(void)
{
#if defined( CLIENT_DLL )
	Assert( Q_strlen( GetClassname() ) > 0 );
#endif

	SetAmmoTypeID(-1);

	// Add this weapon to the weapon registry, and get our index into it
	// Get weapon data from script file
	if (ReadWeaponDataFromFileForSlot(filesystem, GetClassname(), &m_hWeaponFileInfo, GameBaseShared()->GetEncryptionKey()))
	{
		// Get the ammo indexes for the ammo's specified in the data file
		const char *ammoTypeName = GetAmmoTypeName();
		if (ammoTypeName && ammoTypeName[0])
		{
			SetAmmoTypeID(GetAmmoDef()->Index(ammoTypeName));
			if (GetAmmoTypeID() == -1)
				Msg("ERROR: Weapon (%s) using undefined ammo type (%s)\n", GetClassname(), ammoTypeName);
		}

#if defined( CLIENT_DLL )
		gWR.LoadWeaponSprites( GetWeaponFileInfoHandle() );
#endif

		// Precache models (preload to avoid hitch)
		m_iViewModelIndex = 0;
		m_iWorldModelIndex = 0;

		if (GetViewModel() && GetViewModel()[0])
		{
			m_iViewModelIndex = CBaseEntity::PrecacheModel(GetViewModel());
		}

		if (GetWorldModel() && GetWorldModel()[0])
		{
			m_iWorldModelIndex = CBaseEntity::PrecacheModel(GetWorldModel());
		}

		// Precache Particle Links
		for (int i = 0; i < GetWpnData().pszMuzzleParticles.Count(); i++)
		{
			PrecacheParticleSystem(GetWpnData().pszMuzzleParticles[i].szFirstpersonParticle);
			PrecacheParticleSystem(GetWpnData().pszMuzzleParticles[i].szThirdpersonParticle);
		}

		for (int i = 0; i < GetWpnData().pszSmokeParticles.Count(); i++)
		{
			PrecacheParticleSystem(GetWpnData().pszSmokeParticles[i].szFirstpersonParticle);
			PrecacheParticleSystem(GetWpnData().pszSmokeParticles[i].szThirdpersonParticle);
		}

		for (int i = 0; i < GetWpnData().pszTracerParticles.Count(); i++)
		{
			PrecacheParticleSystem(GetWpnData().pszTracerParticles[i].szFirstpersonParticle);
			PrecacheParticleSystem(GetWpnData().pszTracerParticles[i].szThirdpersonParticle);
		}

		// Precache sounds, too
		for (int i = 0; i < NUM_SHOOT_SOUND_TYPES; ++i)
		{
			const char *shootsound = GetShootSound(i);
			if (shootsound && shootsound[0])
			{
				CBaseEntity::PrecacheScriptSound(shootsound);
			}
		}
	}
	else
	{
		// Couldn't read data file, remove myself
		Warning("Error reading weapon data file for: %s\n", GetClassname());
		//	Remove( );	//don't remove, this gets released soon!
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get my data in the file weapon info array
//-----------------------------------------------------------------------------
const FileWeaponInfo_t &CBaseCombatWeapon::GetWpnData( void ) const
{
	return *GetFileWeaponInfoFromHandle( m_hWeaponFileInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetViewModel(int viewmodelindex/*viewmodelindex = 0 -- this is ignored in the base class here*/) const
{
	return GetWpnData().szViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetWorldModel( void ) const
{
	return GetWpnData().szWorldModel;
}

const char *CBaseCombatWeapon::GetAttachmentLink( void ) const
{
	return GetWpnData().szAttachmentLink;
}

//-----------------------------------------------------------------------------
// Purpose: Returns attachment coordinates for weapons on the back/hip, etc...
//-----------------------------------------------------------------------------
const Vector &CBaseCombatWeapon::GetAttachmentPositionOffset( void ) const
{
	return GetWpnData().vecAttachmentPosOffset;
}

//-----------------------------------------------------------------------------
// Purpose: Returns attachment coordinates for weapons on the back/hip, etc...
//-----------------------------------------------------------------------------
const QAngle &CBaseCombatWeapon::GetAttachmentAngleOffset( void ) const
{
	return GetWpnData().angAttachmentAngOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetPrintName( void ) const
{
	return GetWpnData().szPrintName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetMaxClip(void) const
{
	return GetWpnData().iMaxClip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetDefaultClip(void) const
{
	return GetWpnData().iDefaultClip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::UsesClipsForAmmo(void) const
{
	return (GetMaxClip() != WEAPON_NOCLIP);
}

bool CBaseCombatWeapon::IsMeleeWeapon() const
{
	return GetWpnData().m_bMeleeWeapon;
}

float CBaseCombatWeapon::GetSpecialDamage() const
{
	return GetWpnData().m_flSpecialDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetWeight( void ) const
{
	return GetWpnData().iWeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetWeaponFlags( void ) const
{
	return GetWpnData().iFlags;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetSlot( void ) const
{
	return GetWpnData().iSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetName( void ) const
{
	return GetWpnData().szClassName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteActive( void ) const
{
	return GetWpnData().iconActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteInactive( void ) const
{
	return GetWpnData().iconInactive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *CBaseCombatWeapon::GetSpriteAmmo( void ) const
{
	return GetWpnData().iconAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseCombatWeapon::GetShootSound( int iIndex ) const
{
	return GetWpnData().aShootSounds[ iIndex ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetRumbleEffect() const
{
	return GetWpnData().iRumbleEffect;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseCombatCharacter	*CBaseCombatWeapon::GetOwner() const
{
	return ToBaseCombatCharacter( m_hOwner.Get() );
}	

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : BaseCombatCharacter - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetOwner( CBaseCombatCharacter *owner )
{
	if ( !owner )
	{ 
#ifndef CLIENT_DLL
		// Make sure the weapon updates its state when it's removed from the player
		// We have to force an active state change, because it's being dropped and won't call UpdateClientData()
		int iOldState = m_iState;
		m_iState = WEAPON_NOT_CARRIED;
		OnActiveStateChanged( iOldState );
#endif

		// make sure we clear out our HideThink if we have one pending
		SetContextThink( NULL, 0, HIDEWEAPON_THINK_CONTEXT );
	}

	m_hOwner = owner;

#ifndef CLIENT_DLL
	DispatchUpdateTransmitState();
#else
	UpdateVisibility();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Return false if this weapon won't let the player switch away from it
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::IsAllowedToSwitch( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon can be selected via the weapon selection
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::CanBeSelected( void )
{
	if ( !VisibleInWeaponSelection() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasAmmo( void )
{
	// Weapons with no ammo types can always be selected
	if (GetAmmoTypeID() == -1)
		return true;

	if (GetWeaponFlags() & ITEM_FLAG_SELECTONEMPTY)
		return true;

	return (m_iClip > 0 || GetAmmoCount() > 0);
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon should be seen, and hence be selectable, in the weapon selection
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::VisibleInWeaponSelection( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasWeaponIdleTimeElapsed( void )
{
	if ( gpGlobals->curtime > m_flTimeWeaponIdle )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetWeaponIdleTime( float time )
{
	m_flTimeWeaponIdle = time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBaseCombatWeapon::GetWeaponIdleTime( void )
{
	return m_flTimeWeaponIdle;
}

//-----------------------------------------------------------------------------
// Purpose: Drop/throw the weapon with the given velocity.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Drop( const Vector &vecVelocity )
{
	m_bFiringWholeClip = false;
	m_bInReload = false;

#if !defined( CLIENT_DLL )
	m_bCanRemoveWeapon = true;
	m_bWantsHolster = false;
	m_flViewKickPenalty = 1.0f;
	m_flViewKickTime = 0.0f;

	m_iMeleeAttackType = 0;
	m_flLastTraceTime = 0.0f;
	m_flMeleeCooldown = 0.0f;

	//If it was dropped then there's no need to respawn it.
	AddSpawnFlags( SF_NORESPAWN );
	RemoveSpawnFlags(SF_WEAPON_NO_MOTION | SF_WEAPON_NO_PLAYER_PICKUP);
	ResetAllParticles();
	StopAnimation();
	StopFollowingEntity( );
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetGlowMode(GLOW_MODE_RADIUS);
	// clear follow stuff, setup for collision
	SetGravity(1.0);
	m_iState = WEAPON_NOT_CARRIED;
	RemoveEffects( EF_NODRAW );
	FallInit();
	SetGroundEntity( NULL );
	SetThink( &CBaseCombatWeapon::SetPickupTouch );
	SetTouch(NULL);

	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( pObj != NULL )
	{
		AngularImpulse	angImp( 200, 200, 200 );
		pObj->AddVelocity( &vecVelocity, &angImp );
	}
	else
	{
		SetAbsVelocity( vecVelocity );
	}

	CBaseEntity *pOwner = GetOwnerEntity();

	SetNextThink( gpGlobals->curtime + 1.0f );
	SetOwnerEntity( NULL );
	SetOwner( NULL );

	if ( pOwner )
	{
		if (pOwner->IsNPC())
		{
			// If we're not allowing to spawn due to the gamerules,
			// remove myself when I'm dropped by an NPC.
			if (g_pGameRules->IsAllowedToSpawn(this) == false)
			{
				UTIL_Remove(this);
				return;
			}
		}
		else if (pOwner->IsPlayer())
		{
			CHL2MP_Player *pPlayerOwner = ToHL2MPPlayer(pOwner);
			if (pPlayerOwner)
				pPlayerOwner->CheckShouldEnableFlashlightOnSwitch();
		}
	}
#endif
}

void CBaseCombatWeapon::ResetAllParticles(void)
{
	// This doesn't actually do what it is supposed to do.
//#if !defined( CLIENT_DLL )
//	CBaseViewModel *vm = NULL;
//	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
//	if (pOwner)
//		vm = pOwner->GetViewModel(m_nViewModelIndex);
//
//	StopParticleEffects(this);
//	if (vm)
//		StopParticleEffects(vm);
//#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPicker - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
#if !defined( CLIENT_DLL )
	RemoveEffects( EF_ITEM_BLINK );

	if( pNewOwner->IsPlayer() )
	{
		m_OnPlayerPickup.FireOutput(pNewOwner, this);

		// Play the pickup sound for 1st-person observers
		CRecipientFilter filter;
		for ( int i=1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *player = UTIL_PlayerByIndex(i);
			if ( player && !player->IsAlive() && player->GetObserverMode() == OBS_MODE_IN_EYE )
			{
				filter.AddRecipient( player );
			}
		}
		if ( filter.GetRecipientCount() )
		{
			CBaseEntity::EmitSound( filter, pNewOwner->entindex(), "Player.PickupWeapon" );
		}

		// Robin: We don't want to delete weapons the player has picked up, so 
		// clear the name of the weapon. This prevents wildcards that are meant 
		// to find NPCs finding weapons dropped by the NPCs as well.
		SetName( NULL_STRING );
	}
	else
	{
		m_OnNPCPickup.FireOutput(pNewOwner, this);
	}

	m_bCanRemoveWeapon = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecTracerSrc - 
//			&tr - 
//			iTracerType - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CBaseEntity *pOwner = GetOwner();
	if ( pOwner == NULL )
	{
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		return;
	}

	const char *pszTracerName = GetTracerType();

	Vector vNewSrc = vecTracerSrc;
	int iEntIndex = entindex();
	int iAttachment = (IsAkimboWeapon() ? LookupAttachment(GetMuzzleflashAttachment(DidFirePrimary())) : GetTracerAttachment());

	// Players fire off particle based tracers...
	if (pOwner->IsPlayer())
	{
		UTIL_ParticleTracer(GetParticleEffect(PARTICLE_TYPE_TRACER), vNewSrc, tr.endpos, iEntIndex, iAttachment);
		return;
	}

	switch ( iTracerType )
	{
	case TRACER_LINE:
		UTIL_Tracer( vNewSrc, tr.endpos, iEntIndex, iAttachment, 0.0f, true, pszTracerName );
		break;

	case TRACER_LINE_AND_WHIZ:
		UTIL_Tracer( vNewSrc, tr.endpos, iEntIndex, iAttachment, 0.0f, true, pszTracerName );
		break;
	}
}

void CBaseCombatWeapon::GiveTo( CBaseEntity *pOther )
{
	DefaultTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Default Touch function for player picking up a weapon (not AI)
// Input  : pOther - the entity that touched me
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::DefaultTouch( CBaseEntity *pOther )
{
}

void CBaseCombatWeapon::SetPickupTouch(void)
{
#if !defined( CLIENT_DLL )
	SetTouch(&CBaseCombatWeapon::DefaultTouch);

	// If this weapon has been dropped:
	if (m_bCanRemoveWeapon)
	{
		// Non-Special Weps Will be removed after 15 sec:
		if (CanRespawnWeapon())
		{
			SetThink(&CBaseEntity::SUB_Remove);
			SetNextThink(gpGlobals->curtime + (HL2MPRules()->IsFastPacedGameplay() ? 45.0f : 15.0f));
		}
		else
		{
			SetThink(&CBaseCombatWeapon::ForceOriginalPosition);
			SetNextThink(gpGlobals->curtime + 10.0f);
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Become a child of the owner (MOVETYPE_FOLLOW)
//			disables collisions, touch functions, thinking
// Input  : *pOwner - new owner/operator
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Equip( CBaseCombatCharacter *pOwner )
{
	// Attach the weapon to an owner
	SetAbsVelocity( vec3_origin );
	RemoveSolidFlags( FSOLID_TRIGGER );
	FollowEntity(pOwner);
	SetOwner( pOwner );
	SetOwnerEntity( pOwner );

	// Break any constraint I might have to the world.
	RemoveEffects( EF_ITEM_BLINK );

#if !defined( CLIENT_DLL )
	SetGlowMode(GLOW_MODE_NONE);
#endif

	m_flNextPrimaryAttack		= gpGlobals->curtime;
	m_flNextSecondaryAttack		= gpGlobals->curtime;
	m_flMeleeCooldown = 0.0f;
	m_flNextBashAttack = gpGlobals->curtime;
	SetTouch( NULL );
	SetThink( NULL );
#if !defined( CLIENT_DLL )
	VPhysicsDestroyObject();
#endif

	if ( pOwner->IsPlayer() )
	{
		SetModel( GetViewModel() );
	}
	else
	{
		// Make the weapon ready as soon as any NPC picks it up.
		m_flNextPrimaryAttack = gpGlobals->curtime;
		m_flNextSecondaryAttack = gpGlobals->curtime;
		SetModel( GetWorldModel() );
	}
}

void CBaseCombatWeapon::SetActivity( Activity act, float duration ) 
{ 
	//Adrian: Oh man...
#ifndef CLIENT_DLL
	SetModel( GetWorldModel() );
#endif

	int sequence = SelectWeightedSequence( act ); 

	// FORCE IDLE on sequences we don't have (which should be many)
	if ( sequence == ACTIVITY_NOT_AVAILABLE )
		sequence = SelectWeightedSequence( ACT_VM_IDLE );

	//Adrian: Oh man again...
#ifndef CLIENT_DLL
	if (GetOwner() && GetOwner()->IsPlayer())
		SetModel(GetViewModel());
#endif

	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		SetSequence( sequence );
		SetActivity( act ); 
		SetCycle( 0 );
		ResetSequenceInfo( );

		if ( duration > 0 )
		{
			// FIXME: does this even make sense in non-shoot animations?
			m_flPlaybackRate = SequenceDuration( sequence ) / duration;
			m_flPlaybackRate = MIN( m_flPlaybackRate, 12.0);  // FIXME; magic number!, network encoding range
		}
		else
		{
			m_flPlaybackRate = 1.0;
		}
	}
}

//====================================================================================
// WEAPON CLIENT HANDLING
//====================================================================================
int CBaseCombatWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	int iNewState = WEAPON_IS_CARRIED_BY_PLAYER;

	if ( pPlayer->GetActiveWeapon() == this )
	{
		iNewState = WEAPON_IS_ACTIVE;
	}
	else
	{
		iNewState = WEAPON_IS_CARRIED_BY_PLAYER;
	}

	if ( m_iState != iNewState )
	{
		int iOldState = m_iState;
		m_iState = iNewState;
		OnActiveStateChanged( iOldState );
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetViewModelIndex( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );
	m_nViewModelIndex = index;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iActivity - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SendViewModelAnim( int nSequence )
{
#if defined( CLIENT_DLL )
	if ( !IsPredicted() )
		return;
#endif

	if ( nSequence < 0 )
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex, false );

	if ( vm == NULL )
		return;

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	vm->SendViewModelMatchingSequence( nSequence );
}

float CBaseCombatWeapon::GetViewModelSequenceDuration()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
	{
		Assert( false );
		return 0;
	}

	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		Assert( false );
		return 0;
	}

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	return vm->SequenceDuration();
}

bool CBaseCombatWeapon::IsViewModelSequenceFinished( void )
{
	// These are not valid activities and always complete immediately
	if ( GetActivity() == ACT_RESET || GetActivity() == ACT_INVALID )
		return true;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
	{
		Assert( false );
		return false;
	}

	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
	{
		Assert( false );
		return false;
	}

	return vm->IsSequenceFinished();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetViewModel()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;
	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex, false );
	if ( vm == NULL )
		return;
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	vm->SetWeaponModel( GetViewModel( m_nViewModelIndex ), this );
}

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::SendWeaponAnim( int iActivity )
{
	//For now, just set the ideal activity and be done with it
	return SetIdealActivity( (Activity) iActivity );
}

//====================================================================================
// WEAPON SELECTION
//====================================================================================

//-----------------------------------------------------------------------------
// Purpose: Returns true if the weapon currently has ammo or doesn't need ammo
// Output :
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::HasAnyAmmo(void)
{
	// If I don't use ammo of any kind, I can always fire
	if (GetAmmoTypeID() == -1)
		return true;

	if (UsesClipsForAmmo() && (m_iClip > 0))
		return true;

	return (GetAmmoCount() > 0);
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
	}

	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
		if ( vm )
		{
			vm->RemoveEffects( EF_NODRAW );
		}
	}
	else
	{
		AddEffects( EF_NODRAW );
		if ( vm )
		{
			vm->AddEffects(EF_NODRAW);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::IsWeaponVisible( void )
{
	CBaseViewModel *vm = NULL;
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		vm = pOwner->GetViewModel( m_nViewModelIndex );
		if ( vm )
			return ( !vm->IsEffectActive(EF_NODRAW) );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szViewModel - 
//			*szWeaponModel - 
//			iActivity - 
//			*szAnimExt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity )
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		// Dead men deploy no weapons
		if (pOwner->IsAlive() == false)
			return false;

		SetViewModel();
		PlayAnimation(iActivity);
		pOwner->SetNextAttack(gpGlobals->curtime + SequenceDuration());
	}

	// Can't shoot again until we've finished deploying
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration() + 0.05f;
	m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration() + 0.05f;
	m_flMeleeCooldown = 0.0f;
	m_flNextBashAttack = gpGlobals->curtime;

	m_bWantsHolster = false;
	m_iMeleeAttackType = 0;
	
#ifndef CLIENT_DLL
	m_flLastTraceTime = 0.0f;
#endif

	WeaponSound( DEPLOY );

	SetWeaponVisible( true );

	SetContextThink( NULL, 0, HIDEWEAPON_THINK_CONTEXT );

	m_iShotsFired = 0;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Check if you can holster or deploy!
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::CanHolster( void )
{
	CHL2MP_Player* pClient = ToHL2MPPlayer(GetOwner());
	if (pClient && !pClient->IsAlive())
		return false;

	return true;
}

bool CBaseCombatWeapon::CanDeploy( void )
{
	CHL2MP_Player *pClient = ToHL2MPPlayer( GetOwner() );
	if ( pClient ) 
	{
		if ( !pClient->IsAlive() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::Deploy( )
{
	MDLCACHE_CRITICAL_SECTION();
	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), GetDrawActivity() );
}

Activity CBaseCombatWeapon::GetDrawActivity(void)
{
	if (UsesEmptyAnimation() && (m_iClip <= 0))
		return ACT_VM_DRAW_EMPTY;

	return ACT_VM_DRAW;
}

Activity CBaseCombatWeapon::GetHolsterActivity(void)
{
	if (UsesEmptyAnimation() && (m_iClip <= 0))
		return ACT_VM_HOLSTER_EMPTY;

	return ACT_VM_HOLSTER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{ 
	MDLCACHE_CRITICAL_SECTION();

	// kill any think functions
	SetThink(NULL);

	SetWeaponVisible( false );

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
		pOwner->Weapon_DeployNextWeapon();

	return true;
}

bool CBaseCombatWeapon::FullHolster(void)
{
	MDLCACHE_CRITICAL_SECTION();

	m_bInReload = false;
	m_bFiringWholeClip = false;
	ResetAllParticles();
	SetThink(NULL);
	SetWeaponVisible(false);

	return true;
}

#ifdef CLIENT_DLL

void CBaseCombatWeapon::BoneMergeFastCullBloat( Vector &localMins, Vector &localMaxs, const Vector &thisEntityMins, const Vector &thisEntityMaxs ) const
{
	// The default behavior pushes it out by BONEMERGE_FASTCULL_BBOX_EXPAND in all directions, but we can do better
	// since we know the weapon will never point behind him.

	localMaxs.x += 20;	// Leaves some space in front for long weapons.

	localMins.y -= 20;	// Fatten it to his left and right since he can rotate that way.
	localMaxs.y += 20;	

	localMaxs.z += 15;	// Leave some space at the top.
}

#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::InputHideWeapon( inputdata_t &inputdata )
{
	// Only hide if we're still the active weapon. If we're not the active weapon
	if ( GetOwner() && GetOwner()->GetActiveWeapon() == this )
	{
		SetWeaponVisible( false );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::HideThink( void )
{
	// Only hide if we're still the active weapon. If we're not the active weapon
	if ( GetOwner() && GetOwner()->GetActiveWeapon() == this )
		SetWeaponVisible( false );
}

bool CBaseCombatWeapon::CanReload( void )
{
	if ( AutoFiresFullClip() && m_bFiringWholeClip )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::ItemPreFrame( void )
{
	MaintainIdealActivity();
}

void CBaseCombatWeapon::SetupWeaponRanges(void)
{
	m_fMinRange1 = (float)GetWpnData().m_iRangeMin;
	m_fMaxRange1 = (float)GetWpnData().m_iRangeMax;

	m_fMinRange2 = (float)GetWpnData().m_iRangeMin;
	m_fMaxRange2 = (float)GetWpnData().m_iRangeMax;
}

//-----------------------------------------------------------------------------
// Purpose: Play a certain anim and set the time until allowing the next "play"
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::PlayAnimation( int iActivity )
{
	SendWeaponAnim( iActivity );

	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
}

//====================================================================================
// Whenever we holster (want to holster) we start a timer which will let us fully holster then we do the holster to the new wep.
//====================================================================================
void CBaseCombatWeapon::HandleWeaponSelectionTime(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	if (m_bWantsHolster && (m_flHolsteredTime <= gpGlobals->curtime))
	{
		m_bWantsHolster = false;
		Holster();
	}
}

//====================================================================================
// Animation goes here, such as lowering and other shared qualities which every wep should have despite its type!
//====================================================================================
void CBaseCombatWeapon::GenericBB2Animations( void )
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer( GetOwner() );
	if (!pOwner)
		return;

	DoWeaponFX();

	// Prevent bash if we're a melee wep.
	if (IsMeleeWeapon() || !CanPerformMeleeAttacks())
		return;

	MeleeAttackUpdate();

	if ((m_iMeleeAttackType <= 0) && ((pOwner->m_nButtons & IN_BASH) && (m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime) && (m_flNextBashAttack <= gpGlobals->curtime)))
	{	
		WeaponSound(MELEE_MISS);
		pOwner->ViewPunch(QAngle(2, 2, 0));

		int bashAct = ACT_VM_MELEE;
		// Send the anim
		if (UsesEmptyAnimation() && (m_iClip <= 0))
			bashAct = ACT_VM_MELEE_EMPTY;

		PlayAnimation(bashAct);

		m_flNextBashAttack = gpGlobals->curtime + GetViewModelSequenceDuration() + 0.1f;

		pOwner->DoAnimationEvent(PLAYERANIMEVENT_BASH, bashAct);
	}
}

//====================================================================================
// Weapon Effects will be handled here... ~Bernt.
//====================================================================================
// Default 3 bullets will cause the wep to overheat and will do some sort of visual effect... Every wep can override this one. IMPORTANT. 
int CBaseCombatWeapon::GetOverloadCapacity()
{
	return 3;
}

void CBaseCombatWeapon::DoWeaponFX( void )
{
	if( m_iShotsFired > GetOverloadCapacity() )
	{
		// Dispatch smoke... Overload smoke.
		CBasePlayer *pPlayer = ToBasePlayer(this->GetOwner());
		if (pPlayer)
		{
			if (IsAkimboWeapon())
			{
				DispatchParticleEffect(GetParticleEffect(PARTICLE_TYPE_SMOKE), PATTACH_POINT_FOLLOW, pPlayer->GetViewModel(), GetMuzzleflashAttachment(true));
				DispatchParticleEffect(GetParticleEffect(PARTICLE_TYPE_SMOKE), PATTACH_POINT_FOLLOW, pPlayer->GetViewModel(), GetMuzzleflashAttachment(false));
			}
			else
				DispatchParticleEffect(GetParticleEffect(PARTICLE_TYPE_SMOKE), PATTACH_POINT_FOLLOW, pPlayer->GetViewModel(), "muzzle");
		}
		else
			DispatchParticleEffect(GetParticleEffect(PARTICLE_TYPE_SMOKE), PATTACH_POINT_FOLLOW, this, "muzzle");

		m_iShotsFired = 0;
		OnWeaponOverload();
	}
}

//====================================================================================
// Called X sec after a special wep has been dropped. We add a special wep to be moved back to its original position.
//====================================================================================
void CBaseCombatWeapon::ForceOriginalPosition()
{
#ifndef CLIENT_DLL

	// BB2 WARN 
	Vector originalPos = HL2MPRules()->VecWeaponRespawnSpot(this);
	QAngle angles = GetAbsAngles();

	float flDistanceFromSpawn = (originalPos - GetAbsOrigin()).Length();
	if (flDistanceFromSpawn > 64.0f)
	{
		bool shouldReset = false;
		IPhysicsObject *pPhysics = VPhysicsGetObject();
		if (pPhysics)
			shouldReset = pPhysics->IsAsleep();
		else
			shouldReset = (GetFlags() & FL_ONGROUND) ? true : false;

		if (shouldReset)
		{
			Teleport(&originalPos, &angles, NULL);
			IPhysicsObject *pPhys = VPhysicsGetObject();
			if (pPhys)
				pPhys->Wake();
		}
	}

#endif

	SetThink(NULL);
}

//====================================================================================
// Proper Holster
//====================================================================================
void CBaseCombatWeapon::StartHolsterSequence()
{
	m_bWantsHolster = true;
	m_bInReload = false;
	m_bFiringWholeClip = false;

	m_flViewKickPenalty = 1.0f;
	m_flViewKickTime = 0.0f;

	// Send holster animation
	Activity act = GetHolsterActivity();
	if (act != ACT_INVALID)
		SendWeaponAnim(act);

	float flHolsterTime = gpGlobals->curtime + GetViewModelSequenceDuration();
	if ((HL2MPRules() && HL2MPRules()->IsFastPacedGameplay()) || (act == ACT_INVALID))
		flHolsterTime = gpGlobals->curtime + 0.1f;

	m_flHolsteredTime = flHolsterTime;
	m_flNextPrimaryAttack = flHolsterTime + 0.05f;
	m_flNextSecondaryAttack = flHolsterTime + 0.05f;
	m_flMeleeCooldown = 0.0f;

	ResetAllParticles();

	// We will stay active in the pre frame until we're done so we don't override the visibility state or animation state as we change to the new active weapon.
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (pOwner)
	{
		pOwner->m_flNextAttack = flHolsterTime + 0.05f;

#ifndef CLIENT_DLL
		pOwner->CheckShouldEnableFlashlightOnSwitch();
#endif
	}
}

const char *CBaseCombatWeapon::GetParticleEffect(int iType, bool bThirdperson)
{
	int index = 0;
	switch (iType)
	{
	case PARTICLE_TYPE_MUZZLE:
		if (!GetWpnData().pszMuzzleParticles.Count())
			break;

		index = random->RandomInt(0, (GetWpnData().pszMuzzleParticles.Count() - 1));
		return (bThirdperson ? GetWpnData().pszMuzzleParticles[index].szThirdpersonParticle : GetWpnData().pszMuzzleParticles[index].szFirstpersonParticle);
	case PARTICLE_TYPE_SMOKE:
		if (!GetWpnData().pszSmokeParticles.Count())
			break;

		index = random->RandomInt(0, (GetWpnData().pszSmokeParticles.Count() - 1));
		return (bThirdperson ? GetWpnData().pszSmokeParticles[index].szThirdpersonParticle : GetWpnData().pszSmokeParticles[index].szFirstpersonParticle);
	case PARTICLE_TYPE_TRACER:
		if (!GetWpnData().pszTracerParticles.Count())
			break;

		index = random->RandomInt(0, (GetWpnData().pszTracerParticles.Count() - 1));
		return (bThirdperson ? GetWpnData().pszTracerParticles[index].szThirdpersonParticle : GetWpnData().pszTracerParticles[index].szFirstpersonParticle);
	}

	return "";
}

#ifndef CLIENT_DLL
void CBaseCombatWeapon::SetBloodState(bool value)
{ 
	m_bIsBloody.Set(value); 

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	pPlayer->AddMaterialOverlayFlag(MAT_OVERLAY_BLOOD);
}
#endif

//====================================================================================
// WEAPON BEHAVIOUR
//====================================================================================
void CBaseCombatWeapon::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	GenericBB2Animations();

	UpdateAutoFire();

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = ( pOwner->m_nButtons & IN_ATTACK ) ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;

	if (UsesClipsForAmmo())
		CheckReload();

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		bool bHasEmptyClipOrAmmo = (UsesClipsForAmmo() && (m_iClip <= 0)) || (!UsesClipsForAmmo() && (GetAmmoCount() <= 0));

		if (!IsMeleeWeapon() && bHasEmptyClipOrAmmo)		
			HandleFireOnEmpty();
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			if ((pOwner->m_afButtonPressed & IN_ATTACK) || (pOwner->m_afButtonReleased & IN_ATTACK2))
				m_flNextPrimaryAttack = gpGlobals->curtime;

			PrimaryAttack();
			if (AutoFiresFullClip())
				m_bFiringWholeClip = true;
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if ( ( pOwner->m_nButtons & IN_RELOAD ) && UsesClipsForAmmo() && !m_bInReload ) 
	{
		// reload when reload is pressed
		Reload();
		m_fFireDuration = 0.0f;
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (((m_flNextBashAttack <= gpGlobals->curtime) && !(pOwner->m_nButtons & IN_BASH)) && !((pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2) && !(pOwner->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down or reloading
		if (!m_bInReload && m_flNextPrimaryAttack <= gpGlobals->curtime && IsViewModelSequenceFinished())
		{
			WeaponIdle();
		}
	}
}

void CBaseCombatWeapon::HandleFireOnEmpty()
{
	WeaponSound(EMPTY);

	if (IsAkimboWeapon()) // TODO, play shared dryfire for both weps...
		SendWeaponAnim(TryTheLuck(0.5f) ? ACT_VM_SHOOT_LEFT_DRYFIRE : ACT_VM_SHOOT_RIGHT_DRYFIRE);
	else
		SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();
}

int CBaseCombatWeapon::GetReloadActivity(bool bCanDoEmpty)
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return ACT_VM_RELOAD;

	int activity = ACT_VM_RELOAD0;
	int skillType = PLAYER_SKILL_HUMAN_RIFLE_MASTER;
	switch (GetWeaponType())
	{
	case WEAPON_TYPE_SMG:
	case WEAPON_TYPE_PISTOL:
	case WEAPON_TYPE_REVOLVER:
		skillType = PLAYER_SKILL_HUMAN_PISTOL_MASTER;
		break;

	case WEAPON_TYPE_SNIPER:
		skillType = PLAYER_SKILL_HUMAN_SNIPER_MASTER;
		break;

	case WEAPON_TYPE_SHOTGUN:
		skillType = PLAYER_SKILL_HUMAN_SHOTGUN_MASTER;
		break;
	}

	bool bDontDoEmpty = (m_iClip > 0) || !bCanDoEmpty;
	activity = (bDontDoEmpty ? ACT_VM_RELOAD0 : ACT_VM_RELOAD_EMPTY0) + pOwner->GetSkillValue(skillType);
	if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
		activity = bDontDoEmpty ? ACT_VM_RELOAD10 : ACT_VM_RELOAD_EMPTY10;

	return activity;
}

//-----------------------------------------------------------------------------
// Purpose: Called each frame by the player PostThink, if the player's not ready to attack yet
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::ItemBusyFrame( void )
{
	UpdateAutoFire();

	HandleWeaponSelectionTime();
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting bullet type
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CBaseCombatWeapon::GetBulletType( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: We calculate the bullet accuracy (cone).
//-----------------------------------------------------------------------------
Vector CBaseCombatWeapon::GetBulletConeByAccLevel(void)
{
	int iAccuracyLevel = GetWpnData().m_iAccuracy;
	if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay() && (GetWpnData().m_iAccuracyPvP > 0))
		iAccuracyLevel = GetWpnData().m_iAccuracyPvP;

	if (iAccuracyLevel <= 0) // This weapon uses new accuracy.
	{
		float delta = (gpGlobals->curtime - m_flViewKickTime);
		if (delta >= 0.5f)
			m_flViewKickPenalty = 1.0f;
		else
			m_flViewKickPenalty += (1.0f - (MAX((delta - GetWpnData().m_flFireRate), 0.0f) / 0.5f)) * GetWpnData().m_flAccuracyFactor;

		m_flViewKickTime = gpGlobals->curtime; // reset the timer.		
		iAccuracyLevel = MIN(((int)round(m_flViewKickPenalty)), 10);
	}

	switch (iAccuracyLevel)
	{
	case 1:
		return VECTOR_CONE_1DEGREES;
	case 2:
		return VECTOR_CONE_2DEGREES;
	case 3:
		return VECTOR_CONE_3DEGREES;
	case 4:
		return VECTOR_CONE_4DEGREES;
	case 5:
		return VECTOR_CONE_5DEGREES;
	case 6:
		return VECTOR_CONE_6DEGREES;
	case 7:
		return VECTOR_CONE_7DEGREES;
	case 8:
		return VECTOR_CONE_8DEGREES;
	case 9:
		return VECTOR_CONE_9DEGREES;
	case 10:
		return VECTOR_CONE_10DEGREES;
	case 11:
		return VECTOR_CONE_15DEGREES;
	case 12:
	default:
		return VECTOR_CONE_20DEGREES;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting spread
// Input  :
// Output :
//-----------------------------------------------------------------------------
const Vector& CBaseCombatWeapon::GetBulletSpread(void)
{
	static Vector cone;

	cone = GetBulletConeByAccLevel();

	return cone;
}

//-----------------------------------------------------------------------------
// Purpose: Base class for getting recoil viewkick
// Input  :
// Output :
//-----------------------------------------------------------------------------
QAngle CBaseCombatWeapon::GetViewKickAngle(void)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return QAngle(0, 0, 0);

	m_iShotsFired++;

	QAngle viewKick;
	float flAngX, flAngY, flAngZ;

	flAngX = random->RandomFloat(viewpunch_x_min.GetFloat(), viewpunch_x_max.GetFloat()); // this is the direction we add to as you shoot.
	flAngY = random->RandomFloat(viewpunch_y_min.GetFloat(), viewpunch_y_max.GetFloat());
	flAngZ = random->RandomFloat(viewpunch_z_min.GetFloat(), viewpunch_z_max.GetFloat());

	// We only want to move upwards here!!
	if (flAngX > 0)
		flAngX = 0;

	viewKick = QAngle(flAngX, flAngY, flAngZ);

	return viewKick;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for getting firerate
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CBaseCombatWeapon::GetFireRate( void )
{
	float flFireRate = GetWpnData().m_flFireRate;

	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (pOwner)
	{
		if (pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_SHOUT_AND_SPRAY) > 0)
			flFireRate -= ((flFireRate / 100.0f) * ((((float)pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_SHOUT_AND_SPRAY)) * GetWpnData().m_flSkillFireRateFactor)));
	}

	return flFireRate;
}

//-----------------------------------------------------------------------------
// Purpose: Base class default for playing shoot sound
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::WeaponSound( WeaponSound_t sound_type, float soundtime /* = 0.0f */ )
{
	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *shootsound = GetShootSound( sound_type );
	if ( !shootsound || !shootsound[0] )
		return;

	CSoundParameters params;

	if ( !GetParametersForSound( shootsound, params, NULL ) )
		return;

	if ( params.play_to_owner_only )
	{
		// Am I only to play to my owner?
		if ( GetOwner() && GetOwner()->IsPlayer() )
		{
			CSingleUserRecipientFilter filter( ToBasePlayer( GetOwner() ) );
			if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), shootsound, NULL, soundtime );
		}
	}
	else
	{
		// Play weapon sound from the owner
		if ( GetOwner() )
		{
			CPASAttenuationFilter filter( GetOwner(), params.soundlevel );
			if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, GetOwner()->entindex(), shootsound, NULL, soundtime ); 

#if !defined( CLIENT_DLL )
			if( sound_type == EMPTY )
			{
				CSoundEnt::InsertSound( SOUND_COMBAT, GetOwner()->GetAbsOrigin(), SOUNDENT_VOLUME_EMPTY, 0.2, GetOwner() );
			}
#endif
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			CPASAttenuationFilter filter( this, params.soundlevel );
			if ( IsPredicted() && CBaseEntity::GetPredictionPlayer() )
			{
				filter.UsePredictionRules();
			}
			EmitSound( filter, entindex(), shootsound, NULL, soundtime ); 
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop a sound played by this weapon.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::StopWeaponSound( WeaponSound_t sound_type )
{
	//if ( IsPredicted() )
	//	return;

	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *shootsound = GetShootSound( sound_type );
	if ( !shootsound || !shootsound[0] )
		return;

	CSoundParameters params;
	if ( !GetParametersForSound( shootsound, params, NULL ) )
		return;

	// Am I only to play to my owner?
	if ( params.play_to_owner_only )
	{
		if ( GetOwner() )
		{
			StopSound( GetOwner()->entindex(), shootsound );
		}
	}
	else
	{
		// Play weapon sound from the owner
		if ( GetOwner() )
		{
			StopSound( GetOwner()->entindex(), shootsound );
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			StopSound( entindex(), shootsound );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::DefaultReload(int iClipSize, int iActivity)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;
	
	if (GetAmmoCount() <= 0) // If I don't have any spare ammo, I can't reload
		return false;

	int ammo = UsesClipsForAmmo() ? MIN((iClipSize - m_iClip), GetAmmoCount()) : 0;
	if (ammo <= 0)
		return false;

	m_iMeleeAttackType = 0;

#ifdef CLIENT_DLL
	// Play reload
	WeaponSound( RELOAD );
#endif
	SendWeaponAnim( iActivity );

	MDLCACHE_CRITICAL_SECTION();
	float flSequenceEndTime = gpGlobals->curtime + SequenceDuration();
	pOwner->SetNextAttack( flSequenceEndTime );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = flSequenceEndTime;
	m_bInReload = true;

	return true;
}

bool CBaseCombatWeapon::ReloadsSingly( void ) const
{
	return m_bReloadsSingly;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::Reload( void )
{
	return DefaultReload(GetMaxClip(), ACT_VM_RELOAD);
}

//=========================================================
void CBaseCombatWeapon::WeaponIdle( void )
{
	if (HasWeaponIdleTimeElapsed())
	{
		Activity idle = ACT_VM_IDLE;
		if (UsesEmptyAnimation() && (m_iClip <= 0))
			idle = ACT_VM_IDLE_EMPTY;

		SendWeaponAnim(idle);
	}
}

//=========================================================
Activity CBaseCombatWeapon::GetPrimaryAttackActivity(void)
{
	if (UsesEmptyAnimation() && (m_iClip <= 0))
		return ACT_VM_LASTBULLET;

	return ACT_VM_PRIMARYATTACK;
}

//=========================================================
Activity CBaseCombatWeapon::GetSecondaryAttackActivity( void )
{
	return ACT_VM_SECONDARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: Adds in view kick and weapon accuracy degradation effect
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::AddViewKick( void )
{
	// Firearms use GetViewKickAngle() while melee weapons override this function for now.
}

//====================================================================================
// WEAPON RELOAD TYPES
//====================================================================================
void CBaseCombatWeapon::CheckReload( void )
{
	if ( m_bReloadsSingly )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			if (pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) && (m_iClip > 0))
			{
				m_bInReload = false;
				return;
			}

			// If out of ammo end reload
			if (GetAmmoCount() <= 0)
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			else if (m_iClip < GetMaxClip())
			{
				// Add them to the clip
				m_iClip += 1;
				RemoveAmmo(1);
				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				return;
			}
		}
	}
	else
	{
		if ( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			FinishReload();
			m_flNextPrimaryAttack	= gpGlobals->curtime;
			m_flNextSecondaryAttack = gpGlobals->curtime;
			m_bInReload = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::FinishReload(void)
{
	int ammo = UsesClipsForAmmo() ? MIN((GetMaxClip() - m_iClip), GetAmmoCount()) : 0;
	m_iClip += ammo;
	RemoveAmmo(ammo);
	if (m_bReloadsSingly)
		m_bInReload = false;
}

//-----------------------------------------------------------------------------
// Purpose: Abort any reload we have in progress
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::AbortReload( void )
{
#ifdef CLIENT_DLL
	StopWeaponSound( RELOAD ); 
#endif
	m_bInReload = false;
}

void CBaseCombatWeapon::UpdateAutoFire( void )
{
	if ( !AutoFiresFullClip() )
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if (m_iClip == 0)
	{
		// Ready to reload again
		m_bFiringWholeClip = false;
	}

	if ( m_bFiringWholeClip )
	{
		// If it's firing the clip don't let them repress attack to reload
		pOwner->m_nButtons &= ~IN_ATTACK;
	}

	// Don't use the regular reload key
	if ( pOwner->m_nButtons & IN_RELOAD )
	{
		pOwner->m_nButtons &= ~IN_RELOAD;
	}

	// Try to fire if there's ammo in the clip and we're not holding the button
	bool bReleaseClip = m_iClip > 0 && !(pOwner->m_nButtons & IN_ATTACK);
	if ( !bReleaseClip )
	{
		if ( CanReload() && ( pOwner->m_nButtons & IN_ATTACK ) )
		{
			// Convert the attack key into the reload key
			pOwner->m_nButtons |= IN_RELOAD;
		}

		// Don't allow attack button if we're not attacking
		pOwner->m_nButtons &= ~IN_ATTACK;
	}
	else
	{
		// Fake the attack key
		pOwner->m_nButtons |= IN_ATTACK;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Primary fire button attack
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::PrimaryAttack( void )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	// Abort here to handle burst and auto fire modes
	if ((UsesClipsForAmmo() && m_iClip <= 0) || (!UsesClipsForAmmo() && !GetAmmoCount()))
		return;

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	info.m_vecFirstStartPos = pPlayer->GetLocalOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;
	info.m_vecDirShooting = pPlayer->GetAutoaimVector();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo() )
	{
		info.m_iShots = MIN(info.m_iShots, m_iClip);
		m_iClip -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN(info.m_iShots, GetAmmoCount());
		RemoveAmmo(info.m_iShots);
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = GetAmmoTypeID();
	info.m_iTracerFreq = 2;
	info.m_vecSpread = pPlayer->GetAttackSpread( this );

	pPlayer->FireBullets( info );

	// Do the viewkick
	pPlayer->ViewPunch(GetViewKickAngle());

	int shootAct = GetPrimaryAttackActivity();
	SendWeaponAnim(shootAct);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
}

#define TRACE_FREQUENCY 0.0225f

void CBaseCombatWeapon::MeleeAttackStart(int type)
{
	if (m_iMeleeAttackType > 0 || !CanPerformMeleeAttacks())
		return;

	m_iMeleeAttackType = type;

#ifdef CLIENT_DLL
	MeleeAttackTrace();
#else
	m_pEnemiesStruck.Purge();
	m_flLastTraceTime = 0.0f;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (pPlayer)
		pPlayer->MeleeSwingSound(!IsMeleeWeapon());
#endif
}

void CBaseCombatWeapon::MeleeAttackEnd(void)
{
	if (m_iMeleeAttackType <= 0)
		return;

	m_iMeleeAttackType = 0;

#ifndef CLIENT_DLL
	m_pEnemiesStruck.Purge();
	m_flLastTraceTime = 0.0f;
#endif
}

void CBaseCombatWeapon::MeleeAttackUpdate(void)
{
	if (m_iMeleeAttackType <= 0)
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

#ifndef CLIENT_DLL
	if (m_flLastTraceTime <= gpGlobals->curtime)
	{
		m_flLastTraceTime = gpGlobals->curtime + TRACE_FREQUENCY;
		MeleeAttackTrace();
	}
#endif
}

void CBaseCombatWeapon::MeleeAttackTrace(void)
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return;

	CBaseViewModel *pvm = pOwner->GetViewModel();
	if (!pvm)
		return;

	float range = GetRange();

	trace_t traceHit;
	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;
	AngleVectors(pOwner->EyeAngles(), &forward);

	VectorNormalize(forward);
	Vector swingEnd = swingStart + (forward * range);

	IPredictionSystem::SuppressHostEvents(NULL);
	Activity activity = pvm->GetSequenceActivity(pvm->GetSequence());

	lagcompensation->TraceRealtime(pOwner, swingStart, swingEnd, -Vector(5, 5, 5), Vector(5, 5, 5), &traceHit, LAGCOMP_TRACE_REVERT_HULL, range);
	forward = (traceHit.endpos - traceHit.startpos);
	VectorNormalize(forward);

	if (traceHit.fraction == 1.0f)
	{
		if (CanHitThisTarget(-1) && ImpactWater(swingStart, swingEnd))
			StruckTarget(-1);
	}
	else
	{
		CBaseEntity *pHitEnt = traceHit.m_pEnt;
		if (pHitEnt != NULL)
		{
			if (!CanHitThisTarget(pHitEnt->entindex()))
				return;

			if (m_pEnemiesStruck.Count() <= 1)
				AddViewKick();

			StruckTarget(pHitEnt->entindex());
		
			CTakeDamageInfo info(GetOwner(), GetOwner(), GetDamageForActivity(activity), GetMeleeDamageType());
			CalculateMeleeDamageForce(&info, forward, traceHit.endpos);
			info.SetSkillFlags(GetMeleeSkillFlags());

			pHitEnt->DispatchTraceAttack(info, forward, &traceHit);
			ApplyMultiDamage();

			// Now hit all triggers along the ray that... 
			TraceAttackToTriggers(info, traceHit.startpos, traceHit.endpos, forward);

			// push the enemy away if you're bashing..
			CAI_BaseNPC *m_pNPC = pHitEnt->MyNPCPointer();
			if ((m_iMeleeAttackType > MELEE_TYPE_SLASH) && pHitEnt->IsNPC() && m_pNPC && (pHitEnt->IsMercenary() || pHitEnt->IsZombie(true)))
			{
				if ((m_pNPC->GetNavType() != NAV_CLIMB) && !m_pNPC->IsBreakingDownObstacle())
				{
					Vector vecExtraVelocity = (forward * GetWpnData().m_flBashForce);
					vecExtraVelocity.z = 0; // Don't send them flying upwards...
					pHitEnt->SetAbsVelocity(pHitEnt->GetAbsVelocity() + vecExtraVelocity);
				}
			}

			if (pHitEnt->IsNPC() || pHitEnt->IsPlayer())
				WeaponSound(MELEE_HIT);
			else
				WeaponSound(MELEE_HIT_WORLD);
		}

		// See if we hit water (we don't do the other impact effects in this case)
		if (ImpactWater(traceHit.startpos, traceHit.endpos))
			return;

		UTIL_ImpactTrace(&traceHit, GetMeleeDamageType());
	}
#endif
}

#ifndef CLIENT_DLL
bool CBaseCombatWeapon::WantsLagCompensation(const CBaseEntity *pEntity)
{
	if (pEntity && (m_iMeleeAttackType.Get() > 0) && m_pEnemiesStruck.Count())
		return CanHitThisTarget(pEntity->entindex());

	return true;
}

bool CBaseCombatWeapon::CanHitThisTarget(int index)
{
	for (int i = 0; i < m_pEnemiesStruck.Count(); i++)
	{
		if (m_pEnemiesStruck[i] == index)
			return false;
	}

	return true;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &traceHit - 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::ImpactWater(const Vector &start, const Vector &end)
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...

	// We must start outside the water
	if (UTIL_PointContents(start) & (CONTENTS_WATER | CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if (!(UTIL_PointContents(end) & (CONTENTS_WATER | CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine(start, end, (CONTENTS_WATER | CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace);

	if (waterTrace.fraction < 1.0f)
	{
#ifndef CLIENT_DLL
		CEffectData	data;

		data.m_fFlags = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if (waterTrace.contents & CONTENTS_SLIME)
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect("watersplash", data);
#endif
	}

	return true;
}

float CBaseCombatWeapon::GetDamageForActivity(Activity hitActivity)
{
	return GetSpecialDamage();
}

float CBaseCombatWeapon::GetRange(void)
{
	return GetWpnData().m_flBashRange;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame to check if the weapon is going through transition animations
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::MaintainIdealActivity( void )
{
	// Must be transitioning
	if ( GetActivity() != ACT_TRANSITION )
		return;

	// Must not be at our ideal already 
	if ( ( GetActivity() == m_IdealActivity ) && ( GetSequence() == m_nIdealSequence ) )
		return;

	// Must be finished with the current animation
	if ( IsViewModelSequenceFinished() == false )
		return;

	// Move to the next animation towards our ideal
	SendWeaponAnim( m_IdealActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the ideal activity for the weapon to be in, allowing for transitional animations inbetween
// Input  : ideal - activity to end up at, ideally
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::SetIdealActivity( Activity ideal )
{
	MDLCACHE_CRITICAL_SECTION();
	int	idealSequence = SelectWeightedSequence( ideal );

	if ( idealSequence == -1 )
		return false;

	//Take the new activity
	m_IdealActivity	 = ideal;
	m_nIdealSequence = idealSequence;

	//Find the next sequence in the potential chain of sequences leading to our ideal one
	int nextSequence = FindTransitionSequence( GetSequence(), m_nIdealSequence, NULL );

	// Don't use transitions when we're deploying
	if ( (ideal != ACT_VM_DRAW) && (ideal != ACT_VM_DRAW_EMPTY) && IsWeaponVisible() && (nextSequence != m_nIdealSequence) )
	{
		//Set our activity to the next transitional animation
		SetActivity( ACT_TRANSITION );
		SetSequence( nextSequence );	
		SendViewModelAnim( nextSequence );
	}
	else
	{
		//Set our activity to the ideal
		SetActivity( m_IdealActivity );
		SetSequence( m_nIdealSequence );	
		SendViewModelAnim( m_nIdealSequence );
	}

	//Set the next time the weapon will idle
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
	return true;
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = NULL;
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}


//-----------------------------------------------------------------------------
// Locking a weapon is an exclusive action. If you lock a weapon, that means 
// you are preventing others from doing so for themselves.
//-----------------------------------------------------------------------------
void CBaseCombatWeapon::Lock( float lockTime, CBaseEntity *pLocker )
{
	m_flUnlockTime = gpGlobals->curtime + lockTime;
	m_hLocker.Set( pLocker );
}

//-----------------------------------------------------------------------------
// If I'm still locked for a period of time, tell everyone except the person
// that locked me that I'm not available. 
//-----------------------------------------------------------------------------
bool CBaseCombatWeapon::IsLocked( CBaseEntity *pAsker )
{
	return ( m_flUnlockTime > gpGlobals->curtime && m_hLocker != pAsker );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CBaseCombatWeapon::ActivityOverride( Activity baseAct, bool *pRequired )
{
	acttable_t *pTable = ActivityList();
	int actCount = ActivityListCount();

	for ( int i = 0; i < actCount; i++, pTable++ )
	{
		if ( baseAct == pTable->baseAct )
		{
			if (pRequired)
			{
				*pRequired = pTable->required;
			}
			return (Activity)pTable->weaponAct;
		}
	}
	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDmgAccumulator::CDmgAccumulator( void )
{
#ifdef GAME_DLL
	SetDefLessFunc( m_TargetsDmgInfo );
#endif // GAME_DLL

	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDmgAccumulator::~CDmgAccumulator()
{
	// Did a weapon get deleted while aggregating CTakeDamageInfo events?
	Assert( !m_bActive );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Collect trace attacks for weapons that fire multiple bullets per attack that also penetrate
//-----------------------------------------------------------------------------
void CDmgAccumulator::AccumulateMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity )
{
	if ( !pEntity )
		return;

	Assert( m_bActive );

#if defined( GAME_DLL )
	int iIndex = m_TargetsDmgInfo.Find( pEntity->entindex() );
	if ( iIndex == m_TargetsDmgInfo.InvalidIndex() )
	{
		m_TargetsDmgInfo.Insert( pEntity->entindex(), info );
	}
	else
	{
		CTakeDamageInfo *pInfo = &m_TargetsDmgInfo[iIndex];
		if ( pInfo )
		{
			// Update
			m_TargetsDmgInfo[iIndex].AddDamageType( info.GetDamageType() );
			m_TargetsDmgInfo[iIndex].SetDamage( pInfo->GetDamage() + info.GetDamage() );
			m_TargetsDmgInfo[iIndex].SetDamageForce( pInfo->GetDamageForce() + info.GetDamageForce() );
			m_TargetsDmgInfo[iIndex].SetDamagePosition( info.GetDamagePosition() );
			m_TargetsDmgInfo[iIndex].SetReportedPosition( info.GetReportedPosition() );
			m_TargetsDmgInfo[iIndex].SetMaxDamage( MAX( pInfo->GetMaxDamage(), info.GetDamage() ) );
			m_TargetsDmgInfo[iIndex].SetAmmoType( info.GetAmmoType() );
		}

	}
#endif	// GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Send aggregate info
//-----------------------------------------------------------------------------
void CDmgAccumulator::Process( void )
{
	FOR_EACH_MAP( m_TargetsDmgInfo, i )
	{
		CBaseEntity *pEntity = UTIL_EntityByIndex( m_TargetsDmgInfo.Key( i ) );
		if ( pEntity )
		{
			AddMultiDamage( m_TargetsDmgInfo[i], pEntity );
		}
	}

	m_bActive = false;
	m_TargetsDmgInfo.Purge();
}
#endif // GAME_DLL

#if defined( CLIENT_DLL )

BEGIN_PREDICTION_DATA( CBaseCombatWeapon )

	DEFINE_PRED_FIELD( m_nNextThinkTick, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	// Networked
	DEFINE_PRED_FIELD( m_hOwner, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	// DEFINE_FIELD( m_hWeaponFileInfo, FIELD_SHORT ),
	DEFINE_PRED_FIELD( m_iState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			 
	DEFINE_PRED_FIELD( m_iViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD( m_iWorldModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD_TOL( m_flNextPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( m_flNextSecondaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD_TOL( m_flMeleeCooldown, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD_TOL(m_flNextBashAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
	DEFINE_PRED_FIELD_TOL( m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD_TOL(m_flHolsteredTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),

	DEFINE_PRED_FIELD(m_bWantsHolster, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD(m_bIsBloody, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),

	DEFINE_PRED_FIELD( m_iAmmoType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iClip, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),			

	DEFINE_PRED_FIELD( m_iAmmoCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),	

	DEFINE_PRED_FIELD( m_nViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD_TOL(m_flViewKickTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
	DEFINE_PRED_FIELD_TOL(m_flViewKickPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE),
	DEFINE_PRED_FIELD(m_iShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_iMeleeAttackType, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),

	// Not networked

	DEFINE_FIELD( m_bInReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFiringWholeClip, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextEmptySoundTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( m_fFireDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_iszName, FIELD_INTEGER ),		
	DEFINE_FIELD( m_bFiresUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fMinRange1, FIELD_FLOAT ),		
	DEFINE_FIELD( m_fMinRange2, FIELD_FLOAT ),		
	DEFINE_FIELD( m_fMaxRange1, FIELD_FLOAT ),		
	DEFINE_FIELD( m_fMaxRange2, FIELD_FLOAT ),		
	DEFINE_FIELD( m_bReloadsSingly, FIELD_BOOLEAN ),	
	DEFINE_FIELD( m_iAmmoCount, FIELD_INTEGER ),

	//DEFINE_PHYSPTR( m_pConstraint ),

	// DEFINE_FIELD( m_iOldState, FIELD_INTEGER ),
	// DEFINE_FIELD( m_bJustRestored, FIELD_BOOLEAN ),

	// DEFINE_FIELD( m_OnPlayerPickup, COutputEvent ),
	// DEFINE_FIELD( m_pConstraint, FIELD_INTEGER ),

	END_PREDICTION_DATA()

#endif	// ! CLIENT_DLL

	// Special hack since we're aliasing the name C_BaseCombatWeapon with a macro on the client
	IMPLEMENT_NETWORKCLASS_ALIASED( BaseCombatWeapon, DT_BaseCombatWeapon )

#if !defined( CLIENT_DLL )
	//-----------------------------------------------------------------------------
	// Purpose: Save Data for Base Weapon object
	//-----------------------------------------------------------------------------// 
BEGIN_DATADESC( CBaseCombatWeapon )

	DEFINE_FIELD( m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD(m_flMeleeCooldown, FIELD_TIME),
	DEFINE_FIELD( m_flNextBashAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeWeaponIdle, FIELD_TIME ),

	DEFINE_FIELD( m_bInReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hOwner, FIELD_EHANDLE ),

	DEFINE_FIELD( m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( m_iszName, FIELD_STRING ),
	DEFINE_FIELD(m_iAmmoType, FIELD_INTEGER),
	DEFINE_FIELD(m_iClip, FIELD_INTEGER),
	DEFINE_FIELD( m_bFiresUnderwater, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fMinRange1, FIELD_FLOAT ),
	DEFINE_FIELD( m_fMinRange2, FIELD_FLOAT ),
	DEFINE_FIELD( m_fMaxRange1, FIELD_FLOAT ),
	DEFINE_FIELD( m_fMaxRange2, FIELD_FLOAT ),

	DEFINE_FIELD(m_iAmmoCount, FIELD_INTEGER),

	DEFINE_FIELD( m_nViewModelIndex, FIELD_INTEGER ),

	// don't save these, init to 0 and regenerate
	//	DEFINE_FIELD( m_flNextEmptySoundTime, FIELD_TIME ),
	//	DEFINE_FIELD( m_Activity, FIELD_INTEGER ),
	DEFINE_FIELD( m_nIdealSequence, FIELD_INTEGER ),
	DEFINE_FIELD( m_IdealActivity, FIELD_INTEGER ),

	DEFINE_FIELD( m_fFireDuration, FIELD_FLOAT ),

	DEFINE_FIELD( m_bReloadsSingly, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flUnlockTime,		FIELD_TIME ),
	DEFINE_FIELD( m_hLocker,			FIELD_EHANDLE ),

	//	DEFINE_FIELD( m_iViewModelIndex, FIELD_INTEGER ),
	//	DEFINE_FIELD( m_iWorldModelIndex, FIELD_INTEGER ),
	//  DEFINE_FIELD( m_hWeaponFileInfo, ???? ),

	// Just to quiet classcheck.. this field exists only on the client
	//	DEFINE_FIELD( m_iOldState, FIELD_INTEGER ),
	//	DEFINE_FIELD( m_bJustRestored, FIELD_BOOLEAN ),

	// Function pointers
	DEFINE_ENTITYFUNC( DefaultTouch ),
	DEFINE_THINKFUNC( FallThink ),
	DEFINE_THINKFUNC( Materialize ),
	DEFINE_THINKFUNC( AttemptToMaterialize ),
	DEFINE_THINKFUNC( DestroyItem ),
	DEFINE_THINKFUNC( SetPickupTouch ),

	DEFINE_THINKFUNC( HideThink ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HideWeapon", InputHideWeapon ),

	// Outputs
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse"),
	DEFINE_OUTPUT( m_OnPlayerPickup, "OnPlayerPickup"),
	DEFINE_OUTPUT( m_OnNPCPickup, "OnNPCPickup"),
	DEFINE_OUTPUT( m_OnCacheInteraction, "OnCacheInteraction" ),

	// Hammer Keyfields
	DEFINE_KEYFIELD(m_iDefaultAmmoCount, FIELD_INTEGER, "AmmoCount"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Only send the LocalWeaponData to the player carrying the weapon
//-----------------------------------------------------------------------------
void* SendProxy_SendLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendLocalWeaponDataTable );

//-----------------------------------------------------------------------------
// Purpose: Only send to non-local players
//-----------------------------------------------------------------------------
void* SendProxy_SendNonLocalWeaponDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();

	CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
	if ( pWeapon )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->ClearRecipient( pPlayer->GetClientIndex() );
			return ( void * )pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalWeaponDataTable );

#endif

#if PREDICTION_ERROR_CHECK_LEVEL > 1
#define SendPropTime SendPropFloat
#define RecvPropTime RecvPropFloat
#endif

//-----------------------------------------------------------------------------
// Purpose: Propagation data for weapons. Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( CBaseCombatWeapon, DT_LocalWeaponData )
#if !defined( CLIENT_DLL )
	SendPropIntWithMinusOneFlag(SENDINFO(m_iClip), 10),
	SendPropInt(SENDINFO(m_iAmmoType), 6),
	SendPropInt(SENDINFO(m_nViewModelIndex), VIEWMODEL_INDEX_BITS, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_iShotsFired), 7, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN),
	SendPropTime(SENDINFO(m_flNextPrimaryAttack)),
	SendPropTime(SENDINFO(m_flNextSecondaryAttack)),
	SendPropTime(SENDINFO(m_flMeleeCooldown)),
	SendPropTime(SENDINFO(m_flNextBashAttack)),
	SendPropTime(SENDINFO(m_flTimeWeaponIdle)),
	SendPropTime(SENDINFO(m_flHolsteredTime)),
	SendPropTime(SENDINFO(m_flViewKickTime)),
	SendPropFloat(SENDINFO(m_flViewKickPenalty), -1, SPROP_CHANGES_OFTEN),
	SendPropInt(SENDINFO(m_nNextThinkTick)),
	SendPropBool(SENDINFO(m_bWantsHolster)),
#else
	RecvPropIntWithMinusOneFlag(RECVINFO(m_iClip)),
	RecvPropInt(RECVINFO(m_iAmmoType)),
	RecvPropInt(RECVINFO(m_nViewModelIndex)),
	RecvPropInt(RECVINFO(m_iShotsFired)),
	RecvPropTime(RECVINFO(m_flNextPrimaryAttack)),
	RecvPropTime(RECVINFO(m_flNextSecondaryAttack)),
	RecvPropTime(RECVINFO(m_flMeleeCooldown)),
	RecvPropTime(RECVINFO(m_flNextBashAttack)),
	RecvPropTime(RECVINFO(m_flTimeWeaponIdle)),
	RecvPropTime(RECVINFO(m_flHolsteredTime)),
	RecvPropTime(RECVINFO(m_flViewKickTime)),
	RecvPropFloat(RECVINFO(m_flViewKickPenalty)),
	RecvPropInt(RECVINFO(m_nNextThinkTick)),
	RecvPropBool(RECVINFO(m_bWantsHolster)),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CBaseCombatWeapon, DT_BaseCombatWeapon)
#if !defined( CLIENT_DLL )
	SendPropDataTable("LocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_LocalWeaponData), SendProxy_SendLocalWeaponDataTable),
	SendPropModelIndex(SENDINFO(m_iViewModelIndex)),
	SendPropModelIndex(SENDINFO(m_iWorldModelIndex)),
	SendPropInt(SENDINFO(m_iState), 3, SPROP_UNSIGNED),
	SendPropEHandle(SENDINFO(m_hOwner)),
	SendPropBool(SENDINFO(m_bIsBloody)),
	SendPropInt(SENDINFO(m_iAmmoCount), 13, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_iMeleeAttackType), 3, SPROP_UNSIGNED),
#else
	RecvPropDataTable("LocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalWeaponData)),
	RecvPropInt(RECVINFO(m_iViewModelIndex)),
	RecvPropInt(RECVINFO(m_iWorldModelIndex)),
	RecvPropInt(RECVINFO(m_iState)),
	RecvPropEHandle(RECVINFO(m_hOwner)),
	RecvPropBool(RECVINFO(m_bIsBloody)),
	RecvPropInt(RECVINFO(m_iAmmoCount)),
	RecvPropInt(RECVINFO(m_iMeleeAttackType)),
#endif
END_NETWORK_TABLE()

//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Fragmentation Grenade! 
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "GameBase_Shared.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "grenade_frag.h"
#endif

#include "basegrenade_shared.h"
#include "effect_dispatch_data.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER	2.5f // seconds
#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_RADIUS	4.0f // inches
#define RETHROW_DELAY	0.5

#ifdef CLIENT_DLL
#define CWeaponFrag C_WeaponFrag
#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag: public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponFrag, CBaseHL2MPCombatWeapon );
public:

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponFrag();

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame( void );
	void    Drop(const Vector &vecVelocity);

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );	
	bool	Reload( void );

#ifndef CLIENT_DLL
	void OnThrewGrenade(bool bCheckRemove = false);
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	bool m_bRemoveWeapon;
	EHANDLE m_hLastPuller;
#endif

	void	ThrowGrenade( CBasePlayer *pPlayer, float timer = GRENADE_TIMER );

	bool CanPickupWeaponAsAmmo() { return true; }
	bool CanPerformMeleeAttacks() { return false; }
	
private:

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	CNetworkVar( bool,	m_bRedraw );	//Draw the weapon again after throwing a grenade
	CNetworkVar(float, m_flSoonestThrowTime);
	CNetworkVar(float, m_flLongestHoldTime);
	CNetworkVar( int,	m_AttackPaused );

	CWeaponFrag( const CWeaponFrag & );

	DECLARE_ACTTABLE();
};

acttable_t	CWeaponFrag::m_acttable[] = 
{
#ifdef BB2_AI
{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true }, 

	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },

	{ ACT_MP_WALK, ACT_HL2MP_WALK_GRENADE, false },
	{ ACT_MP_RUN,						ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_GRENADE,			false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_GRENADE, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_GRENADE,					false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE(CWeaponFrag);

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFrag, DT_WeaponFrag )

BEGIN_NETWORK_TABLE( CWeaponFrag, DT_WeaponFrag )

#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bRedraw ) ),
	RecvPropInt( RECVINFO( m_AttackPaused ) ),
	RecvPropFloat(RECVINFO(m_flSoonestThrowTime)),
	RecvPropFloat(RECVINFO(m_flLongestHoldTime)),
#else
	SendPropBool( SENDINFO( m_bRedraw ) ),
	SendPropInt( SENDINFO( m_AttackPaused ) ),
	SendPropFloat(SENDINFO(m_flSoonestThrowTime)),
	SendPropFloat(SENDINFO(m_flLongestHoldTime)),
#endif
	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponFrag )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSoonestThrowTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLongestHoldTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_frag, CWeaponFrag );
PRECACHE_WEAPON_REGISTER(weapon_frag);

CWeaponFrag::CWeaponFrag( void ) : CBaseHL2MPCombatWeapon()
{
	m_bRedraw = false;
	m_flSoonestThrowTime = 0.0f;
	m_flLongestHoldTime = 0.0f;
	m_AttackPaused = 0;

#ifndef CLIENT_DLL
	m_bRemoveWeapon = false;
	m_hLastPuller = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "npc_grenade_frag" );
#endif
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponFrag::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	bool fThrewGrenade = false;

	switch (pEvent->event)
	{
	case EVENT_WEAPON_THROW:
	{
		ThrowGrenade(pPlayer, MAX((m_flLongestHoldTime - gpGlobals->curtime), 0));
		DecrementAmmo(pPlayer);
		fThrewGrenade = true;
		HL2MPRules()->EmitSoundToClient(pPlayer, "ThrowGrenade", pPlayer->GetSoundType(), pPlayer->GetSoundsetGender());
		break;
	}

	case EVENT_WEAPON_SEQUENCE_FINISHED:
	{
		if (GetPrimaryAmmoCount() <= 0)
		{
			m_bRemoveWeapon = true;
			pPlayer->Weapon_DropSlot(GetSlot()); // throw this weapon away!
		}
		break;
	}

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}

	if (fThrewGrenade)
		OnThrewGrenade();
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFrag::Deploy( void )
{
#ifndef CLIENT_DLL
	m_hLastPuller = NULL;
#endif

	m_bRedraw = false;
	m_flSoonestThrowTime = 0.0f;
	m_flLongestHoldTime = 0.0f;
	m_AttackPaused = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifndef CLIENT_DLL
	m_hLastPuller = NULL;
#endif

	m_bRedraw = false;
	m_flSoonestThrowTime = 0.0f;
	m_flLongestHoldTime = 0.0f;
	m_AttackPaused = 0;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );;
	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );

	m_flSoonestThrowTime = gpGlobals->curtime + GetViewModelSequenceDuration();
	m_flLongestHoldTime = m_flSoonestThrowTime + GRENADE_TIMER;
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

#ifndef CLIENT_DLL
	m_hLastPuller = pPlayer;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SecondaryAttack(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponFrag::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

#ifndef CLIENT_DLL
void CWeaponFrag::OnThrewGrenade(bool bCheckRemove)
{
	m_hLastPuller = NULL;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());

	m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
	m_flNextSecondaryAttack = gpGlobals->curtime + RETHROW_DELAY;
	m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!

#ifdef BB2_AI		
	// Make a sound designed to scare snipers back into their holes!
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner)
	{
		Vector vecSrc = pOwner->Weapon_ShootPosition();
		Vector	vecDir;

		AngleVectors(pOwner->EyeAngles(), &vecDir);

		trace_t tr;

		UTIL_TraceLine(vecSrc, vecSrc + vecDir * 1024, MASK_SOLID_BRUSHONLY, pOwner, COLLISION_GROUP_NONE, &tr);

		CSoundEnt::InsertSound(SOUND_DANGER_SNIPERONLY, tr.endpos, 384, 0.2, pOwner);
	}
#endif //BB2_AI

	if (bCheckRemove && GetPrimaryAmmoCount() <= 0)
	{
		m_bRemoveWeapon = true;
		pPlayer->Weapon_DropSlot(GetSlot()); // throw this weapon away!
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemPostFrame( void )
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (m_flSoonestThrowTime < gpGlobals->curtime && m_flSoonestThrowTime > 0.0f)
	{
		if (pOwner)
		{
			if (m_flLongestHoldTime < gpGlobals->curtime)
			{
#ifndef CLIENT_DLL
				m_flSoonestThrowTime = 0.0f;
				m_AttackPaused = 0;
				ThrowGrenade(pOwner, 0.0f);
				DecrementAmmo(pOwner);
				OnThrewGrenade(true);
				GameBaseShared()->GetAchievementManager()->WriteToAchievement(pOwner, "ACH_WEAPON_GRENADE_FAIL");
#endif
			}
			else
			{
				switch (m_AttackPaused)
				{
				case GRENADE_PAUSED_PRIMARY:
					if (!(pOwner->m_nButtons & IN_ATTACK))
					{
						SendWeaponAnim(ACT_VM_THROW);
						pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_THROW);
						m_flSoonestThrowTime = 0.0f;
						m_AttackPaused = 0;
						WeaponSound(SINGLE);
					}
					break;

				default:
					break;
				}
			}
		}
	}

	BaseClass::ItemPostFrame();

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}
}

void CWeaponFrag::Drop(const Vector &vecVelocity)
{
#ifndef CLIENT_DLL
	if (m_bRemoveWeapon)
	{
		UTIL_Remove(this);
		return;
	}

	// If we die and we actually pulled the pin then we want this frag grenade to explode...
	CBasePlayer *pPuller = ToBasePlayer(m_hLastPuller.Get());
	if (pPuller)
	{
		Fraggrenade_Create(pPuller->GetAbsOrigin(), vec3_angle, pPuller->GetAbsVelocity(), AngularImpulse(300, random->RandomInt(-600, 600), 0), pPuller, MAX((m_flLongestHoldTime - gpGlobals->curtime), 0), false);
		SetPrimaryAmmoCount(MAX(GetPrimaryAmmoCount() - 1, 0));
		if (GetPrimaryAmmoCount() <= 0)
		{
			UTIL_Remove(this);
			return;
		}
	}

	m_hLastPuller = NULL;
#endif

	BaseClass::Drop(vecVelocity);
}

// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFrag::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::ThrowGrenade(CBasePlayer *pPlayer, float timer)
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 1200;

	if (timer <= 0)
		vecThrow = vec3_origin;

	CBaseGrenade *pGrenade = Fraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer, timer, false);
	if ( pGrenade )
	{
		if ( pPlayer && pPlayer->m_lifeState != LIFE_ALIVE )
		{
			pPlayer->GetVelocity( &vecThrow, NULL );

			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetVelocity( &vecThrow, NULL );
			}
		}
	}
#endif

	m_bRedraw = true;
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}
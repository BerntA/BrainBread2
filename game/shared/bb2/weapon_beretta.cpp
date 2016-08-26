//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Beretta Handgun
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponBeretta C_WeaponBeretta
#endif

//-----------------------------------------------------------------------------
// CWeaponBeretta
//-----------------------------------------------------------------------------

class CWeaponBeretta : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CWeaponBeretta, CHL2MPMachineGun);

	CWeaponBeretta(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	DryFire( void );

	int GetOverloadCapacity() { return 4; }
	bool UsesEmptyAnimation() { return true; }
	int GetWeaponType(void) { return WEAPON_TYPE_PISTOL; }

	Activity	GetPrimaryAttackActivity( void );

	virtual int	GetMinBurst() 
	{ 
		return 1; 
	}

	virtual int	GetMaxBurst() 
	{ 
		return 3; 
	}

	DECLARE_ACTTABLE();

#ifdef BB2_AI
#ifndef CLIENT_DLL
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif
#endif //BB2_AI

private:
	CWeaponBeretta( const CWeaponBeretta & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponBeretta, DT_WeaponBeretta )

	BEGIN_NETWORK_TABLE( CWeaponBeretta, DT_WeaponBeretta )
	END_NETWORK_TABLE()

#ifdef CLIENT_DLL
	BEGIN_PREDICTION_DATA( CWeaponBeretta )
	END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_beretta, CWeaponBeretta );
PRECACHE_WEAPON_REGISTER( weapon_beretta );

acttable_t CWeaponBeretta::m_acttable[] = 
{
#ifdef BB2_AI
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_PISTOL, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_PISTOL, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_PISTOL, false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_PISTOL,					false },

	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },

	// HL2
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE( CWeaponBeretta );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponBeretta::CWeaponBeretta( void )
{
	m_fMinRange1 = 24;
	m_fMaxRange1 = 1500;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;

	m_bFiresUnderwater	= true;
}

#ifdef BB2_AI
#ifndef CLIENT_DLL
void CWeaponBeretta::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

			WeaponSound( SINGLE_NPC );
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			m_iClip1 = m_iClip1 - 1;
		}
		break;
	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#endif
#endif //BB2_AI

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponBeretta::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponBeretta::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		if (pOwner->m_afButtonPressed & IN_ATTACK)
			BaseClass::PrimaryAttack();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponBeretta::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if (m_bInReload || (m_flNextBashAttack > gpGlobals->curtime))
		return;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack < gpGlobals->curtime) && (m_iClip1 <= 0))
		DryFire();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponBeretta::GetPrimaryAttackActivity( void )
{
	if (m_iClip1 <= 0)
		return ACT_VM_LASTBULLET;

	return ACT_VM_PRIMARYATTACK;
}
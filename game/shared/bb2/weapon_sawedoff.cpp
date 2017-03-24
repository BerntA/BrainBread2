//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Sawed-Off Double Barrel Shotgun
//
//========================================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "baseentity_shared.h"
#include "ammodef.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "c_baseentity.h"
#include "GameBase_Client.h"
#else
#include "baseentity.h"
#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponSawedOff C_WeaponSawedOff
#endif

class CWeaponSawedOff : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponSawedOff, CBaseHL2MPCombatWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifdef BB2_AI
#ifndef CLIENT_DLL
	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary );
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
#endif
#endif //BB2_AI

private:

	CNetworkVar(bool, m_bInReload);
	CNetworkVar(bool, m_bFiredPrimary);
	CNetworkVar(bool, m_bFiredSecondary);

public:

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	int GetOverloadCapacity() { return 1; }
	int GetWeaponType(void) { return WEAPON_TYPE_SHOTGUN; }
	const char *GetAmmoEntityLink(void) { return "ammo_slugs"; }

	bool Reload( void );
	void FillClip( int iAmount );
	void StartHolsterSequence();
	bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void Drop(const Vector &vecVelocity);
	void ItemPostFrame( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void DryFire( void );
	float GetFireRate( void ) { return GetWpnData().m_flFireRate; }
	void AffectedByPlayerSkill(int skill);

	const char *GetMuzzleflashAttachment(bool bPrimaryAttack)
	{
		if (bPrimaryAttack)
			return "left_muzzle";

		return "right_muzzle";
	}

	DECLARE_ACTTABLE();

	CWeaponSawedOff(void);

private:
	CWeaponSawedOff( const CWeaponSawedOff & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSawedOff, DT_WeaponSawedOff )

	BEGIN_NETWORK_TABLE( CWeaponSawedOff, DT_WeaponSawedOff )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInReload ) ),
	RecvPropBool( RECVINFO( m_bFiredPrimary ) ),
	RecvPropBool(RECVINFO(m_bFiredSecondary)),
#else
	SendPropBool(SENDINFO(m_bInReload)),
	SendPropBool(SENDINFO(m_bFiredPrimary)),
	SendPropBool(SENDINFO(m_bFiredSecondary)),
#endif
	END_NETWORK_TABLE()

#ifdef CLIENT_DLL
	BEGIN_PREDICTION_DATA( CWeaponSawedOff )
	DEFINE_PRED_FIELD( m_bInReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bFiredPrimary, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bFiredSecondary, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_sawedoff, CWeaponSawedOff );
PRECACHE_WEAPON_REGISTER(weapon_sawedoff);

acttable_t	CWeaponSawedOff::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_SHOTGUN,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },

	{ ACT_MP_INFECTED, ACT_HL2MP_GESTURE_INFECTED, false },
	{ ACT_MP_KICK, ACT_HL2MP_GESTURE_KICK, false },
	{ ACT_MP_BASH, ACT_HL2MP_GESTURE_BASH_SHOTGUN, false },
	{ ACT_MP_SLIDE, ACT_HL2MP_SLIDE_SHOTGUN, false },
	{ ACT_MP_SLIDE_IDLE, ACT_HL2MP_SLIDE_IDLE_SHOTGUN, false },
	{ ACT_MP_WALK, ACT_HL2MP_WALK_SHOTGUN, false },

	{ ACT_MP_RUN,						ACT_HL2MP_RUN_SHOTGUN,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },

	{ ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD_SAWEDOFF, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD_SAWEDOFF, false },

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_SHOTGUN,					false },
#ifdef BB2_AI
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SAWEDOFF, false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },

	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },

	// HL2
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD,			false },
#endif //BB2_AI
};

IMPLEMENT_ACTTABLE(CWeaponSawedOff);

#ifndef CLIENT_DLL
#ifdef BB2_AI

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOperator - 
//-----------------------------------------------------------------------------
void CWeaponSawedOff::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );
	WeaponSound( SINGLE_NPC );
	m_iClip1 = m_iClip1 - 1;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	pOperator->FireBullets( GetWpnData().m_iPellets, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSawedOff::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	FireNPCPrimaryAttack( pOperator, true );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponSawedOff::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_SHOTGUN_FIRE:
		{
			FireNPCPrimaryAttack( pOperator, false );
		}
		break;

	default:
		CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif //BB2_AI
#endif

void CWeaponSawedOff::AffectedByPlayerSkill(int skill)
{
	switch (skill)
	{
	case PLAYER_SKILL_HUMAN_GUNSLINGER:
	case PLAYER_SKILL_HUMAN_MAGAZINE_REFILL:
	{
		m_bFiredPrimary = m_bFiredSecondary = false;
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponSawedOff::Reload( void )
{
	CHL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return false;

	int iAmmo = pOwner->GetAmmoCount(m_iPrimaryAmmoType);
	if (iAmmo <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, iAmmo);
	if (j <= 0)
		return false;

	m_bInReload = true;
	m_bFiredPrimary = m_bFiredSecondary = false;

	int iReloadActivity = ACT_VM_RELOAD0;
	if (m_iClip1 <= 0 && iAmmo > 1)
		iReloadActivity = ACT_VM_RELOAD_EMPTY0;

	iReloadActivity += pOwner->GetSkillValue(PLAYER_SKILL_HUMAN_SHOTGUN_MASTER);
	if (HL2MPRules() && HL2MPRules()->IsFastPacedGameplay())
	{
		if (m_iClip1 <= 0 && iAmmo > 1)
			iReloadActivity = ACT_VM_RELOAD_EMPTY10;
		else
			iReloadActivity = ACT_VM_RELOAD10;
	}

	SendWeaponAnim(iReloadActivity);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, iReloadActivity);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponSawedOff::FillClip( int iAmount )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner == NULL )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1 += iAmount;
			pOwner->RemoveAmmo( iAmount, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponSawedOff::DryFire( void )
{
	WeaponSound(EMPTY);
	SendWeaponAnim( ACT_VM_DRYFIRE );
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponSawedOff::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	//m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration() + 0.1f;
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_PRIMARYATTACK);

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );	

	FireBulletsInfo_t info(GetWpnData().m_iPellets, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;
	info.m_vecFirstStartPos = pPlayer->GetAbsOrigin();
	info.m_flDropOffDist = GetWpnData().m_flDropOffDistance;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );

#ifdef BB2_AI
#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 1.0 );
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2 );
#endif
#endif //BB2_AI

	pPlayer->ViewPunch( GetViewKickAngle() );
}

void CWeaponSawedOff::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if (!pPlayer)
		return;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Don't fire again until fire animation has completed
	//m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration() + 0.1f;
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY, ACT_VM_PRIMARYATTACK);

	Vector	vecSrc = pPlayer->Weapon_ShootPosition();
	Vector	vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	FireBulletsInfo_t info(GetWpnData().m_iPellets, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;
	info.m_bPrimaryAttack = false;
	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);

#ifdef BB2_AI
#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 1.0);
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2);
#endif
#endif //BB2_AI

	pPlayer->ViewPunch(GetViewKickAngle());
}

//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponSawedOff::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
		return;

	// Implement base properties:
	if (!m_bInReload)
		BaseClass::GenericBB2Animations();

	if (m_bInReload)
	{
		if (IsViewModelSequenceFinished() && (m_flNextSecondaryAttack <= gpGlobals->curtime) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			int iAmmo = pOwner->GetAmmoCount(m_iPrimaryAmmoType);
			if (m_iClip1 <= 0 && iAmmo > 1)
				FillClip(2);
			else
				FillClip(1);

			m_bInReload = false;
		}
	}

	if (!m_bInReload && IsViewModelSequenceFinished() && (m_flNextBashAttack <= gpGlobals->curtime) && !(pOwner->m_nButtons & IN_BASH) && !(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		WeaponIdle();

	if ((pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		if ((m_iClip1 <= 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType)) || m_bFiredPrimary)
				DryFire();
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else 
		{
			// If the firing button was just pressed, reset the firing time
			if (pOwner->m_afButtonPressed & IN_ATTACK)
				m_flNextPrimaryAttack = gpGlobals->curtime;

			m_bFiredPrimary = true;
			PrimaryAttack();
		}
	}

	if ( (pOwner->m_nButtons & IN_ATTACK2) && m_flNextSecondaryAttack <= gpGlobals->curtime)
	{
		if ((m_iClip1 <= 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType)) || m_bFiredSecondary)
				DryFire();
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if (pOwner->m_afButtonPressed & IN_ATTACK2)
				m_flNextSecondaryAttack = gpGlobals->curtime;

			m_bFiredSecondary = true;
			SecondaryAttack();
		}
	}

	if ( (pOwner->m_nButtons & IN_RELOAD) && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		if ( (m_iClip1 < GetMaxClip1()) && (pOwner->GetAmmoCount(m_iPrimaryAmmoType) >= 1) )
			Reload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSawedOff::CWeaponSawedOff( void )
{
	m_bReloadsSingly = true;
	m_bFiredPrimary = false;
	m_bFiredSecondary = false;
}

void CWeaponSawedOff::StartHolsterSequence()
{
	m_bInReload = false;
	m_bFiredPrimary = false;
	m_bFiredSecondary = false;

	BaseClass::StartHolsterSequence();
}

bool CWeaponSawedOff::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bInReload = false;
	m_bFiredPrimary = false;
	m_bFiredSecondary = false;

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponSawedOff::Drop(const Vector &vecVelocity)
{
	m_bInReload = false;
	m_bFiredPrimary = false;
	m_bFiredSecondary = false;

	BaseClass::Drop(vecVelocity);
}
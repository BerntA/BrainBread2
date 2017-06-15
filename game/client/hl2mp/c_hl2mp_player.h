//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Client Player Class
//
//========================================================================================//

#ifndef HL2MP_PLAYER_H
#define HL2MP_PLAYER_H
#pragma once

#include "hl2mp_playeranimstate.h"
#include "c_basehlplayer.h"
#include "hl2mp_player_shared.h"
#include "beamdraw.h"
#include "skills_shareddefs.h"
#include "c_bb2_playerlocaldata.h"
#include "c_client_attachment.h"

class C_ClientAttachment;
//=============================================================================
// >> HL2MP_Player
//=============================================================================
class C_HL2MP_Player : public C_BaseHLPlayer
{
public:
	DECLARE_CLASS( C_HL2MP_Player, C_BaseHLPlayer );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_HL2MP_Player();
	~C_HL2MP_Player( void );

	void ClientThink( void );

	static C_HL2MP_Player* GetLocalHL2MPPlayer();

	virtual int DrawModel( int flags );
	virtual void AddEntity( void );
	virtual bool CanDrawGlowEffects(void);

	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	// Should this object cast shadows?
	virtual ShadowType_t		ShadowCastType( void );
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual const QAngle& GetRenderAngles();
	virtual bool ShouldDraw( void );
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual float GetFOV( void );
	virtual CStudioHdr *OnNewModel( void );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );
	virtual float GetMinFOV()	const { return 5.0f; }
	virtual Vector GetAutoaimVector( float flDelta );
	virtual void NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void CreateLightEffects( void ) {}
	virtual bool ShouldReceiveProjectedTextures( int flags );
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void PreThink( void );
	virtual void DoImpactEffect( trace_t &tr, int nDamageType );
	IRagdoll* GetRepresentativeRagdoll() const;
	virtual void CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual const QAngle& EyeAngles( void );

	virtual void FireEvent(const Vector& origin, const QAngle& angles, int event, const char *options);

	// Movement:
	virtual float GetPlayerSpeed();
	virtual float GetLeapLength();
	virtual float GetJumpHeight();

	virtual const Vector GetPlayerMins(void) const; 
	virtual const Vector GetPlayerMaxs(void) const;

	void	HandleSpeedChanges( void );
	void	Initialize( void );
	void    OnDormantStateChange(void);
	void    OnUpdateInfected(void);
	void    SharedPostThinkHL2MP(void);
	bool    IsSliding(void) const;

	HL2MPPlayerState State_Get() const;

	// Walking
	void StartWalking( void );
	void StopWalking( void );
	bool IsWalking( void ) { return m_fIsWalking; }
	bool HasArmor(void) { return (m_BB2Local.m_iActiveArmorType > 0); }

	// Skill Accessors
	int GetSkillValue(int index);
	float GetSkillValue(int skillType, int team, bool bDataValueOnly = false, int dataSubType = -1);
	float GetSkillValue(const char *pszType, int skillType, int team = 2, int dataSubType = -1);
	float GetSkillCombination(int skillDefault, int skillExtra);

	virtual void UpdateClientSideAnimation();
	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);
	virtual void CalculateIKLocks(float currentTime);

	float GetRespawnTime(void) { return m_BB2Local.m_flPlayerRespawnTime; }

	C_BB2PlayerLocalData m_BB2Local;

	int GetPerkFlags(void) { return m_nPerkFlags; }
	bool IsPerkFlagActive(int nFlag) { return (m_nPerkFlags & nFlag) != 0; }
	
	void SetPlayerSlideState(bool value) { m_bIsInSlide = value; }

	void SetZombieVision(bool state);
	bool IsZombieVisionOn(void) { return m_bIsZombieVisionEnabled; }

protected:
	virtual void DoPlayerKick(void);

private:
	
	C_HL2MP_Player( const C_HL2MP_Player & );

	CHL2MPPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;

	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	float m_flLastInfectionTwitchTime;

	int	  m_iSpawnInterpCounter;
	int	  m_iSpawnInterpCounterCache;

	int m_nPerkFlags;
	bool m_bIsInSlide;

	void ReleaseFlashlight( void );
	Beam_t	*m_pFlashlightBeam;

	CNetworkVar( HL2MPPlayerState, m_iPlayerState );	

	bool m_fIsWalking;
	bool m_bIsZombieVisionEnabled;

	C_ClientAttachment *m_pAttachments[3];
};

inline C_HL2MP_Player *ToHL2MPPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<C_HL2MP_Player*>( pEntity );
}

extern EHANDLE m_pPlayerRagdoll;

#endif //HL2MP_PLAYER_H
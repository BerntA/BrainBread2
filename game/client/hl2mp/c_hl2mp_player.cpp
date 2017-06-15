//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Client Player Class
//
//========================================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "c_hl2mp_player.h"
#include "view.h"
#include "takedamageinfo.h"
#include "hl2mp_gamerules.h"
#include "in_buttons.h"
#include "iviewrender_beams.h"			// flashlight beam
#include "r_efx.h"
#include "dlight.h"
#include "c_basetempentity.h"
#include "prediction.h"
#include "bone_setup.h"
#include "engine/IEngineSound.h"
#include "GameBase_Client.h"
#include "filesystem.h"
#include "fmod_manager.h"
#include "GameBase_Shared.h"
#include "c_objective_icon.h"
#include "c_client_gib.h"
#include "model_types.h"
#include "GlobalRenderEffects.h"
#include "c_bb2_player_shared.h"
#include "eventlist.h"

// Don't alias here
#if defined( CHL2MP_Player )
#undef CHL2MP_Player	
#endif

LINK_ENTITY_TO_CLASS( player, C_HL2MP_Player );

BEGIN_RECV_TABLE_NOBASE(C_HL2MP_Player, DT_HL2MPLocalPlayerExclusive)
RecvPropVector(RECVINFO_NAME(m_vecNetworkOrigin, m_vecOrigin)),
RecvPropFloat(RECVINFO(m_angEyeAngles[0])),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE(C_HL2MP_Player, DT_HL2MPNonLocalPlayerExclusive)
RecvPropVector(RECVINFO_NAME(m_vecNetworkOrigin, m_vecOrigin)),
RecvPropFloat(RECVINFO(m_angEyeAngles[0])),
RecvPropFloat(RECVINFO(m_angEyeAngles[1])),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_HL2MP_Player, DT_HL2MP_Player, CHL2MP_Player)
    RecvPropDataTable(RECVINFO_DT(m_BB2Local), 0, &REFERENCE_RECV_TABLE(DT_BB2Local)),
    RecvPropDataTable("hl2mplocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HL2MPLocalPlayerExclusive)),
    RecvPropDataTable("hl2mpnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_HL2MPNonLocalPlayerExclusive)),

	RecvPropInt( RECVINFO( m_iSpawnInterpCounter ) ),
	RecvPropBool( RECVINFO( m_fIsWalking ) ),
	RecvPropInt(RECVINFO(m_nPerkFlags)), 
	RecvPropBool(RECVINFO(m_bIsInSlide)),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_HL2MP_Player )
DEFINE_PRED_TYPEDESCRIPTION(m_BB2Local, C_BB2PlayerLocalData),
DEFINE_PRED_FIELD(m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
DEFINE_PRED_FIELD(m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
DEFINE_PRED_FIELD(m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
DEFINE_PRED_ARRAY_TOL(m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f),
DEFINE_PRED_FIELD(m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
DEFINE_PRED_FIELD(m_bIsInSlide, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()

// BB2 WARN Firstperson death
static ConVar cl_fp_ragdoll ( "cl_fp_ragdoll", "1", FCVAR_ARCHIVE, "Allow first person ragdolls" );
static ConVar cl_fp_ragdoll_auto ( "cl_fp_ragdoll_auto", "1", FCVAR_ARCHIVE, "Autoswitch to ragdoll thirdperson-view when necessary" );

ConVar bb2_zombie_vision_radius("bb2_zombie_vision_radius", "400", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Sets the radius of the zombie vision lighting.", true, 50.0f, true, 500.0f);
ConVar bb2_zombie_vision_color_r("bb2_zombie_vision_color_r", "80", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Sets the color for the zombie vision lighting.", true, 0.0f, true, 255.0f);
ConVar bb2_zombie_vision_color_g("bb2_zombie_vision_color_g", "25", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Sets the color for the zombie vision lighting.", true, 0.0f, true, 255.0f);
ConVar bb2_zombie_vision_color_b("bb2_zombie_vision_color_b", "15", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Sets the color for the zombie vision lighting.", true, 0.0f, true, 255.0f);

void SpawnBlood (Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage, int hitbox);

EHANDLE m_pPlayerRagdoll = NULL;
dlight_t *m_pZombieLighting = 0;

C_HL2MP_Player::C_HL2MP_Player() : m_iv_angEyeAngles("C_HL2MP_Player::m_iv_angEyeAngles")
{
	m_bIsZombieVisionEnabled = false;
	m_iSpawnInterpCounterCache = 0;

	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	//m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;
	m_PlayerAnimState = CreateHL2MPPlayerAnimState(this);

	m_pFlashlightBeam = NULL;

	for (int i = 0; i < _ARRAYSIZE(m_pAttachments); i++)
		m_pAttachments[i] = NULL;
}

C_HL2MP_Player::~C_HL2MP_Player( void )
{
	// Make sure that the disconnected player was us and not someone else. Then remove our inv. items!
	if (IsLocalPlayer())
	{		
		BB2PlayerGlobals->Shutdown();
		m_pPlayerRagdoll = NULL;

		if (m_pZombieLighting != 0)
		{
			m_pZombieLighting->die = gpGlobals->curtime;
			m_pZombieLighting = 0;
		}
	}

	if (HasAnyClientAttachments())
	{
		for (int i = (_ARRAYSIZE(m_pAttachments) - 1); i >= 0; i--)
		{
			if (m_pAttachments[i] != NULL)
				m_pAttachments[i]->ReleaseSafely();
		}
	}

	ReleaseFlashlight();
	m_PlayerAnimState->Release();
}

void C_HL2MP_Player::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	Vector vecOrigin = ptr->endpos - vecDir * 4;

	float flDistance = 0.0f;
	
	if ( info.GetAttacker() )
	{
		flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();
	}

	if ( m_takedamage )
	{
		AddMultiDamage( info, this );

		int blood = BloodColor();
		
		CBaseEntity *pAttacker = info.GetAttacker();

		if ( pAttacker )
		{
			if ( HL2MPRules()->IsTeamplay() && pAttacker->InSameTeam( this ) == true )
				return;
			else
			{
				// Zombie Players can't kill each other, not even in deathmatch.
				if (pAttacker->GetTeamNumber() == TEAM_DECEASED && this->GetTeamNumber() == TEAM_DECEASED)
					return;
			}
		}

		if ( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, vecDir, blood, flDistance, ptr->hitgroup );// a little surface blood.
			TraceBleed( flDistance, vecDir, ptr, info.GetDamageType() );
		}
	}
}


C_HL2MP_Player* C_HL2MP_Player::GetLocalHL2MPPlayer()
{
	return (C_HL2MP_Player*)C_BasePlayer::GetLocalPlayer();
}

void C_HL2MP_Player::Initialize( void )
{
	CStudioHdr *hdr = GetModelPtr();
	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}

CStudioHdr *C_HL2MP_Player::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	
	Initialize( );

	// Reset the players animation states, gestures
	if (m_PlayerAnimState)
	{
		m_PlayerAnimState->OnNewModel();
	}

	BB2PlayerGlobals->OnNewModel();
	return hdr;
}

void C_HL2MP_Player::ClientThink( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_HL2MP_Player::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	bool bShouldDrawOverrides = (!(flags & STUDIO_SKIP_MATERIAL_OVERRIDES));

	if (IsPerkFlagActive(PERK_POWERUP_PREDATOR) && bShouldDrawOverrides)
	{
		modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetCloakOverlay());
		int retVal = BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
		modelrender->ForcedMaterialOverride(0);
		return retVal;
	}

	int retVal = BaseClass::DrawModel(flags);

	if (bShouldDrawOverrides)
	{
		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_SPAWNPROTECTION))
		{
			modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetSpawnProtectionOverlay());
			BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
		}
		else
		{
			if (IsMaterialOverlayFlagActive(MAT_OVERLAY_BLOOD))
			{
				modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetBloodOverlay());
				BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
				modelrender->ForcedMaterialOverride(0);
			}

			if (GetPerkFlags())
			{
				modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetPerkOverlay());
				BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
				modelrender->ForcedMaterialOverride(0);
			}
		}
	}

	return retVal;
}

//-----------------------------------------------------------------------------
// Should this object receive shadows?
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::ShouldReceiveProjectedTextures( int flags )
{
	Assert( flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK );

	if ( IsEffectActive( EF_NODRAW ) || IsPerkFlagActive(PERK_POWERUP_PREDATOR) )
		 return false;

	if( flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		return true;
	}

	return BaseClass::ShouldReceiveProjectedTextures( flags );
}

void C_HL2MP_Player::DoImpactEffect( trace_t &tr, int nDamageType )
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
		return;
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

void C_HL2MP_Player::PreThink( void )
{
	BaseClass::PreThink();
	HandleSpeedChanges();
}

const QAngle &C_HL2MP_Player::EyeAngles()
{
	if( IsLocalPlayer() )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HL2MP_Player::AddEntity( void )
{
	BaseClass::AddEntity();

	// Zero out model pitch, blending takes care of all of it.
	SetLocalAnglesDim( X_INDEX, 0 );

	if( this != C_BasePlayer::GetLocalPlayer() )
	{
		if ( IsEffectActive( EF_DIMLIGHT ) )
		{
			bool bGetWeaponAttachment = false;
			int iAttachment = LookupAttachment( "anim_attachment_RH" );

			if ( GetActiveWeapon() )
			{
				bGetWeaponAttachment = true;
				iAttachment = GetActiveWeapon()->LookupAttachment( "flashlight" );
			}

			if ( iAttachment < 0 )
				return;

			Vector vecOrigin;
			QAngle eyeAngles = m_angEyeAngles;

			if ( !bGetWeaponAttachment )
				GetAttachment( iAttachment, vecOrigin, eyeAngles );
			else
				GetActiveWeapon()->GetAttachment( iAttachment, vecOrigin, eyeAngles );

			Vector vForward;
			AngleVectors( eyeAngles, &vForward );
				
			trace_t tr;
			UTIL_TraceLine( vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if( !m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_nType = TE_BEAMPOINTS;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_pszModelName = "sprites/glow01.vmt";
				beamInfo.m_pszHaloName = "sprites/glow01.vmt";
				beamInfo.m_flHaloScale = 3.0;
				beamInfo.m_flWidth = 8.0f;
				beamInfo.m_flEndWidth = 35.0f;
				beamInfo.m_flFadeLength = 300.0f;
				beamInfo.m_flAmplitude = 0;
				beamInfo.m_flBrightness = 60.0;
				beamInfo.m_flSpeed = 0.0f;
				beamInfo.m_nStartFrame = 0.0;
				beamInfo.m_flFrameRate = 0.0;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;
				beamInfo.m_nSegments = 8;
				beamInfo.m_bRenderable = true;
				beamInfo.m_flLife = 0.5;
				beamInfo.m_nFlags = FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
				
				m_pFlashlightBeam = beams->CreateBeamPoints( beamInfo );
			}

			if( m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;

				beams->UpdateBeamInfo( m_pFlashlightBeam, beamInfo );

				dlight_t *el = effects->CL_AllocDlight( 0 );
				el->origin = tr.endpos;
				el->radius = 50; 
				el->color.r = 200;
				el->color.g = 200;
				el->color.b = 200;
				el->die = gpGlobals->curtime + 0.1;
			}
		}
		else if ( m_pFlashlightBeam )
		{
			ReleaseFlashlight();
		}
	}
	else
	{
		if (m_bIsZombieVisionEnabled && (GetTeamNumber() == TEAM_DECEASED) && IsAlive() && (m_pZombieLighting != 0))
		{
			m_pZombieLighting->origin = (GetAbsOrigin() + Vector(0, 0, 36));
			m_pZombieLighting->die = gpGlobals->curtime + 1e6;
		}
	}
}

bool C_HL2MP_Player::CanDrawGlowEffects(void)
{
	if (BB2PlayerGlobals->IsSniperScopeActive())
		return false;

	return true;
}

void C_HL2MP_Player::SetZombieVision(bool state)
{ 
	m_bIsZombieVisionEnabled = state; 
	if (m_bIsZombieVisionEnabled)
	{
		if (m_pZombieLighting == 0)
		{
			m_pZombieLighting = effects->CL_AllocDlight(index);
			m_pZombieLighting->die = gpGlobals->curtime + 1e6;
			m_pZombieLighting->radius = bb2_zombie_vision_radius.GetFloat();
			m_pZombieLighting->color.r = (byte)bb2_zombie_vision_color_r.GetInt();
			m_pZombieLighting->color.g = (byte)bb2_zombie_vision_color_g.GetInt();
			m_pZombieLighting->color.b = (byte)bb2_zombie_vision_color_b.GetInt();
		}

		return;
	}
	
	if (m_pZombieLighting != 0)
	{
		m_pZombieLighting->die = gpGlobals->curtime;
		m_pZombieLighting = 0;
	}
}

ShadowType_t C_HL2MP_Player::ShadowCastType( void ) 
{
	if (!IsVisible() || IsPerkFlagActive(PERK_POWERUP_PREDATOR))
		 return SHADOWS_NONE;

	if (IsLocalPlayer() && !ShouldDrawThisPlayer())
		return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}


const QAngle& C_HL2MP_Player::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

bool C_HL2MP_Player::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( !IsAlive() )
		return false;

//	if( GetTeamNumber() == TEAM_SPECTATOR )
//		return false;

	if( IsLocalPlayer() && IsRagdoll() )
		return true;
	
	if ( IsRagdoll() )
		return false;

	return BaseClass::ShouldDraw();
}

void C_HL2MP_Player::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	if ( state == SHOULDTRANSMIT_END )
	{
		if( m_pFlashlightBeam != NULL )
		{
			ReleaseFlashlight();
		}
	}

	BaseClass::NotifyShouldTransmit( state );
}

void C_HL2MP_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateVisibility();
}

void C_HL2MP_Player::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( m_iSpawnInterpCounter != m_iSpawnInterpCounterCache )
	{
		MoveToLastReceivedPosition( true );
		ResetLatched();
		m_iSpawnInterpCounterCache = m_iSpawnInterpCounter;
	}

	for (int i = 0; i < _ARRAYSIZE(m_pAttachments); i++)
	{
		if (m_pAttachments[i] == NULL)
			m_pAttachments[i] = CreateClientAttachment(this, CLIENT_ATTACHMENT_WEAPON, i, false);
	}

	if (IsLocalPlayer())
		BB2PlayerGlobals->CreateEntities();

	BaseClass::PostDataUpdate( updateType );
}

void C_HL2MP_Player::ReleaseFlashlight( void )
{
	if( m_pFlashlightBeam )
	{
		m_pFlashlightBeam->flags = 0;
		m_pFlashlightBeam->die = gpGlobals->curtime - 1;

		m_pFlashlightBeam = NULL;
	}
}

float C_HL2MP_Player::GetFOV( void )
{
	//Find our FOV with offset zoom value
	float flFOVOffset = C_BasePlayer::GetFOV() + GetZoom();

	// Clamp FOV in MP
	int min_fov = GetMinFOV();
	
	// Don't let it go too low
	flFOVOffset = MAX( min_fov, flFOVOffset );

	return flFOVOffset;
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector C_HL2MP_Player::GetAutoaimVector( float flDelta )
{
	// Never autoaim a predicted weapon (for now)
	Vector	forward;
	AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &forward );
	return	forward;
}

void C_HL2MP_Player::HandleSpeedChanges( void )
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	if (buttonsChanged & IN_WALK)
	{
		// The state of the WALK button has changed. 
		if (IsWalking() && !(m_afButtonPressed & IN_WALK))
		{
			StopWalking();
		}
		else if (!IsWalking() && (m_afButtonPressed & IN_WALK) && !(m_nButtons & IN_DUCK))
		{
			StartWalking();
		}
	}

	if ( m_fIsWalking && !(m_nButtons & IN_WALK)  ) 
		StopWalking();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_HL2MP_Player::StartWalking( void )
{
	SetMaxSpeed((GetPlayerSpeed() / 2.0f));
	m_fIsWalking = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_HL2MP_Player::StopWalking( void )
{
	SetMaxSpeed(GetPlayerSpeed());
	m_fIsWalking = false;
}

void C_HL2MP_Player::ItemPreFrame( void )
{
	if ( GetFlags() & FL_FROZEN )
		 return;

	BaseClass::ItemPreFrame();
}

void C_HL2MP_Player::ItemPostFrame( void )
{
	if ( GetFlags() & FL_FROZEN )
		return;

	BaseClass::ItemPostFrame();
}

C_BaseAnimating *C_HL2MP_Player::BecomeRagdollOnClient()
{
	return NULL;
}

void C_HL2MP_Player::FireEvent(const Vector& origin, const QAngle& angles, int event, const char *options)
{
	BaseClass::FireEvent(origin, angles, event, options);
}

void C_HL2MP_Player::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	if ( m_lifeState != LIFE_ALIVE && !IsObserver() )
	{
		// BB2 FP Death
		if ( cl_fp_ragdoll.GetBool() && m_pPlayerRagdoll.Get() )
		{
			// pointer to the ragdoll
			C_ClientRagdollGib *pRagdoll = (C_ClientRagdollGib*)m_pPlayerRagdoll.Get();

			// gets its origin and angles
			pRagdoll->GetAttachment( pRagdoll->LookupAttachment( "eyes" ), eyeOrigin, eyeAngles );
			Vector vForward; 
			AngleVectors( eyeAngles, &vForward );

			if ( cl_fp_ragdoll_auto.GetBool() )
			{
				// DM: Don't use first person view when we are very close to something
				trace_t tr;
				UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vForward * 5 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr );

				if ( (!(tr.fraction < 1) || (tr.endpos.DistTo(eyeOrigin) > 25)) )
					return;
			}
			else
				return;
		}

		eyeOrigin = vec3_origin;
		eyeAngles = vec3_angle;

		Vector origin = EyePosition();			

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
		{
			origin = pRagdoll->GetRagdollOrigin();
			origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z; // look over ragdoll, not through
		}

		BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

		eyeOrigin = origin;
		
		Vector vForward; 
		AngleVectors( eyeAngles, &vForward );

		VectorNormalize( vForward );
		VectorMA( origin, -CHASE_CAM_DISTANCE_MAX, vForward, eyeOrigin );

		Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
		Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

		trace_t trace; // clip against world
		C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
		UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
		C_BaseEntity::PopEnableAbsRecomputations();

		if (trace.fraction < 1.0)
		{
			eyeOrigin = trace.endpos;
		}
		
		return;
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

IRagdoll* C_HL2MP_Player::GetRepresentativeRagdoll() const
{
	if ( m_pPlayerRagdoll.Get() )
	{
		C_ClientRagdollGib *pRagdoll = (C_ClientRagdollGib*)m_pPlayerRagdoll.Get();
		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

int C_HL2MP_Player::GetSkillValue(int index)
{
	if ((index < 0) || (index >= PLAYER_SKILL_END))
		return 0;

	return m_BB2Local.m_iPlayerSkills[index];
}

float C_HL2MP_Player::GetSkillValue(int skillType, int team, bool bDataValueOnly, int dataSubType)
{
	if (bDataValueOnly)
		return GameBaseShared()->GetSharedGameDetails()->GetPlayerSkillValue(skillType, team, dataSubType);

	return (GameBaseShared()->GetSharedGameDetails()->GetPlayerSkillValue(skillType, team, dataSubType) * (float)GetSkillValue(skillType));
}

float C_HL2MP_Player::GetSkillValue(const char *pszType, int skillType, int team, int dataSubType)
{
	float flDefaultValue = GameBaseShared()->GetSharedGameDetails()->GetPlayerSharedValue(pszType, team);
	if (flDefaultValue <= 0.0f)
		return (GetSkillValue(skillType, team, false, dataSubType));

	float flReturn = (flDefaultValue + ((flDefaultValue / 100) * ((float)GetSkillValue(skillType) * GameBaseShared()->GetSharedGameDetails()->GetPlayerSkillValue(skillType, team, dataSubType))));
	return flReturn;
}

float C_HL2MP_Player::GetSkillCombination(int skillDefault, int skillExtra)
{
	float flDefault = (float)skillDefault;
	return (flDefault + ((flDefault / 100) * skillExtra));
}

void C_HL2MP_Player::UpdateClientSideAnimation()
{
	m_PlayerAnimState->Update(EyeAngles()[YAW], EyeAngles()[PITCH]);
	BB2PlayerGlobals->BodyUpdate(this);

	BaseClass::UpdateClientSideAnimation();
}

void C_HL2MP_Player::OnDormantStateChange(void)
{
	// We're either in our out of this local client's PVS, update check @attachments!
	for (int i = 0; i < _ARRAYSIZE(m_pAttachments); i++)
	{
		if (m_pAttachments[i] != NULL)
			m_pAttachments[i]->PerformUpdateCheck();
	}
}

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEPlayerAnimEvent, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate(DataUpdateType_t updateType)
	{
		// Create the effect.
		C_HL2MP_Player *pPlayer = dynamic_cast< C_HL2MP_Player* >(m_hPlayer.Get());
		if (pPlayer && !pPlayer->IsDormant())
		{
			pPlayer->DoAnimationEvent((PlayerAnimEvent_t)m_iEvent.Get(), m_nData);
		}
	}

public:
	CNetworkHandle(CBasePlayer, m_hPlayer);
	CNetworkVar(int, m_iEvent);
	CNetworkVar(int, m_nData);
};

IMPLEMENT_CLIENTCLASS_EVENT(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent);

BEGIN_RECV_TABLE_NOBASE(C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent)
RecvPropEHandle(RECVINFO(m_hPlayer)),
RecvPropInt(RECVINFO(m_iEvent)),
RecvPropInt(RECVINFO(m_nData))
END_RECV_TABLE()

void C_HL2MP_Player::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	if (IsLocalPlayer())
	{
		if ((prediction->InPrediction() && !prediction->IsFirstTimePredicted()))
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent(event, nData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HL2MP_Player::CalculateIKLocks(float currentTime)
{
	if (!m_pIk)
		return;

	int targetCount = m_pIk->m_target.Count();
	if (targetCount == 0)
		return;

	// In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
	// get in here during the view setup code, and it's not normally supposed to be able to access the spatial
	// partition that early in the rendering loop. So we allow access right here for that special case.
	SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
	partition->SuppressLists(PARTITION_ALL_CLIENT_EDICTS, false);
	CBaseEntity::PushEnableAbsRecomputations(false);

	for (int i = 0; i < targetCount; i++)
	{
		trace_t trace;
		CIKTarget *pTarget = &m_pIk->m_target[i];

		if (!pTarget->IsActive())
			continue;

		switch (pTarget->type)
		{
		case IK_GROUND:
		{
			pTarget->SetPos(Vector(pTarget->est.pos.x, pTarget->est.pos.y, GetRenderOrigin().z));
			pTarget->SetAngles(GetRenderAngles());
		}
		break;

		case IK_ATTACHMENT:
		{
			C_BaseEntity *pEntity = NULL;
			float flDist = pTarget->est.radius;

			// FIXME: make entity finding sticky!
			// FIXME: what should the radius check be?
			for (CEntitySphereQuery sphere(pTarget->est.pos, 64); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
			{
				C_BaseAnimating *pAnim = pEntity->GetBaseAnimating();
				if (!pAnim)
					continue;

				int iAttachment = pAnim->LookupAttachment(pTarget->offset.pAttachmentName);
				if (iAttachment <= 0)
					continue;

				Vector origin;
				QAngle angles;
				pAnim->GetAttachment(iAttachment, origin, angles);

				// debugoverlay->AddBoxOverlay( origin, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );

				float d = (pTarget->est.pos - origin).Length();

				if (d >= flDist)
					continue;

				flDist = d;
				pTarget->SetPos(origin);
				pTarget->SetAngles(angles);
				// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
			}

			if (flDist >= pTarget->est.radius)
			{
				// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 0, 255, 0, 0 );
				// no solution, disable ik rule
				pTarget->IKFailed();
			}
		}
		break;
		}
	}

	CBaseEntity::PopEnableAbsRecomputations();
	partition->SuppressLists(curSuppressed, true);
}
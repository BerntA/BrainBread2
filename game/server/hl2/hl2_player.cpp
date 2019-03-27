//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "globalstate.h"
#include "game.h"
#include "gamerules.h"
#include "trains.h"
#include "vcollide_parse.h"
#include "in_buttons.h"
#include "ai_interactions.h"
#include "ai_squad.h"
#include "igamemovement.h"
#include "ai_hull.h"
#include "hl2_shareddefs.h"
#include "info_camera_link.h"
#include "point_camera.h"
#include "engine/IEngineSound.h"
#include "ndebugoverlay.h"
#include "iservervehicle.h"
#include "IVehicle.h"
#include "globals.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "effect_color_tables.h"
#include "vphysics/player_controller.h"
#include "player_pickup.h"
#include "hl2_shared_misc.h"
#include "script_intro.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h" 
#include "ai_basenpc.h"
#include "AI_Criteria.h"
#include "entitylist.h"
#include "env_zoom.h"
#include "datacache/imdlcache.h"
#include "eventqueue.h"
#include "gamestats.h"
#include "filters.h"
#include "hl2mp_gamerules.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar weapon_showproficiency;

// Do not touch with without seeing me, please! (sjb)
// For consistency's sake, enemy gunfire is traced against a scaled down
// version of the player's hull, not the hitboxes for the player's model
// because the player isn't aware of his model, and can't do anything about
// preventing headshots and other such things. Also, game difficulty will
// not change if the model changes. This is the value by which to scale
// the X/Y of the player's hull to get the volume to trace bullets against.
#define PLAYER_HULL_REDUCTION	0.70

#define TIME_IGNORE_FALL_DAMAGE 10.0

ConVar player_showpredictedposition( "player_showpredictedposition", "0" );
ConVar player_showpredictedposition_timestep( "player_showpredictedposition_timestep", "1.0" );

#ifndef HL2MP
#ifndef PORTAL
LINK_ENTITY_TO_CLASS( player, CHL2_Player );
#endif
#endif

PRECACHE_REGISTER(player);

CBaseEntity *FindEntityForward( CBasePlayer *pMe, bool fHull );

BEGIN_SIMPLE_DATADESC( LadderMove_t )
	DEFINE_FIELD( m_bForceLadderMove, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bForceMount, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_flArrivalTime, FIELD_TIME ),
	DEFINE_FIELD( m_vecGoalPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecStartPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_hForceLadder, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hReservedSpot, FIELD_EHANDLE ),
END_DATADESC()

	// Global Savedata for HL2 player
BEGIN_DATADESC( CHL2_Player )

	DEFINE_FIELD( m_nControlClass, FIELD_INTEGER ),
	DEFINE_EMBEDDED( m_HL2Local ),

	DEFINE_FIELD( m_fIsWalking, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flTimeIgnoreFallDamage, FIELD_TIME ),
	DEFINE_FIELD( m_bIgnoreFallDamageResetAfterImpact, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flLastDamageTime, FIELD_TIME ),
	DEFINE_FIELD( m_flTargetFindTime, FIELD_TIME ),

	DEFINE_FIELD( m_flNextFlashlightCheckTime, FIELD_TIME ),
	DEFINE_FIELD( m_bFlashlightDisabled, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "IgnoreFallDamage", InputIgnoreFallDamage ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "IgnoreFallDamageWithoutReset", InputIgnoreFallDamageWithoutReset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableFlashlight", InputDisableFlashlight ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableFlashlight", InputEnableFlashlight ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDropPhysObjects", InputForceDropPhysObjects ),

	DEFINE_SOUNDPATCH( m_sndLeeches ),
	DEFINE_SOUNDPATCH( m_sndWaterSplashes ),

	DEFINE_FIELD( m_flTimeUseSuspended, FIELD_TIME ),

END_DATADESC()

CHL2_Player::CHL2_Player()
{
	m_flNextEntityTraceCheck = 0.0f;
	m_flLastTimeReplenishedAmmo = 0.0f;
	m_flNextAmmoReplenishTime = 0.0f;
	m_iNumAmmoReplenishes = 0;
}

IMPLEMENT_SERVERCLASS_ST(CHL2_Player, DT_HL2_Player)
	SendPropDataTable(SENDINFO_DT(m_HL2Local), &REFERENCE_SEND_TABLE(DT_HL2Local), SendProxy_SendLocalDataTable),
END_SEND_TABLE()


void CHL2_Player::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "HL2Player.UseDeny" );
	PrecacheScriptSound( "HL2Player.FlashLightOn" );
	PrecacheScriptSound( "HL2Player.FlashLightOff" );
	PrecacheScriptSound( "HL2Player.PickupWeapon" );
	PrecacheScriptSound( "HL2Player.TrainUse" );
	PrecacheScriptSound( "HL2Player.Use" );
}

void CHL2_Player::HandleSpeedChanges( void )
{
	bool bIsWalking = IsWalking();
	bool bWantWalking = (m_nButtons & IN_WALK) && !(m_nButtons & IN_DUCK);
	if( bIsWalking != bWantWalking )
	{
		if ( bWantWalking )
			StartWalking();
		else
			StopWalking();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allow pre-frame adjustments on the player
//-----------------------------------------------------------------------------
void CHL2_Player::PreThink(void)
{
	if ( player_showpredictedposition.GetBool() )
	{
		Vector	predPos;

		UTIL_PredictedPosition( this, player_showpredictedposition_timestep.GetFloat(), &predPos );

		NDebugOverlay::Box( predPos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), 0, 255, 0, 0, 0.01f );
		NDebugOverlay::Line( GetAbsOrigin(), predPos, 0, 255, 0, 0, 0.01f );
	}

	// Riding a vehicle?
	if ( IsInAVehicle() )	
	{
		VPROF( "CHL2_Player::PreThink-Vehicle" );
		// make sure we update the client, check for timed damage and update suit even if we are in a vehicle
		UpdateClientData();		
		CheckTimeBasedDamage();
		WaterMove();	
		return;
	}

	VPROF_SCOPE_BEGIN( "CHL2_Player::PreThink-Speed" );
	HandleSpeedChanges();
	VPROF_SCOPE_END();

	if ( g_fGameOver || IsPlayerLockedInPlace() )
		return;         // finale

	VPROF_SCOPE_BEGIN( "CHL2_Player::PreThink-ItemPreFrame" );
	ItemPreFrame( );
	VPROF_SCOPE_END();

	VPROF_SCOPE_BEGIN( "CHL2_Player::PreThink-WaterMove" );
	WaterMove();
	VPROF_SCOPE_END();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
	{
		m_Local.m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	}
	else
	{
		m_Local.m_iHideHUD |= HIDEHUD_FLASHLIGHT;
	}

	// checks if new client data (for HUD and view control) needs to be sent to the client
	VPROF_SCOPE_BEGIN( "CHL2_Player::PreThink-UpdateClientData" );
	UpdateClientData();
	VPROF_SCOPE_END();

	VPROF_SCOPE_BEGIN( "CHL2_Player::PreThink-CheckTimeBasedDamage" );
	CheckTimeBasedDamage();
	VPROF_SCOPE_END();

	if (m_lifeState >= LIFE_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
		AddFlag( FL_ONTRAIN );
	else 
		RemoveFlag( FL_ONTRAIN );

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
	{
		CBaseEntity *pTrain = GetGroundEntity();
		float vel;

		if ( pTrain )
		{
			if ( !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) )
				pTrain = NULL;
		}

		if ( !pTrain )
		{
			if ( GetActiveWeapon() && (GetActiveWeapon()->ObjectCaps() & FCAP_DIRECTIONAL_USE) )
			{
				m_iTrain = TRAIN_ACTIVE | TRAIN_NEW;

				if ( m_nButtons & IN_FORWARD )
				{
					m_iTrain |= TRAIN_FAST;
				}
				else if ( m_nButtons & IN_BACK )
				{
					m_iTrain |= TRAIN_BACK;
				}
				else
				{
					m_iTrain |= TRAIN_NEUTRAL;
				}
				return;
			}
			else
			{
				trace_t trainTrace;
				// Maybe this is on the other side of a level transition
				UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-38), 
					MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trainTrace );

				if ( trainTrace.fraction != 1.0 && trainTrace.m_pEnt )
					pTrain = trainTrace.m_pEnt;


				if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(this) )
				{
					//					Warning( "In train mode with no train!\n" );
					m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
					m_iTrain = TRAIN_NEW|TRAIN_OFF;
					return;
				}
			}
		}
		else if ( !( GetFlags() & FL_ONGROUND ) || pTrain->HasSpawnFlags( SF_TRACKTRAIN_NOCONTROL ) || (m_nButtons & (IN_MOVELEFT|IN_MOVERIGHT) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			return;
		}

		SetAbsVelocity( vec3_origin );
		vel = 0;
		if ( m_afButtonPressed & IN_FORWARD )
		{
			vel = 1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}
		else if ( m_afButtonPressed & IN_BACK )
		{
			vel = -1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
			m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
		}
	} 
	else if (m_iTrain & TRAIN_ACTIVE)
	{
		m_iTrain = TRAIN_NEW; // turn off train
	}

	//
	// If we're not on the ground, we're falling. Update our falling velocity.
	//
	if ( !( GetFlags() & FL_ONGROUND ) )
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?
}

void CHL2_Player::PostThink( void )
{
	BaseClass::PostThink();

	CHL2MP_Player *pClient = ToHL2MPPlayer(this);
	if (!g_fGameOver && !IsPlayerLockedInPlace() && IsAlive() && !IsZombie() && pClient)
	{
		if (m_flNextEntityTraceCheck > gpGlobals->curtime)
			return;

		float flTime = 1.0f;
		int iResourcefulSkillValue = pClient->GetSkillValue(PLAYER_SKILL_HUMAN_RESOURCEFUL);
		if (iResourcefulSkillValue > 0)
			flTime -= ((1.0f / 100.0f) * pClient->GetSkillValue(PLAYER_SKILL_HUMAN_RESOURCEFUL, TEAM_HUMANS));

		flTime = clamp(flTime, 0.1f, 1.0f);

		m_flNextEntityTraceCheck = gpGlobals->curtime + flTime;

		if (m_nButtons & IN_USE)
		{
			Vector vecAim = BaseClass::GetAutoaimVector(AUTOAIM_SCALE_DIRECT_ONLY);
			trace_t	tr;
			CTraceFilterNoNPCsOrPlayer traceFilter(this, COLLISION_GROUP_NONE);
			UTIL_TraceLine(EyePosition(), EyePosition() + vecAim * 300.0f, MASK_SHOT, &traceFilter, &tr);

			CBaseEntity *aimTarget = tr.m_pEnt;
			if (aimTarget && !tr.DidHitWorld())
			{
				if (EyePosition().DistTo(aimTarget->GetAbsOrigin()) <= PLAYER_USE_RADIUS + 16.0f)
				{
					if (!strcmp(aimTarget->GetClassname(), "bbc_ammo_box"))
					{
						int timeLeft = ((int)(m_flNextAmmoReplenishTime - gpGlobals->curtime));
						if (timeLeft > 0)
						{
							char pchArg1[16];
							Q_snprintf(pchArg1, 16, "%d:%02d", (timeLeft / 60), (timeLeft % 60));
							GameBaseServer()->SendToolTip("#TOOLTIP_AMMOBOX_WAIT", "", 2.0f, GAME_TIP_WARNING, entindex(), pchArg1);
							return;
						}

						bool bDidReplenish = false;
						
						// Check through all available weapons that the user currently holds, give max ammo clip every sec.
						for (int i = 0; i < MAX_WEAPONS; i++)
						{
							CBaseCombatWeapon *pWeapon = GetWeapon(i);
							if ((pWeapon != NULL) && (pWeapon->GetWeaponType() != WEAPON_TYPE_SPECIAL))
							{
								bool bCanCheckSecondary = (pWeapon->GetPrimaryAmmoType() != pWeapon->GetSecondaryAmmoType());

								if (pWeapon->UsesPrimaryAmmo())
								{
									if (GiveAmmo(pWeapon->GetMaxClip1(), pWeapon->GetPrimaryAmmoType(), false))
										bDidReplenish = true;
								}

								if (bCanCheckSecondary && pWeapon->UsesSecondaryAmmo())
								{
									if (GiveAmmo(pWeapon->GetMaxClip2(), pWeapon->GetSecondaryAmmoType(), false))
										bDidReplenish = true;
								}
							}
						}

						// We received some ammo, check if we should be punished:
						if (bDidReplenish && !GameBaseServer()->IsTutorialModeEnabled())
						{
							float timeSinceLastReplenish = gpGlobals->curtime - m_flLastTimeReplenishedAmmo;
							if (timeSinceLastReplenish <= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flMaxAmmoReplensihInterval)
							{
								m_iNumAmmoReplenishes++;
								if (m_iNumAmmoReplenishes >= GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->iMaxAmmoReplenishWithinInterval)
								{
									float penaltyTime = GameBaseShared()->GetSharedGameDetails()->GetGamemodeData()->flAmmoReplenishPenalty;
									m_flNextAmmoReplenishTime = gpGlobals->curtime + penaltyTime;
									m_iNumAmmoReplenishes = 0;
									m_flLastTimeReplenishedAmmo = 0.0f;

									// Reduce the penalty if we have this skill!
									if (iResourcefulSkillValue > 0) // Reduces the value by max 1 min.
									{
										float fraction = ((float)iResourcefulSkillValue) / 10.0f;
										m_flNextAmmoReplenishTime -= clamp((fraction * (penaltyTime / 2.0f)), 0.0f, 60.0f);
									}

									return;
								}
							}
							else
							{
								m_flNextAmmoReplenishTime = 0.0f;
								m_iNumAmmoReplenishes = 0;
							}

							m_flLastTimeReplenishedAmmo = gpGlobals->curtime;
						}
					}
				}
			}
		}
	}
}

void CHL2_Player::Activate( void )
{
	BaseClass::Activate();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Class_T CHL2_Player::Classify ( void )
{
	// If player controlling another entity?  If so, return this class
	if (m_nControlClass != CLASS_NONE)
		return m_nControlClass;
	else
	{
		if(IsInAVehicle())
		{
			IServerVehicle *pVehicle = GetVehicle();
			if ( GetTeamNumber() == TEAM_DECEASED )
				return pVehicle->ClassifyPassenger( this, CLASS_PLAYER_ZOMB );
			else
				return pVehicle->ClassifyPassenger( this, CLASS_PLAYER );		
		}
	}

	return CLASS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CHL2_Player::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	return false;
}

void CHL2_Player::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
	// Can't use stuff while dead
	if ( IsDead() )	
		ucmd->buttons &= ~IN_USE;	

	BaseClass::PlayerRunCommand( ucmd, moveHelper );
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2_Player::Spawn(void)
{
	BaseClass::Spawn();

	//
	// Our player movement speed is set once here. This will override the cl_xxxx
	// cvars unless they are set to be lower than this.
	//
	//m_flMaxspeed = 320;

	m_Local.m_iHideHUD |= HIDEHUD_CHAT;

	m_flLastTimeReplenishedAmmo = 0.0f;
	m_flNextAmmoReplenishTime = 0.0f;
	m_iNumAmmoReplenishes = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::StartWalking( void )
{
	SetMaxSpeed((GetPlayerSpeed() / 2.0f));
	m_fIsWalking = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::StopWalking( void )
{
	SetMaxSpeed(GetPlayerSpeed());
	m_fIsWalking = false;
}

class CPhysicsPlayerCallback : public IPhysicsPlayerControllerEvent
{
public:
	int ShouldMoveTo( IPhysicsObject *pObject, const Vector &position )
	{
		CHL2_Player *pPlayer = (CHL2_Player *)pObject->GetGameData();
		if ( pPlayer )
		{
			if ( pPlayer->TouchedPhysics() )
			{
				return 0;
			}
		}
		return 1;
	}
};

static CPhysicsPlayerCallback playerCallback;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::InitVCollision( const Vector &vecAbsOrigin, const Vector &vecAbsVelocity )
{
	BaseClass::InitVCollision( vecAbsOrigin, vecAbsVelocity );

	// Setup the HL2 specific callback.
	IPhysicsPlayerController *pPlayerController = GetPhysicsController();
	if ( pPlayerController )
	{
		pPlayerController->SetEventHandler( &playerCallback );
	}
}

CHL2_Player::~CHL2_Player( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iImpulse - 
//-----------------------------------------------------------------------------
void CHL2_Player::CheatImpulseCommands( int iImpulse )
{
	switch( iImpulse )
	{
	case 52:
		{
			// Rangefinder
			trace_t tr;
			UTIL_TraceLine( EyePosition(), EyePosition() + EyeDirection3D() * MAX_COORD_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if( tr.fraction != 1.0 )
			{
				float flDist = (tr.startpos - tr.endpos).Length();
				float flDist2D = (tr.startpos - tr.endpos).Length2D();
				DevMsg( 1,"\nStartPos: %.4f %.4f %.4f --- EndPos: %.4f %.4f %.4f\n", tr.startpos.x,tr.startpos.y,tr.startpos.z,tr.endpos.x,tr.endpos.y,tr.endpos.z );
				DevMsg( 1,"3D Distance: %.4f units  (%.2f feet) --- 2D Distance: %.4f units  (%.2f feet)\n", flDist, flDist / 12.0, flDist2D, flDist2D / 12.0 );
			}

			break;
		}

	default:
		BaseClass::CheatImpulseCommands( iImpulse );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::SetupVisibility( CBaseEntity *pViewEntity, unsigned char *pvs, int pvssize )
{
	BaseClass::SetupVisibility( pViewEntity, pvs, pvssize );

	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	PointCameraSetupVisibility( this, area, pvs, pvssize );

	// If the intro script is playing, we want to get it's visibility points
	if ( g_hIntroScript )
	{
		Vector vecOrigin;
		CBaseEntity *pCamera;
		if ( g_hIntroScript->GetIncludedPVSOrigin( &vecOrigin, &pCamera ) )
		{
			// If it's a point camera, turn it on
			CPointCamera *pPointCamera = dynamic_cast< CPointCamera* >(pCamera); 
			if ( pPointCamera )
			{
				pPointCamera->SetActive( true );
			}
			engine->AddOriginToPVS( vecOrigin );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2_Player::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::FlashlightTurnOn( void )
{
	if( m_bFlashlightDisabled )
		return;

	AddEffects( EF_DIMLIGHT );
	EmitSound( "HL2Player.FlashLightOn" );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	EmitSound( "HL2Player.FlashLightOff" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define FLASHLIGHT_RANGE	Square(600)
bool CHL2_Player::IsIlluminatedByFlashlight( CBaseEntity *pEntity, float *flReturnDot )
{
	if( !FlashlightIsOn() )
		return false;

	// Within 50 feet?
	float flDistSqr = GetLocalOrigin().DistToSqr(pEntity->GetLocalOrigin());
	if( flDistSqr > FLASHLIGHT_RANGE )
		return false;

	// Within 45 degrees?
	Vector vecSpot = pEntity->WorldSpaceCenter();
	Vector los;

	// If the eyeposition is too close, move it back. Solves problems
	// caused by the player being too close the target.
	if ( flDistSqr < (128 * 128) )
	{
		Vector vecForward;
		EyeVectors( &vecForward );
		Vector vecMovedEyePos = EyePosition() - (vecForward * 128);
		los = ( vecSpot - vecMovedEyePos );
	}
	else
	{
		los = ( vecSpot - EyePosition() );
	}

	VectorNormalize( los );
	Vector facingDir = EyeDirection3D( );
	float flDot = DotProduct( los, facingDir );

	if ( flReturnDot )
	{
		*flReturnDot = flDot;
	}

	if ( flDot < 0.92387f )
		return false;

	if( !FVisible(pEntity) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Let NPCs know when the flashlight is trained on them
//-----------------------------------------------------------------------------
void CHL2_Player::CheckFlashlight( void )
{
	if ( !FlashlightIsOn() )
		return;

	if ( m_flNextFlashlightCheckTime > gpGlobals->curtime )
		return;

	m_flNextFlashlightCheckTime = gpGlobals->curtime + FLASHLIGHT_NPC_CHECK_INTERVAL;

	// Loop through NPCs looking for illuminated ones
	for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		CAI_BaseNPC *pNPC = g_AI_Manager.AccessAIs()[i];
		if (pNPC == NULL)
			continue;

		float flDot;

		if ( IsIlluminatedByFlashlight( pNPC, &flDot ) )
		{
			pNPC->PlayerHasIlluminatedNPC( this, flDot );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::SetPlayerUnderwater( bool state )
{
	BaseClass::SetPlayerUnderwater( state );
}

//-----------------------------------------------------------------------------
bool CHL2_Player::PassesDamageFilter( const CTakeDamageInfo &info )
{
	CBaseEntity *pAttacker = info.GetAttacker();
	if (pAttacker && 
		((pAttacker->MyNPCPointer() && pAttacker->MyNPCPointer()->IsPlayerAlly()) ||
		((pAttacker->Classify() == CLASS_MILITARY_VEHICLE) && IsHuman()))		
		)
	{
		return false;
	}

	return BaseClass::PassesDamageFilter( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::SetFlashlightEnabled( bool bState )
{
	m_bFlashlightDisabled = !bState;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::InputDisableFlashlight( inputdata_t &inputdata )
{
	if( FlashlightIsOn() )
		FlashlightTurnOff();

	SetFlashlightEnabled( false );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::InputEnableFlashlight( inputdata_t &inputdata )
{
	SetFlashlightEnabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: Prevent the player from taking fall damage for [n] seconds, but
// reset back to taking fall damage after the first impact (so players will be
// hurt if they bounce off what they hit). This is the original behavior.
//-----------------------------------------------------------------------------
void CHL2_Player::InputIgnoreFallDamage( inputdata_t &inputdata )
{
	float timeToIgnore = inputdata.value.Float();

	if ( timeToIgnore <= 0.0 )
		timeToIgnore = TIME_IGNORE_FALL_DAMAGE;

	m_flTimeIgnoreFallDamage = gpGlobals->curtime + timeToIgnore;
	m_bIgnoreFallDamageResetAfterImpact = true;
}


//-----------------------------------------------------------------------------
// Purpose: Absolutely prevent the player from taking fall damage for [n] seconds. 
//-----------------------------------------------------------------------------
void CHL2_Player::InputIgnoreFallDamageWithoutReset( inputdata_t &inputdata )
{
	float timeToIgnore = inputdata.value.Float();

	if ( timeToIgnore <= 0.0 )
		timeToIgnore = TIME_IGNORE_FALL_DAMAGE;

	m_flTimeIgnoreFallDamage = gpGlobals->curtime + timeToIgnore;
	m_bIgnoreFallDamageResetAfterImpact = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::NotifyFriendsOfDamage( CBaseEntity *pAttackerEntity )
{
	CAI_BaseNPC *pAttacker = pAttackerEntity->MyNPCPointer();
	if ( pAttacker )
	{
		const Vector &origin = GetAbsOrigin();
		const float NEAR_Z = 12 * 12;
		const float NEAR_XY_SQ = Square(50 * 12);
		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_BaseNPC *pNpc = g_AI_Manager.AccessAIs()[i];
			if (!pNpc)
				continue;

			if ( pNpc->IsPlayerAlly() )
			{
				const Vector &originNpc = pNpc->GetAbsOrigin();
				if ( fabsf( originNpc.z - origin.z ) < NEAR_Z )
				{
					if ( (originNpc.AsVector2D() - origin.AsVector2D()).LengthSqr() < NEAR_XY_SQ )
					{
						pNpc->OnFriendDamaged( this, pAttacker );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar test_massive_dmg("test_massive_dmg", "30" );
ConVar test_massive_dmg_clip("test_massive_dmg_clip", "0.5" );
int	CHL2_Player::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( GlobalEntity_GetState( "gordon_invulnerable" ) == GLOBAL_ON )
		return 0;

	// ignore fall damage if instructed to do so by input
	if ( ( info.GetDamageType() & DMG_FALL ) && m_flTimeIgnoreFallDamage > gpGlobals->curtime )
	{
		// usually, we will reset the input flag after the first impact. However there is another input that
		// prevents this behavior.
		if ( m_bIgnoreFallDamageResetAfterImpact )
		{
			m_flTimeIgnoreFallDamage = 0;
		}
		return 0;
	}

	if( info.GetDamageType() & DMG_BLAST_SURFACE )
	{
		if( GetWaterLevel() > 2 )
		{
			// Don't take blast damage from anything above the surface.
			if( info.GetInflictor()->GetWaterLevel() == 0 )
			{
				return 0;
			}
		}
	}

	if ( info.GetDamage() > 0.0f )
	{
		m_flLastDamageTime = gpGlobals->curtime;

		if ( info.GetAttacker() )
			NotifyFriendsOfDamage( info.GetAttacker() );
	}

	// Modify the amount of damage the player takes, based on skill.
	CTakeDamageInfo playerDamage = info;

	// Should we run this damage through the skill level adjustment?
	bool bAdjustForSkillLevel = true;

	if( info.GetDamageType() == DMG_GENERIC && info.GetAttacker() == this && info.GetInflictor() == this )
	{
		// Only do a skill level adjustment if the player isn't his own attacker AND inflictor.
		// This prevents damage from SetHealth() inputs from being adjusted for skill level.
		bAdjustForSkillLevel = false;
	}

	if ( GetVehicleEntity() != NULL && GlobalEntity_GetState("gordon_protect_driver") == GLOBAL_ON )
	{
		if( playerDamage.GetDamage() > test_massive_dmg.GetFloat() && playerDamage.GetInflictor() == GetVehicleEntity() && (playerDamage.GetDamageType() & DMG_CRUSH) )
		{
			playerDamage.ScaleDamage( test_massive_dmg_clip.GetFloat() / playerDamage.GetDamage() );
		}
	}

	if( bAdjustForSkillLevel )
	{
		playerDamage.AdjustPlayerDamageTakenForSkillLevel();
	}

	gamestats->Event_PlayerDamage( this, info );

	return BaseClass::OnTakeDamage( playerDamage );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CHL2_Player::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Drown
	if( info.GetDamageType() & DMG_DROWN )
	{
		HL2MPRules()->EmitSoundToClient(this, "DrownPain", GetSoundType(), GetSoundsetGender());
	}

	// Burnt
	if ( info.GetDamageType() & DMG_BURN )
	{
		HL2MPRules()->EmitSoundToClient(this, "BurnPain", GetSoundType(), GetSoundsetGender());
	}

	// Call the base class implementation
	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2_Player::OnDamagedByExplosion( const CTakeDamageInfo &info )
{
	if ( info.GetInflictor() && info.GetInflictor()->ClassMatches( "mortarshell" ) )
	{
		// No ear ringing for mortar
		UTIL_ScreenShake( info.GetInflictor()->GetAbsOrigin(), 4.0, 1.0, 0.5, 1000, SHAKE_START, false );
		return;
	}
	BaseClass::OnDamagedByExplosion( info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHL2_Player::ShouldShootMissTarget( CBaseCombatCharacter *pAttacker )
{
	if( gpGlobals->curtime > m_flTargetFindTime )
	{
		// Put this off into the future again.
		m_flTargetFindTime = gpGlobals->curtime + random->RandomFloat( 3, 5 );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iCount - 
//			iAmmoIndex - 
//			bSuppressSound - 
// Output : int
//-----------------------------------------------------------------------------
int CHL2_Player::GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound)
{
	// Don't try to give the player invalid ammo indices.
	if (nAmmoIndex < 0)
		return 0;

	int nAdd = BaseClass::GiveAmmo(nCount, nAmmoIndex, bSuppressSound);

	if ( nCount > 0 && nAdd == 0 )
	{
		// we've been denied the pickup, display a hud icon to show that
		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "AmmoDenied" );
		WRITE_SHORT( nAmmoIndex );
		MessageEnd();
	}

	if (nAdd > 0)
	{
		// Show the pickup icon:
		CSingleUserRecipientFilter user(this);
		user.MakeReliable();
		UserMessageBegin(user, "ItemPickup");
		WRITE_STRING("AMMO");
		WRITE_SHORT(nAmmoIndex);
		MessageEnd();
	}

	return nAdd;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHL2_Player::Weapon_CanUse( CBaseCombatWeapon *pWeapon )
{
	return BaseClass::Weapon_CanUse( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pWeapon - 
//-----------------------------------------------------------------------------
void CHL2_Player::Weapon_Equip( CBaseCombatWeapon *pWeapon )
{
	if ( GetActiveWeapon() && ( Weapon_GetSlot( pWeapon->GetSlot() ) != NULL ) )
		Weapon_DropSlot( pWeapon->GetSlot() );

	BaseClass::Weapon_Equip( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *cmd - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2_Player::ClientCommand( const CCommand &args )
{
	// Drop primary weapon:
	if (!Q_stricmp(args[0], "DropPrimary"))
	{
		if ((!HL2MPRules()->m_bRoundStarted && HL2MPRules()->ShouldHideHUDDuringRoundWait()) || HL2MPRules()->IsGameoverOrScoresVisible())
			return true;

		CBaseCombatWeapon *pActiveWeapon = GetActiveWeapon();
		if (pActiveWeapon)
		{
			if (!pActiveWeapon->CanHolster() || pActiveWeapon->m_bWantsHolster)
				return true;

			if (FClassnameIs(pActiveWeapon, "weapon_zombhands") || FClassnameIs(pActiveWeapon, "weapon_hands"))
				return true;

			Weapon_DropSlot(pActiveWeapon->GetSlot());
			return true;
		}

		return true;
	}

	return BaseClass::ClientCommand( args );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void CBasePlayer::PlayerUse
//-----------------------------------------------------------------------------
void CHL2_Player::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((m_nButtons | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	if ( m_afButtonPressed & IN_USE )
	{
		// Currently using a latched entity?
		if ( ClearUseEntity() )
		{
			return;
		}
		else
		{
			m_flPlayerUseTime = gpGlobals->curtime;

			if ( m_afPhysicsFlags & PFLAG_DIROVERRIDE )
			{
				m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = GetGroundEntity();
				if ( pTrain && !(m_nButtons & IN_JUMP) && (GetFlags() & FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(this) )
				{
					m_afPhysicsFlags |= PFLAG_DIROVERRIDE;
					m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
					m_iTrain |= TRAIN_NEW;
					EmitSound( "HL2Player.TrainUse" );
					return;
				}
			}
		}

		// Tracker 3926:  We can't +USE something if we're climbing a ladder
		if ( GetMoveType() == MOVETYPE_LADDER )
		{
			return;
		}
	}

	if( m_flTimeUseSuspended > gpGlobals->curtime )
	{
		// Something has temporarily stopped us being able to USE things.
		// Obviously, this should be used very carefully.(sjb)
		return;
	}

	CBaseEntity *pUseEntity = FindUseEntity();

	bool usedSomething = false;

	// Found an object
	if ( pUseEntity )
	{
		float flSec = gpGlobals->curtime - m_flPlayerUseTime;

		//!!!UNDONE: traceline here to prevent +USEing buttons through walls			
		int caps = pUseEntity->ObjectCaps();
		variant_t emptyVariant;

		if ( m_afButtonPressed & IN_USE )
		{
			// Robin: Don't play sounds for NPCs, because NPCs will allow respond with speech.
			if ( !pUseEntity->MyNPCPointer() )
			{
				EmitSound( "HL2Player.Use" );
			}
		}

		if ( ( (m_nButtons & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );

			usedSomething = true;
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pUseEntity->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pUseEntity->AcceptInput( "Use", this, this, emptyVariant, USE_TOGGLE );

			usedSomething = true;
		}

		if ((flSec > DELAYED_USE_TIME))
		{
			m_flPlayerUseTime = gpGlobals->curtime;
			pUseEntity->DelayedUse(this);
			usedSomething = true;
		}
	}
	else if ( m_afButtonPressed & IN_USE )
	{
		// Signal that we want to play the deny sound, unless the user is +USEing on a ladder!
		// The sound is emitted in ItemPostFrame, since that occurs after GameMovement::ProcessMove which
		// lets the ladder code unset this flag.
		m_bPlayUseDenySound = true;
	}

	// Debounce the use key
	if ( usedSomething && pUseEntity )
	{
		m_Local.m_nOldButtons |= IN_USE;
		m_afButtonPressed &= ~IN_USE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we can switch to the given weapon.
// Input  : pWeapon - 
//-----------------------------------------------------------------------------
bool CHL2_Player::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	CBasePlayer *pPlayer = (CBasePlayer *)this;
#if !defined( CLIENT_DLL )
	IServerVehicle *pVehicle = pPlayer->GetVehicle();
#else
	IClientVehicle *pVehicle = pPlayer->GetVehicle();
#endif

	if (pVehicle && !pPlayer->UsingStandardWeaponsInVehicle())
		return false;

	if ( GetActiveWeapon() )
	{
		if ( !GetActiveWeapon()->CanHolster() )
			return false;
	}

	return true;
}

extern ConVar sv_turbophysics;

void CHL2_Player::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	// BB2 Warn
	//if ((VPhysicsGetObject() == NULL) || sv_turbophysics.GetBool())
	//	return;

	// can't pick up what you're standing on
	if ( GetGroundEntity() == pObject )
		return;

	if ( !GetGroundEntity() )
		return;

	if ( bLimitMassAndSize == true )
	{
		if ( CBasePlayer::CanPickupObject( pObject, 45, 200 ) == false )
			return;
	}

	// Can't be picked up if NPCs are on me
	if ( pObject->HasNPCsOnIt() )
		return;

	PlayerPickupObject( this, pObject );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
bool CHL2_Player::IsHoldingEntity( CBaseEntity *pEnt )
{
	return PlayerPickupControllerIsHoldingEntity( m_hUseEntity, pEnt );
}

float CHL2_Player::GetHeldObjectMass( IPhysicsObject *pHeldObject )
{
	return PlayerPickupGetHeldObjectMass(m_hUseEntity, pHeldObject);
}

//-----------------------------------------------------------------------------
// Purpose: Force the player to drop any physics objects he's carrying
//-----------------------------------------------------------------------------
void CHL2_Player::ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis )
{
	if ( PhysIsInCallback() )
	{
		variant_t value;
		g_EventQueue.AddEvent( this, "ForceDropPhysObjects", value, 0.01f, pOnlyIfHoldingThis, this );
		return;
	}

	// Drop any objects being handheld.
	ClearUseEntity();
}

void CHL2_Player::InputForceDropPhysObjects( inputdata_t &data )
{
	ForceDropOfCarriedPhysObjects( data.pActivator );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2_Player::UpdateClientData( void )
{
	if (m_DmgTake || m_DmgSave || m_bitsHUDDamage != m_bitsDamageType)
	{
		CSingleUserRecipientFilter user(this);
		user.MakeReliable();
		UserMessageBegin(user, "Damage");
		WRITE_BYTE(1);
		MessageEnd();

		m_DmgTake = 0;
		m_DmgSave = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= iTimeBasedDamage;
	}

	BaseClass::UpdateClientData();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CHL2_Player::OnRestore()
{
	BaseClass::OnRestore();
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CHL2_Player::EyeDirection2D( void )
{
	Vector vecReturn = EyeDirection3D();
	vecReturn.z = 0;
	vecReturn.AsVector2D().NormalizeInPlace();

	return vecReturn;
}

//---------------------------------------------------------
//---------------------------------------------------------
Vector CHL2_Player::EyeDirection3D( void )
{
	Vector vecForward;

	// Return the vehicle angles if we request them
	if ( GetVehicle() != NULL )
	{
		CacheVehicleView();
		EyeVectors( &vecForward );
		return vecForward;
	}

	AngleVectors( EyeAngles(), &vecForward );
	return vecForward;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CHL2_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, bool bWantDraw, int viewmodelindex )
{
	MDLCACHE_CRITICAL_SECTION();

	bool bRet = BaseClass::Weapon_Switch( pWeapon, bWantDraw, viewmodelindex );
	if ( bRet )
	{
		// Recalculate proficiency!
		SetCurrentWeaponProficiency( CalcWeaponProficiency( pWeapon ) );
	}

	return bRet;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
WeaponProficiency_t CHL2_Player::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	WeaponProficiency_t proficiency;

	proficiency = WEAPON_PROFICIENCY_PERFECT;

	if( weapon_showproficiency.GetBool() != 0 )
	{
		Msg("Player switched to %s, proficiency is %s\n", pWeapon->GetClassname(), GetWeaponProficiencyName( proficiency ) );
	}

	return proficiency;
}

//-----------------------------------------------------------------------------
// Purpose: override how single player rays hit the player
//-----------------------------------------------------------------------------

bool LineCircleIntersection(
	const Vector2D &center,
	const float radius,
	const Vector2D &vLinePt,
	const Vector2D &vLineDir,
	float *fIntersection1,
	float *fIntersection2)
{
	// Line = P + Vt
	// Sphere = r (assume we've translated to origin)
	// (P + Vt)^2 = r^2
	// VVt^2 + 2PVt + (PP - r^2)
	// Solve as quadratic:  (-b  +/-  sqrt(b^2 - 4ac)) / 2a
	// If (b^2 - 4ac) is < 0 there is no solution.
	// If (b^2 - 4ac) is = 0 there is one solution (a case this function doesn't support).
	// If (b^2 - 4ac) is > 0 there are two solutions.
	Vector2D P;
	float a, b, c, sqr, insideSqr;


	// Translate circle to origin.
	P[0] = vLinePt[0] - center[0];
	P[1] = vLinePt[1] - center[1];

	a = vLineDir.Dot(vLineDir);
	b = 2.0f * P.Dot(vLineDir);
	c = P.Dot(P) - (radius * radius);

	insideSqr = b*b - 4*a*c;
	if(insideSqr <= 0.000001f)
		return false;

	// Ok, two solutions.
	sqr = (float)FastSqrt(insideSqr);

	float denom = 1.0 / (2.0f * a);

	*fIntersection1 = (-b - sqr) * denom;
	*fIntersection2 = (-b + sqr) * denom;

	return true;
}

static void Collision_ClearTrace( const Vector &vecRayStart, const Vector &vecRayDelta, CBaseTrace *pTrace )
{
	pTrace->startpos = vecRayStart;
	pTrace->endpos = vecRayStart;
	pTrace->endpos += vecRayDelta;
	pTrace->startsolid = false;
	pTrace->allsolid = false;
	pTrace->fraction = 1.0f;
	pTrace->contents = 0;
}

bool IntersectRayWithAACylinder( const Ray_t &ray, 
	const Vector &center, float radius, float height, CBaseTrace *pTrace )
{
	Assert( ray.m_IsRay );
	Collision_ClearTrace( ray.m_Start, ray.m_Delta, pTrace );

	// First intersect the ray with the top + bottom planes
	float halfHeight = height * 0.5;

	// Handle parallel case
	Vector vStart = ray.m_Start - center;
	Vector vEnd = vStart + ray.m_Delta;

	float flEnterFrac, flLeaveFrac;
	if (FloatMakePositive(ray.m_Delta.z) < 1e-8)
	{
		if ( (vStart.z < -halfHeight) || (vStart.z > halfHeight) )
		{
			return false; // no hit
		}
		flEnterFrac = 0.0f; flLeaveFrac = 1.0f;
	}
	else
	{
		// Clip the ray to the top and bottom of box
		flEnterFrac = IntersectRayWithAAPlane( vStart, vEnd, 2, 1, halfHeight);
		flLeaveFrac = IntersectRayWithAAPlane( vStart, vEnd, 2, 1, -halfHeight);

		if ( flLeaveFrac < flEnterFrac )
		{
			float temp = flLeaveFrac;
			flLeaveFrac = flEnterFrac;
			flEnterFrac = temp;
		}

		if ( flLeaveFrac < 0 || flEnterFrac > 1)
		{
			return false;
		}
	}

	// Intersect with circle
	float flCircleEnterFrac, flCircleLeaveFrac;
	if ( !LineCircleIntersection( vec3_origin.AsVector2D(), radius,
		vStart.AsVector2D(), ray.m_Delta.AsVector2D(), &flCircleEnterFrac, &flCircleLeaveFrac ) )
	{
		return false; // no hit
	}

	Assert( flCircleEnterFrac <= flCircleLeaveFrac );
	if ( flCircleLeaveFrac < 0 || flCircleEnterFrac > 1)
	{
		return false;
	}

	if ( flEnterFrac < flCircleEnterFrac )
		flEnterFrac = flCircleEnterFrac;
	if ( flLeaveFrac > flCircleLeaveFrac )
		flLeaveFrac = flCircleLeaveFrac;

	if ( flLeaveFrac < flEnterFrac )
		return false;

	VectorMA( ray.m_Start, flEnterFrac , ray.m_Delta, pTrace->endpos );
	pTrace->fraction = flEnterFrac;
	pTrace->contents = CONTENTS_SOLID;

	// Calculate the point on our center line where we're nearest the intersection point
	Vector collisionCenter;
	CalcClosestPointOnLineSegment( pTrace->endpos, center + Vector( 0, 0, halfHeight ), center - Vector( 0, 0, halfHeight ), collisionCenter );

	// Our normal is the direction from that center point to the intersection point
	pTrace->plane.normal = pTrace->endpos - collisionCenter;
	VectorNormalize( pTrace->plane.normal );

	return true;
}

bool CHL2_Player::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	if( g_pGameRules->IsMultiplayer() )
	{
		return BaseClass::TestHitboxes( ray, fContentsMask, tr );
	}
	else
	{
		Assert( ray.m_IsRay );

		Vector mins, maxs;

		mins = WorldAlignMins();
		maxs = WorldAlignMaxs();

		if ( IntersectRayWithAACylinder( ray, WorldSpaceCenter(), maxs.x * PLAYER_HULL_REDUCTION, maxs.z - mins.z, &tr ) )
		{
			tr.hitbox = 0;
			CStudioHdr *pStudioHdr = GetModelPtr( );
			if (!pStudioHdr)
				return false;

			mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
			if ( !set || !set->numhitboxes )
				return false;

			mstudiobbox_t *pbox = set->pHitbox( tr.hitbox );
			mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);
			tr.surface.name = "**studio**";
			tr.surface.flags = SURF_HITBOX;
			tr.surface.surfaceProps = physprops->GetSurfaceIndex( pBone->pszSurfaceProp() );
		}

		return true;
	}
}

//---------------------------------------------------------
// Show the player's scaled down bbox that we use for
// bullet impacts.
//---------------------------------------------------------
void CHL2_Player::DrawDebugGeometryOverlays(void) 
{
	BaseClass::DrawDebugGeometryOverlays();

	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{	
		Vector mins, maxs;

		mins = WorldAlignMins();
		maxs = WorldAlignMaxs();

		mins.x *= PLAYER_HULL_REDUCTION;
		mins.y *= PLAYER_HULL_REDUCTION;

		maxs.x *= PLAYER_HULL_REDUCTION;
		maxs.y *= PLAYER_HULL_REDUCTION;

		NDebugOverlay::Box( GetAbsOrigin(), mins, maxs, 255, 0, 0, 100, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper to remove from ladder
//-----------------------------------------------------------------------------
void CHL2_Player::ExitLadder()
{
	if ( MOVETYPE_LADDER != GetMoveType() )
		return;

	SetMoveType( MOVETYPE_WALK );
	SetMoveCollide( MOVECOLLIDE_DEFAULT );
	// Remove from ladder
	m_HL2Local.m_hLadder.Set( NULL );
}

surfacedata_t *CHL2_Player::GetLadderSurface( const Vector &origin )
{
	extern const char *FuncLadder_GetSurfaceprops(CBaseEntity *pLadderEntity);

	CBaseEntity *pLadder = m_HL2Local.m_hLadder.Get();
	if ( pLadder )
	{
		const char *pSurfaceprops = FuncLadder_GetSurfaceprops(pLadder);
		// get ladder material from func_ladder
		return physprops->GetSurfaceData( physprops->GetSurfaceIndex( pSurfaceprops ) );

	}
	return BaseClass::GetLadderSurface(origin);
}

//-----------------------------------------------------------------------------
// Purpose: Queues up a use deny sound, played in ItemPostFrame.
//-----------------------------------------------------------------------------
void CHL2_Player::PlayUseDenySound()
{
	m_bPlayUseDenySound = true;
}

void CHL2_Player::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if ( m_bPlayUseDenySound )
	{
		m_bPlayUseDenySound = false;
		EmitSound( "HL2Player.UseDeny" );
	}
}

void CHL2_Player::StartWaterDeathSounds( void )
{
	CPASAttenuationFilter filter( this );

	if ( m_sndLeeches == NULL )
	{
		m_sndLeeches = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "coast.leech_bites_loop" , ATTN_NORM );
	}

	if ( m_sndLeeches )
	{
		(CSoundEnvelopeController::GetController()).Play( m_sndLeeches, 1.0f, 100 );
	}

	if ( m_sndWaterSplashes == NULL )
	{
		m_sndWaterSplashes = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "coast.leech_water_churn_loop" , ATTN_NORM );
	}

	if ( m_sndWaterSplashes )
	{
		(CSoundEnvelopeController::GetController()).Play( m_sndWaterSplashes, 1.0f, 100 );
	}
}

void CHL2_Player::StopWaterDeathSounds( void )
{
	if ( m_sndLeeches )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndLeeches, 0.5f, true );
		m_sndLeeches = NULL;
	}

	if ( m_sndWaterSplashes )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndWaterSplashes, 0.5f, true );
		m_sndWaterSplashes = NULL;
	}
}

//-----------------------------------------------------------------------------
// Shuts down sounds
//-----------------------------------------------------------------------------
void CHL2_Player::StopLoopingSounds( void )
{
	if ( m_sndLeeches != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundDestroy( m_sndLeeches );
		m_sndLeeches = NULL;
	}

	if ( m_sndWaterSplashes != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundDestroy( m_sndWaterSplashes );
		m_sndWaterSplashes = NULL;
	}

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
void CHL2_Player::ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendPlayerCriteria( set );

	if ( GlobalEntity_GetIndex( "gordon_precriminal" ) == -1 )
	{
		set.AppendCriteria( "gordon_precriminal", "0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes a splash when the player transitions between water states
//-----------------------------------------------------------------------------
void CHL2_Player::Splash( void )
{
	CEffectData data;
	data.m_fFlags = 0;
	data.m_vOrigin = GetAbsOrigin();
	data.m_vNormal = Vector(0,0,1);
	data.m_vAngles = QAngle( 0, 0, 0 );

	if ( GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	float flSpeed = GetAbsVelocity().Length();
	if ( flSpeed < 300 )
	{
		data.m_flScale = random->RandomFloat( 10, 12 );
		DispatchEffect( "waterripple", data );
	}
	else
	{
		data.m_flScale = random->RandomFloat( 6, 8 );
		DispatchEffect( "watersplash", data );
	}
}
//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: BB2 Player Animstate.
//
//========================================================================================//

#include "cbase.h"
#include "hl2mp_playeranimstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "activitylist.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "datacache/imdlcache.h"
#include "GameBase_Shared.h"
#include "weapon_hl2mpbasebasebludgeon.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include "c_bb2_player_shared.h"
#include "c_playermodel.h"
#include "eventlist.h"
#else
#include "hl2mp_player.h"
#endif

#define MOVING_MINIMUM_SPEED 0.5f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CHL2MPPlayerAnimState*
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState *CreateHL2MPPlayerAnimState(CHL2MP_Player *pPlayer)
{
	MDLCACHE_CRITICAL_SECTION();
	return new CHL2MPPlayerAnimState(pPlayer);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState::CHL2MPPlayerAnimState(CHL2MP_Player *pPlayer)
#ifdef CLIENT_DLL
	: m_iv_flMaxGroundSpeed("CHL2MPPlayerAnimState::m_iv_flMaxGroundSpeed")
#endif
{
	m_pHL2MPPlayer = pPlayer;

	// Pose parameters.
	m_bPoseParameterInit = false;
	m_PoseParameterData.Init();

	m_angRender.Init();
	m_vecCachedVelocity.Init();

	m_flLastAnimationStateClearTime = 0.0f;
	m_flEyeYaw = 0.0f;
	m_flEyePitch = 0.0f;
	m_flGoalFeetYaw = 0.0f;
	m_flCurrentFeetYaw = 0.0f;
	m_flLastAimTurnTime = 0.0f;

	// Jumping.
	m_bJumping = false;
	m_flJumpStartTime = 0.0f;
	m_bFirstJumpFrame = false;

	// Sliding
	m_bSliding = false;
	m_bFirstSlideFrame = false;
	m_flSlideGestureTime = 0.0f;

	// Swimming
	m_bInSwim = false;
	m_bFirstSwimFrame = true;

	// Dying
	m_bDying = false;
	m_bFirstDyingFrame = true;

	m_eCurrentMainSequenceActivity = ACT_INVALID;
	m_nSpecificMainSequence = -1;

	// Ground speed interpolators.
#ifdef CLIENT_DLL
	m_iv_flMaxGroundSpeed.Setup(&m_flMaxGroundSpeed, LATCH_ANIMATION_VAR | INTERPOLATE_LINEAR_ONLY);
	m_flLastGroundSpeedUpdateTime = 0.0f;
#endif

	m_flMaxGroundSpeed = 0.0f;
	InitGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState::~CHL2MPPlayerAnimState()
{
	ShutdownGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::Release(void)
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: Allows us to use the new player model object for the client only.
//-----------------------------------------------------------------------------
CBaseAnimatingOverlay *CHL2MPPlayerAnimState::GetBaseAnimatable(void)
{
#ifdef CLIENT_DLL
	if (m_pHL2MPPlayer && m_pHL2MPPlayer->GetNewPlayerModel())
		return m_pHL2MPPlayer->GetNewPlayerModel();
#endif
	return m_pHL2MPPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : const QAngle&
//-----------------------------------------------------------------------------
const QAngle& CHL2MPPlayerAnimState::GetRenderAngles()
{
	return m_angRender;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ClearAnimationState(void)
{
	m_bJumping = false;
	m_bDying = false;
	m_bSliding = false;
	m_flLastAnimationStateClearTime = gpGlobals->curtime;
	m_nSpecificMainSequence = -1;
	ResetGestureSlots();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::ShouldUpdateAnimState()
{
	// Don't update anim state if we're not visible
	if (GetBasePlayer()->IsEffectActive(EF_NODRAW))
		return false;

	// By default, don't update their animation state when they're dead because they're
	// either a ragdoll or they're not drawn.
#ifdef CLIENT_DLL
	if (GetBasePlayer()->IsDormant())
		return false;
#endif

	return (GetBasePlayer()->IsAlive() || m_bDying);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::Update(float eyeYaw, float eyePitch)
{
	// Profile the animation update.
	VPROF("CHL2MPPlayerAnimState::Update");

	CBaseAnimatingOverlay *pAnimating = GetBaseAnimatable();
	if (!pAnimating)
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pAnimating->GetModelPtr();
	if (!pStudioHdr)
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if (!ShouldUpdateAnimState())
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize(eyeYaw);
	m_flEyePitch = AngleNormalize(eyePitch);	

#if defined( CLIENT_DLL )
	GetBasePlayer()->EstimateAbsVelocity(m_vecCachedVelocity);
	if (GetBasePlayer()->IsLocalPlayer() && (GetOuterXYSpeed() <= 0.002f))
		m_vecCachedVelocity = vec3_origin;
#else
	m_vecCachedVelocity = GetBasePlayer()->GetAbsVelocity();
#endif

	// Compute the player sequences.
	ComputeSequences(pStudioHdr);

	if (SetupPoseParameters(pStudioHdr))
	{
		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw(pStudioHdr);

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch(pStudioHdr);

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw(pStudioHdr);
	}

#ifdef CLIENT_DLL 
	if (C_BasePlayer::ShouldDrawLocalPlayer())
	{
		pAnimating->SetPlaybackRate(1.0f);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputeSequences(CStudioHdr *pStudioHdr)
{
	VPROF("CHL2MPPlayerAnimState::ComputeSequences");

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();
	ComputeGestureSequence(pStudioHdr);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputeMainSequence()
{
	VPROF("CHL2MPPlayerAnimState::ComputeMainSequence");

#ifdef CLIENT_DLL
	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();
#endif

	CBaseAnimatingOverlay *pPlayerNew = GetBaseAnimatable();

	// Have our class or the mod-specific class determine what the current activity is.
	Activity idealActivity = CalcMainActivity();

#ifdef CLIENT_DLL
	Activity oldActivity = m_eCurrentMainSequenceActivity;
#endif

	// Store our current activity so the aim and fire layers know what to do.
	m_eCurrentMainSequenceActivity = idealActivity;

	// Hook to force playback of a specific requested full-body sequence
	if (m_nSpecificMainSequence >= 0)
	{
		if (pPlayerNew->GetSequence() != m_nSpecificMainSequence)
		{
			pPlayerNew->ResetSequence(m_nSpecificMainSequence);
			ResetGroundSpeed();

#ifdef CLIENT_DLL
			BB2PlayerGlobals->BodyResetSequence(pPlayer, m_nSpecificMainSequence);
#endif
			return;
		}

		if (!pPlayerNew->IsSequenceFinished())
			return;

		m_nSpecificMainSequence = -1;
		RestartMainSequence();
		ResetGroundSpeed();
	}

	// Export to our outer class..
	int animDesired = SelectWeightedSequence(TranslateActivity(idealActivity));
	if (pPlayerNew->GetSequenceActivity(pPlayerNew->GetSequence()) == pPlayerNew->GetSequenceActivity(animDesired))
		return;

	if (animDesired < 0)
		animDesired = 0;

	pPlayerNew->ResetSequence(animDesired);

#ifdef CLIENT_DLL
	BB2PlayerGlobals->BodyResetSequence(pPlayer, animDesired);
#endif

#ifdef CLIENT_DLL
	// If we went from idle to walk, reset the interpolation history.
	// Kind of hacky putting this here.. it might belong outside the base class.
	if ((oldActivity == ACT_MP_CROUCH_IDLE || oldActivity == ACT_MP_STAND_IDLE || oldActivity == ACT_MP_DEPLOYED_IDLE || oldActivity == ACT_MP_CROUCH_DEPLOYED_IDLE) &&
		(idealActivity == ACT_MP_WALK || idealActivity == ACT_MP_CROUCHWALK))
	{
		ResetGroundSpeed();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::UpdateInterpolators()
{
	VPROF("CHL2MPPlayerAnimState::UpdateInterpolators");

	// First, figure out their current max speed based on their current activity.
	float flCurMaxSpeed = GetCurrentMaxGroundSpeed();

#ifdef CLIENT_DLL
	float flGroundSpeedInterval = 0.1;

	// Only update this 10x/sec so it has an interval to interpolate over.
	if (gpGlobals->curtime - m_flLastGroundSpeedUpdateTime >= flGroundSpeedInterval)
	{
		m_flLastGroundSpeedUpdateTime = gpGlobals->curtime;
		m_flMaxGroundSpeed = flCurMaxSpeed;
		m_iv_flMaxGroundSpeed.NoteChanged(gpGlobals->curtime, flGroundSpeedInterval, false);
	}

	m_iv_flMaxGroundSpeed.Interpolate(gpGlobals->curtime, flGroundSpeedInterval);
#else
	m_flMaxGroundSpeed = flCurMaxSpeed;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ResetGroundSpeed(void)
{
#ifdef CLIENT_DLL
	m_flMaxGroundSpeed = GetCurrentMaxGroundSpeed();
	m_iv_flMaxGroundSpeed.Reset();
	m_iv_flMaxGroundSpeed.NoteChanged(gpGlobals->curtime, 0, false);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::DoAnimationEvent(PlayerAnimEvent_t event, int nData, float flData)
{
	Activity iGestureActivity = ACT_INVALID;

	switch (event)
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
	{
		if (m_pHL2MPPlayer->GetFlags() & FL_DUCKING)
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE);
		else
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE);

		iGestureActivity = ACT_VM_PRIMARYATTACK;
		break;
	}

	case PLAYERANIMEVENT_ATTACK_SECONDARY:
	{
		// Weapon secondary fire.
		if (m_pHL2MPPlayer->GetFlags() & FL_DUCKING)
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE);
		else
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE);

		iGestureActivity = ACT_VM_PRIMARYATTACK;
		break;
	}

	case PLAYERANIMEVENT_BASH:
	{
		// Weapon bashing.
		if (GetBasePlayer()->GetFlags() & FL_DUCKING)
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_BASH);
		else
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_BASH);
		break;
	}

	case PLAYERANIMEVENT_INFECTED:
	{
		RestartGesture(GESTURE_SLOT_FLINCH, ACT_MP_INFECTED);
		break;
	}

	case PLAYERANIMEVENT_KICK:
	{
		RestartGesture(GESTURE_SLOT_FLINCH, ACT_MP_KICK);

#ifdef CLIENT_DLL
		BB2PlayerGlobals->BodyRestartGesture(GetBasePlayer(), ACT_MP_KICK, 0);
#endif
		break;
	}

	case PLAYERANIMEVENT_SLIDE:
	{
		/*
		RestartGesture(GESTURE_SLOT_SWIM, ACT_MP_SLIDE);
		#ifdef CLIENT_DLL
		BB2PlayerGlobals->BodyRestartGesture(GetBasePlayer(), ACT_MP_SLIDE, 0);
		#endif
		m_flSlideGestureTime = gpGlobals->curtime + 0.39f;
		*/

		m_bSliding = m_bFirstSlideFrame = true;
		m_flSlideGestureTime = gpGlobals->curtime;
		RestartMainSequence();
		break;
	}

	case PLAYERANIMEVENT_RELOAD:
	{
		// Weapon reload.
		if (GetBasePlayer()->GetFlags() & FL_DUCKING)
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH);
		else
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND);
		break;
	}

	case PLAYERANIMEVENT_RELOAD_LOOP:
	{
		// Weapon reload.
		if (GetBasePlayer()->GetFlags() & FL_DUCKING)
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP);
		else
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP);
		break;
	}

	case PLAYERANIMEVENT_RELOAD_END:
	{
		// Weapon reload.
		if (GetBasePlayer()->GetFlags() & FL_DUCKING)
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END);
		else
			RestartGesture(GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END);
		break;
	}

	case PLAYERANIMEVENT_ATTACK_GRENADE:
	{
		// Grenade throw.
		RestartGesture(GESTURE_SLOT_GRENADE, ACT_MP_ATTACK_STAND_GRENADE);
		break;
	}

	case PLAYERANIMEVENT_JUMP:
	{
		// Jump.
		m_bJumping = true;
		m_bFirstJumpFrame = true;
		m_flJumpStartTime = gpGlobals->curtime;

		RestartMainSequence();

		break;
	}

	case PLAYERANIMEVENT_DIE:
	{
		// Should be here - not supporting this yet!
		Assert(0);

		// Start playing the death animation
		m_bDying = true;

		RestartMainSequence();
		break;
	}

	case PLAYERANIMEVENT_SPAWN:
	{
		// Player has respawned. Clear flags.
		ClearAnimationState();
		break;
	}

	case PLAYERANIMEVENT_SNAP_YAW:
		m_PoseParameterData.m_flLastAimTurnTime = 0.0f;
		break;

	case PLAYERANIMEVENT_CUSTOM:
	{
		Activity iIdealActivity = TranslateActivity((Activity)nData);
		m_nSpecificMainSequence = GetBaseAnimatable()->SelectWeightedSequence(iIdealActivity);
		RestartMainSequence();
		break;
	}

	case PLAYERANIMEVENT_CUSTOM_GESTURE:
		RestartGesture(GESTURE_SLOT_CUSTOM, (Activity)nData);
		break;

	case PLAYERANIMEVENT_CUSTOM_SEQUENCE:
		m_nSpecificMainSequence = nData;
		RestartMainSequence();
		break;

	case PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE:
		break;

	case PLAYERANIMEVENT_FLINCH_CHEST:
		PlayFlinchGesture(ACT_MP_GESTURE_FLINCH_CHEST);
		break;
	case PLAYERANIMEVENT_FLINCH_HEAD:
		PlayFlinchGesture(ACT_MP_GESTURE_FLINCH_HEAD);
		break;
	case PLAYERANIMEVENT_FLINCH_LEFTARM:
		PlayFlinchGesture(ACT_MP_GESTURE_FLINCH_LEFTARM);
		break;
	case PLAYERANIMEVENT_FLINCH_RIGHTARM:
		PlayFlinchGesture(ACT_MP_GESTURE_FLINCH_RIGHTARM);
		break;
	case PLAYERANIMEVENT_FLINCH_LEFTLEG:
		PlayFlinchGesture(ACT_MP_GESTURE_FLINCH_LEFTLEG);
		break;
	case PLAYERANIMEVENT_FLINCH_RIGHTLEG:
		PlayFlinchGesture(ACT_MP_GESTURE_FLINCH_RIGHTLEG);
		break;

	default:
		break;
	}

	m_aGestureSlots[GESTURE_SLOT_ATTACK_AND_RELOAD].m_pAnimLayer->m_flPlaybackRate = flData;

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if (iGestureActivity != ACT_INVALID)
	{
		CBaseCombatWeapon *pWeapon = GetBasePlayer()->GetActiveWeapon();
		if (pWeapon)
		{
			//pWeapon->SendWeaponAnim( iGestureActivity );
			pWeapon->DoAnimationEvents(pWeapon->GetModelPtr());
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: New Model, init the pose parameters
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::OnNewModel(void)
{
	m_bPoseParameterInit = false;
	m_PoseParameterData.Init();
	ClearAnimationState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleSwimming(Activity &idealActivity)
{
	return false; // Bernt: Add swim anims later on?
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleMoving(Activity &idealActivity)
{
	float flSpeed = GetOuterXYSpeed();
	if (flSpeed > MOVING_MINIMUM_SPEED)
	{
		CHL2MP_Player *pPlayer = GetBasePlayer();
		if (pPlayer)
		{
			float flPlayerSpeed = pPlayer->GetPlayerSpeed();
			if (flSpeed > ((flPlayerSpeed / 2.0f) + 10.0f))
				idealActivity = ACT_MP_RUN;
			else
				idealActivity = ACT_MP_WALK;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleDucking(Activity &idealActivity)
{
	if (m_pHL2MPPlayer->GetFlags() & FL_DUCKING)
	{
		if (GetOuterXYSpeed() < MOVING_MINIMUM_SPEED)
		{
			idealActivity = ACT_MP_CROUCH_IDLE;
		}
		else
		{
			idealActivity = ACT_MP_CROUCHWALK;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleJumping(Activity &idealActivity)
{
	if (m_bJumping)
	{
		static bool bNewJump = false; //Tony; hl2mp players only have a 'hop'

		if (m_bFirstJumpFrame)
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if (m_pHL2MPPlayer->GetWaterLevel() >= WL_Waist)
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if (gpGlobals->curtime - m_flJumpStartTime > 0.2f)
		{
			if (m_pHL2MPPlayer->GetFlags() & FL_ONGROUND)
			{
				m_bJumping = false;
				RestartMainSequence();

				if (bNewJump)
				{
					RestartGesture(GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND);
				}
			}
		}

		// if we're still jumping
		if (m_bJumping)
		{
			if (bNewJump)
			{
				if (gpGlobals->curtime - m_flJumpStartTime > 0.5)
				{
					idealActivity = ACT_MP_JUMP_FLOAT;
				}
				else
				{
					idealActivity = ACT_MP_JUMP_START;
				}
			}
			else
			{
				idealActivity = ACT_MP_JUMP;
			}
		}
	}

	if (m_bJumping)
		return true;

	return false;
}

bool CHL2MPPlayerAnimState::HandleSliding(Activity &idealActivity)
{
	if (m_bSliding)
	{
		if (m_bFirstSlideFrame)
		{
			m_bFirstSlideFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		if ((gpGlobals->curtime - m_flSlideGestureTime > 0.2f) && (GetBasePlayer() && !GetBasePlayer()->IsSliding()))
		{
			m_bSliding = false;
			RestartMainSequence();
		}

		if (m_bSliding)
			idealActivity = ACT_MP_SLIDE_IDLE;
	}

	if (m_bSliding)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleDying(Activity &idealActivity)
{
	if (m_bDying)
	{
		if (m_bFirstDyingFrame)
		{
			// Reset the animation.
			RestartMainSequence();
			m_bFirstDyingFrame = false;
		}

		idealActivity = ACT_DIESIMPLE;
		return true;
	}
	else
	{
		if (!m_bFirstDyingFrame)
		{
			m_bFirstDyingFrame = true;
		}
	}

	return false;
}

bool CHL2MPPlayerAnimState::SetupPoseParameters(CStudioHdr *pStudioHdr)
{
	// Check to see if this has already been done.
	if (m_bPoseParameterInit)
		return true;

	// Save off the pose parameter indices.
	if (!pStudioHdr)
		return false;

	// Tony; just set them both to the same for now.
	m_PoseParameterData.m_iMoveX = GetBaseAnimatable()->LookupPoseParameter(pStudioHdr, "move_yaw");
	m_PoseParameterData.m_iMoveY = GetBaseAnimatable()->LookupPoseParameter(pStudioHdr, "move_yaw");
	if ((m_PoseParameterData.m_iMoveX < 0) || (m_PoseParameterData.m_iMoveY < 0))
		return false;

	// Look for the aim pitch blender.
	m_PoseParameterData.m_iAimPitch = GetBaseAnimatable()->LookupPoseParameter(pStudioHdr, "aim_pitch");
	if (m_PoseParameterData.m_iAimPitch < 0)
		return false;

	// Look for aim yaw blender.
	m_PoseParameterData.m_iAimYaw = GetBaseAnimatable()->LookupPoseParameter(pStudioHdr, "aim_yaw");
	if (m_PoseParameterData.m_iAimYaw < 0)
		return false;

	m_bPoseParameterInit = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::EstimateYaw(void)
{
	if (m_vecCachedVelocity.y == 0 && m_vecCachedVelocity.x == 0)
		return;

	m_PoseParameterData.m_flEstimateYaw = (atan2(m_vecCachedVelocity.y, m_vecCachedVelocity.x) * 180 / M_PI);
	m_PoseParameterData.m_flEstimateYaw = clamp(m_PoseParameterData.m_flEstimateYaw, -180.0f, 180.0f);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flGoalYaw - 
//			flYawRate - 
//			flDeltaTime - 
//			&flCurrentYaw - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ConvergeYawAngles(float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw)
{
#define FADE_TURN_DEGREES 60.0f

	// Find the yaw delta.
	float flDeltaYaw = flGoalYaw - flCurrentYaw;
	float flDeltaYawAbs = fabs(flDeltaYaw);
	flDeltaYaw = AngleNormalize(flDeltaYaw);

	// Always do at least a bit of the turn (1%).
	float flScale = 1.0f;
	flScale = flDeltaYawAbs / FADE_TURN_DEGREES;
	flScale = clamp(flScale, 0.01f, 1.0f);

	float flYaw = flYawRate * flDeltaTime * flScale;
	if (flDeltaYawAbs < flYaw)
	{
		flCurrentYaw = flGoalYaw;
	}
	else
	{
		float flSide = (flDeltaYaw < 0.0f) ? -1.0f : 1.0f;
		flCurrentYaw += (flYaw * flSide);
	}

	flCurrentYaw = AngleNormalize(flCurrentYaw);

#undef FADE_TURN_DEGREES
}

//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputePoseParam_MoveYaw(CStudioHdr *pStudioHdr)
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// view direction relative to movement
	float flYaw;
	float ang = m_flEyeYaw;
	if (ang > 180.0f)
	{
		ang -= 360.0f;
	}
	else if (ang < -180.0f)
	{
		ang += 360.0f;
	}

	// calc side to side turning
	flYaw = ang - m_PoseParameterData.m_flEstimateYaw;
	// Invert for mapping into 8way blend
	flYaw = -flYaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;

	if (flYaw < -180)
	{
		flYaw = flYaw + 360;
	}
	else if (flYaw > 180)
	{
		flYaw = flYaw - 360;
	}

	//Tony; oops, i inverted this previously above.
	GetBaseAnimatable()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, flYaw);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputePoseParam_AimPitch(CStudioHdr *pStudioHdr)
{
	// Get the view pitch.
	float flAimPitch = m_flEyePitch;

	// Set the aim pitch pose parameter and save.
	GetBaseAnimatable()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iAimPitch, flAimPitch);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputePoseParam_AimYaw(CStudioHdr *pStudioHdr)
{
	// Get the movement velocity.
	// Check to see if we are moving.
	bool bMoving = (m_vecCachedVelocity.Length() > 1.0f) ? true : false;

	// If we are moving or are prone and undeployed.
	if (bMoving)
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if (m_PoseParameterData.m_flLastAimTurnTime <= 0.0f)
		{
			m_flGoalFeetYaw = m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(m_flGoalFeetYaw - m_flEyeYaw);

			if (fabs(flYawDelta) > 45.0f)
			{
				float flSide = (flYawDelta > 0.0f) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += (45.0f * flSide);
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize(m_flGoalFeetYaw);
	if (m_flGoalFeetYaw != m_flCurrentFeetYaw)
	{
		ConvergeYawAngles(m_flGoalFeetYaw, 720.0f, gpGlobals->frametime, m_flCurrentFeetYaw);
		m_flLastAimTurnTime = gpGlobals->curtime;
	}

	// Rotate the body into position.
	m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.
	GetBaseAnimatable()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw );

#ifndef CLIENT_DLL
	QAngle angle = GetBasePlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetBasePlayer()->SetAbsAngles(angle);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Override the default, because hl2mp models don't use moveX
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CHL2MPPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	CStudioHdr *pStudioHdr = GetBaseAnimatable()->GetModelPtr();

	if (pStudioHdr == NULL)
		return 1.0f;

	//	float prevX = GetBaseAnimatable()->GetPoseParameter( m_PoseParameterData.m_iMoveX );
	float prevY = GetBaseAnimatable()->GetPoseParameter(m_PoseParameterData.m_iMoveY);

	float d = sqrt( /*prevX * prevX + */prevY * prevY);
	float newY;//, newX;
	if (d == 0.0)
	{
		//		newX = 1.0;
		newY = 0.0;
	}
	else
	{
		//		newX = prevX / d;
		newY = prevY / d;
	}

	//	GetBaseAnimatable()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, newX );
	GetBaseAnimatable()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, newY);

	float speed = GetBaseAnimatable()->GetSequenceGroundSpeed(GetBaseAnimatable()->GetSequence());

	//	GetBaseAnimatable()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, prevX );
	GetBaseAnimatable()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, prevY);

	return speed;
}

Activity CHL2MPPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	if (HandleJumping(idealActivity) ||
		HandleDucking(idealActivity) ||
		HandleSwimming(idealActivity) ||
		HandleSliding(idealActivity) ||
		HandleDying(idealActivity))
	{
		// intentionally blank
	}
	else
	{
		HandleMoving(idealActivity);
	}

	return idealActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CHL2MPPlayerAnimState::TranslateActivity(Activity actDesired)
{
	Activity translateActivity = actDesired;

	if (GetBasePlayer()->GetActiveWeapon())
		translateActivity = GetBasePlayer()->GetActiveWeapon()->ActivityOverride(translateActivity);

	// No act change? Must be bugged! Prevent t-posing... Can happen during wep selection when the client thinks no wep is available during the switching!!
	if ((GetBasePlayer()->GetActiveWeapon() == NULL) || (translateActivity == actDesired))
	{
		switch (actDesired)
		{
		case ACT_MP_SLIDE:
			return ACT_HL2MP_SLIDE;

		case ACT_MP_SLIDE_IDLE:
			return ACT_HL2MP_SLIDE_IDLE;

		case ACT_MP_WALK:
			return ((GetBasePlayer()->GetTeamNumber() == TEAM_DECEASED) ? ACT_HL2MP_WALK : ACT_HL2MP_WALK_MELEE);

		case ACT_MP_CROUCHWALK:
			return ((GetBasePlayer()->GetTeamNumber() == TEAM_DECEASED) ? ACT_HL2MP_WALK_CROUCH : ACT_HL2MP_WALK_CROUCH_MELEE);

		case ACT_MP_RUN:
			return ((GetBasePlayer()->GetTeamNumber() == TEAM_DECEASED) ? ACT_HL2MP_RUN : ACT_HL2MP_RUN_MELEE);

		case ACT_MP_JUMP:
			return ((GetBasePlayer()->GetTeamNumber() == TEAM_DECEASED) ? ACT_HL2MP_JUMP : ACT_HL2MP_JUMP_MELEE);
		}

		// Last Resort:
		if (GetBasePlayer()->GetTeamNumber() == TEAM_DECEASED)
			return ((GetBasePlayer()->GetFlags() & FL_DUCKING) ? ACT_HL2MP_IDLE_CROUCH : ACT_HL2MP_IDLE);
		else
			return ((GetBasePlayer()->GetFlags() & FL_DUCKING) ? ACT_HL2MP_IDLE_CROUCH_MELEE : ACT_HL2MP_IDLE_MELEE);
	}

	return translateActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::PlayFlinchGesture(Activity iActivity)
{
	if (!IsGestureSlotActive(GESTURE_SLOT_FLINCH))
	{
		// See if we have the custom flinch. If not, revert to chest
		if (iActivity != ACT_MP_GESTURE_FLINCH_CHEST && GetBaseAnimatable()->SelectWeightedSequence(iActivity) == -1)
		{
			RestartGesture(GESTURE_SLOT_FLINCH, ACT_MP_GESTURE_FLINCH_CHEST);
		}
		else
		{
			RestartGesture(GESTURE_SLOT_FLINCH, iActivity);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ResetGestureSlots(void)
{
	for (int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture)
		ResetGestureSlot(iGesture);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ResetGestureSlot(int iGestureSlot)
{
	// Sanity Check
	Assert(iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT);
	GestureSlot_t *pGestureSlot = &m_aGestureSlots[iGestureSlot];
	if (pGestureSlot)
	{
#ifdef CLIENT_DLL
		// briefly set to 1.0 so we catch the events, before we reset the slot
		pGestureSlot->m_pAnimLayer->m_flCycle = 1.0;
		RunGestureSlotAnimEventsToCompletion(pGestureSlot);
#endif
		pGestureSlot->m_iGestureSlot = GESTURE_SLOT_INVALID;
		pGestureSlot->m_iActivity = ACT_INVALID;
		pGestureSlot->m_bAutoKill = false;
		pGestureSlot->m_bActive = false;
		if (pGestureSlot->m_pAnimLayer)
		{
			pGestureSlot->m_pAnimLayer->SetOrder(CBaseAnimatingOverlay::MAX_OVERLAYS);
#ifdef CLIENT_DLL
			pGestureSlot->m_pAnimLayer->Reset();
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAnimationLayer* CHL2MPPlayerAnimState::GetGestureSlotLayer(int iGestureSlot)
{
	return m_aGestureSlots[iGestureSlot].m_pAnimLayer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::IsGestureSlotActive(int iGestureSlot)
{
	// Sanity Check
	Assert(iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT);
	return m_aGestureSlots[iGestureSlot].m_bActive;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::RunGestureSlotAnimEventsToCompletion(GestureSlot_t *pGesture)
{
	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();
	CBaseAnimatingOverlay *pAnimatable = GetBaseAnimatable();
	if (!pPlayer || !pAnimatable)
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pAnimatable->GetModelPtr();
	if (!pStudioHdr)
		return;

	// Do all the anim events between previous cycle and 1.0, inclusive
	mstudioseqdesc_t &seqdesc = pStudioHdr->pSeqdesc(pGesture->m_pAnimLayer->m_nSequence);
	if (seqdesc.numevents > 0)
	{
		mstudioevent_t *pevent = seqdesc.pEvent(0);

		for (int i = 0; i < (int)seqdesc.numevents; i++)
		{
			if (pevent[i].type & AE_TYPE_NEWEVENTSYSTEM)
			{
				if (!(pevent[i].type & AE_TYPE_CLIENT))
					continue;
			}
			else if (pevent[i].event < 5000) //Adrian - Support the old event system
				continue;

			if (pevent[i].cycle > pGesture->m_pAnimLayer->m_flPrevCycle &&
				pevent[i].cycle <= pGesture->m_pAnimLayer->m_flCycle)
			{
				pPlayer->FireEvent(pPlayer->GetAbsOrigin(), pPlayer->GetAbsAngles(), pevent[i].event, pevent[i].pszOptions());
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::InitGestureSlots(void)
{
	// Get the base player.
	CBaseAnimatingOverlay *pPlayer = GetBaseAnimatable();
	if (pPlayer == NULL)
		return false;

	// Set the number of animation overlays we will use.
	pPlayer->SetNumAnimOverlays(GESTURE_SLOT_COUNT);

	// Setup the number of gesture slots. 
	m_aGestureSlots.AddMultipleToTail(GESTURE_SLOT_COUNT);
	for (int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture)
	{
		m_aGestureSlots[iGesture].m_pAnimLayer = pPlayer->GetAnimOverlay(iGesture);
		if (!m_aGestureSlots[iGesture].m_pAnimLayer)
			return false;

		ResetGestureSlot(iGesture);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ShutdownGestureSlots(void)
{
	// Clean up the gesture slots.
	m_aGestureSlots.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::IsGestureSlotPlaying(int iGestureSlot, Activity iGestureActivity)
{
	// Sanity Check
	Assert(iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT);

	// Check to see if the slot is active.
	if (!IsGestureSlotActive(iGestureSlot))
		return false;

	return (m_aGestureSlots[iGestureSlot].m_iActivity == iGestureActivity);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::RestartGesture(int iGestureSlot, Activity iGestureActivity, bool bAutoKill)
{
	// Sanity Check
	Assert(iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT);

	if (!IsGestureSlotPlaying(iGestureSlot, iGestureActivity))
	{
#ifdef CLIENT_DLL
		if (IsGestureSlotActive(iGestureSlot))
		{
			GestureSlot_t *pGesture = &m_aGestureSlots[iGestureSlot];
			if (pGesture && pGesture->m_pAnimLayer)
			{
				pGesture->m_pAnimLayer->m_flCycle = 1.0; // run until the end
				RunGestureSlotAnimEventsToCompletion(&m_aGestureSlots[iGestureSlot]);
			}
		}
#endif

		Activity iIdealGestureActivity = TranslateActivity(iGestureActivity);
		AddToGestureSlot(iGestureSlot, iIdealGestureActivity, bAutoKill);
		return;
	}

	// Reset the cycle = restart the gesture.
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::AddToGestureSlot(int iGestureSlot, Activity iGestureActivity, bool bAutoKill)
{
	// Sanity Check
	Assert(iGestureSlot >= 0 && iGestureSlot < GESTURE_SLOT_COUNT);

	CBaseAnimatingOverlay *pPlayer = GetBaseAnimatable();
	if (!pPlayer)
		return;

	// Make sure we have a valid animation layer to fill out.
	if (!m_aGestureSlots[iGestureSlot].m_pAnimLayer)
		return;

	// Get the sequence.
	int iGestureSequence = pPlayer->SelectWeightedSequence(iGestureActivity);
	if (iGestureSequence <= 0)
		return;

#ifdef CLIENT_DLL 

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerAnimtime = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLayerFadeOuttime = 0.0f;

	pPlayer->m_flOverlayPrevEventCycle[iGestureSlot] = -1.0;

#else

	// Setup the gesture.
	m_aGestureSlots[iGestureSlot].m_iGestureSlot = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_iActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_bAutoKill = bAutoKill;
	m_aGestureSlots[iGestureSlot].m_bActive = true;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nOrder = iGestureSlot;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nPriority = 0;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPrevCycle = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flPlaybackRate = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nActivity = iGestureActivity;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_nSequence = iGestureSequence;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flWeight = 1.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendIn = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flBlendOut = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bSequenceFinished = false;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = 0.0f;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_flLastEventCheck = gpGlobals->curtime;
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_bLooping = false;//( ( GetSequenceFlags( GetModelPtr(), iGestureSequence ) & STUDIO_LOOPING ) != 0);
	if (bAutoKill)
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
	m_aGestureSlots[iGestureSlot].m_pAnimLayer->m_fFlags |= ANIM_LAYER_ACTIVE;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputeGestureSequence(CStudioHdr *pStudioHdr)
{
	// Update all active gesture layers.
	for (int iGesture = 0; iGesture < GESTURE_SLOT_COUNT; ++iGesture)
	{
		if (!m_aGestureSlots[iGesture].m_bActive)
			continue;

		UpdateGestureLayer(pStudioHdr, &m_aGestureSlots[iGesture]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::UpdateGestureLayer(CStudioHdr *pStudioHdr, GestureSlot_t *pGesture)
{
	// Sanity check.
	if (!pStudioHdr || !pGesture)
		return;

	CBaseAnimatingOverlay *pPlayer = GetBaseAnimatable();
	if (!pPlayer)
		return;

#ifdef CLIENT_DLL 

	// Get the current cycle.
	float flCycle = pGesture->m_pAnimLayer->m_flCycle;
	flCycle += pPlayer->GetSequenceCycleRate(pStudioHdr, pGesture->m_pAnimLayer->m_nSequence) * gpGlobals->frametime * GetGesturePlaybackRate() * pGesture->m_pAnimLayer->m_flPlaybackRate.GetRaw();

	pGesture->m_pAnimLayer->m_flPrevCycle = pGesture->m_pAnimLayer->m_flCycle;
	pGesture->m_pAnimLayer->m_flCycle = flCycle;

	if (flCycle > 1.0f)
	{
		RunGestureSlotAnimEventsToCompletion(pGesture);

		if (pGesture->m_bAutoKill)
		{
			ResetGestureSlot(pGesture->m_iGestureSlot);
			return;
		}
		else
		{
			pGesture->m_pAnimLayer->m_flCycle = 1.0f;
		}
	}

#else

	if (pGesture->m_iActivity != ACT_INVALID && pGesture->m_pAnimLayer->m_nActivity == ACT_INVALID)
	{
		ResetGestureSlot(pGesture->m_iGestureSlot);
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: Cancel the current gesture and restart the main sequence.
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::RestartMainSequence(void)
{
	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();
	if (pPlayer)
	{
		GetBaseAnimatable()->m_flAnimTime = gpGlobals->curtime;
		GetBaseAnimatable()->SetCycle(0);

#ifdef CLIENT_DLL
		BB2PlayerGlobals->BodyRestartMainSequence(pPlayer);
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CHL2MPPlayerAnimState::GetOuterXYSpeed()
{
	return m_vecCachedVelocity.Length2D();
}
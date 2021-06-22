//=========       Copyright © Reperio Studios 2021 @ Bernt Andreas Eide!       ============//
//
// Purpose: BB2 Player Animstate.
//
//========================================================================================//

#ifndef HL2MP_PLAYERANIMSTATE_H
#define HL2MP_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "basecombatweapon_shared.h"

#if defined( CLIENT_DLL )
class C_HL2MP_Player;
#define CHL2MP_Player C_HL2MP_Player
#else
class CHL2MP_Player;
#endif

enum PlayerAnimEvent_t
{
	PLAYERANIMEVENT_ATTACK_PRIMARY,
	PLAYERANIMEVENT_ATTACK_SECONDARY,
	PLAYERANIMEVENT_ATTACK_GRENADE,
	PLAYERANIMEVENT_RELOAD,
	PLAYERANIMEVENT_RELOAD_LOOP,
	PLAYERANIMEVENT_RELOAD_END,
	PLAYERANIMEVENT_JUMP,
	PLAYERANIMEVENT_SWIM,
	PLAYERANIMEVENT_DIE,
	PLAYERANIMEVENT_FLINCH_CHEST,
	PLAYERANIMEVENT_FLINCH_HEAD,
	PLAYERANIMEVENT_FLINCH_LEFTARM,
	PLAYERANIMEVENT_FLINCH_RIGHTARM,
	PLAYERANIMEVENT_FLINCH_LEFTLEG,
	PLAYERANIMEVENT_FLINCH_RIGHTLEG,
	PLAYERANIMEVENT_BASH,
	PLAYERANIMEVENT_INFECTED,
	PLAYERANIMEVENT_KICK,
	PLAYERANIMEVENT_SLIDE,

	// Cancel.
	PLAYERANIMEVENT_CANCEL,
	PLAYERANIMEVENT_SPAWN,

	// Snap to current yaw exactly
	PLAYERANIMEVENT_SNAP_YAW,

	PLAYERANIMEVENT_CUSTOM,				// Used to play specific activities
	PLAYERANIMEVENT_CUSTOM_GESTURE,
	PLAYERANIMEVENT_CUSTOM_SEQUENCE,	// Used to play specific sequences
	PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE,

	PLAYERANIMEVENT_COUNT
};

// Gesture Slots.
enum
{
	GESTURE_SLOT_ATTACK_AND_RELOAD,
	GESTURE_SLOT_GRENADE,
	GESTURE_SLOT_JUMP,
	GESTURE_SLOT_SWIM,
	GESTURE_SLOT_FLINCH,
	GESTURE_SLOT_VCD,
	GESTURE_SLOT_CUSTOM,

	GESTURE_SLOT_COUNT,
};

#define GESTURE_SLOT_INVALID	-1

struct GestureSlot_t
{
	int					m_iGestureSlot;
	Activity			m_iActivity;
	bool				m_bAutoKill;
	bool				m_bActive;
	CAnimationLayer		*m_pAnimLayer;
};

struct MultiPlayerPoseData_t
{
	int			m_iMoveX;
	int			m_iMoveY;
	int			m_iAimYaw;
	int			m_iAimPitch;
	int			m_iMoveYaw;

	float		m_flEstimateYaw;
	float		m_flLastAimTurnTime;

	void Init()
	{
		m_iMoveX = 0;
		m_iMoveY = 0;
		m_iAimYaw = 0;
		m_iAimPitch = 0;
		m_iMoveYaw = 0;
		m_flEstimateYaw = 0.0f;
		m_flLastAimTurnTime = 0.0f;
	}
};

class CHL2MPPlayerAnimState
{
public:
	DECLARE_CLASS_NOBASE(CHL2MPPlayerAnimState);

	CHL2MPPlayerAnimState(CHL2MP_Player *pPlayer);
	~CHL2MPPlayerAnimState();

	void Release(void);
	CHL2MP_Player *GetBasePlayer(void)							{ return m_pHL2MPPlayer; }
	CBaseAnimatingOverlay *GetBaseAnimatable(void);

	const QAngle &GetRenderAngles();

	bool ShouldUpdateAnimState();
	void Update(float eyeYaw, float eyePitch);
	void ComputeSequences(CStudioHdr *pStudioHdr);
	void ComputeMainSequence();
	void UpdateInterpolators();
	void ResetGroundSpeed(void);
	void ClearAnimationState();

	void	DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0, float flData = 1.0f);
	void	OnNewModel(void);

protected:	

	bool	HandleMoving(Activity &idealActivity);
	bool	HandleJumping(Activity &idealActivity);
	bool	HandleDucking(Activity &idealActivity);
	bool	HandleSwimming(Activity &idealActivity);
	bool    HandleSliding(Activity &idealActivity);
	bool	HandleDying(Activity &idealActivity);

	float	GetCurrentMaxGroundSpeed();

	Activity CalcMainActivity();
	Activity TranslateActivity(Activity actDesired);
	void PlayFlinchGesture(Activity iActivity);

	// Gestures.
	void	ResetGestureSlots(void);
	void	ResetGestureSlot(int iGestureSlot);
	CAnimationLayer* GetGestureSlotLayer(int iGestureSlot);
	bool	IsGestureSlotActive(int iGestureSlot);
#ifdef CLIENT_DLL
	void	RunGestureSlotAnimEventsToCompletion(GestureSlot_t *pGesture);
#endif

	CUtlVector<GestureSlot_t>		m_aGestureSlots;
	bool	InitGestureSlots(void);
	void	ShutdownGestureSlots(void);
	bool	IsGestureSlotPlaying(int iGestureSlot, Activity iGestureActivity);
	void	AddToGestureSlot(int iGestureSlot, Activity iGestureActivity, bool bAutoKill);
	void	RestartGesture(int iGestureSlot, Activity iGestureActivity, bool bAutoKill = true);
	void	ComputeGestureSequence(CStudioHdr *pStudioHdr);
	void	UpdateGestureLayer(CStudioHdr *pStudioHdr, GestureSlot_t *pGesture);
	float	GetGesturePlaybackRate(void) { return 1.0f; }

	int SelectWeightedSequence(Activity activity) { return GetBaseAnimatable()->SelectWeightedSequence(activity); }
	void RestartMainSequence();
	float GetOuterXYSpeed();

private:

	bool	SetupPoseParameters(CStudioHdr *pStudioHdr);
	void	EstimateYaw(void);
	void	ConvergeYawAngles(float flGoalYaw, float flYawRate, float flDeltaTime, float &flCurrentYaw);
	void	ComputePoseParam_MoveYaw(CStudioHdr *pStudioHdr);
	void	ComputePoseParam_AimPitch(CStudioHdr *pStudioHdr);
	void	ComputePoseParam_AimYaw(CStudioHdr *pStudioHdr);

	CHL2MP_Player   *m_pHL2MPPlayer;
	QAngle m_angRender;

	float m_flEyeYaw;
	float m_flEyePitch;
	float m_flGoalFeetYaw;
	float m_flCurrentFeetYaw;
	float m_flLastAimTurnTime;

	// Jumping.
	bool	m_bJumping;
	float	m_flJumpStartTime;
	bool	m_bFirstJumpFrame;

	// Sliding
	bool m_bSliding;
	bool m_bFirstSlideFrame;
	float m_flSlideGestureTime;

	// Swimming.
	bool	m_bInSwim;
	bool	m_bFirstSwimFrame;

	// Dying
	bool	m_bDying;
	bool	m_bFirstDyingFrame;

	// Last activity we've used on the lower body. Used to determine if animations should restart.
	Activity m_eCurrentMainSequenceActivity;

	// Specific full-body sequence to play
	int		m_nSpecificMainSequence;

	// Ground speed interpolators.
#ifdef CLIENT_DLL
	float m_flLastGroundSpeedUpdateTime;
	CInterpolatedVar<float> m_iv_flMaxGroundSpeed;
#endif
	float m_flMaxGroundSpeed;

	// Pose parameters.
	bool						m_bPoseParameterInit;
	MultiPlayerPoseData_t		m_PoseParameterData;

	float						m_flLastAnimationStateClearTime;

	Vector m_vecCachedVelocity;
};

CHL2MPPlayerAnimState *CreateHL2MPPlayerAnimState(CHL2MP_Player *pPlayer);

#endif // HL2MP_PLAYERANIMSTATE_H
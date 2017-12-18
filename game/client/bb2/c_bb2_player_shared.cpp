//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Data for BB2 Players.
//
//========================================================================================//

#include "cbase.h"
#include "c_bb2_player_shared.h"
#include "hl2mp_gamerules.h"
#include "c_ai_basenpc.h"
#include "hud_npc_health_bar.h"
#include "GameBase_Shared.h"
#include "GameBase_Client.h"
#include "view.h"
#include "cdll_util.h"

static ConVar bb2_legs_angle_shift("bb2_legs_angle_shift", "0", FCVAR_CHEAT, "Rotate the legs counter-clockwise or clockwise with its origin.");
static ConVar bb2_legs_angle_shift_crouch("bb2_legs_angle_shift_crouch", "4", FCVAR_CHEAT, "Rotate the legs counter-clockwise or clockwise with its origin.");

static ConVar bb2_legs_origin_shift_x("bb2_legs_origin_shift_x", "-20", FCVAR_CHEAT, "Shift the legs forward or backward.");
static ConVar bb2_legs_origin_shift_y("bb2_legs_origin_shift_y", "0", FCVAR_CHEAT, "Shift the legs to the right or left.");

static ConVar bb2_legs_origin_shift_x_crouch("bb2_legs_origin_shift_x_crouch", "-28", FCVAR_CHEAT, "Shift the legs forward or backward.");
static ConVar bb2_legs_origin_shift_y_crouch("bb2_legs_origin_shift_y_crouch", "-6", FCVAR_CHEAT, "Shift the legs to the right or left.");

static CHudNPCHealthBar *pHealthBarHUD = NULL;

void CBB2PlayerShared::Initialize()
{
	m_pPlayerHands = NULL;
	m_pPlayerBody = NULL;
	m_bHasCreated = m_bIsDrawingSniperScope = m_bShouldDrawVotePanel = false;
	m_iPlayerVoteNum = 0;

	m_vecBodyOffset.Init(bb2_legs_origin_shift_x.GetFloat(), 0, 0);
}

void CBB2PlayerShared::Shutdown()
{
	if (m_pPlayerHands)
	{
		m_pPlayerHands->Remove();
		m_pPlayerHands = NULL;
	}

	if (m_pPlayerBody)
	{
		m_pPlayerBody->Remove();
		m_pPlayerBody = NULL;
	}

	GameBaseShared()->GetGameInventory().Purge();
	g_GlowObjectManager.Shutdown();
	m_bHasCreated = m_bIsDrawingSniperScope = m_bShouldDrawVotePanel = false;
	m_iPlayerVoteNum = 0;
}

void CBB2PlayerShared::CreateEntities()
{
	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	if (m_bHasCreated)
		return;

	m_bHasCreated = true;

	pHealthBarHUD = GET_HUDELEMENT(CHudNPCHealthBar);

	m_pPlayerBody = new C_FirstpersonBody();
	m_pPlayerBody->InitializeAsClientEntity(GetPlayerBodyModel(pLocal), RENDER_GROUP_OPAQUE_ENTITY);
	m_pPlayerBody->SetModelName(GetPlayerBodyModel(pLocal));
	m_pPlayerBody->SetPlaybackRate(1.0f);
	m_pPlayerBody->SetNumAnimOverlays(3);

	for (int i = 0; i < m_pPlayerBody->GetNumAnimOverlays(); i++)
	{
		m_pPlayerBody->GetAnimOverlay(i)->Reset();
		m_pPlayerBody->GetAnimOverlay(i)->SetOrder(i);
	}

	m_pPlayerBody->AddEffects(EF_NOINTERP);
	m_pPlayerBody->AddEFlags(EFL_USE_PARTITION_WHEN_NOT_SOLID);
	m_pPlayerBody->UpdatePartitionListEntry();
	m_pPlayerBody->UpdateVisibility();
	m_pPlayerBody->DestroyShadow();

	m_pPlayerHands = new C_ViewModelAttachment();
	m_pPlayerHands->InitializeAsClientEntity(GetPlayerHandModel(pLocal), RENDER_GROUP_VIEW_MODEL_OPAQUE);
	m_pPlayerHands->SetModelName(GetPlayerHandModel(pLocal));

	// Misc
	m_flUpdateTime = 0.0f;
}

void CBB2PlayerShared::OnNewModel()
{
	C_HL2MP_Player *pOwner = NULL;

	if (m_pPlayerBody)
	{
		pOwner = ToHL2MPPlayer(m_pPlayerBody->GetOwnerEntity());
		if (pOwner)
		{
			m_pPlayerBody->SetModel(GetPlayerBodyModel(pOwner));
			m_pPlayerBody->m_nBody = pOwner->GetBody();
			m_pPlayerBody->m_nSkin = pOwner->GetSkin();
		}
	}

	if (m_pPlayerHands)
	{
		pOwner = ToHL2MPPlayer(m_pPlayerHands->GetOwner());
		if (pOwner)
		{
			m_pPlayerHands->SetWeaponModel(GetPlayerHandModel(pOwner), NULL);
			m_pPlayerHands->m_nSkin = pOwner->GetSkin();
		}
	}
}

void CBB2PlayerShared::UpdatePlayerBody(C_HL2MP_Player *pOwner)
{
	if (!m_pPlayerBody || !pOwner)
		return;

	if (m_pPlayerBody->GetOwnerEntity() != pOwner)
	{
		m_pPlayerBody->SetOwnerEntity(pOwner);
		OnNewModel();
		m_pPlayerBody->UpdateVisibility();
	}
}

void CBB2PlayerShared::UpdatePlayerHands(C_BaseViewModel *pParent, C_HL2MP_Player *pOwner)
{
	if (!m_pPlayerHands || !pOwner)
		return;

	bool bCanFollow = (pOwner->GetTeamNumber() >= TEAM_HUMANS);

	if (m_pPlayerHands->GetOwner() != pOwner)
	{
		if (m_pPlayerHands->IsFollowingEntity())
			m_pPlayerHands->StopFollowingEntity();

		m_pPlayerHands->SetOwnerEntity(pOwner);
		m_pPlayerHands->SetOwner(pOwner);
		if (bCanFollow)
			m_pPlayerHands->AttachmentFollow(pParent);
		OnNewModel();
		m_pPlayerHands->UpdateVisibility();
	}

	if ((m_pPlayerHands->GetMoveParent() != pParent) && bCanFollow)
		m_pPlayerHands->AttachmentFollow(pParent);

	m_pPlayerHands->StudioFrameAdvance();
}

const char *CBB2PlayerShared::GetPlayerHandModel(C_HL2MP_Player *pOwner)
{
	if (!pOwner)
		return "";

	const model_t *model = modelinfo->GetModel(pOwner->GetModelIndex());
	if (!model)
		return "";

	const char *pchModel = modelinfo->GetModelName(model);
	if (!pchModel || (strlen(pchModel) <= 0))
		return "";

	return (GameBaseShared()->GetSharedGameDetails()->GetPlayerHandModel(pchModel));
}

const char *CBB2PlayerShared::GetPlayerBodyModel(C_HL2MP_Player *pOwner)
{
	if (!pOwner)
		return "";

	const model_t *model = modelinfo->GetModel(pOwner->GetModelIndex());
	if (!model)
		return "";

	const char *pchModel = modelinfo->GetModelName(model);
	if (!pchModel || (strlen(pchModel) <= 0))
		return "";

	return (GameBaseShared()->GetSharedGameDetails()->GetPlayerBodyModel(pchModel));
}

bool CBB2PlayerShared::IsBodyOwner(C_BaseAnimating *pTarget)
{
	if (!pTarget || !m_pPlayerBody)
		return false;

	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return false;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer(m_pPlayerBody->GetOwnerEntity());
	if (!pOwner)
		return false;

	return (pOwner->entindex() == pTarget->entindex());
}

void CBB2PlayerShared::BodyUpdateVisibility(void)
{
	if (!m_pPlayerBody)
		return;

	m_pPlayerBody->UpdateVisibility();
}

void CBB2PlayerShared::BodyUpdate(C_HL2MP_Player *pOwner)
{
	if (!IsBodyOwner(pOwner))
		return;

	QAngle angle = pOwner->GetRenderAngles();

	const float flViewPitch = angle.x;

	angle.x = 0;
	angle.z = 0;

	Vector fwd, right, up;
	AngleVectors(angle, &fwd, &right, &up);

	const bool bDuck = (pOwner->m_Local.m_bDucked || pOwner->m_Local.m_bDucking || pOwner->IsSliding());
	const bool bInAir = ((pOwner->GetFlags() & FL_ONGROUND) == 0);

	if (bDuck)
		angle[YAW] += bb2_legs_angle_shift_crouch.GetFloat();
	else
		angle[YAW] += bb2_legs_angle_shift.GetFloat();

	float flBackOffsetSpeed = 35.0f;
	float flBackOffsetSpeedQuad = 5.0f;

	Vector vecOffsetStanding = Vector(bb2_legs_origin_shift_x.GetFloat(), bb2_legs_origin_shift_y.GetFloat(), 0);
	Vector vecOffsetCrouching = Vector(bb2_legs_origin_shift_x_crouch.GetFloat(), bb2_legs_origin_shift_y_crouch.GetFloat(), 0);
	Vector vecOffsetDesired = bDuck ? vecOffsetCrouching : vecOffsetStanding;

	Vector vecOffsetDelta = vecOffsetDesired - m_vecBodyOffset;
	if (vecOffsetDelta.LengthSqr() > 2.0f)
	{
		float len = vecOffsetDelta.NormalizeInPlace();
		float deltaLen = MAX(gpGlobals->frametime * flBackOffsetSpeed,
			len * gpGlobals->frametime * flBackOffsetSpeedQuad);
		deltaLen = MIN(len, deltaLen);

		vecOffsetDelta *= deltaLen;

		m_vecBodyOffset += vecOffsetDelta;
	}
	else
	{
		m_vecBodyOffset = vecOffsetDesired;
	}

	// Adjust z position when ducking, duck-jumping... etc...
	Vector origin = pOwner->GetRenderOrigin() + fwd * m_vecBodyOffset.x
		+ right * m_vecBodyOffset.y
		+ up * m_vecBodyOffset.z;

	if (!bDuck || (pOwner->m_Local.m_bInDuckJump && bInAir))
	{
		origin.z += pOwner->GetViewOffset().z - VEC_VIEW.z;
	}
	else if (bDuck && flViewPitch < -20.0f) // Hide body when ducking and looking up
	{
		origin.z -= 50.0f;
	}

	m_pPlayerBody->m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;
	m_pPlayerBody->SetAbsOrigin(origin);
	m_pPlayerBody->SetAbsAngles(angle);

	m_pPlayerBody->StudioFrameAdvance();
}

void CBB2PlayerShared::BodyResetSequence(C_BaseAnimating *pOwner, int sequence)
{
	if (!IsBodyOwner(pOwner))
		return;

	m_pPlayerBody->ResetSequence(sequence);
}

void CBB2PlayerShared::BodyRestartMainSequence(C_BaseAnimating *pOwner)
{
	if (!IsBodyOwner(pOwner))
		return;

	m_pPlayerBody->m_flAnimTime = gpGlobals->curtime;
	m_pPlayerBody->SetCycle(0);
}

void CBB2PlayerShared::BodyRestartGesture(C_HL2MP_Player *pOwner, Activity activity, int slot)
{
	if (!IsBodyOwner(pOwner))
		return;

	C_BaseCombatWeapon *pWeapon = pOwner->GetActiveWeapon();
	if (!pWeapon)
		return;

	if ((slot < 0) || (slot >= m_pPlayerBody->GetNumAnimOverlays()))
		return;

	int iSequence = m_pPlayerBody->SelectWeightedSequence(pWeapon->ActivityOverride(activity, NULL));
	if (iSequence >= 0)
	{
		C_AnimationLayer *pLayer = m_pPlayerBody->GetAnimOverlay(slot);
		if (!pLayer)
			return;

		pLayer->m_nSequence = iSequence;
		pLayer->m_flWeight = 1.0f;
		pLayer->m_flCycle = 0.0f;
		pLayer->m_flPrevCycle = 0.0f;
		pLayer->m_flPlaybackRate = 1.0f;
		pLayer->m_nOrder = slot;
		pLayer->m_flLayerAnimtime = 0.0f;
		pLayer->m_flLayerFadeOuttime = 0.0f;

		m_pPlayerBody->m_flOverlayPrevEventCycle[slot] = -1.0;
	}
}

void CBB2PlayerShared::CreateNPCHealthbars(void)
{
	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	if (m_flUpdateTime < gpGlobals->curtime)
	{
		m_flUpdateTime = gpGlobals->curtime + 0.4f;

		for (C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity))
		{
			// Populate our health bar item list.
			if (pEntity->IsNPC() && !pEntity->IsDormant())
			{
				C_AI_BaseNPC *pNPC = pEntity->MyNPCPointer();
				if (pNPC)
				{
					if (strlen(pNPC->GetNPCName()) > 0)
					{
						if (pHealthBarHUD)
							pHealthBarHUD->AddHealthBarItem(pNPC, pNPC->entindex(), pNPC->IsBoss(), pNPC->m_iHealth, pNPC->m_iMaxHealth, pNPC->GetNPCName());
					}
				}
			}
		}
	}
}

void CBB2PlayerShared::OnUpdate()
{
	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	C_HL2MP_Player *pOwner = pLocal;
	C_BaseViewModel *pViewModel = pOwner->GetViewModel();

	int specTargetIndex = GetSpectatorTarget();
	if (specTargetIndex > 0 && (pOwner->GetObserverMode() == OBS_MODE_IN_EYE))
	{
		pOwner = ToHL2MPPlayer(UTIL_PlayerByIndex(specTargetIndex));
		if (pOwner)
			pViewModel = pOwner->GetViewModel();
	}

	UpdatePlayerHands(pViewModel, pOwner);
	UpdatePlayerBody(pOwner);
	CreateNPCHealthbars();
}

static CBB2PlayerShared g_BB2PlayerShared;
CBB2PlayerShared *BB2PlayerGlobals = &g_BB2PlayerShared;
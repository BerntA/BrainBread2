//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: First Person Body
//
//========================================================================================//

#include "cbase.h"
#include "c_firstperson_body.h"
#include "cl_animevent.h"
#include "bone_setup.h"
#include "jigglebones.h"
#include "view.h"
#include "iinput.h"
#include "viewrender.h"
#include "c_hl2mp_player.h"
#include "model_types.h"
#include "GlobalRenderEffects.h"
#include "GameBase_Shared.h"

static ConVar bb2_render_body("bb2_render_body", "1", FCVAR_ARCHIVE, "Render firstperson body.");
extern ConVar bb2_body_blood;

C_FirstpersonBody::C_FirstpersonBody()
{
	m_bPreferModelPointerOverIndex = true;
}

int C_FirstpersonBody::DrawModel(int flags)
{
	if (!bb2_render_body.GetBool())
		return 0;

	C_HL2MP_Player *pLocalPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocalPlayer || (pLocalPlayer->GetTeamNumber() <= TEAM_UNASSIGNED) || input->CAM_IsThirdPerson() || g_bShouldRenderLocalPlayerExternally)
		return 0;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer(GetOwnerEntity());
	if (!pOwner)
		return 0;

	if ((pOwner == pLocalPlayer) && IsInOtherView())
		return 0;

	if ((pLocalPlayer->GetObserverMode() != OBS_MODE_IN_EYE) && (pLocalPlayer->GetTeamNumber() <= TEAM_SPECTATOR))
		return 0;

	int observerTarget = GetSpectatorTarget();
	if ((pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE) && ((observerTarget <= 0) || (observerTarget != pOwner->entindex()) || (observerTarget == pLocalPlayer->entindex())))
		return 0;

	if ((pLocalPlayer->GetTeamNumber() >= TEAM_HUMANS) && !pLocalPlayer->IsAlive())
		return 0;

	if ((pOwner != pLocalPlayer) && (pOwner->IsObserver() || !pOwner->IsAlive() || pOwner->IsPlayerDead() || (pOwner->GetTeamNumber() <= TEAM_SPECTATOR)))
		return 0;

	int ret = BaseClass::DrawModel(flags);

	if (pOwner->IsMaterialOverlayFlagActive(MAT_OVERLAY_BLOOD) && bb2_body_blood.GetBool())
	{
		modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetBloodOverlay());
		BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
		modelrender->ForcedMaterialOverride(0);
	}

	return ret;
}

void C_FirstpersonBody::GetRenderBounds(Vector &mins, Vector &maxs)
{
	mins.Init(-32, -32, 0.0f);
	maxs.Init(32, 32, 96.0f);
}

void C_FirstpersonBody::StudioFrameAdvance()
{
	BaseClass::StudioFrameAdvance();

	for (int i = 0; i < GetNumAnimOverlays(); i++)
	{
		C_AnimationLayer *pLayer = GetAnimOverlay(i);

		if (pLayer->m_nSequence < 0)
			continue;

		float rate = GetSequenceCycleRate(GetModelPtr(), pLayer->m_nSequence);

		pLayer->m_flCycle += pLayer->m_flPlaybackRate * rate * gpGlobals->frametime;

		if (pLayer->m_flCycle > 1.0f)
		{
			pLayer->m_nSequence = -1;
			pLayer->m_flWeight = 0.0f;
		}
	}
}
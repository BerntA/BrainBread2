//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom viewmodel for the hands. L4D styled system, separate hands.
//
//========================================================================================//

#include "cbase.h"
#include "c_viewmodel_attachment.h"
#include "c_hl2mp_player.h"

C_ViewModelAttachment::C_ViewModelAttachment()
{
}

void C_ViewModelAttachment::Spawn()
{
	BaseClass::Spawn();
}

bool C_ViewModelAttachment::ShouldDraw()
{
	return true;
}

int C_ViewModelAttachment::DrawModel(int flags)
{
	if (g_bShouldRenderLocalPlayerExternally)
		return 0;

	C_HL2MP_Player *pLocalPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocalPlayer)
		return 0;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer(GetOwner());
	if (!pOwner)
		return 0;

	if ((pLocalPlayer->GetObserverMode() != OBS_MODE_IN_EYE) && (pLocalPlayer->GetTeamNumber() <= TEAM_SPECTATOR))
		return 0;

	if ((pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE) && (GetSpectatorTarget() <= 0))
		return 0;

	if ((pLocalPlayer->GetTeamNumber() >= TEAM_HUMANS) && !pLocalPlayer->IsAlive())
		return 0;

	if (pLocalPlayer->GetTeamNumber() <= TEAM_UNASSIGNED)
		return 0;

	if ((pOwner != pLocalPlayer) && (pOwner->IsObserver() || !pOwner->IsAlive() || pOwner->IsPlayerDead() || (pOwner->GetTeamNumber() <= TEAM_SPECTATOR)))
		return 0;

	return BaseClass::DrawModel(flags);
}
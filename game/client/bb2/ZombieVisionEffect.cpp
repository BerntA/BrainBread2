//=========       Copyright © Reperio Studios 2024 @ Bernt Andreas Eide!       ============//
//
// Purpose: Zombie Vision Screen Space Effect.
//
//========================================================================================//

#include "cbase.h"
#include "ScreenSpaceEffects.h"
#include "GlobalRenderEffects.h"
#include "c_hl2mp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CZombieVisionEffect : public IScreenSpaceEffect
{
public:
	CZombieVisionEffect(void) { }

	void Init(void);
	void Shutdown(void);
	void SetParameters(KeyValues* params);
	void Enable(bool bEnable);
	bool IsEnabled() { return true; }
	void Render(int x, int y, int w, int h);

private:
	CMaterialReference m_ZombieVisionMaterial;
};

ADD_SCREENSPACE_EFFECT(CZombieVisionEffect, zombievision);

void CZombieVisionEffect::Init(void)
{
	m_ZombieVisionMaterial.Init(materials->FindMaterial("effects/zombie_vision", TEXTURE_GROUP_CLIENT_EFFECTS));
}

void CZombieVisionEffect::Shutdown(void)
{
	m_ZombieVisionMaterial.Shutdown();
}

void CZombieVisionEffect::SetParameters(KeyValues* params)
{
}

void CZombieVisionEffect::Enable(bool bEnable)
{
}

void CZombieVisionEffect::Render(int x, int y, int w, int h)
{
	if (!IsEnabled())
		return;

	C_HL2MP_Player* pLocalPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if ((pLocalPlayer == NULL) || (pLocalPlayer->GetTeamNumber() != TEAM_DECEASED) || !pLocalPlayer->IsZombieVisionOn())
		return;

	RenderMaterialOverlay(m_ZombieVisionMaterial, x, y, w, h);
}
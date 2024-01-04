//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Handle/Store Shared Effect Data
//
//========================================================================================//

#include "cbase.h"
#include "GlobalRenderEffects.h"
#include "c_hl2mp_player.h"
#include "view.h"
#include "view_scene.h"
#include "viewrender.h"
#include "c_playerresource.h"

ConVar bb2_human_indication_icon_size("bb2_human_indication_icon_size", "16", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "How big should the human indication icons be? These are only drawn in objective mode when you're playing as a zombie.", true, 8.0f, true, 32.0f);
ConVar bb2_human_indication_icon_min_dist("bb2_human_indication_icon_min_dist", "400", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "If you're further than this distance from the icon it will start growing in size proportionally with the distance increase in % from this dist.", true, 100.0f, true, 16000.0f);

void CGlobalRenderEffects::Initialize()
{
	if (m_bInitialized)
		return;

	m_bInitialized = true;

	m_MatBloodOverlay = materials->FindMaterial("effects/blood_overlay_1", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatBloodOverlay->IncrementReferenceCount();

	m_MatPerkOverlay = materials->FindMaterial("effects/com_shield003a", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatPerkOverlay->IncrementReferenceCount();

	m_MatSpawnProtectionOverlay = materials->FindMaterial("effects/spawnprotection_overlay", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatSpawnProtectionOverlay->IncrementReferenceCount();

	m_MatHumanIndicatorIcon = materials->FindMaterial("effects/bb_human_indicator", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatHumanIndicatorIcon->IncrementReferenceCount();

	m_MatCloakOverlay = materials->FindMaterial("effects/cloak_overlay", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatCloakOverlay->IncrementReferenceCount();

	m_MatBleedOverlay = materials->FindMaterial("effects/bleed_overlay", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatBleedOverlay->IncrementReferenceCount();

	m_MatBurnOverlay = materials->FindMaterial("effects/burn_overlay", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatBurnOverlay->IncrementReferenceCount();

	m_MatIceOverlay = materials->FindMaterial("effects/frozen_overlay", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatIceOverlay->IncrementReferenceCount();

	m_MatDizzyIcon = materials->FindMaterial("effects/dizzy", TEXTURE_GROUP_CLIENT_EFFECTS);
	m_MatDizzyIcon->IncrementReferenceCount();
}

void CGlobalRenderEffects::Shutdown()
{
	if (!m_bInitialized)
		return;

	m_bInitialized = false;

	m_MatBloodOverlay->DecrementReferenceCount();
	m_MatBloodOverlay = NULL;

	m_MatPerkOverlay->DecrementReferenceCount();
	m_MatPerkOverlay = NULL;

	m_MatSpawnProtectionOverlay->DecrementReferenceCount();
	m_MatSpawnProtectionOverlay = NULL;

	m_MatHumanIndicatorIcon->DecrementReferenceCount();
	m_MatHumanIndicatorIcon = NULL;

	m_MatCloakOverlay->DecrementReferenceCount();
	m_MatCloakOverlay = NULL;

	m_MatBleedOverlay->DecrementReferenceCount();
	m_MatBleedOverlay = NULL;

	m_MatBurnOverlay->DecrementReferenceCount();
	m_MatBurnOverlay = NULL;

	m_MatIceOverlay->DecrementReferenceCount();
	m_MatIceOverlay = NULL;

	m_MatDizzyIcon->DecrementReferenceCount();
	m_MatDizzyIcon = NULL;
}

bool CGlobalRenderEffects::CanDrawOverlay(C_BaseEntity *pTarget)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer || !pTarget)
		return false;

	if (pPlayer->entindex() == pTarget->entindex())
		return true;

	if (pPlayer->IsInVGuiInputMode() || (pPlayer->GetLocalOrigin().DistTo(pTarget->GetLocalOrigin()) > MAX_TEAMMATE_DISTANCE))
		return false;

	return true;
}

static CGlobalRenderEffects g_GlobalRenderFX;
CGlobalRenderEffects *GlobalRenderEffects = &g_GlobalRenderFX;

void DrawHumanIndicators(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!g_PR || !HL2MPRules() || !pPlayer || (pPlayer->GetTeamNumber() != TEAM_DECEASED) || !pPlayer->IsAlive())
		return;

	IMaterial *renderTexture = GlobalRenderEffects->GetHumanIndicationIcon();
	bool bCanShow = ((HL2MPRules()->GetCurrentGamemode() == MODE_OBJECTIVE) || ((HL2MPRules()->GetCurrentGamemode() == MODE_ELIMINATION) && pPlayer->IsZombieVisionOn()));
	if (!bCanShow || !renderTexture)
		return;

	CMatRenderContextPtr pRenderContext(materials);
	const Vector &vecLocalPlayerPos = pPlayer->GetLocalOrigin();

	// Align it so it never points up or down.
	Vector vUp(0, 0, 1);
	Vector vRight = CurrentViewRight();
	if (fabs(vRight.z) > 0.95)	// don't draw it edge-on
		return;

	vRight.z = 0.0f;
	VectorNormalize(vRight);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if ((pPlayer->entindex() == i) || !g_PR->IsConnected(i) || !g_PR->IsAlive(i) || (g_PR->GetTeam(i) != TEAM_HUMANS))
			continue;

		Vector vecPosition = g_PR->GetPosition(i);
		C_BasePlayer *pOther = UTIL_PlayerByIndex(i);
		if (pOther && !pOther->IsDormant())
			vecPosition = pOther->GetLocalOrigin();

		vecPosition.z += (36.0f + (bb2_human_indication_icon_size.GetFloat() / 2.0f));

		// Increase the size of the icon as we walk away from it, but only after X units away from it..
		float iconSize = bb2_human_indication_icon_size.GetFloat();
		float dist = (vecLocalPlayerPos - vecPosition).Length(), minDist = bb2_human_indication_icon_min_dist.GetFloat();
		if (dist > minDist)
		{
			dist -= minDist;
			iconSize += (iconSize * (dist / minDist));
		}

		pRenderContext->Bind(renderTexture);
		IMesh *pMesh = pRenderContext->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

		meshBuilder.Color3f(1.0, 1.0, 1.0);
		meshBuilder.TexCoord2f(0, 0, 0);
		meshBuilder.Position3fv((vecPosition + (vRight * -iconSize) + (vUp * iconSize)).Base());
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f(1.0, 1.0, 1.0);
		meshBuilder.TexCoord2f(0, 1, 0);
		meshBuilder.Position3fv((vecPosition + (vRight * iconSize) + (vUp * iconSize)).Base());
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f(1.0, 1.0, 1.0);
		meshBuilder.TexCoord2f(0, 1, 1);
		meshBuilder.Position3fv((vecPosition + (vRight * iconSize) + (vUp * -iconSize)).Base());
		meshBuilder.AdvanceVertex();

		meshBuilder.Color3f(1.0, 1.0, 1.0);
		meshBuilder.TexCoord2f(0, 0, 1);
		meshBuilder.Position3fv((vecPosition + (vRight * -iconSize) + (vUp * -iconSize)).Base());
		meshBuilder.AdvanceVertex();
		meshBuilder.End();

		pMesh->Draw();
	}
}

void DrawDizzyIcon(const Vector &vecOrigin)
{
	IMaterial *renderTexture = GlobalRenderEffects->GetDizzyIcon();
	if (!renderTexture)
		return;

	CMatRenderContextPtr pRenderContext(materials);
	const Vector &vOrigin = vecOrigin;

	// Align it so it never points up or down.
	Vector vUp(0, 0, 1);
	Vector vRight = CurrentViewRight();
	if (fabs(vRight.z) > 0.95)	// don't draw it edge-on
		return;

	vRight.z = 0.0f;
	VectorNormalize(vRight);

	float flSize = 8.0f;

	pRenderContext->Bind(renderTexture);
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

	meshBuilder.Color3f(1.0, 1.0, 1.0);
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.Position3fv((vOrigin + (vRight * -flSize) + (vUp * flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3f(1.0, 1.0, 1.0);
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.Position3fv((vOrigin + (vRight * flSize) + (vUp * flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3f(1.0, 1.0, 1.0);
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.Position3fv((vOrigin + (vRight * flSize) + (vUp * -flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color3f(1.0, 1.0, 1.0);
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.Position3fv((vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base());
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
}

void RenderMaterialOverlay(IMaterial* texture, int x, int y, int w, int h)
{
	if (texture == NULL)
		return;

	if (texture->NeedsFullFrameBufferTexture())
		DrawScreenEffectMaterial(texture, x, y, w, h);
	else if (texture->NeedsPowerOfTwoFrameBufferTexture())
	{
		UpdateRefractTexture(x, y, w, h, true);

		// Now draw the entire screen using the material...
		CMatRenderContextPtr pRenderContext(materials);
		ITexture* pTexture = GetPowerOfTwoFrameBufferTexture();
		int sw = pTexture->GetActualWidth();
		int sh = pTexture->GetActualHeight();
		// Note - don't offset by x,y - already done by the viewport.
		pRenderContext->DrawScreenSpaceRectangle(
			texture,
			0, 0, w, h,
			0, 0, sw - 1, sh - 1, sw, sh
		);
	}
	else
	{
		byte color[4] = { 255, 255, 255, 255 };
		render->ViewDrawFade(color, texture);
	}
}
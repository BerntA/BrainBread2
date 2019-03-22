//=========       Copyright © Reperio Studios 2014 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective Icon Sprite - Used to mark objective areas to guide the players.
//
//========================================================================================//

#include "cbase.h"
#include "c_objective_icon.h"
#include "view.h"
#include "baseentity_shared.h"

static CUtlVector<C_ObjectiveIcon*> m_pObjectiveIcons;

IMPLEMENT_CLIENTCLASS_DT(C_ObjectiveIcon, DT_ObjectiveIcon, CObjectiveIcon)
RecvPropBool(RECVINFO(m_bShouldBeHidden)),
RecvPropInt(RECVINFO(m_iTeamNumber)),
RecvPropString(RECVINFO(m_szTextureFile)),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS(objective_icon, C_ObjectiveIcon);

ConVar bb2_objective_icon_size("bb2_objective_icon_size", "16", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "How big should the objective icons be?", true, 16.0f, true, 32.0f);

C_ObjectiveIcon::C_ObjectiveIcon()
{
	pMaterialLink = NULL;
	m_pObjectiveIcons.AddToTail(this);
}

C_ObjectiveIcon::~C_ObjectiveIcon()
{
	if (pMaterialLink)
	{
		pMaterialLink->DecrementReferenceCount();
		pMaterialLink = NULL;
	}

	m_pObjectiveIcons.FindAndRemove(this);
}

void C_ObjectiveIcon::Spawn(void)
{
	BaseClass::Spawn();
}

bool C_ObjectiveIcon::ShouldDraw()
{
	return true;
}

void C_ObjectiveIcon::PostDataUpdate(DataUpdateType_t updateType)
{
	BaseClass::PostDataUpdate(updateType);

	if ((pMaterialLink == NULL) && m_szTextureFile && m_szTextureFile[0])
	{
		pMaterialLink = materials->FindMaterial(m_szTextureFile, TEXTURE_GROUP_OTHER);
		pMaterialLink->IncrementReferenceCount();
	}
}

void RenderObjectiveIcons(void)
{
	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pPlayer)
		return;

	if (pPlayer->IsInVGuiInputMode())
		return;

	CMatRenderContextPtr pRenderContext(materials);
	int iTeam = pPlayer->GetTeamNumber();
	int iNumObjIcons = m_pObjectiveIcons.Count();
	float flSize = bb2_objective_icon_size.GetFloat();

	for (int i = 0; i < iNumObjIcons; i++)
	{
		C_ObjectiveIcon *pIcon = m_pObjectiveIcons[i];
		if (!pIcon)
			continue;

		IMaterial *renderTexture = pIcon->GetIconMaterial();
		int iconTeamLink = pIcon->GetTeamLink();
		if ((iconTeamLink != iTeam && (iconTeamLink > 0)) || pIcon->IsHidden() || (renderTexture == NULL))
			continue;

		Vector vOrigin = pIcon->WorldSpaceCenter();

		// Align it so it never points up or down.
		Vector vUp(0, 0, 1);
		Vector vRight = CurrentViewRight();
		if (fabs(vRight.z) > 0.95)	// don't draw it edge-on
			continue;

		vRight.z = 0;
		VectorNormalize(vRight);

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
}
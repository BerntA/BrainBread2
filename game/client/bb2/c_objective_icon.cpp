//=========       Copyright © Reperio Studios 2014 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective Icon Sprite - Used to mark objective areas to guide the players.
//
//========================================================================================//

#include "cbase.h"
#include "baseplayer_shared.h"
#include "c_objective_icon.h"
#include "c_baseanimating.h"
#include "c_baseentity.h"
#include "c_hl2mp_player.h"
#include "c_baseplayer.h"
#include "iclientmode.h"
#include "c_basehlplayer.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui/ISurface.h"
#include "hud_macros.h"
#include "vgui_controls/Panel.h"
#include "usermessages.h"
#include "vgui/ISurface.h"
#include <vgui/ILocalize.h>
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "view.h"

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
}

void C_ObjectiveIcon::Spawn(void)
{
	BaseClass::Spawn();
}

C_ObjectiveIcon::~C_ObjectiveIcon()
{
	if (pMaterialLink)
	{
		pMaterialLink->DecrementReferenceCount();
		pMaterialLink = NULL;
	}
}

bool C_ObjectiveIcon::ShouldDraw()
{
	return true;
}

void C_ObjectiveIcon::PostDataUpdate(DataUpdateType_t updateType)
{
	BaseClass::PostDataUpdate(updateType);

	if ((pMaterialLink == NULL) && (strlen(m_szTextureFile) > 0))
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

	CMatRenderContextPtr pRenderContext(materials);
	int iTeam = pPlayer->GetTeamNumber();

	for (C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity))
	{
		C_ObjectiveIcon *pIcon = dynamic_cast<C_ObjectiveIcon*> (pEntity);
		if (pIcon)
		{
			int iconTeamLink = pIcon->GetTeamLink();
			if ((iconTeamLink != iTeam && (iconTeamLink > 0)) || pIcon->IsHidden() || (strlen(pIcon->GetTexture()) <= 0))
				continue;

			IMaterial *renderTexture = pIcon->GetIconMaterial();
			if (renderTexture != NULL)
			{
				Vector vOrigin = pIcon->WorldSpaceCenter();

				// Align it so it never points up or down.
				Vector vUp(0, 0, 1);
				Vector vRight = CurrentViewRight();
				if (fabs(vRight.z) > 0.95)	// don't draw it edge-on
					continue;

				vRight.z = 0;
				VectorNormalize(vRight);

				float flSize = bb2_objective_icon_size.GetFloat();

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
	}
}
//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Enables support for client-based playermodels like in Sven Co-Op | HL1 goldsrc...
//
//========================================================================================//

#include "cbase.h"
#include "c_playermodel.h"
#include "bone_setup.h"
#include "GameBase_Client.h"
#include "GameBase_Shared.h"
#include "c_te_effect_dispatch.h"
#include "datacache/imdlcache.h"
#include "iinput.h"
#include "c_playerresource.h"
#include "model_types.h"
#include "GlobalRenderEffects.h"
#include "c_bb2_player_shared.h"
#include "view.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CUtlVector<C_Playermodel*> s_ClientPlayermodelList;

bool RemoveAllClientPlayermodels()
{
	if (!s_ClientPlayermodelList.Count())
		return false;

	while (s_ClientPlayermodelList.Count() > 0)
	{
		C_Playermodel *p = s_ClientPlayermodelList[0];
		p->Release();
	}

	return true;
}

C_Playermodel *CreateClientPlayermodel(C_HL2MP_Player *pParent)
{
	if (pParent == NULL)
		return NULL;

	C_Playermodel *pClientPlayermdl = new C_Playermodel();
	pClientPlayermdl->SetPlayerOwner(pParent);

	if (!pClientPlayermdl->Initialize())
	{
		pClientPlayermdl->Release();
		return NULL;
	}

	pClientPlayermdl->SetModelPointer(GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(-1)->m_pClientModelPtrHuman); // TODO ?
	return pClientPlayermdl;
}

C_Playermodel::C_Playermodel(void)
{
	m_pPlayer = NULL;
	m_bNoModelParticles = false;
	ForceClientSideAnimationOn();
	s_ClientPlayermodelList.AddToTail(this);
}

C_Playermodel::~C_Playermodel()
{
	m_pPlayer = NULL;
	s_ClientPlayermodelList.FindAndRemove(this);
}

bool C_Playermodel::Initialize(void)
{
	if (InitializeAsClientEntity(STRING(GetModelName()), RENDER_GROUP_OPAQUE_ENTITY) == false)
		return false;

	if (engine->IsInEditMode())
		return false;

	m_takedamage = DAMAGE_EVENTS_ONLY;
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);

	AddEffects(EF_NOINTERP);
	AddEFlags(EFL_USE_PARTITION_WHEN_NOT_SOLID);
	UpdatePartitionListEntry();
	CollisionProp()->UpdatePartition();
	SetBlocksLOS(false);
	UpdateVisibility();
	return true;
}

void C_Playermodel::Release(void)
{
	if (GetThinkHandle() != INVALID_THINK_HANDLE)
		ClientThinkList()->RemoveThinkable(GetClientHandle());

	ClientEntityList().RemoveEntity(GetClientHandle());

	partition->Remove(PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, CollisionProp()->GetPartitionHandle());
	RemoveFromLeafSystem();

	BaseClass::Release();
}

bool C_Playermodel::IsDormant(void)
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner)
		return pOwner->IsDormant();

	return true;
}

int C_Playermodel::DrawModel(int flags)
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if ((pOwner == NULL) || !pOwner->m_bReadyToDraw ||
		!((bb2_render_client_in_mirrors.GetBool() && pOwner->IsLocalPlayer() && g_bShouldRenderLocalPlayerExternally) || pOwner->ShouldDrawThisPlayer()))
		return 0;

	QAngle saveAngles = pOwner->GetLocalAngles(); // Remove pitch+ROLL, use poseparam instead.. HACK
	QAngle useAngles = saveAngles;
	useAngles[PITCH] = 0.0f;
	useAngles[ROLL] = 0.0f;
	pOwner->SetLocalAngles(useAngles);

	bool bShouldDrawOverrides = (!(flags & STUDIO_SKIP_MATERIAL_OVERRIDES));

	if (pOwner->IsPerkFlagActive(PERK_POWERUP_PREDATOR) && bShouldDrawOverrides)
	{
		modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetCloakOverlay());
		int retVal = BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
		modelrender->ForcedMaterialOverride(0);
		return retVal;
	}

	int retVal = BaseClass::DrawModel(flags);

	if (bShouldDrawOverrides)
	{
		if (pOwner->IsMaterialOverlayFlagActive(MAT_OVERLAY_SPAWNPROTECTION))
		{
			modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetSpawnProtectionOverlay());
			BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
		}
		else if (GlobalRenderEffects->CanDrawOverlay(this))
		{
			if (pOwner->IsMaterialOverlayFlagActive(MAT_OVERLAY_BLOOD))
			{
				modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetBloodOverlay());
				BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
				modelrender->ForcedMaterialOverride(0);
			}

			if (pOwner->GetPerkFlags())
			{
				modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetPerkOverlay());
				BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
				modelrender->ForcedMaterialOverride(0);
			}

			if (pOwner->IsMaterialOverlayFlagActive(MAT_OVERLAY_BURNING))
			{
				modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetBurnOverlay());
				BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
				modelrender->ForcedMaterialOverride(0);
			}
		}
	}

	pOwner->SetLocalAngles(saveAngles);
	return retVal;
}

ShadowType_t C_Playermodel::ShadowCastType()
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner == NULL)
		return SHADOWS_NONE;

	if (!pOwner->IsVisible() || pOwner->IsPerkFlagActive(PERK_POWERUP_PREDATOR) || (pOwner->IsLocalPlayer() && !pOwner->ShouldDrawThisPlayer()))
		return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

bool C_Playermodel::ShouldDraw()
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner)
		return pOwner->ShouldDraw();

	return false;
}

bool C_Playermodel::ShouldReceiveProjectedTextures(int flags)
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner == NULL)
		return false;

	Assert(flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK);

	if (pOwner->IsEffectActive(EF_NODRAW) || pOwner->IsPerkFlagActive(PERK_POWERUP_PREDATOR))
		return false;

	if (flags & SHADOW_FLAGS_FLASHLIGHT)
		return true;

	return false;
}

void C_Playermodel::OnDormantStateChange(void)
{
	UpdateVisibility();
}

void C_Playermodel::OnUpdate(void)
{
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner == NULL)
		return;

	QAngle angles = pOwner->GetLocalAngles();
	angles[PITCH] = 0;
	angles[ROLL] = 0;

	SetAbsOrigin(pOwner->GetLocalOrigin());
	SetAbsAngles(angles);

	if ((GetModel() != NULL) && (GetShadowHandle() == CLIENTSHADOW_INVALID_HANDLE))
		CreateShadow();
}

CStudioHdr *C_Playermodel::OnNewModel(void)
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	if (hdr)
	{
		for (int i = 0; i < hdr->GetNumPoseParameters(); i++)
			SetPoseParameter(hdr, i, 0.0);
	}

	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner && (pOwner->m_PlayerAnimState != NULL))
		pOwner->m_PlayerAnimState->OnNewModel();

	BB2PlayerGlobals->OnNewModel();

	DestroyShadow();
	return hdr;
}
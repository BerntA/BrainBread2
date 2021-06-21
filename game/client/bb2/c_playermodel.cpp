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

	pClientPlayermdl->SetModel(DEFAULT_PLAYER_MODEL(pParent->GetTeamNumber()));
	return pClientPlayermdl;
}

C_Playermodel::C_Playermodel(void)
{
	m_pPlayer = NULL;
	m_pParticleHelmet = NULL;
	m_bNoModelParticles = false;
	m_bPreferModelPointerOverIndex = true;
	m_takedamage = DAMAGE_EVENTS_ONLY;
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
	if (InitializeAsClientEntity(NULL, RENDER_GROUP_OPAQUE_ENTITY) == false)
		return false;

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
	if (m_pParticleHelmet)
		::ParticleMgr()->RemoveEffect(m_pParticleHelmet);
	m_pParticleHelmet = NULL;

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
	{
		DeleteHelmet();
		return 0;
	}

	bool bShouldDrawOverrides = (!(flags & STUDIO_SKIP_MATERIAL_OVERRIDES));

	if (pOwner->IsPerkFlagActive(PERK_POWERUP_PREDATOR) && bShouldDrawOverrides)
	{
		DeleteHelmet();
		modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetCloakOverlay());
		int retVal = BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
		modelrender->ForcedMaterialOverride(0);
		return retVal;
	}

	int retVal = BaseClass::DrawModel(flags);

	if (bShouldDrawOverrides)
	{
		DrawHelmet(true);

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
	DrawHelmet();
}

void C_Playermodel::OnUpdate(void)
{
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner == NULL)
		return;

	QAngle angles = pOwner->EyeAngles();
	angles[PITCH] = 0;
	angles[ROLL] = 0;

	SetAbsOrigin(pOwner->GetLocalOrigin());
	SetAbsAngles(angles);

	if (GetModel() != NULL)
		CreateShadow();

	DrawHelmet();
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
	DeleteHelmet();

	return hdr;
}

void C_Playermodel::UpdateModel(void)
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (pOwner == NULL || GameBaseShared() == NULL || GameBaseShared()->GetSharedGameDetails() == NULL)
		return;

	const DataPlayerItem_Survivor_Shared_t *data = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(pOwner->m_szModelChoice, true);
	if (data == NULL)
	{
		SetModel(DEFAULT_PLAYER_MODEL(pOwner->GetTeamNumber()));
		m_nSkin = m_nBody = 0;
		return;
	}
	else
		SetModelPointer((pOwner->GetTeamNumber() == TEAM_DECEASED) ? data->m_pClientModelPtrZombie : data->m_pClientModelPtrHuman);

	m_nSkin = pOwner->m_nSkin;
	m_nBody = 0;

	for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
	{
		int accessoryGroup = FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[i]);
		if (accessoryGroup == -1)
			continue;

		SetBodygroup(accessoryGroup, pOwner->m_iCustomizationChoices[i]);
	}
}

void C_Playermodel::DrawHelmet(bool bRender)
{
	C_HL2MP_Player *pOwner = GetPlayerOwner();
	if (!pOwner || !g_PR || !HL2MPRules() || !pOwner->IsAlive() || pOwner->IsDormant() || pOwner->IsObserver() || (pOwner->GetTeamNumber() != TEAM_HUMANS) || pOwner->IsPerkFlagActive(PERK_POWERUP_PREDATOR) ||
		(g_PR->GetLevel(pOwner->entindex()) < MAX_PLAYER_LEVEL) || HL2MPRules()->IsFastPacedGameplay())
	{
		DeleteHelmet();
		return;
	}

	if (!m_pParticleHelmet && bRender) // Create if otherwise OK.
	{
		int iAttachment = LookupAttachment("gore_head");
		if (iAttachment <= 0) // Invalid ...
			return;

		m_pParticleHelmet = ParticleProp()->Create("helm_halo01", PATTACH_POINT_FOLLOW, iAttachment);
	}
}

void C_Playermodel::DeleteHelmet(void)
{
	if (!m_pParticleHelmet)
		return;

	m_pParticleHelmet->StopEmission(false, false, true);
	m_pParticleHelmet = NULL;
}
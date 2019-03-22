//=========       Copyright © Reperio Studios 2013-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: NPC BaseClass.
//
//==============================================================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "engine/ivdebugoverlay.h"
#include "GameBase_Shared.h"
#include "model_types.h"
#include "GlobalRenderEffects.h"
#include "death_pose.h"
#include "hud_npc_health_bar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_AI_BaseNPC, DT_AI_BaseNPC, CAI_BaseNPC )
	RecvPropInt(RECVINFO(m_lifeState)),
	RecvPropBool(RECVINFO(m_bPerformAvoidance)),
	RecvPropBool(RECVINFO(m_bIsMoving)),
	RecvPropInt(RECVINFO(m_iHealth)),
	RecvPropInt(RECVINFO(m_iMaxHealth)),
	RecvPropString(RECVINFO(m_szNPCName)),
	RecvPropBool(RECVINFO(m_bIsBoss)),
END_RECV_TABLE()

C_AI_BaseNPC::C_AI_BaseNPC()
{
	m_bCreatedHealthBar = false;
}

C_AI_BaseNPC::~C_AI_BaseNPC()
{
	if (m_bCreatedHealthBar && GetHealthBarHUD())
		GetHealthBarHUD()->RemoveHealthBarItem(entindex());
}

//-----------------------------------------------------------------------------
// Makes ragdolls ignore npcclip brushes
//-----------------------------------------------------------------------------
unsigned int C_AI_BaseNPC::PhysicsSolidMaskForEntity( void ) const 
{
	// This allows ragdolls to move through npcclip brushes
	if ( !IsRagdoll() )	
		return MASK_NPCSOLID; 

	return MASK_SOLID;
}

void C_AI_BaseNPC::ClientThink( void )
{
	BaseClass::ClientThink();
}

void C_AI_BaseNPC::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ((m_bCreatedHealthBar == false) && GetHealthBarHUD() && (type == DATA_UPDATE_CREATED))
	{
		m_bCreatedHealthBar = true;
		RegisterHUDHealthBar();
	}
}

bool C_AI_BaseNPC::GetRagdollInitBoneArrays( matrix3x4_t *pDeltaBones0, matrix3x4_t *pDeltaBones1, matrix3x4_t *pCurrentBones, float boneDt )
{
	bool bRet = true;
	if ( !ForceSetupBonesAtTime( pDeltaBones0, gpGlobals->curtime - boneDt ) )
		bRet = false;

	GetRagdollCurSequenceWithDeathPose(this, pDeltaBones1, gpGlobals->curtime, ACT_INVALID, DEATH_FRAME_STOMACH); // HL1 death anims are old!!
	float ragdollCreateTime = PhysGetSyncCreateTime();
	if ( ragdollCreateTime != gpGlobals->curtime )
	{
		// The next simulation frame begins before the end of this frame
		// so initialize the ragdoll at that time so that it will reach the current
		// position at curtime.  Otherwise the ragdoll will simulate forward from curtime
		// and pop into the future a bit at this point of transition
		if (!ForceSetupBonesAtTime(pCurrentBones, ragdollCreateTime))
			bRet = false;
	}
	else
	{
		if ( !SetupBones( pCurrentBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, gpGlobals->curtime ) )
			bRet = false;
	}

	return bRet;
}

int C_AI_BaseNPC::DrawModel(int flags)
{
	int retVal = BaseClass::DrawModel(flags);

	if (!(flags & STUDIO_SKIP_MATERIAL_OVERRIDES) && GlobalRenderEffects->CanDrawOverlay(this))
	{
		IMaterial *overlay = NULL;
		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_BURNING))
			overlay = GlobalRenderEffects->GetBurnOverlay();
		else if (IsMaterialOverlayFlagActive(MAT_OVERLAY_COLDSNAP))
			overlay = GlobalRenderEffects->GetFrozenOverlay();
		else if (IsMaterialOverlayFlagActive(MAT_OVERLAY_BLEEDING))
			overlay = GlobalRenderEffects->GetBleedOverlay();

		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_CRIPPLED))
		{			
			Vector maxs = WorldAlignMaxs();
			Vector vOrigin = GetLocalOrigin() + Vector(0, 0, maxs.z + 16.0f);
			DrawDizzyIcon(vOrigin);
		}

		if (overlay != NULL)
		{
			modelrender->ForcedMaterialOverride(overlay);
			BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
		}
	}

	return retVal;
}

void C_AI_BaseNPC::RegisterHUDHealthBar()
{
	if (!(m_szNPCName && m_szNPCName[0]))
		return;

	GetHealthBarHUD()->AddHealthBarItem(this, entindex(), IsBoss());
}
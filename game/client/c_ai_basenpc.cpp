//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

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
	RecvPropInt( RECVINFO( m_lifeState ) ),
	RecvPropBool( RECVINFO( m_bPerformAvoidance ) ),
	RecvPropBool( RECVINFO( m_bIsMoving ) ),
	RecvPropBool( RECVINFO( m_bFadeCorpse ) ),
	RecvPropInt( RECVINFO ( m_iDeathPose) ),
	RecvPropInt( RECVINFO( m_iDeathFrame) ),
	RecvPropBool( RECVINFO( m_bImportanRagdoll ) ),
	RecvPropInt(RECVINFO(m_iHealth)),
	RecvPropInt(RECVINFO(m_iMaxHealth)),
	RecvPropString(RECVINFO(m_szNPCName)),
	RecvPropBool(RECVINFO(m_bIsBoss)),
END_RECV_TABLE()

bool NPC_IsImportantNPC( C_BaseAnimating *pAnimating )
{
	C_AI_BaseNPC *pBaseNPC = dynamic_cast < C_AI_BaseNPC* > ( pAnimating );
	if ( pBaseNPC == NULL )
		return false;

	return pBaseNPC->ImportantRagdoll();
}

C_AI_BaseNPC::C_AI_BaseNPC()
{
	m_bCreatedHealthBar = false;
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

	GetRagdollCurSequenceWithDeathPose( this, pDeltaBones1, gpGlobals->curtime, m_iDeathPose, m_iDeathFrame );
	float ragdollCreateTime = PhysGetSyncCreateTime();
	if ( ragdollCreateTime != gpGlobals->curtime )
	{
		// The next simulation frame begins before the end of this frame
		// so initialize the ragdoll at that time so that it will reach the current
		// position at curtime.  Otherwise the ragdoll will simulate forward from curtime
		// and pop into the future a bit at this point of transition
		if ( !ForceSetupBonesAtTime( pCurrentBones, ragdollCreateTime ) )
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

	if (!(flags & STUDIO_SKIP_MATERIAL_OVERRIDES))
	{
		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_BURNING))
		{
			modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetBurnOverlay());
			BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
		}

		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_COLDSNAP))
		{
			modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetFrozenOverlay());
			BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
		}

		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_CRIPPLED))
		{
			Vector mins, maxs;
			GetRenderBounds(mins, maxs);
			Vector vOrigin = GetLocalOrigin() + Vector(0, 0, maxs.z + 16.0f);
			DrawDizzyIcon(vOrigin);
		}

		if (IsMaterialOverlayFlagActive(MAT_OVERLAY_BLEEDING))
		{
			modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetBleedOverlay());
			BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
			modelrender->ForcedMaterialOverride(0);
		}
	}

	return retVal;
}

void C_AI_BaseNPC::RegisterHUDHealthBar()
{
	if (strlen(GetNPCName()) <= 0)
		return;

	GetHealthBarHUD()->AddHealthBarItem(this, entindex(), IsBoss());
}
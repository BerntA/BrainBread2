//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render a glowing outline around client renderable objects.
//
//===============================================================================

#include "cbase.h"
#include "glow_outline_effect.h"
#include "model_types.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "viewpostprocess.h"
#include "c_hl2mp_player.h"

#define FULL_FRAME_TEXTURE "_rt_FullFrameFB"
#define DEFAULT_RENDER_FLAGS_FOR_GLOW_ENTS (STUDIO_RENDER | STUDIO_SKIP_MATERIAL_OVERRIDES)

#ifdef GLOWS_ENABLE

ConVar glow_outline_effect_enable("glow_outline_effect_enable", "1", FCVAR_ARCHIVE, "Enable entity outline glow effects.");
ConVar bb2_max_glow_effects("bb2_max_glow_effects", "5", FCVAR_ARCHIVE, "Maximum amount of glow render effects at once.", true, 1.0f, true, 20.0f);

extern bool g_bDumpRenderTargets; // in viewpostprocess.cpp

CGlowObjectManager g_GlowObjectManager;

struct ShaderStencilState_t
{
	bool m_bEnable;
	StencilOperation_t m_FailOp;
	StencilOperation_t m_ZFailOp;
	StencilOperation_t m_PassOp;
	StencilComparisonFunction_t m_CompareFunc;
	int m_nReferenceValue;
	uint32 m_nTestMask;
	uint32 m_nWriteMask;

	ShaderStencilState_t()
	{
		m_bEnable = false;
		m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
		m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		m_nReferenceValue = 0;
		m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
	}

	void SetStencilState(CMatRenderContextPtr &pRenderContext)
	{
		pRenderContext->SetStencilEnable(m_bEnable);
		pRenderContext->SetStencilFailOperation(m_FailOp);
		pRenderContext->SetStencilZFailOperation(m_ZFailOp);
		pRenderContext->SetStencilPassOperation(m_PassOp);
		pRenderContext->SetStencilCompareFunction(m_CompareFunc);
		pRenderContext->SetStencilReferenceValue(m_nReferenceValue);
		pRenderContext->SetStencilTestMask(m_nTestMask);
		pRenderContext->SetStencilWriteMask(m_nWriteMask);
	}
};

static CUtlVector<C_BaseEntity *> m_pGlowEntities;

void CGlowObjectManager::AddEntityToGlowList(C_BaseEntity *pEntity)
{
	if (m_pGlowEntities.Find(pEntity) != -1)
		return;

	m_pGlowEntities.AddToTail(pEntity);
}

void CGlowObjectManager::RemoveEntityFromGlowList(C_BaseEntity *pEntity)
{
	m_pGlowEntities.FindAndRemove(pEntity);
}

void CGlowObjectManager::Shutdown(void)
{
	m_pGlowEntities.RemoveAll();
}

void CGlowObjectManager::RenderGlowEffects(const CViewSetup *pSetup)
{
	if (g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0())
	{
		C_HL2MP_Player *pLocalPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();
		if (!pLocalPlayer)
			return;

		if (glow_outline_effect_enable.GetBool() && pLocalPlayer->CanDrawGlowEffects() && m_pGlowEntities.Count())
		{
			int numRadiusObjects = 0;
			for (int i = 0; i < m_pGlowEntities.Count(); i++)
			{
				m_pGlowEntities[i]->m_bGlowSuppressRender = false;
				if (!m_pGlowEntities[i]->CanGlowEntity())
					continue;

				if (m_pGlowEntities[i]->GetGlowMode() == GLOW_MODE_RADIUS)
				{
					if (numRadiusObjects >= bb2_max_glow_effects.GetInt())
					{
						m_pGlowEntities[i]->m_bGlowSuppressRender = true;
						continue;
					}

					numRadiusObjects++;
				}
			}

			CMatRenderContextPtr pRenderContext(materials);

			int nX, nY, nWidth, nHeight;
			pRenderContext->GetViewport(nX, nY, nWidth, nHeight);

			PIXEvent _pixEvent(pRenderContext, "EntityGlowEffects");
			ApplyEntityGlowEffects(pSetup, pRenderContext, 10.0f, nX, nY, nWidth, nHeight);
		}
	}
}

static void SetRenderTargetAndViewPort(ITexture *rt, int w, int h)
{
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->SetRenderTarget(rt);
	pRenderContext->Viewport(0, 0, w, h);
}

void CGlowObjectManager::RenderGlowModels(const CViewSetup *pSetup, CMatRenderContextPtr &pRenderContext)
{
	//==========================================================================================//
	// This renders solid pixels with the correct coloring for each object that needs the glow.	//
	// After this function returns, this image will then be blurred and added into the frame	//
	// buffer with the objects stenciled out.													//
	//==========================================================================================//
	pRenderContext->PushRenderTargetAndViewport();

	// Save modulation color and blend
	Vector vOrigColor;
	render->GetColorModulation(vOrigColor.Base());
	float flOrigBlend = render->GetBlend();

	// Get pointer to FullFrameFB
	ITexture *pRtFullFrame = NULL;
	pRtFullFrame = materials->FindTexture(FULL_FRAME_TEXTURE, TEXTURE_GROUP_RENDER_TARGET);

	SetRenderTargetAndViewPort(pRtFullFrame, pSetup->width, pSetup->height);

	pRenderContext->ClearColor3ub(0, 0, 0);
	pRenderContext->ClearBuffers(true, false, false);

	// Set override material for glow color
	IMaterial *pMatGlowColor = NULL;

	pMatGlowColor = materials->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, true);
	g_pStudioRender->ForcedMaterialOverride(pMatGlowColor);

	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = false;
	stencilState.m_nReferenceValue = 0;
	stencilState.m_nTestMask = 0xFF;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_KEEP;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_KEEP;

	stencilState.SetStencilState(pRenderContext);

	//==================//
	// Draw the objects //
	//==================//
	for (int i = 0; i < m_pGlowEntities.Count(); ++i)
	{
		if (!m_pGlowEntities[i]->CanGlowEntity())
			continue;

		float alpha = m_pGlowEntities[i]->GetGlowAlpha();
		render->SetBlend(alpha);
		Vector vGlowColor = m_pGlowEntities[i]->GetGlowColor() * alpha;
		render->SetColorModulation(&vGlowColor[0]); // This only sets rgb, not alpha
		m_pGlowEntities[i]->DrawModel(DEFAULT_RENDER_FLAGS_FOR_GLOW_ENTS);
	}

	if (g_bDumpRenderTargets)
	{
		DumpTGAofRenderTarget(pSetup->width, pSetup->height, "GlowModels");
	}

	g_pStudioRender->ForcedMaterialOverride(NULL);
	render->SetColorModulation(vOrigColor.Base());
	render->SetBlend(flOrigBlend);

	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	stencilStateDisable.SetStencilState(pRenderContext);

	pRenderContext->PopRenderTargetAndViewport();
}

void CGlowObjectManager::ApplyEntityGlowEffects(const CViewSetup *pSetup, CMatRenderContextPtr &pRenderContext, float flBloomScale, int x, int y, int w, int h)
{
	//=======================================================//
	// Render objects into stencil buffer					 //
	//=======================================================//
	// Set override shader to the same simple shader we use to render the glow models
	IMaterial *pMatGlowColor = materials->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, true);
	g_pStudioRender->ForcedMaterialOverride(pMatGlowColor);

	ShaderStencilState_t stencilStateDisable;
	stencilStateDisable.m_bEnable = false;
	float flSavedBlend = render->GetBlend();

	// Set alpha to 0 so we don't touch any color pixels
	render->SetBlend(0.0f);
	pRenderContext->OverrideDepthEnable(true, false);

	for (int i = 0; i < m_pGlowEntities.Count(); ++i)
	{
		if (!m_pGlowEntities[i]->CanGlowEntity())
			continue;

		if (m_pGlowEntities[i]->ShouldGlowWhenOccluded() || m_pGlowEntities[i]->ShouldGlowWhenUnoccluded())
		{
			if (m_pGlowEntities[i]->ShouldGlowWhenOccluded() && m_pGlowEntities[i]->ShouldGlowWhenUnoccluded())
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 1;
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
				stencilState.m_PassOp = STENCILOPERATION_REPLACE;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState(pRenderContext);

				m_pGlowEntities[i]->DrawModel(DEFAULT_RENDER_FLAGS_FOR_GLOW_ENTS);
			}
			else if (m_pGlowEntities[i]->ShouldGlowWhenOccluded())
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 1;
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
				stencilState.m_PassOp = STENCILOPERATION_KEEP;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState(pRenderContext);

				m_pGlowEntities[i]->DrawModel(DEFAULT_RENDER_FLAGS_FOR_GLOW_ENTS);
			}
			else if (m_pGlowEntities[i]->ShouldGlowWhenUnoccluded())
			{
				ShaderStencilState_t stencilState;
				stencilState.m_bEnable = true;
				stencilState.m_nReferenceValue = 2;
				stencilState.m_nTestMask = 0x1;
				stencilState.m_nWriteMask = 0x3;
				stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
				stencilState.m_PassOp = STENCILOPERATION_INCRSAT;
				stencilState.m_FailOp = STENCILOPERATION_KEEP;
				stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;

				stencilState.SetStencilState(pRenderContext);

				m_pGlowEntities[i]->DrawModel(DEFAULT_RENDER_FLAGS_FOR_GLOW_ENTS);
			}
		}
	}

	// Need to do a 2nd pass to warm stencil for objects which are rendered only when occluded
	for (int i = 0; i < m_pGlowEntities.Count(); ++i)
	{
		if (!m_pGlowEntities[i]->CanGlowEntity())
			continue;

		if (m_pGlowEntities[i]->ShouldGlowWhenOccluded() && !m_pGlowEntities[i]->ShouldGlowWhenUnoccluded())
		{
			ShaderStencilState_t stencilState;
			stencilState.m_bEnable = true;
			stencilState.m_nReferenceValue = 2;
			stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
			stencilState.m_PassOp = STENCILOPERATION_REPLACE;
			stencilState.m_FailOp = STENCILOPERATION_KEEP;
			stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
			stencilState.SetStencilState(pRenderContext);

			m_pGlowEntities[i]->DrawModel(DEFAULT_RENDER_FLAGS_FOR_GLOW_ENTS);
		}
	}

	pRenderContext->OverrideDepthEnable(false, false);
	render->SetBlend(flSavedBlend);
	stencilStateDisable.SetStencilState(pRenderContext);
	g_pStudioRender->ForcedMaterialOverride(NULL);

	//=============================================
	// Render the glow colors to _rt_FullFrameFB 
	//=============================================
	{
		PIXEvent pixEvent(pRenderContext, "RenderGlowModels");
		RenderGlowModels(pSetup, pRenderContext);
	}

	// Get viewport
	int nSrcWidth = pSetup->width;
	int nSrcHeight = pSetup->height;
	int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
	pRenderContext->GetViewport(nViewportX, nViewportY, nViewportWidth, nViewportHeight);

	// Get material and texture pointers
	ITexture *pRtQuarterSize1 = materials->FindTexture("_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET);

	{
		//=======================================================================================================//
		// At this point, pRtQuarterSize0 is filled with the fully colored glow around everything as solid glowy //
		// blobs. Now we need to stencil out the original objects by only writing pixels that have no            //
		// stencil bits set in the range we care about.                                                          //
		//=======================================================================================================//
		IMaterial *pMatHaloAddToScreen = materials->FindMaterial("dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, true);

		// Do not fade the glows out at all (weight = 1.0)
		IMaterialVar *pDimVar = pMatHaloAddToScreen->FindVar("$C0_X", NULL);
		pDimVar->SetFloatValue(1.0f);

		// Set stencil state
		ShaderStencilState_t stencilState;
		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0x0; // We're not changing stencil
		stencilState.m_nTestMask = 0xFF;
		stencilState.m_nReferenceValue = 0x0;
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
		stencilState.m_PassOp = STENCILOPERATION_KEEP;
		stencilState.m_FailOp = STENCILOPERATION_KEEP;
		stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
		stencilState.SetStencilState(pRenderContext);

		// Draw quad
		pRenderContext->DrawScreenSpaceRectangle(pMatHaloAddToScreen, 0, 0, nViewportWidth, nViewportHeight,
			0.0f, -0.5f, nSrcWidth / 4 - 1, nSrcHeight / 4 - 1,
			pRtQuarterSize1->GetActualWidth(),
			pRtQuarterSize1->GetActualHeight());

		stencilStateDisable.SetStencilState(pRenderContext);
	}
}

#endif // GLOWS_ENABLE
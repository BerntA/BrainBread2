//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Character 3D Model Preview Panel...
//
//========================================================================================//

#include "cbase.h"
#include "CharacterPreviewPanel.h"
#include <vgui/IInput.h>
#include <inputsystem/iinputsystem.h>
#include "GameBase_Shared.h"
#include "hl2mp_gamerules.h"
#include "gibs_shared.h"
#include "KeyValues.h"
#include "model_types.h"
#include "view_shared.h"
#include "view.h"
#include "filesystem.h"
#include "vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY(CharacterPreviewPanel);

CClientModelPanelModel::CClientModelPanelModel()
{
}

void CClientModelPanelModel::PrepareModel(const model_t *ptr)
{
	Interp_SetupMappings(GetVarMapping());

	index = -1;
	Clear();

	SetModelPointer(ptr);
	SetRenderMode(kRenderNormal);
	m_nRenderFX = kRenderFxNone;
	m_nBody = 0;
	m_nSkin = 0;
}

int	CClientModelPanelModel::DrawModel(int flags)
{
	if (!GetModel() || (GetRenderMode() == kRenderNone))
		return 0;

	int drawn = 0;
	switch (modelinfo->GetModelType(GetModel()))
	{
	case mod_studio:
		drawn = DrawStudioModel(flags);
		break;
	default:
		break;
	}

	return drawn;
}

int CClientModelPanelModel::DrawStudioModel(int flags)
{
	if (!GetModel() || (modelinfo->GetModelType(GetModel()) != mod_studio))
		return 0;

	VPROF_BUDGET("CClientModelPanelModel::DrawStudioModel", VPROF_BUDGETGROUP_MODEL_RENDERING);

	// Make sure m_pstudiohdr is valid for drawing
	MDLCACHE_CRITICAL_SECTION();
	if (!GetModelPtr())
		return 0;

	int drawn = modelrender->DrawModel(
		flags,
		this,
		MODEL_INSTANCE_INVALID,
		index,
		GetModel(),
		GetAbsOrigin(),
		GetAbsAngles(),
		m_nSkin,
		m_nBody,
		m_nHitboxSet);

	return drawn;
}

void CClientModelPanelModel::Release(void)
{
	RemoveFromLeafSystem();
	BaseClass::Release();
}

CharacterPreviewPanel::CharacterPreviewPanel(vgui::Panel *pParent, const char *pName)
	: vgui::EditablePanel(pParent, pName)
{
	m_nFOV = 54;
	m_hModel = NULL;
	m_bPanelDirty = true;
	m_bStartFramed = false;
	m_bAllowOffscreen = false;

	pchSurvivorName[0] = 0;
	m_iTeam = TEAM_HUMANS;

	m_iSkin = 0;
	m_iHead = 0;
	m_iBody = 0;
	m_iLeftLeg = 0;
	m_iRightLeg = 0;

	vecCameraPosition = Vector(0, 0, 0);
	angCameraAngle = QAngle(0, 0, 0);

	m_flAngleY = 0.0f;
	m_flOriginX = 0.0f;
	m_iOriginalCursorXPos = 0;
	m_bWantsToRotate = false;

	ListenForGameEvent("game_newmap");

	m_pInputPanel = new MouseInputPanel(this, "InputPanel");
	m_pInputPanel->SetZPos(65);
	m_pInputPanel->MoveToFront();
}

CharacterPreviewPanel::~CharacterPreviewPanel()
{
	DeleteModelData();
}

void CharacterPreviewPanel::DeleteModelData(void)
{
	if (m_hModel)
	{
		m_hModel->Remove();
		m_hModel = NULL;
	}
}

void CharacterPreviewPanel::FireGameEvent(IGameEvent * event)
{
	const char *type = event->GetName();
	if (Q_strcmp(type, "game_newmap") == 0)
		m_bPanelDirty = true;
}

void CharacterPreviewPanel::LoadModel(const char *name, int team)
{
	Q_strncpy(pchSurvivorName, name, 32);
	m_iTeam = team;

	m_vecViewportOffset.Init();

	m_bPanelDirty = true;

	m_pInputPanel->RequestFocus();
}

void CharacterPreviewPanel::SetProperties(int skin, int head, int body, int rightLeg, int leftLeft)
{
	m_iSkin = skin;
	m_iHead = head;
	m_iBody = body;
	m_iLeftLeg = leftLeft;
	m_iRightLeg = rightLeg;

	m_bPanelDirty = true;
}

void CharacterPreviewPanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	m_nFOV = inResourceData->GetInt("fov", 54);
	m_bStartFramed = inResourceData->GetInt("start_framed", false);
	m_bAllowOffscreen = inResourceData->GetInt("allow_offscreen", false);
}

void CharacterPreviewPanel::SetupModel(void)
{
	MDLCACHE_CRITICAL_SECTION();

	// remove any current models we're using
	DeleteModelData();

	if (!GameBaseShared()->GetSharedGameDetails())
		return;

	const DataPlayerItem_Survivor_Shared_t *modelInfo = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(pchSurvivorName);
	if (modelInfo == NULL)
		return;

	vecCameraPosition = modelInfo->vecPosition;
	angCameraAngle = modelInfo->angAngles;

	m_flAngleY = angCameraAngle.y;
	m_flOriginX = vecCameraPosition.x;
	m_iOriginalCursorXPos = 0;
	m_bWantsToRotate = false;

	const char *pszModelName = (m_iTeam == TEAM_HUMANS) ? modelInfo->szHumanModelPath : modelInfo->szZombieModelPath;
	if (!pszModelName || !pszModelName[0])
		return;

	m_hModel = new CClientModelPanelModel;
	m_hModel->PrepareModel((m_iTeam == TEAM_HUMANS) ? modelInfo->m_pClientModelPtrHuman : modelInfo->m_pClientModelPtrZombie);
	m_hModel->AddToLeafSystem(RENDER_GROUP_OPAQUE_ENTITY);
	m_hModel->SetRenderMode(kRenderNormal);
	m_hModel->DontRecordInTools();
	m_hModel->AddEffects(EF_NODRAW); // don't let the renderer draw the model normally
	m_hModel->m_nSkin = m_iSkin;
	m_hModel->m_nBody = 0;

	int bodygroupAccessoryValues[PLAYER_ACCESSORY_MAX] =
	{
		m_iHead,
		m_iBody,
		m_iLeftLeg,
		m_iRightLeg,
	};

	for (int i = 0; i < PLAYER_ACCESSORY_MAX; i++)
	{
		int accessoryGroup = m_hModel->FindBodygroupByName(PLAYER_BODY_ACCESSORY_BODYGROUPS[i]);
		if (accessoryGroup == -1)
			continue;

		m_hModel->SetBodygroup(accessoryGroup, bodygroupAccessoryValues[i]);
	}

	const char *pchSequence = modelInfo->szSequence;
	if (m_iTeam == TEAM_DECEASED)
		pchSequence = "melee_idle";

	m_hModel->ResetSequence(m_hModel->LookupSequence(pchSequence));
	m_hModel->SetCycle(0);
	m_hModel->m_flAnimTime = gpGlobals->curtime;

	CalculateFrameDistance();
}

void CharacterPreviewPanel::InitCubeMaps()
{
	ITexture *pCubemapTexture;

	// Deal with the default cubemap
	if (g_pMaterialSystemHardwareConfig->GetHDREnabled())
	{
		pCubemapTexture = materials->FindTexture("editor/cubemap.hdr", NULL, true);
		m_DefaultHDREnvCubemap.Init(pCubemapTexture);
	}
	else
	{
		pCubemapTexture = materials->FindTexture("editor/cubemap", NULL, true);
		m_DefaultEnvCubemap.Init(pCubemapTexture);
	}
}

void CharacterPreviewPanel::UpdateModel()
{
	if (m_bPanelDirty)
	{
		InitCubeMaps();
		SetupModel();
		m_bPanelDirty = false;
	}
}

void CharacterPreviewPanel::Paint()
{
	BaseClass::Paint();

	m_pInputPanel->SetPos(0, 0);
	m_pInputPanel->SetSize(GetWide(), GetTall());

	MDLCACHE_CRITICAL_SECTION();

	UpdateModel();

	if (!m_hModel)
		return;

	OnCheckCameraRotationOrZoom();

	int x, y, w, h;

	GetBounds(x, y, w, h);
	ParentLocalToScreen(x, y);

	if (!m_bAllowOffscreen && x < 0)
	{
		// prevent x from being pushed off the left side of the screen
		// for modes like 1280 x 1024 (prevents model from being drawn in the panel)
		x = 0;
	}

	Vector vecExtraModelOffset(0, 0, 0);
	float flWidthRatio = ((float)w / (float)h) / (4.0f / 3.0f);

	// need to know if the ratio is not 4/3
	// HACK! HACK! to get our player models to appear the way they do in 4/3 if we're using other aspect ratios
	if (flWidthRatio > 1.05f)
	{
		vecExtraModelOffset.Init(-60, 0, 0);
	}
	else if (flWidthRatio < 0.95f)
	{
		vecExtraModelOffset.Init(15, 0, 0);
	}

	m_hModel->SetAbsOrigin(vecCameraPosition + vecExtraModelOffset);
	m_hModel->SetAbsAngles(angCameraAngle);

	// do we have a valid sequence?
	if (m_hModel->GetSequence() != -1)
	{
		m_hModel->FrameAdvance(gpGlobals->frametime);
	}

	CMatRenderContextPtr pRenderContext(materials);

	// figure out what our viewport is right now
	int viewportX, viewportY, viewportWidth, viewportHeight;
	pRenderContext->GetViewport(viewportX, viewportY, viewportWidth, viewportHeight);

	// Now draw it.
	CViewSetup view;
	view.x = x + m_vecViewportOffset.x + viewportX; // we actually want to offset by the 
	view.y = y + m_vecViewportOffset.y + viewportY; // viewport origin here because Push3DView expects global coords below
	view.width = w;
	view.height = h;

	view.m_bOrtho = false;

	// scale the FOV for aspect ratios other than 4/3
	view.fov = ScaleFOVByWidthRatio(m_nFOV, flWidthRatio);

	view.origin = vec3_origin;
	view.angles.Init();
	view.zNear = VIEW_NEARZ;
	view.zFar = 1000;

	// Not supported by queued material system - doesn't appear to be necessary
	//	ITexture *pLocalCube = pRenderContext->GetLocalCubemap();
	if (g_pMaterialSystemHardwareConfig->GetHDREnabled())
	{
		pRenderContext->BindLocalCubemap(m_DefaultHDREnvCubemap);
	}
	else
	{
		pRenderContext->BindLocalCubemap(m_DefaultEnvCubemap);
	}

	pRenderContext->SetLightingOrigin(vec3_origin);
	pRenderContext->SetAmbientLight(0.4, 0.4, 0.4);

	static Vector white[6] =
	{
		Vector(0.4, 0.4, 0.4),
		Vector(0.4, 0.4, 0.4),
		Vector(0.4, 0.4, 0.4),
		Vector(0.4, 0.4, 0.4),
		Vector(0.4, 0.4, 0.4),
		Vector(0.4, 0.4, 0.4),
	};

	g_pStudioRender->SetAmbientLightColors(white);
	g_pStudioRender->SetLocalLights(0, NULL);

	Vector vecMins, vecMaxs;
	m_hModel->GetRenderBounds(vecMins, vecMaxs);
	LightDesc_t spotLight(vec3_origin + Vector(0, 0, 200), Vector(1, 1, 1), m_hModel->GetAbsOrigin() + Vector(0, 0, (vecMaxs.z - vecMins.z) * 0.75), 0.035, 0.873);
	g_pStudioRender->SetLocalLights(1, &spotLight);

	Frustum dummyFrustum;
	render->Push3DView(view, 0, NULL, dummyFrustum);

	modelrender->SuppressEngineLighting(true);
	float color[3] = { 1.0f, 1.0f, 1.0f };
	render->SetColorModulation(color);
	render->SetBlend(1.0f);
	m_hModel->DrawModel(STUDIO_RENDER);

	modelrender->SuppressEngineLighting(false);

	render->PopView(dummyFrustum);

	pRenderContext->BindLocalCubemap(NULL);
}

void CharacterPreviewPanel::CalculateFrameDistanceInternal(const model_t *pModel)
{
	// Get the model space render bounds.
	Vector vecMin, vecMax;
	modelinfo->GetModelRenderBounds(pModel, vecMin, vecMax);
	Vector vecCenter = (vecMax + vecMin) * 0.5f;
	vecMin -= vecCenter;
	vecMax -= vecCenter;

	// Get the bounds points and transform them by the desired model panel rotation.
	Vector aBoundsPoints[8];
	aBoundsPoints[0].Init(vecMax.x, vecMax.y, vecMax.z);
	aBoundsPoints[1].Init(vecMin.x, vecMax.y, vecMax.z);
	aBoundsPoints[2].Init(vecMax.x, vecMin.y, vecMax.z);
	aBoundsPoints[3].Init(vecMin.x, vecMin.y, vecMax.z);
	aBoundsPoints[4].Init(vecMax.x, vecMax.y, vecMin.z);
	aBoundsPoints[5].Init(vecMin.x, vecMax.y, vecMin.z);
	aBoundsPoints[6].Init(vecMax.x, vecMin.y, vecMin.z);
	aBoundsPoints[7].Init(vecMin.x, vecMin.y, vecMin.z);

	// Translated center point (offset from camera center).
	Vector vecTranslateCenter = -vecCenter;

	// Build the rotation matrix.
	QAngle angPanelAngles = angCameraAngle;
	matrix3x4_t matRotation;
	AngleMatrix(angPanelAngles, matRotation);

	Vector aXFormPoints[8];
	for (int iPoint = 0; iPoint < 8; ++iPoint)
	{
		VectorTransform(aBoundsPoints[iPoint], matRotation, aXFormPoints[iPoint]);
	}

	Vector vecXFormCenter;
	VectorTransform(-vecTranslateCenter, matRotation, vecXFormCenter);

	int w, h;
	GetSize(w, h);
	float flW = (float)w;
	float flH = (float)h;

	float flFOVx = DEG2RAD(m_nFOV * 0.5f);
	float flFOVy = CalcFovY((m_nFOV * 0.5f), flW / flH);
	flFOVy = DEG2RAD(flFOVy);

	float flTanFOVx = tan(flFOVx);
	float flTanFOVy = tan(flFOVy);

	// Find the max value of x, y, or z
	float flDist = 0.0f;
	for (int iPoint = 0; iPoint < 8; ++iPoint)
	{
		float flDistZ = fabs(aXFormPoints[iPoint].z / flTanFOVy - aXFormPoints[iPoint].x);
		float flDistY = fabs(aXFormPoints[iPoint].y / flTanFOVx - aXFormPoints[iPoint].x);
		float flTestDist = MAX(flDistZ, flDistY);
		flDist = MAX(flDist, flTestDist);
	}

	// Scale the object down by 10%.
	flDist *= 1.10f;

	// Add the framing offset.
	//vecXFormCenter += m_pModelInfo->m_vecFramedOriginOffset;

	// Zoom to the frame distance
	vecCameraPosition.x = flDist - vecXFormCenter.x;
	vecCameraPosition.y = -vecXFormCenter.y;
	vecCameraPosition.z = -vecXFormCenter.z;

	// Screen space points.
	Vector2D aScreenPoints[8];
	Vector aCameraPoints[8];
	for (int iPoint = 0; iPoint < 8; ++iPoint)
	{
		aCameraPoints[iPoint] = aXFormPoints[iPoint];
		aCameraPoints[iPoint].x += flDist;

		aScreenPoints[iPoint].x = aCameraPoints[iPoint].y / (flTanFOVx * aCameraPoints[iPoint].x);
		aScreenPoints[iPoint].y = aCameraPoints[iPoint].z / (flTanFOVy * aCameraPoints[iPoint].x);

		aScreenPoints[iPoint].x = (aScreenPoints[iPoint].x * 0.5f + 0.5f) * flW;
		aScreenPoints[iPoint].y = (aScreenPoints[iPoint].y * 0.5f + 0.5f) * flH;
	}

	// Find the min/max and center of the 2D bounding box of the object.
	Vector2D vecScreenMin(99999.0f, 99999.0f), vecScreenMax(-99999.0f, -99999.0f);
	for (int iPoint = 0; iPoint < 8; ++iPoint)
	{
		vecScreenMin.x = MIN(vecScreenMin.x, aScreenPoints[iPoint].x);
		vecScreenMin.y = MIN(vecScreenMin.y, aScreenPoints[iPoint].y);
		vecScreenMax.x = MAX(vecScreenMax.x, aScreenPoints[iPoint].x);
		vecScreenMax.y = MAX(vecScreenMax.y, aScreenPoints[iPoint].y);
	}

	vecScreenMin.x = clamp(vecScreenMin.x, 0.0f, flW);
	vecScreenMin.y = clamp(vecScreenMin.y, 0.0f, flH);
	vecScreenMax.x = clamp(vecScreenMax.x, 0.0f, flW);
	vecScreenMax.y = clamp(vecScreenMax.y, 0.0f, flH);

	// Offset the view port based on the calculated model 2D center and the center of the viewport.
	Vector2D vecScreenCenter = (vecScreenMax + vecScreenMin) * 0.5f;
	m_vecViewportOffset.x = -((flW * 0.5f) - vecScreenCenter.x);
	m_vecViewportOffset.y = -((flH * 0.5f) - vecScreenCenter.y);
}

//-----------------------------------------------------------------------------
// Purpose: Calculates the distance the camera should be at to frame the model on the screen.
//-----------------------------------------------------------------------------
void CharacterPreviewPanel::CalculateFrameDistance(void)
{
	if (!m_hModel)
		return;

	// Compute a bounding radius for the model
	const model_t *mod = modelinfo->GetModel(m_hModel->GetModelIndex());
	if (!mod)
		return;

	if (m_bStartFramed)
	{
		CalculateFrameDistanceInternal(mod);
	}
}

void CharacterPreviewPanel::OnCheckCameraRotationOrZoom(void)
{
	if (bb2_inventory_item_debugging.GetBool())
	{
		m_bWantsToRotate = false;
		vecCameraPosition = Vector(bb2_inventory_origin_x.GetFloat(), bb2_inventory_origin_y.GetFloat(), bb2_inventory_origin_z.GetFloat());
		angCameraAngle = QAngle(bb2_inventory_angle_x.GetFloat(), bb2_inventory_angle_y.GetFloat(), bb2_inventory_angle_z.GetFloat());
		return;
	}

	if (m_bWantsToRotate)
	{
		if (!g_pInputSystem->IsButtonDown(MOUSE_LEFT))
		{
			m_bWantsToRotate = false;
			return;
		}

		int x, y;
		vgui::input()->GetCursorPos(x, y);

		if (x != m_iOriginalCursorXPos)
		{
			int iDiff = x - m_iOriginalCursorXPos;
			m_flAngleY += iDiff;

			// Clamp the angle between 0 - 360 degrees.
			if (m_flAngleY > 360)
				m_flAngleY = 0;
			else if (m_flAngleY < 0)
				m_flAngleY = 360;

			angCameraAngle.y = m_flAngleY;
			vecCameraPosition.x = m_flOriginX;
			m_iOriginalCursorXPos = x;
		}
	}
}

void CharacterPreviewPanel::OnMouseReleased(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT)
		m_bWantsToRotate = false;
	else
		BaseClass::OnMouseReleased(code);
}

void CharacterPreviewPanel::OnMousePressed(vgui::MouseCode code)
{
	if (m_bWantsToRotate)
	{
		BaseClass::OnMousePressed(code);
		return;
	}

	int x, y;
	vgui::input()->GetCursorPos(x, y);

	if (code == MOUSE_LEFT)
	{
		m_bWantsToRotate = true;
		m_iOriginalCursorXPos = x;
	}
	else
		BaseClass::OnMousePressed(code);
}

void CharacterPreviewPanel::OnMouseWheeled(int delta)
{
	bool bScrollingUp = (delta > 0);
	if (!m_bWantsToRotate)
	{
		m_flOriginX += (bScrollingUp ? -1 : 1);
		vecCameraPosition.x = m_flOriginX;
	}

	BaseClass::OnMouseWheeled(delta);
}

void CharacterPreviewPanel::PaintBackground()
{
	SetBgColor(Color(0, 0, 0, 0));
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}
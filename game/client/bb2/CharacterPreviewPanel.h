//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Character 3D Model Preview Panel...
//
//========================================================================================//

#ifndef CHARACTER_PREVIEW_PANEL_H
#define CHARACTER_PREVIEW_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/PHandle.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include "GameEventListener.h"
#include "KeyValues.h"
#include "basemodelpanel.h"
#include "MouseInputPanel.h"

class CClientModelPanelModel : public C_BaseFlex
{
public:
	CClientModelPanelModel();
	DECLARE_CLASS(CClientModelPanelModel, C_BaseFlex);

	bool IsMenuModel() const { return true; }
	bool IsClientCreated(void) const { return true; }

	void PrepareModel(const model_t *ptr);
	void Release(void);

	int DrawModel(int flags);

private:
	int DrawStudioModel(int flags);
};

namespace vgui
{
	class CharacterPreviewPanel;
	class CharacterPreviewPanel : public vgui::EditablePanel, public CGameEventListener
	{
	public:
		DECLARE_CLASS_SIMPLE(CharacterPreviewPanel, vgui::EditablePanel);
		CharacterPreviewPanel(vgui::Panel *parent, char const *panelName);
		virtual ~CharacterPreviewPanel();;

		virtual void Paint();
		virtual void ApplySettings(KeyValues *inResourceData);
		virtual void DeleteModelData(void);

		virtual void SetFOV(int nFOV) { m_nFOV = nFOV; }
		virtual void SetPanelDirty(void) { m_bPanelDirty = true; }

		void CalculateFrameDistance(void);
		void UpdateModel();

		virtual void FireGameEvent(IGameEvent * event);
		virtual void LoadModel(const char *name, int team);
		virtual void SetProperties(int skin, int head, int body, int rightLeg, int leftLeft);

	protected:
		virtual void SetupModel(void);
		virtual void OnMouseReleased(vgui::MouseCode code);
		virtual void OnMousePressed(vgui::MouseCode code);
		virtual void OnMouseWheeled(int delta);
		virtual void PaintBackground();

	private:
		void InitCubeMaps();
		void CalculateFrameDistanceInternal(const model_t *pModel);

		vgui::MouseInputPanel *m_pInputPanel;
		void OnCheckCameraRotationOrZoom(void);

		char pchSurvivorName[32];
		int m_iTeam;
		int m_iSkin;
		int m_iHead;
		int m_iBody;
		int m_iLeftLeg;
		int m_iRightLeg;

	public:
		int m_nFOV;
		bool m_bStartFramed;

		CClientModelPanelModel *m_hModel;

		Activity m_iCurrentActivicty;
		Vector vecCameraPosition;
		QAngle angCameraAngle;

		Vector m_vecViewportOffset;

	private:
		bool m_bPanelDirty;
		bool m_bAllowOffscreen;

		float m_flAngleY;
		float m_flOriginX;
		int m_iOriginalCursorXPos;
		bool m_bWantsToRotate;

		CTextureReference m_DefaultEnvCubemap;
		CTextureReference m_DefaultHDREnvCubemap;
	};
}

#endif // CHARACTER_PREVIEW_PANEL_H
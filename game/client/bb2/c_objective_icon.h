//=========       Copyright © Reperio Studios 2014 @ Bernt Andreas Eide!       ============//
//
// Purpose: Objective Icon Sprite - Used to mark objective areas to guide the players.
//
//========================================================================================//

#ifndef C_OBJ_ICON_H
#define C_OBJ_ICON_H

#ifdef _WIN32
#pragma once
#endif

#include "c_hl2mp_player.h"

class C_ObjectiveIcon : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_ObjectiveIcon, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_ObjectiveIcon();
	virtual ~C_ObjectiveIcon();

	virtual bool ShouldDraw() { return true; }

	virtual const char *GetTexture(void) { return m_szTextureFile; }
	virtual int GetTeamLink(void) { return m_iTeamNumber; }
	virtual bool IsHidden(void) { return m_bShouldBeHidden; }

	virtual IMaterial *GetIconMaterial(void) { return pMaterialLink; }

	virtual void PostDataUpdate(DataUpdateType_t updateType);

private:
	bool m_bShouldBeHidden;
	int m_iTeamNumber;
	char m_szTextureFile[MAX_WEAPON_STRING];

	IMaterial *pMaterialLink;
};

extern void RenderObjectiveIcons(void);

#endif // C_OBJ_ICON_H
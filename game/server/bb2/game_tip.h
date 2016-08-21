//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Display a tool tip for all the players in-game.
//
//========================================================================================//

#ifndef GAME_TIPS_H
#define GAME_TIPS_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

class CGameTip : public CLogicalEntity
{
public:
	DECLARE_CLASS(CGameTip, CLogicalEntity);
	DECLARE_DATADESC();

	CGameTip();

	void Spawn();

private:

	float m_flTipDuration;
	float m_flRadiusEmit;
	int m_iTipType;

	string_t szTip;
	string_t szKeyBind;
	COutputEvent m_OnShowTip;
	void InputShowTip(inputdata_t &data);
};

#endif // GAME_TIPS_H
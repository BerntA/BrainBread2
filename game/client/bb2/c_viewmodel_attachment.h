//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Custom viewmodel for the hands. L4D styled system, separate hands.
//
//========================================================================================//

#ifndef C_VIEWMODEL_ATTACHMENT_H
#define C_VIEWMODEL_ATTACHMENT_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_baseanimating.h"
#include "baseviewmodel_shared.h"
#include "c_baseviewmodel.h"

class C_ViewModelAttachment : public C_BaseViewModel
{
public:
	DECLARE_CLASS(C_ViewModelAttachment, C_BaseViewModel);

	C_ViewModelAttachment();

	void Spawn();
	int DrawModel(int flags);
	bool ShouldDraw();
	bool IsPlayerHands(void) { return true; }
};

#endif // C_VIEWMODEL_ATTACHMENT_H
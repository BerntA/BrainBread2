//=========       Copyright © Reperio Studios 2024 @ Bernt Andreas Eide!       ============//
//
// Purpose: A simple door transition handler, RE! style.
//
//========================================================================================//

#ifndef FUNC_TRANSITION_H
#define FUNC_TRANSITION_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

struct PlayerTransitionItem_t
{
	EHANDLE hPlayer;
	float flTime;
};

class CFuncTransition : public CBaseEntity
{
public:
	DECLARE_CLASS(CFuncTransition, CBaseEntity);
	DECLARE_DATADESC();

	CFuncTransition();

	void Spawn(void);
	void Precache(void);
	void Activate(void);
	bool CreateVPhysics();

	void TransitionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void TransitionThink(void);
	void TeleportTo(CBasePlayer* pPlayer);

	void Lock();
	void Unlock();

	void InputLock(inputdata_t& inputdata);
	void InputUnlock(inputdata_t& inputdata);

	void PlaySound(CBasePlayer* pPlayer, const char* pSound);

	int	ObjectCaps(void);
	bool IsLocked(void) { return m_bLocked; }

private:
	COutputEvent m_OnUse;
	COutputEvent m_OnUseLocked;

	string_t m_Door;
	string_t m_Destination;

	string_t m_OpenSound;
	string_t m_CloseSound;
	string_t m_LockedSound;

	string_t m_UnlockedMessage;
	string_t m_LockedMessage;

	Vector m_vSaveOrigin;
	QAngle m_vSaveAngles;

	bool m_bLocked;

	hudtextparms_t	m_textParms;

	CUtlVector<PlayerTransitionItem_t> m_pTransitionItems;
};

#endif // FUNC_TRANSITION_H
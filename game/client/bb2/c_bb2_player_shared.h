//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Shared Data for BB2 Players.
//
//========================================================================================//

#if !defined( C_BB2_PLAYER_SHARED )
#define C_BB2_PLAYER_SHARED
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "c_baseanimating.h"
#include "c_hl2mp_player.h"
#include "c_firstperson_body.h"
#include "c_viewmodel_attachment.h"

class CBB2PlayerShared;
class CBB2PlayerShared
{
public:
	void Initialize();
	void Shutdown();

	void CreateEntities();
	void OnNewModel();
	void OnUpdate();

	void UpdatePlayerHands(C_BaseViewModel *pParent, C_HL2MP_Player *pOwner = NULL);
	void UpdatePlayerBody(C_HL2MP_Player *pOwner);

	const model_t *GetPlayerHandModel(C_HL2MP_Player *pOwner);
	const model_t *GetPlayerBodyModel(C_HL2MP_Player *pOwner);

	bool IsBodyOwner(C_BaseAnimating *pTarget);

	// Firstperson Body Helpers:
	void BodyUpdateVisibility(void);
	void BodyUpdate(C_HL2MP_Player *pOwner);
	void BodyResetSequence(C_BaseAnimating *pOwner, int sequence);
	void BodyRestartMainSequence(C_BaseAnimating *pOwner);
	void BodyRestartGesture(C_HL2MP_Player *pOwner, Activity activity, int slot);

	// Misc
	bool IsSniperScopeActive(void) { return m_bIsDrawingSniperScope; }
	void SetSniperScopeActive(bool value) { m_bIsDrawingSniperScope = value; }
	
	bool IsVotePanelActive(void) { return m_bShouldDrawVotePanel; }
	void SetVotePanelActive(bool value) { m_bShouldDrawVotePanel = value; }

	int GetPlayerVoteResponse(void) { return m_iPlayerVoteNum; }
	void SetPlayerVoteResponse(int reply) { m_iPlayerVoteNum = reply; }

	C_HL2MP_Player *GetCurrentViewModelOwner(void);

private:
	bool m_bHasCreated;

	C_ViewModelAttachment *m_pPlayerHands;
	C_FirstpersonBody *m_pPlayerBody;
	Vector m_vecBodyOffset;

	// Misc
	bool m_bIsDrawingSniperScope;
	bool m_bShouldDrawVotePanel;
	int m_iPlayerVoteNum;
};

extern CBB2PlayerShared *BB2PlayerGlobals;

#endif // C_BB2_PLAYER_SHARED
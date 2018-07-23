//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: TempEnt : Player gib & ragdoll, the new client-side player models required a separate event for player related gibbings.
//
//========================================================================================//

#include "cbase.h"
#include "c_basetempentity.h"
#include <cliententitylist.h>
#include "c_te_effect_dispatch.h"
#include "c_hl2mp_player.h"
#include "c_playermodel.h"
#include "c_client_gib.h"
#include "GameBase_Shared.h"

class C_TEPlayerGib : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEPlayerGib, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	void PostDataUpdate(DataUpdateType_t updateType);

	int m_iIndex;
	int m_iFlags;
	int m_iType;
	Vector m_vecOrigin;
	Vector m_vecVelocity;
	QAngle m_angRotation;
};

void C_TEPlayerGib::PostDataUpdate(DataUpdateType_t updateType)
{
	C_HL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(m_iIndex));
	if ((pPlayer == NULL) || (pPlayer->GetNewPlayerModel() == NULL))
		return;

	SpawnGibOrRagdollForPlayer(pPlayer->GetNewPlayerModel(), m_iIndex, pPlayer->GetTeamNumber(), pPlayer->GetSurvivorChoice(), m_iFlags, m_iType, m_vecOrigin, m_vecVelocity, m_angRotation);
}

IMPLEMENT_CLIENTCLASS_EVENT(C_TEPlayerGib, DT_TEPlayerGib, CTEPlayerGib);

BEGIN_RECV_TABLE_NOBASE(C_TEPlayerGib, DT_TEPlayerGib)
RecvPropInt(RECVINFO(m_iIndex)),
RecvPropInt(RECVINFO(m_iFlags)),
RecvPropInt(RECVINFO(m_iType)),
RecvPropVector(RECVINFO(m_vecOrigin)),
RecvPropFloat(RECVINFO(m_angRotation[0])),
RecvPropFloat(RECVINFO(m_angRotation[1])),
RecvPropFloat(RECVINFO(m_angRotation[2])),
RecvPropVector(RECVINFO(m_vecVelocity)),
END_RECV_TABLE()

void SpawnGibOrRagdollForPlayer(C_BaseAnimating *pFrom, int index, int team, const char *survivor, int flags, int type, const Vector &origin, const Vector &velocity, const QAngle &angles)
{
	const DataPlayerItem_Survivor_Shared_t *data = GameBaseShared()->GetSharedGameDetails()->GetSurvivorDataForIndex(survivor);
	if (data == NULL || pFrom == NULL)
		return;

	const model_t *model = NULL;
	if (type == CLIENT_RAGDOLL)
		model = (team == TEAM_DECEASED) ? data->m_pClientModelPtrZombie : data->m_pClientModelPtrHuman;
	else
		model = GameBaseShared()->GetSharedGameDetails()->GetPlayerGibModelPtrForGibID((*data), !(team == TEAM_DECEASED), flags);

	if (model == NULL)
		return;

	C_ClientSideGibBase *pEntity = NULL;
	bool bIsRagdollGib = (type > CLIENT_GIB_RAGDOLL_NORMAL_PHYSICS);
	if (bIsRagdollGib)
		pEntity = new C_ClientRagdollGib();
	else
		pEntity = new C_ClientPhysicsGib();

	pEntity->SetShouldPreferModelPointer(true);
	pEntity->SetAbsOrigin(origin);
	pEntity->SetAbsAngles(angles);

	if (!pEntity->Initialize(type, model))
	{
		pEntity->Release();
		return;
	}

	pEntity->m_nBody = (type == CLIENT_RAGDOLL) ? pFrom->m_nBody : 0;
	pEntity->m_nSkin = pFrom->m_nSkin;
	pEntity->SetEffects(pFrom->GetEffects());

	if ((type == CLIENT_RAGDOLL) && IsPlayerIndex(index)) // An actual plr ragdoll:
	{
		C_ClientRagdollGib *pRagdoll = static_cast<C_ClientRagdollGib*>(pEntity);
		Assert(pRagdoll != NULL);

		pRagdoll->m_nGibFlags = flags;
		pRagdoll->SetPlayerLink(index, team, survivor);

		if (index == GetLocalPlayerIndex())
			m_pPlayerRagdoll = pRagdoll;

		// TODO setup bodygrp! (gib ungibbed stuff) + remove associated gear if need to gib.
	}

	if (bIsRagdollGib && !pEntity->LoadRagdoll())
	{
		pEntity->Release();
		return;
	}

	IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();
	if (pPhysicsObject)
		pPhysicsObject->AddVelocity(&velocity, NULL);
	else
	{
		pEntity->Release();
		return;
	}

	pEntity->SetForceFade();
	pEntity->OnFullyInitialized();
}
//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Client Side Gib - TempEnt Definition.
//
//========================================================================================//

#include "cbase.h"
#include "gibs_shared.h"
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_TEClientSideGib : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEClientSideGib, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	C_TEClientSideGib(void);

	virtual void PostDataUpdate(DataUpdateType_t updateType);

public:
	Vector			m_vecOrigin;
	QAngle			m_angRotation;
	Vector			m_vecVelocity;
	int				m_nModelIndex;
	int             m_nBody;
	int				m_nSkin;
	int				m_nFlags;
	int				m_nEffects;
	int             m_iGibType;
};

C_TEClientSideGib::C_TEClientSideGib(void)
{
	m_vecOrigin.Init();
	m_angRotation.Init();
	m_vecVelocity.Init();
	m_nModelIndex = 0;
	m_nBody = 0;
	m_nSkin = 0;
	m_nFlags = 0;
	m_nEffects = 0;
	m_iGibType = 0;
}

//-----------------------------------------------------------------------------
// Recording 
//-----------------------------------------------------------------------------
static inline void RecordClientGib(const Vector& start, const QAngle &angles,
	const Vector& vel, int nModelIndex, int nFlags, int nBody, int nSkin, int nEffects, int iGibType)
{
	if (!ToolsEnabled())
		return;

	if (clienttools->IsInRecordingMode())
	{
		const model_t* pModel = (nModelIndex != 0) ? modelinfo->GetModel(nModelIndex) : NULL;
		const char *pModelName = pModel ? modelinfo->GetModelName(pModel) : "";

		KeyValues *msg = new KeyValues("TempEntity");

		msg->SetInt("te", TE_CLIENT_GIB);
		msg->SetString("name", "TE_PhysicsProp");
		msg->SetFloat("time", gpGlobals->curtime);
		msg->SetFloat("originx", start.x);
		msg->SetFloat("originy", start.y);
		msg->SetFloat("originz", start.z);
		msg->SetFloat("anglesx", angles.x);
		msg->SetFloat("anglesy", angles.y);
		msg->SetFloat("anglesz", angles.z);
		msg->SetFloat("velx", vel.x);
		msg->SetFloat("vely", vel.y);
		msg->SetFloat("velz", vel.z);
		msg->SetString("model", pModelName);
		msg->SetInt("flags", nFlags);
		msg->SetInt("body", nBody);
		msg->SetInt("skin", nSkin);
		msg->SetInt("effects", nEffects);
		msg->SetInt("gibtype", iGibType);

		ToolFramework_PostToolMessage(HTOOLHANDLE_INVALID, msg);
		msg->deleteThis();
	}
}

void TE_ClientSideGib(IRecipientFilter& filter, float delay, int modelindex, int body, int skin,
	const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects, int gibType)
{
	tempents->ClientSideGib(modelindex, body, skin, pos, angles, vel, flags, effects, gibType);
	RecordClientGib(pos, angles, vel, modelindex, flags, body, skin, effects, gibType);
}

void C_TEClientSideGib::PostDataUpdate(DataUpdateType_t updateType)
{
	VPROF("C_TEClientSideGib::PostDataUpdate");

	tempents->ClientSideGib(m_nModelIndex, m_nBody, m_nSkin, m_vecOrigin, m_angRotation, m_vecVelocity, m_nFlags, m_nEffects, m_iGibType);
	RecordClientGib(m_vecOrigin, m_angRotation, m_vecVelocity, m_nModelIndex, m_nFlags, m_nBody, m_nSkin, m_nEffects, m_iGibType);
}

void TE_ClientSideGib(IRecipientFilter& filter, float delay, KeyValues *pKeyValues)
{
	Vector vecOrigin, vecVel;
	QAngle angles;
	int nSkin, nBody;
	nSkin = pKeyValues->GetInt("skin", 0);
	nBody = pKeyValues->GetInt("body", 0);
	vecOrigin.x = pKeyValues->GetFloat("originx");
	vecOrigin.y = pKeyValues->GetFloat("originy");
	vecOrigin.z = pKeyValues->GetFloat("originz");
	angles.x = pKeyValues->GetFloat("anglesx");
	angles.y = pKeyValues->GetFloat("anglesy");
	angles.z = pKeyValues->GetFloat("anglesz");
	vecVel.x = pKeyValues->GetFloat("velx");
	vecVel.y = pKeyValues->GetFloat("vely");
	vecVel.z = pKeyValues->GetFloat("velz");
	const char *pModelName = pKeyValues->GetString("model");
	int nModelIndex = pModelName[0] ? modelinfo->GetModelIndex(pModelName) : 0;
	int nFlags = pKeyValues->GetInt("flags");
	int nEffects = pKeyValues->GetInt("effects");
	int iGibType = pKeyValues->GetInt("gibtype");

	TE_ClientSideGib(filter, delay, nModelIndex, nBody, nSkin, vecOrigin, angles, vecVel, nFlags, nEffects, iGibType);
}

IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEClientSideGib, DT_TEClientSideGib, CTEClientSideGib)
RecvPropVector(RECVINFO(m_vecOrigin)),
RecvPropFloat(RECVINFO(m_angRotation[0])),
RecvPropFloat(RECVINFO(m_angRotation[1])),
RecvPropFloat(RECVINFO(m_angRotation[2])),
RecvPropVector(RECVINFO(m_vecVelocity)),
RecvPropInt(RECVINFO(m_nModelIndex)),
RecvPropInt(RECVINFO(m_nFlags)),
RecvPropInt(RECVINFO(m_nBody)),
RecvPropInt(RECVINFO(m_nSkin)),
RecvPropInt(RECVINFO(m_nEffects)),
RecvPropInt(RECVINFO(m_iGibType)),
END_RECV_TABLE()
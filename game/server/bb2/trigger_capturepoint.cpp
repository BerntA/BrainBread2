//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: If certain users touches this volume they will capture this zone after x sec, certain entities will 'halt' the progress if they enter the volume!
//
//========================================================================================//

#include "cbase.h"
#include "triggers.h"
#include "hl2mp_player.h"
#include "GameBase_Shared.h"
#include "GameBase_Server.h"
#include "hl2mp_gamerules.h"
#include "team.h"

#define CAPTURE_THINK_FREQ 0.1f
#define CAPTURE_RESET_TIME 2.5f

enum
{
	CAPTURE_STATE_NONE = 0, // not being captured
	CAPTURE_STATE_ACTIVE, // being captured
	CAPTURE_STATE_INSUFFICIENT, // too few actors in the volume
	CAPTURE_STATE_HALTED, // progress has been halted due to enemy presence
	CAPTURE_STATE_CAPTURED, // captured, finished
};

class CTriggerCapturePoint : public CBaseTrigger
{
public:
	DECLARE_CLASS(CTriggerCapturePoint, CBaseTrigger);
	DECLARE_DATADESC();

	CTriggerCapturePoint();

	void Spawn();
	void Reset();
	void StartTouch(CBaseEntity* pOther);
	void EndTouch(CBaseEntity* pOther);
	void OnTouch(CBaseEntity* pOther);
	void OnThink(void);
	void CapturePointThink(void);
	void TransmitCaptureStatus(CBasePlayer* pPlayer, bool value);
	void NotifyCaptureFailed(bool bEnemyEntered = false);
	void NotifyCaptureInsufficient(void);
	int GetProgressStatus(void);

private:

	// Captured Info
	int m_iProgressStatus;
	bool m_bShouldHaltWhenEnemiesTouchUs;

	// Team 
	int m_iTeamLink;

	// Capture Time
	float m_flTimeToCapture;
	float m_flCaptureProgress;
	float m_flTimeToReset;

	// Misc
	float m_flTouchTimer;

	string_t cszMessageTooFewPlayers;
	string_t cszMessageFailed;
	string_t cszMessageProgressHalted;

	COutputEvent m_OnCaptured;
};

LINK_ENTITY_TO_CLASS(trigger_capturepoint, CTriggerCapturePoint);

BEGIN_DATADESC(CTriggerCapturePoint)
DEFINE_KEYFIELD(m_iTeamLink, FIELD_INTEGER, "TeamLink"),
DEFINE_KEYFIELD(m_bShouldHaltWhenEnemiesTouchUs, FIELD_BOOLEAN, "CanHaltProgress"),
DEFINE_KEYFIELD(m_flTimeToCapture, FIELD_FLOAT, "CaptureTime"),

DEFINE_KEYFIELD(cszMessageTooFewPlayers, FIELD_STRING, "TooFewPlayersMessage"),
DEFINE_KEYFIELD(cszMessageFailed, FIELD_STRING, "CaptureFailedMessage"),
DEFINE_KEYFIELD(cszMessageProgressHalted, FIELD_STRING, "CaptureHaltedMessage"),

DEFINE_OUTPUT(m_OnCaptured, "OnCaptured"),

DEFINE_THINKFUNC(OnThink),
END_DATADESC()

CTriggerCapturePoint::CTriggerCapturePoint()
{
	m_iProgressStatus = CAPTURE_STATE_NONE;
	m_iTeamLink = TEAM_HUMANS;
	m_bShouldHaltWhenEnemiesTouchUs = false;

	m_flPercentRequired = 30.0f;
	m_flTimeToCapture = 10.0f;
	m_flCaptureProgress = m_flTimeToReset = m_flTouchTimer = 0.0f;
	m_flWait = 0.2f;

	cszMessageTooFewPlayers = cszMessageFailed = cszMessageProgressHalted = NULL_STRING;
	m_bSkipFilterCheck = true;
}

void CTriggerCapturePoint::Spawn()
{
	m_bSkipFilterCheck = true;
	AddSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS | SF_TRIGGER_ALLOW_NPCS);
	BaseClass::Spawn();
	InitTrigger();
	SetTouch(&CTriggerCapturePoint::OnTouch);
	SetThink(&CTriggerCapturePoint::OnThink);
	SetNextThink(gpGlobals->curtime + 1.0f);
}

void CTriggerCapturePoint::Reset()
{
	m_iProgressStatus = CAPTURE_STATE_NONE;
	m_flCaptureProgress = m_flTimeToReset = 0.0f;
}

void CTriggerCapturePoint::OnThink(void)
{
	CapturePointThink();
	SetNextThink(gpGlobals->curtime + CAPTURE_THINK_FREQ);
}

void CTriggerCapturePoint::CapturePointThink(void)
{
	if (m_bDisabled || (m_iProgressStatus == CAPTURE_STATE_CAPTURED))
		return;

	const int iCurrentProgressStatus = GetProgressStatus();

	if (m_iProgressStatus == CAPTURE_STATE_ACTIVE)
		m_flCaptureProgress += CAPTURE_THINK_FREQ;

	if (m_iProgressStatus != iCurrentProgressStatus)
	{
		m_iProgressStatus = iCurrentProgressStatus;

		switch (m_iProgressStatus)
		{

		case CAPTURE_STATE_NONE:
		{
			m_flTimeToReset = (gpGlobals->curtime + CAPTURE_RESET_TIME); // clear our state in X sec..
			break;
		}

		case CAPTURE_STATE_ACTIVE:
		{
			break;
		}

		case CAPTURE_STATE_INSUFFICIENT:
		{
			NotifyCaptureInsufficient();
			break;
		}

		case CAPTURE_STATE_HALTED:
		{
			NotifyCaptureFailed(true);
			break;
		}

		}
	}

	if ((m_iProgressStatus == CAPTURE_STATE_NONE) && (m_flTimeToReset > 0.0f) && (gpGlobals->curtime > m_flTimeToReset))
	{
		Reset();
		NotifyCaptureFailed();

		for (int i = 0; i < m_hTouchingEntities.Count(); i++)
			TransmitCaptureStatus(ToBasePlayer(m_hTouchingEntities[i].Get()), false);

		return;
	}

	if (m_iProgressStatus == CAPTURE_STATE_NONE)
		return;

	if (m_flCaptureProgress >= m_flTimeToCapture)
	{
		m_iProgressStatus = CAPTURE_STATE_CAPTURED;
		m_OnCaptured.FireOutput((m_hTouchingEntities.Count() > 0) ? m_hTouchingEntities[0].Get() : this, this);
	}

	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
		TransmitCaptureStatus(ToBasePlayer(m_hTouchingEntities[i].Get()), (m_iProgressStatus < CAPTURE_STATE_CAPTURED));
}

int CTriggerCapturePoint::GetProgressStatus(void)
{
	if (m_hTouchingEntities.Count() == 0)
		return CAPTURE_STATE_NONE;

	int iActorsInVolume = 0;
	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBaseEntity* pToucher = m_hTouchingEntities[i].Get();
		if (!pToucher || !pToucher->IsAlive() || !pToucher->IsPlayer() || (m_iTeamLink != pToucher->GetTeamNumber()))
			continue;
		iActorsInVolume++;
	}

	if (iActorsInVolume == 0)
		return CAPTURE_STATE_NONE;

	if (m_bShouldHaltWhenEnemiesTouchUs)
	{
		for (int i = 0; i < m_hTouchingEntities.Count(); i++)
		{
			CBaseEntity* pToucher = m_hTouchingEntities[i].Get();
			if (!pToucher || !pToucher->IsAlive())
				continue;

			if ((m_iTeamLink == TEAM_HUMANS) && (pToucher->IsZombie(true) || pToucher->IsMercenary()))
				return CAPTURE_STATE_HALTED;

			if ((m_iTeamLink == TEAM_DECEASED) && pToucher->IsHuman(true))
				return CAPTURE_STATE_HALTED;
		}
	}

	return (IsEnoughPlayersInVolume(m_iTeamLink) ? CAPTURE_STATE_ACTIVE : CAPTURE_STATE_INSUFFICIENT);
}

void CTriggerCapturePoint::StartTouch(CBaseEntity* pOther)
{
	if (!pOther || m_bDisabled || (m_iProgressStatus == CAPTURE_STATE_CAPTURED))
		return;

	BaseClass::StartTouch(pOther);

	if (pOther->IsPlayer() && (pOther->GetTeamNumber() == m_iTeamLink) && pOther->IsAlive() && !IsEnoughPlayersInVolume(m_iTeamLink))
		NotifyCaptureInsufficient();
}

void CTriggerCapturePoint::EndTouch(CBaseEntity* pOther)
{
	if (!pOther || m_bDisabled || (m_iProgressStatus == CAPTURE_STATE_CAPTURED))
		return;

	BaseClass::EndTouch(pOther);
	TransmitCaptureStatus(ToBasePlayer(pOther), false);
}

void CTriggerCapturePoint::TransmitCaptureStatus(CBasePlayer* pPlayer, bool value)
{
	if (!pPlayer || (pPlayer->GetTeamNumber() != m_iTeamLink))
		return;

	float flFraction = clamp((m_flCaptureProgress / m_flTimeToCapture), 0.0f, 1.0f);

	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();
	UserMessageBegin(filter, "CapturePointProgress");
	WRITE_BYTE(value);
	WRITE_FLOAT(flFraction);
	MessageEnd();
}

void CTriggerCapturePoint::OnTouch(CBaseEntity* pOther)
{
	if ((m_flTouchTimer > gpGlobals->curtime) || !pOther || (m_iProgressStatus == CAPTURE_STATE_CAPTURED) || m_bDisabled || !PassesTriggerFilters(pOther) || !IsFilterPassing(pOther))
		return;

	m_flTouchTimer = (gpGlobals->curtime + m_flWait);
	m_OnTouching.FireOutput(pOther, this);
}

void CTriggerCapturePoint::NotifyCaptureFailed(bool bEnemyEntered)
{
	const char* message = STRING(cszMessageFailed);
	if (bEnemyEntered && m_bShouldHaltWhenEnemiesTouchUs)
		message = STRING(cszMessageProgressHalted);

	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBasePlayer* pToucher = ToBasePlayer(m_hTouchingEntities[i].Get());
		if (!pToucher || (pToucher->GetTeamNumber() != m_iTeamLink))
			continue;

		GameBaseServer()->SendToolTip(message, "", 2.0f, GAME_TIP_WARNING, pToucher->entindex());
	}
}

void CTriggerCapturePoint::NotifyCaptureInsufficient(void)
{
	CTeam* pTeam = GetGlobalTeam(m_iTeamLink);

	if (pTeam == NULL)
		return;

	float flNumPlayers = (float)pTeam->GetNumPlayers();
	float flRequiredPlayers = (pTeam ? ceil(flNumPlayers * (m_flPercentRequired / 100.0f)) : 0.0f);

	if (flRequiredPlayers == 0.0f)
		return;

	char pchArg1[16];
	Q_snprintf(pchArg1, sizeof(pchArg1), "%i", (int)flRequiredPlayers);

	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBasePlayer* pToucher = ToBasePlayer(m_hTouchingEntities[i].Get());
		if (!pToucher || (pToucher->GetTeamNumber() != m_iTeamLink))
			continue;

		GameBaseServer()->SendToolTip(STRING(cszMessageTooFewPlayers), "", 2.0f, GAME_TIP_WARNING, pToucher->entindex(), pchArg1, ((flRequiredPlayers > 1) ? "players" : "player"));
	}
}
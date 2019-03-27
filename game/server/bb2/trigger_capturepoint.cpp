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

class CTriggerCapturePoint : public CBaseTrigger
{
public:
	DECLARE_CLASS(CTriggerCapturePoint, CBaseTrigger);
	DECLARE_DATADESC();

	CTriggerCapturePoint();

	void Spawn();
	void Reset();
	void StartTouch(CBaseEntity *pOther);
	void EndTouch(CBaseEntity *pOther);
	void OnTouch(CBaseEntity *pOther);
	void CapturePointThink(void);
	void HaltProgress(void);
	void TransmitCaptureStatus(CBasePlayer *pPlayer, bool value);
	void NotifyCaptureFailed(bool bEnemyEntered = false);
	bool HasToHaltProgress(CBaseEntity *pOther);
	bool CanContinueProgress(void);
	bool IsEnoughPlayersInVolume(void);

private:

	// Captured Info
	bool m_bIsCaptured;
	bool m_bIsBeingCaptured;
	bool m_bShouldHaltWhenEnemiesTouchUs;

	// Team 
	int m_iTeamLink;
	float m_flPercentRequired;

	// Capture Time
	float m_flTimeToCapture;
	float m_flCaptureTimeStart;
	float m_flCaptureTimeEnd;
	float m_flElapsedTime;
	float m_flTimeLeft;

	// Halting
	bool m_bShouldHaltProgress;

	string_t cszMessageCapturing;
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
DEFINE_KEYFIELD(m_flPercentRequired, FIELD_FLOAT, "PercentRequired"),

DEFINE_KEYFIELD(cszMessageCapturing, FIELD_STRING, "CapturingMessage"),
DEFINE_KEYFIELD(cszMessageTooFewPlayers, FIELD_STRING, "TooFewPlayersMessage"),
DEFINE_KEYFIELD(cszMessageFailed, FIELD_STRING, "CaptureFailedMessage"),
DEFINE_KEYFIELD(cszMessageProgressHalted, FIELD_STRING, "CaptureHaltedMessage"),

DEFINE_OUTPUT(m_OnCaptured, "OnCaptured"),

DEFINE_FIELD(m_bIsCaptured, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsBeingCaptured, FIELD_BOOLEAN),
DEFINE_FIELD(m_bShouldHaltProgress, FIELD_BOOLEAN),

DEFINE_FIELD(m_flCaptureTimeStart, FIELD_FLOAT),
DEFINE_FIELD(m_flCaptureTimeEnd, FIELD_FLOAT),
DEFINE_FIELD(m_flElapsedTime, FIELD_FLOAT),
DEFINE_FIELD(m_flTimeLeft, FIELD_FLOAT),

DEFINE_THINKFUNC(CapturePointThink),
END_DATADESC()

CTriggerCapturePoint::CTriggerCapturePoint()
{
	m_iTeamLink = TEAM_HUMANS;
	m_bIsCaptured = m_bIsBeingCaptured = m_bShouldHaltProgress = m_bShouldHaltWhenEnemiesTouchUs = false;

	m_flPercentRequired = 30.0f;
	m_flTimeToCapture = 10.0f;
	m_flCaptureTimeStart = m_flCaptureTimeEnd = m_flElapsedTime = m_flTimeLeft = 0.0f;

	cszMessageCapturing = cszMessageTooFewPlayers = cszMessageFailed = cszMessageProgressHalted = NULL_STRING;
}

void CTriggerCapturePoint::Spawn()
{
	BaseClass::Spawn();

	InitTrigger();

	SetTouch(&CTriggerCapturePoint::OnTouch);
	SetThink(&CTriggerCapturePoint::CapturePointThink);
	SetNextThink(gpGlobals->curtime + CAPTURE_THINK_FREQ);
}

void CTriggerCapturePoint::Reset()
{
	m_bIsCaptured = m_bIsBeingCaptured = m_bShouldHaltProgress = false;
	m_flCaptureTimeStart = m_flCaptureTimeEnd = m_flElapsedTime = m_flTimeLeft = 0.0f;
}

void CTriggerCapturePoint::CapturePointThink(void)
{
	// Iterate the touching entities:
	if (!m_bDisabled && !m_bIsCaptured)
	{
		bool bCanProceed = CanContinueProgress();
		bool bShouldStartCapturing = IsEnoughPlayersInVolume();
		if (bShouldStartCapturing && !m_bIsBeingCaptured && bCanProceed)
		{
			m_bIsBeingCaptured = true;
			m_flCaptureTimeStart = gpGlobals->curtime;
			m_flCaptureTimeEnd = gpGlobals->curtime + m_flTimeToCapture;
			m_flTimeLeft = m_flTimeToCapture;

			if (m_bShouldHaltProgress)
				HaltProgress();
		}

		if (!bShouldStartCapturing && m_bIsBeingCaptured)
		{
			Reset();
			NotifyCaptureFailed();

			for (int i = 0; i < m_hTouchingEntities.Count(); i++)
				TransmitCaptureStatus(ToBasePlayer(m_hTouchingEntities[i].Get()), false);
		}

		if (bCanProceed && m_bShouldHaltProgress && m_bIsBeingCaptured)
		{
			m_bShouldHaltProgress = false;
			m_flTimeLeft = m_flTimeToCapture - m_flElapsedTime;
			m_flCaptureTimeStart = gpGlobals->curtime;
			m_flCaptureTimeEnd = gpGlobals->curtime + m_flTimeLeft;
		}

		if (m_bIsBeingCaptured)
		{
			if ((gpGlobals->curtime > m_flCaptureTimeEnd) && bCanProceed)
			{
				m_bIsCaptured = true;
				m_OnCaptured.FireOutput(this, this);
			}

			for (int i = 0; i < m_hTouchingEntities.Count(); i++)
				TransmitCaptureStatus(ToBasePlayer(m_hTouchingEntities[i].Get()), !m_bIsCaptured);
		}
	}

	SetNextThink(gpGlobals->curtime + CAPTURE_THINK_FREQ);
}

bool CTriggerCapturePoint::HasToHaltProgress(CBaseEntity *pOther)
{
	if (!m_bShouldHaltWhenEnemiesTouchUs)
		return false;

	if (!pOther || !pOther->IsAlive())
		return false;

	if ((m_iTeamLink == TEAM_HUMANS) && (pOther->IsZombie(true) || pOther->IsMercenary()))
		return true;

	if ((m_iTeamLink == TEAM_DECEASED) && (pOther->IsHuman(true)))
		return true;

	return false;
}

bool CTriggerCapturePoint::CanContinueProgress(void)
{
	if (m_bShouldHaltWhenEnemiesTouchUs)
	{
		for (int i = 0; i < m_hTouchingEntities.Count(); i++)
		{
			CBaseEntity *pToucher = m_hTouchingEntities[i].Get();
			if (!pToucher)
				continue;

			if (HasToHaltProgress(pToucher))
				return false;
		}
	}

	if (!IsEnoughPlayersInVolume())
		return false;

	return true;
}

bool CTriggerCapturePoint::IsEnoughPlayersInVolume(void)
{
	int players = 0;
	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBaseEntity *pToucher = m_hTouchingEntities[i].Get();
		if (!pToucher)
			continue;

		if (pToucher->IsPlayer() && (pToucher->GetTeamNumber() == m_iTeamLink))
			players++;
	}

	float teamSize = 0, teamSizeInVolume = (float)players;
	CTeam *pTeam = GetGlobalTeam(m_iTeamLink);
	if (pTeam)
		teamSize = (float)pTeam->GetNumPlayers();

	float flPercentInVolume = floor((teamSizeInVolume / teamSize) * 100.0f);

	return (flPercentInVolume >= m_flPercentRequired);
}

void CTriggerCapturePoint::StartTouch(CBaseEntity *pOther)
{
	if (!pOther || m_bDisabled || m_bIsCaptured)
		return;

	BaseClass::StartTouch(pOther);

	// If another team or enemy enters this volume it will lock the progress.
	if (HasToHaltProgress(pOther) && !m_bShouldHaltProgress && m_bIsBeingCaptured)
		HaltProgress();

	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if (pPlayer && (pPlayer->GetTeamNumber() == m_iTeamLink) && pPlayer->IsAlive() && !IsEnoughPlayersInVolume())
	{
		float flRequiredPlayers = 0;
		CTeam *pTeam = GetGlobalTeam(m_iTeamLink);
		if (pTeam)
			flRequiredPlayers = floor(((float)pTeam->GetNumPlayers()) * (m_flPercentRequired / 100.0f));

		if (flRequiredPlayers > 0)
		{
			char pchArg1[16];
			Q_snprintf(pchArg1, 16, "%i", (int)flRequiredPlayers);
			GameBaseServer()->SendToolTip(STRING(cszMessageTooFewPlayers), "", 2.0f, GAME_TIP_WARNING, pPlayer->entindex(), pchArg1, ((flRequiredPlayers > 1) ? "players" : "player"));
		}
	}
}

void CTriggerCapturePoint::EndTouch(CBaseEntity *pOther)
{
	if (!pOther || m_bDisabled || m_bIsCaptured)
		return;

	BaseClass::EndTouch(pOther);

	TransmitCaptureStatus(ToBasePlayer(pOther), false);
}

void CTriggerCapturePoint::HaltProgress(void)
{
	m_bShouldHaltProgress = true;
	m_flElapsedTime += (gpGlobals->curtime - m_flCaptureTimeStart);
	NotifyCaptureFailed(true);
}

void CTriggerCapturePoint::TransmitCaptureStatus(CBasePlayer *pPlayer, bool value)
{
	if (!pPlayer || (pPlayer->GetTeamNumber() != m_iTeamLink))
		return;

	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();

	float maxTime = m_flTimeToCapture;
	float timeElapsed = (gpGlobals->curtime - m_flCaptureTimeStart) + m_flElapsedTime;

	if (m_bShouldHaltProgress)
		timeElapsed = m_flElapsedTime;

	if (timeElapsed > maxTime)
		timeElapsed = maxTime;

	UserMessageBegin(filter, "CapturePointProgress");
	WRITE_STRING(STRING(cszMessageCapturing));
	WRITE_BYTE(value);
	WRITE_FLOAT(maxTime);
	WRITE_FLOAT(timeElapsed);
	MessageEnd();
}

void CTriggerCapturePoint::OnTouch(CBaseEntity *pOther)
{
	// TODO: Anything here? This is a 'think' frame which is called all the time for touching ents, they're not added into the ehandle utlvector, which means once that list is empty 
	// it will refresh the halt progress and progress a tiny bit, meaning this func should not be used, ents who spawn in the volume will cause trouble.
}

void CTriggerCapturePoint::NotifyCaptureFailed(bool bEnemyEntered)
{
	const char *message = STRING(cszMessageFailed);
	if (bEnemyEntered && m_bShouldHaltWhenEnemiesTouchUs)
		message = STRING(cszMessageProgressHalted);

	for (int i = 0; i < m_hTouchingEntities.Count(); i++)
	{
		CBasePlayer *pToucher = ToBasePlayer(m_hTouchingEntities[i].Get());
		if (!pToucher || (pToucher->GetTeamNumber() != m_iTeamLink))
			continue;

		GameBaseServer()->SendToolTip(message, "", 2.0f, GAME_TIP_WARNING, pToucher->entindex());
	}
}
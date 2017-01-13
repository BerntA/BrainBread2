//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: Button with model ( dynamic ) for skin change. ++ more - It also has a KeyPad mode for more fancy interaction!
//
//========================================================================================//

#include "cbase.h"
#include "bb2_prop_button.h"
#include "gamerules.h"
#include "baseanimating.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "hl2mp_gamerules.h"
#include "player.h"
#include "ammodef.h"
#include "npcevent.h"
#include "eventlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CBB2Button)

// Think
DEFINE_THINKFUNC(UpdateThink),

// Out
DEFINE_OUTPUT(m_OnUse, "OnUse"),

// Keyfields
DEFINE_KEYFIELD(ClassifyFor, FIELD_INTEGER, "Filter"),
DEFINE_KEYFIELD(m_iGlowType, FIELD_INTEGER, "GlowType"),
DEFINE_KEYFIELD(m_bStartGlowing, FIELD_BOOLEAN, "EnableGlow"),
DEFINE_KEYFIELD(m_bShowModel, FIELD_BOOLEAN, "ShowModel"),
DEFINE_KEYFIELD(m_bIsEnabled, FIELD_BOOLEAN, "StartEnabled"),
DEFINE_KEYFIELD(szKeyPadCode, FIELD_STRING, "KeyPadCode"),
DEFINE_KEYFIELD(m_bIsKeyPad, FIELD_BOOLEAN, "KeyPadMode"),
DEFINE_KEYFIELD(m_clrGlow, FIELD_COLOR32, "GlowOverlayColor"),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "ShowModel", ShowModel),
DEFINE_INPUTFUNC(FIELD_VOID, "HideModel", HideModel),
DEFINE_INPUTFUNC(FIELD_VOID, "ShowGlow", ShowGlow),
DEFINE_INPUTFUNC(FIELD_VOID, "HideGlow", HideGlow),
DEFINE_INPUTFUNC(FIELD_VOID, "Enable", EnableButton),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", DisableButton),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetGlowType", SetGlowType),

END_DATADESC()

LINK_ENTITY_TO_CLASS(bb2_prop_button, CBB2Button);

CBB2Button::CBB2Button(void)
{
	ClassifyFor = 0;
	m_iGlowType = GLOW_MODE_GLOBAL;
	m_clrGlow = { 255, 100, 100, 255 };
	m_bStartGlowing = false;
	m_bShowModel = true;
	szKeyPadCode = NULL_STRING;
	m_bIsKeyPad = false;
	m_bIsEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CBB2Button::Spawn(void)
{
	char *szModel = (char *)STRING(GetModelName());
	if (!szModel || !*szModel)
	{
		Warning("BB2_PROP_BUTTON '%s' has no model!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	m_GlowColor = m_clrGlow = { m_clrGlow.r, m_clrGlow.g, m_clrGlow.b, 255 };

	PrecacheModel(szModel);
	Precache();
	SetModel(szModel);

	AddEffects(EF_NOSHADOW);
	SetMoveType(MOVETYPE_NONE);
	SetSolid(SOLID_BBOX);

	// Further more effects:
	if (m_bStartGlowing && m_bShowModel)
		SetGlowMode(m_iGlowType);
	else
		SetGlowMode(GLOW_MODE_NONE);

	if (!m_bShowModel)
		AddEffects(EF_NODRAW);

	if (m_bIsKeyPad)
	{
		if (szKeyPadCode == NULL_STRING)
		{
			Warning("BB2_PROP_BUTTON '%s' WITH KEY PAD MODE HAS NO KEY CODE SET\nRemoving!\n", STRING(GetEntityName()));
			UTIL_Remove(this);
			return;
		}

		if ((strlen(STRING(szKeyPadCode)) < 4) || (strlen(STRING(szKeyPadCode)) > 4))
		{
			Warning("BB2_PROP_BUTTON '%s' WITH KEYPAD MODE DOES NOT HAVE A CODE CONTAINING 4 DIGITS!\nRemoving!\n", STRING(GetEntityName()));
			UTIL_Remove(this);
			return;
		}

		char szCodeCheck[16];
		Q_strncpy(szCodeCheck, STRING(szKeyPadCode), 16);

		if ((szCodeCheck[0] >= 'a' && szCodeCheck[0] <= 'z') || (szCodeCheck[1] >= 'a' && szCodeCheck[1] <= 'z') || (szCodeCheck[2] >= 'a' && szCodeCheck[2] <= 'z') || (szCodeCheck[3] >= 'a' && szCodeCheck[3] <= 'z'))
		{
			Warning("BB2_PROP_BUTTON '%s' WITH KEYPAD MODE CANNOT CONTAIN ANY CHARACTERS BUT ONLY DIGITS!\nRemoving!\n", STRING(GetEntityName()));
			UTIL_Remove(this);
		}
	}
}

// Precache / Preload
void CBB2Button::Precache(void)
{
	if (GetModelName() == NULL_STRING)
		Warning("BB2_PROP_BUTTON '%s' has no model!\nRemoving!\n", STRING(GetEntityName()));
	else
	{
		PrecacheModel(STRING(GetModelName()));
		BaseClass::Precache();
	}
}

void CBB2Button::UpdateThink(void)
{
	AddEffects(EF_NODRAW);
	SetThink(NULL);
}

// Player clicked USE on the KEYPAD or watevah... :
void CBB2Button::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bShowModel || !m_bIsEnabled)
		return;

	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	CBasePlayer *pPlayer = ToHL2MPPlayer(pActivator);

	if ((ClassifyFor == 1) && (!pPlayer->IsHuman()))
		return;

	if ((ClassifyFor == 2) && (!pPlayer->IsZombie()))
		return;

	if (m_bIsKeyPad)
	{
		KeyValues *data = new KeyValues("data");
		data->SetString("code", STRING(szKeyPadCode));
		data->SetInt("entity", entindex());
		pPlayer->ShowViewPortPanel("keypad", true, data);
		data->deleteThis();
	}
	else
		m_OnUse.FireOutput(pActivator, this);
}

void CBB2Button::ShowModel(inputdata_t &inputdata)
{
	RemoveEffects(EF_NODRAW);
	m_bShowModel = true;
}

void CBB2Button::HideModel(inputdata_t &inputdata)
{
	SetGlowMode(GLOW_MODE_NONE);
	m_bShowModel = false;

	// Because EF_NODRAW gets called at the same frame/time as disabling the glow effect we must delay the EF_NODRAW to be called after glow is disabled or else glow effect will not be disabled at all!!!
	SetThink(&CBB2Button::UpdateThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CBB2Button::FireKeyPadOutput(CBasePlayer *pClient)
{
	m_OnUse.FireOutput(pClient, this);
	m_bIsEnabled = false;

	if (pClient)
		pClient->ShowViewPortPanel("keypad", false);
}

void CBB2Button::ShowGlow(inputdata_t &inputdata)
{
	SetGlowMode(m_iGlowType);
}

void CBB2Button::HideGlow(inputdata_t &inputdata)
{
	SetGlowMode(GLOW_MODE_NONE);
}

void CBB2Button::EnableButton(inputdata_t &inputdata)
{
	m_bIsEnabled = true;
}

void CBB2Button::DisableButton(inputdata_t &inputdata)
{
	m_bIsEnabled = false;
}

void CBB2Button::SetGlowType(inputdata_t &inputData)
{
	m_iGlowType = inputData.value.Int();
	if (GetGlowMode() != GLOW_MODE_NONE) // Glowing is enabled? Update glow then.
		SetGlowMode(m_iGlowType);
}
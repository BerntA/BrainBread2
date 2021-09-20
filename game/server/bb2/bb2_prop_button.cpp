//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread: Button with model ( dynamic ) for skin change. ++ more - It also has a KeyPad mode for more fancy interaction!
//
//========================================================================================//

#include "cbase.h"
#include "bb2_prop_button.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CPropButton)

// Think
DEFINE_THINKFUNC(UpdateThink),

// Out
DEFINE_OUTPUT(m_OnUse, "OnUse"),
DEFINE_OUTPUT(m_OnKeyPadSuccess, "OnKeyPadSuccess"),
DEFINE_OUTPUT(m_OnKeyPadFail, "OnKeyPadFail"),

// Keyfields
DEFINE_KEYFIELD(ClassifyFor, FIELD_INTEGER, "Filter"),
DEFINE_KEYFIELD(m_iGlowType, FIELD_INTEGER, "GlowType"),
DEFINE_KEYFIELD(m_bStartGlowing, FIELD_BOOLEAN, "EnableGlow"),
DEFINE_KEYFIELD(m_bShowModel, FIELD_BOOLEAN, "ShowModel"),
DEFINE_KEYFIELD(szKeyPadCode, FIELD_STRING, "KeyPadCode"),
DEFINE_KEYFIELD(m_bIsKeyPad, FIELD_BOOLEAN, "KeyPadMode"),
DEFINE_KEYFIELD(m_clrGlow, FIELD_COLOR32, "GlowOverlayColor"),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "ShowModel", ShowModel),
DEFINE_INPUTFUNC(FIELD_VOID, "HideModel", HideModel),
DEFINE_INPUTFUNC(FIELD_VOID, "ShowGlow", ShowGlow),
DEFINE_INPUTFUNC(FIELD_VOID, "HideGlow", HideGlow),
DEFINE_INPUTFUNC(FIELD_INTEGER, "SetGlowType", SetGlowType),

END_DATADESC()

LINK_ENTITY_TO_CLASS(bb2_prop_button, CPropButton);

CPropButton::CPropButton(void) : CBaseKeyPadEntity()
{
	ClassifyFor = 0;
	m_iGlowType = m_iOldGlowMode = GLOW_MODE_GLOBAL;
	m_clrGlow = { 255, 100, 100, 255 };
	m_bStartGlowing = false;
	m_bShowModel = true;
	m_bIsKeyPad = false;
}

void CPropButton::Spawn(void)
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

	if (m_bIsKeyPad && !IsKeyCodeValid(this))
	{
		Warning("Removing BB2_PROP_BUTTON!\n");
		UTIL_Remove(this);
		return;
	}

	SetBlocksLOS(false);
}

void CPropButton::Precache(void)
{
	if (GetModelName() == NULL_STRING)
		Warning("BB2_PROP_BUTTON '%s' has no model!\nRemoving!\n", STRING(GetEntityName()));
	else
	{
		PrecacheModel(STRING(GetModelName()));
		BaseClass::Precache();
	}
}

void CPropButton::UpdateThink(void)
{
	AddEffects(EF_NODRAW);
	SetThink(NULL);
}

void CPropButton::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bShowModel || m_bIsDisabled || !pActivator || !pActivator->IsPlayer())
		return;

	CBasePlayer *pPlayer = ToHL2MPPlayer(pActivator);
	if (!pPlayer)
		return;

	if ((ClassifyFor == 1) && (!pPlayer->IsHuman()))
		return;

	if ((ClassifyFor == 2) && (!pPlayer->IsZombie()))
		return;

	m_OnUse.FireOutput(pActivator, this);

	if (m_bIsKeyPad)
	{
		KeyValues *data = new KeyValues("data");
		data->SetInt("entity", entindex());
		pPlayer->ShowViewPortPanel("keypad", true, data);
		data->deleteThis();
	}
}

void CPropButton::ShowModel(inputdata_t &inputdata)
{
	RemoveEffects(EF_NODRAW);
	m_bShowModel = true;
}

void CPropButton::HideModel(inputdata_t &inputdata)
{
	SetGlowMode(GLOW_MODE_NONE);
	m_bShowModel = false;

	// Because EF_NODRAW gets called at the same frame/time as disabling the glow effect we must delay the EF_NODRAW to be called after glow is disabled or else glow effect will not be disabled at all!!!
	SetThink(&CPropButton::UpdateThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CPropButton::UnlockSuccess(CHL2MP_Player *pUnlocker)
{
	CBaseKeyPadEntity::UnlockSuccess(pUnlocker);
	m_bIsDisabled = true;
	pUnlocker->ShowViewPortPanel("keypad", false);
	m_OnKeyPadSuccess.FireOutput(pUnlocker, this);
}

void CPropButton::UnlockFail(CHL2MP_Player *pUnlocker)
{
	CBaseKeyPadEntity::UnlockFail(pUnlocker);

	m_OnKeyPadFail.FireOutput(pUnlocker, this);
}

void CPropButton::ShowGlow(inputdata_t &inputdata)
{
	SetGlowMode(m_iGlowType);
}

void CPropButton::HideGlow(inputdata_t &inputdata)
{
	SetGlowMode(GLOW_MODE_NONE);
}

void CPropButton::SetGlowType(inputdata_t &inputData)
{
	m_iGlowType = inputData.value.Int();
	if (GetGlowMode() != GLOW_MODE_NONE) // Glowing is enabled? Update glow then.
		SetGlowMode(m_iGlowType);
}
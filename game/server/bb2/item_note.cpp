//=========       Copyright � Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Note Item Object
//
//========================================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "items.h"
#include "hl2_player.h"
#include "hl2mp_gamerules.h"

class CItemNote : public CItem
{
public:
	DECLARE_CLASS(CItemNote, CItem);
	DECLARE_DATADESC();

	CItemNote()
	{
		color32 col32 = { 25, 235, 25, 235 };
		m_GlowColor = col32;
	}

	void Spawn(void);
	void Precache(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:

	string_t szFileName;
	string_t szHeader;

	COutputEvent m_OnUse;
};

LINK_ENTITY_TO_CLASS(item_note, CItemNote);
PRECACHE_REGISTER(item_note);

BEGIN_DATADESC(CItemNote)

DEFINE_KEYFIELD(szFileName, FIELD_STRING, "ScriptFile"),
DEFINE_KEYFIELD(szHeader, FIELD_STRING, "Header"),
DEFINE_OUTPUT(m_OnUse, "OnUse"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CItemNote::Spawn(void)
{
	Precache();
	SetModel("models/props/note.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

// Precache / Preload
void CItemNote::Precache(void)
{
	PrecacheModel("models/props/note.mdl");
}

// Player clicked USE on the item :
void CItemNote::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer())
		return;

	if (!pActivator->IsHuman())
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);
	if (pPlayer)
	{
		m_OnUse.FireOutput(this, this);

		CSingleUserRecipientFilter filter(pPlayer);
		filter.MakeReliable();
		UserMessageBegin(filter, "ShowNote");
		WRITE_STRING(STRING(szHeader));
		WRITE_STRING(STRING(szFileName));
		MessageEnd();
	}
}
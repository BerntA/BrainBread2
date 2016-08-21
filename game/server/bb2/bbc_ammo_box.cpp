//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: The good old BB Ammo Box - All you need!
//
//========================================================================================//

#include "cbase.h"
#include "items.h"
#include "in_buttons.h"
#include "gamerules.h"
#include "baseanimating.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "hl2mp_gamerules.h"
#include "player.h"
#include "ammodef.h"
#include "npcevent.h"
#include "eventlist.h"

class CBBCAmmoBox : public CItem
{
public:
	DECLARE_CLASS(CBBCAmmoBox, CItem);

	CBBCAmmoBox()
	{
		color32 col32 = { 10, 100, 150, 240 };
		m_GlowColor = col32;
	}

	bool CanBeUsed() { return false; }
	void Spawn();
	void Precache();
};

LINK_ENTITY_TO_CLASS(bbc_ammo_box, CBBCAmmoBox);
PRECACHE_REGISTER(bbc_ammo_box);

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CBBCAmmoBox::Spawn(void)
{
	Precache();
	SetModel("models/items/ammo_crate.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

// Precache / Preload
void CBBCAmmoBox::Precache(void)
{
	PrecacheModel("models/items/ammo_crate.mdl");
}
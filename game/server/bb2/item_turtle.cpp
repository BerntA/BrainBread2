//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Turtle Entity - Achievement - easter eggggg...
//
//========================================================================================//

#include "cbase.h"
#include "items.h"
#include "gamerules.h"
#include "hl2_player.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Server.h"

struct turtleEasterEgg_t
{
	EHANDLE m_hUseEnt;
	int turtleIndex;
};

static CUtlVector<turtleEasterEgg_t> m_pTurtleList;

bool IsPlayerInList(CBaseEntity *pEnt, int index)
{
	if (pEnt)
	{
		for (int i = 0; i < m_pTurtleList.Count(); i++)
		{
			CBaseEntity *entity = m_pTurtleList[i].m_hUseEnt.Get();
			if (!entity)
				continue;

			if ((entity == pEnt) && (m_pTurtleList[i].turtleIndex == index))
				return true;
		}
	}

	return false;
}

void CheckCanGetTurtleLove(CBasePlayer *pPlayer)
{
	if (!pPlayer)
		return;

	int turtlesViolated = 0;
	for (int i = 0; i < m_pTurtleList.Count(); i++)
	{
		CBaseEntity *entity = m_pTurtleList[i].m_hUseEnt.Get();
		if (!entity)
			continue;

		if (entity != pPlayer)
			continue;

		turtlesViolated++;
	}

	if (turtlesViolated >= 10)
		GameBaseServer()->SendAchievement("ACH_SECRET_TURTLE", pPlayer->entindex());
}

void CleanupTurtleList(int index)
{
	for (int i = (m_pTurtleList.Count() - 1); i >= 0; i--)
	{
		if (m_pTurtleList[i].turtleIndex == index)
			m_pTurtleList.Remove(i);
	}
}

class CTurtleThing : public CItem
{
public:
	DECLARE_CLASS(CTurtleThing, CItem);
	DECLARE_DATADESC();

	CTurtleThing()
	{
		color32 col32 = { 100, 100, 175, 255 };
		m_GlowColor = col32;
		m_iTurtleIndex = 0;
	}

	virtual ~CTurtleThing()
	{
		CleanupTurtleList(m_iTurtleIndex);
	}

	void Spawn();
	void Precache();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
	int m_iTurtleIndex;
};

BEGIN_DATADESC(CTurtleThing)
DEFINE_KEYFIELD(m_iTurtleIndex, FIELD_INTEGER, "TurtleIndex"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(item_turtle, CTurtleThing);

void CTurtleThing::Spawn(void)
{
	Precache();
	SetModel("models/shared_props/turtle.mdl");
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();
}

void CTurtleThing::Precache(void)
{
	PrecacheModel("models/shared_props/turtle.mdl");
	PrecacheScriptSound("Turtle.Enjoy");
}

void CTurtleThing::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);
	if (!pPlayer || !pPlayer->IsHuman(true))
		return;

	if (!IsPlayerInList(pPlayer, m_iTurtleIndex))
	{
		CSingleUserRecipientFilter filter(pPlayer);
		pPlayer->EmitSound(filter, pPlayer->entindex(), "Turtle.Enjoy");

		if (HL2MPRules() && !strcmp(HL2MPRules()->szCurrentMap, "bbc_termoil"))
		{
			turtleEasterEgg_t item;
			item.turtleIndex = m_iTurtleIndex;
			item.m_hUseEnt = pPlayer;
			m_pTurtleList.AddToTail(item);
			CheckCanGetTurtleLove(pPlayer);
		}
	}
}
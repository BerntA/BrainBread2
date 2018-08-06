//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Inventory Item Object
//
//========================================================================================//

#include "cbase.h"
#include "inventory_item.h"
#include "hl2mp_player.h"
#include "GameBase_Server.h"
#include "GameBase_Shared.h"
#include "objective_icon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CInventoryItem)
DEFINE_KEYFIELD(m_iItemID, FIELD_INTEGER, "ItemID"),
DEFINE_KEYFIELD(m_bIsMapItem, FIELD_BOOLEAN, "MapItem"),
DEFINE_KEYFIELD(m_bExcludeFromInventory, FIELD_BOOLEAN, "ExcludeItem"),
DEFINE_FIELD(m_bHasDoneLateUpdate, FIELD_BOOLEAN),
DEFINE_OUTPUT(m_OnUse, "OnUse"),
END_DATADESC()

LINK_ENTITY_TO_CLASS(inventory_item, CInventoryItem);

CInventoryItem::CInventoryItem()
{
	color32 col32 = { 20, 100, 150, 240 };
	m_GlowColor = col32;

	m_iItemID = 0;
	m_bIsMapItem = false;
	m_pObjIcon = NULL;
	m_bExcludeFromInventory = false;
	m_bHasDoneLateUpdate = false;
	m_pData = NULL;

	ListenForGameEvent("reload_game_data");
}

CInventoryItem::~CInventoryItem()
{
	CBaseEntity *pEnt = m_pObjIcon.Get();
	if (pEnt)
	{
		UTIL_Remove(pEnt);
		m_pObjIcon = NULL;
	}
}

void CInventoryItem::Spawn()
{
	Precache();
	if ((m_pData == NULL) && (SetItem(m_iItemID, m_bIsMapItem) == false))
		return;

	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();

	if (m_pData->bGlobalGlow)
	{
		Color glowColor = m_pData->clGlowColor;
		color32 col32 = { (byte)glowColor.r(), (byte)glowColor.g(), (byte)glowColor.b(), (byte)glowColor.a() };
		m_GlowColor = col32;
		SetGlowMode(GLOW_MODE_GLOBAL);
	}

	bool bRenderObjIcon = m_pData->bEnableObjectiveIcon;
	if (bRenderObjIcon)
	{
		CObjectiveIcon *pObjIcon = (CObjectiveIcon*)CreateEntityByName("objective_icon");
		if (pObjIcon)
		{
			const char *szTexture = m_pData->szObjectiveIconTexture;

			Vector vecOrigin = this->GetAbsOrigin();
			vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

			pObjIcon->SetAbsOrigin(vecOrigin);
			pObjIcon->SetAbsAngles(this->GetAbsAngles());
			pObjIcon->SetObjectiveIconTexture(szTexture, true);
			pObjIcon->Spawn();
			m_pObjIcon = pObjIcon;
		}
	}

	m_nSkin = m_pData->iSkin;
	if (m_pData->flScale != 1.0f)
		SetModelScale(m_pData->flScale);
	SetLocalAngles(m_pData->angOffset);

	EnableRotationEffect();

	AddSolidFlags(FSOLID_TRIGGER);
}

void CInventoryItem::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CHL2MP_Player *pPlayer = GetHumanInteractor(pActivator);
	if (!pPlayer)
		return;

	CSingleUserRecipientFilter filter(pPlayer);

	bool bAutoConsume = m_pData->bAutoConsume;
	if (!bAutoConsume && !m_bExcludeFromInventory && (GameBaseShared()->GetInventoryItemCountForPlayer(pActivator->entindex()) >= MAX_INVENTORY_ITEM_COUNT))
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_INVENTORY_FULL", 0, pActivator->entindex());
		EmitSound(filter, pPlayer->entindex(), m_pData->szSoundScriptFailure);
		return;
	}

	if (!bAutoConsume)
	{
		if (!m_bExcludeFromInventory)
		{
			// We have enough space!! Get it before it is 2 late!
			GameBaseShared()->AddInventoryItem(pActivator->entindex(), m_pData, m_bIsMapItem);
		}
	}
	else
	{
		if (!GameBaseShared()->UseInventoryItem(pActivator->entindex(), m_iItemID, m_bIsMapItem, true))
		{
			EmitSound(filter, pPlayer->entindex(), m_pData->szSoundScriptFailure);
			return;
		}
	}

	EmitSound(filter, pPlayer->entindex(), m_pData->szSoundScriptSuccess);
	m_OnUse.FireOutput(this, this);
	UTIL_Remove(this);
}

void CInventoryItem::DelayedUse(CBaseEntity *pActivator)
{
	bool bAutoConsume = m_pData->bAutoConsume;
	if (!bAutoConsume)
		return;

	CHL2MP_Player *pPlayer = GetHumanInteractor(pActivator);
	if (!pPlayer)
		return;

	CSingleUserRecipientFilter filter(pPlayer);

	if (!GameBaseShared()->UseInventoryItem(pActivator->entindex(), m_iItemID, m_bIsMapItem, true, true))
	{
		EmitSound(filter, pPlayer->entindex(), m_pData->szSoundScriptFailure);
		return;
	}

	EmitSound(filter, pPlayer->entindex(), m_pData->szSoundScriptSuccess);
	m_OnUse.FireOutput(this, this);
	UTIL_Remove(this);
}

CHL2MP_Player *CInventoryItem::GetHumanInteractor(CBaseEntity *pActivator)
{
	if (!pActivator || !pActivator->IsPlayer() || !pActivator->IsHuman())
		return NULL;

	if (!HL2MPRules()->m_bRoundStarted && HL2MPRules()->ShouldHideHUDDuringRoundWait())
		return NULL;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pActivator);
	if (!pPlayer)
		return NULL;

	if (pPlayer->IsPlayerInfected())
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_ITEM_DENY_INFECTION", 0, pActivator->entindex());
		return NULL;
	}

	int iRequiredLevel = m_pData->iLevelReq;
	if (pPlayer->GetPlayerLevel() < iRequiredLevel)
	{
		char pchArg1[16];
		Q_snprintf(pchArg1, 16, "%i", iRequiredLevel);
		GameBaseServer()->SendToolTip("#TOOLTIP_ITEM_DENY_LEVEL", 0, pActivator->entindex(), pchArg1);
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, pPlayer->entindex(), m_pData->szSoundScriptFailure);
		return NULL;
	}

	return pPlayer;
}

bool CInventoryItem::SetItem(const DataInventoryItem_Base_t &data, bool bMapItem)
{
	m_pData = &data;
	m_iItemID = data.iItemID;
	m_bIsMapItem = bMapItem;

	SetModel(data.szModelPath);
	return true;
}

bool CInventoryItem::SetItem(uint itemID, bool bMapItem)
{
	if (itemID <= 0)
	{
		Warning("Item '%s' has an invalid itemID, the ID must be greater than 0!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return false;
	}

	const DataInventoryItem_Base_t *data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(itemID, bMapItem);
	if (data == NULL)
	{
		Warning("Item '%s' doesn't exist!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return false;
	}

	return SetItem((*data), bMapItem);
}

void CInventoryItem::OnRotationEffect(void)
{
	BaseClass::OnRotationEffect();

	if ((EnablePhysics() == false) && m_bHasDoneLateUpdate)
		return;

	UpdateObjectiveIconPosition(GetAbsOrigin());
	m_bHasDoneLateUpdate = true;
}

void CInventoryItem::Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity)
{
	BaseClass::Teleport(newPosition, newAngles, newVelocity);

	UpdateObjectiveIconPosition(GetAbsOrigin());
}

void CInventoryItem::UpdateObjectiveIconPosition(const Vector &pos)
{
	CBaseEntity *pEnt = m_pObjIcon.Get();
	if (pEnt)
	{
		Vector vecOrigin = pos;
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;
		pEnt->SetAbsOrigin(vecOrigin);
	}
}

void CInventoryItem::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (!strcmp(type, "reload_game_data"))
	{
		m_pData = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(m_iItemID, m_bIsMapItem);
		if (m_pData == NULL)
		{
			Warning("Item (%u) '%s' doesn't exist!\nRemoving!\n", m_iItemID, STRING(GetEntityName()));
			UTIL_Remove(this);
			return;
		}
	}
}

CON_COMMAND_F(bb2_create_inventory_item, "Create some inventory item.", FCVAR_CHEAT)
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
		return;

	if (args.ArgC() < 2)
	{
		Warning("This command requires at least 2 arguments!\n");
		return;
	}

	bool bMapItem = false;
	if (args.ArgC() >= 3)
		bMapItem = (atoi(args[2]) >= 1);

	int iItemID = atoi(args[1]);
	if (!iItemID)
	{
		Warning("The itemID must be 1 or greater!\n");
		return;
	}

	const DataInventoryItem_Base_t *data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(iItemID, bMapItem);
	if (data == NULL)
	{
		Warning("This item doesn't exist!\nInvalid itemID!\n");
		return;
	}

	CInventoryItem *pItem = (CInventoryItem *)CreateEntityByName("inventory_item");
	if (pItem)
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

		pItem->SetAbsOrigin(vecOrigin);
		pItem->SetItem((*data), bMapItem);
		pItem->Spawn();
		UTIL_DropToFloor(pItem, MASK_SOLID_BRUSHONLY, pPlayer);
	}
}
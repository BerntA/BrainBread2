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
DEFINE_KEYFIELD(szModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(m_iItemID, FIELD_INTEGER, "ItemID"),
DEFINE_KEYFIELD(szEntityLink, FIELD_STRING, "EntityLink"),
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
	szEntityLink = NULL_STRING;
	m_pObjIcon = NULL;
	m_bExcludeFromInventory = false;
	m_bHasDoneLateUpdate = false;
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
	SetModel(STRING(szModel));

	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();

	// All inventory items bust have an itemID that is 1 or greater.
	if (m_iItemID <= 0)
	{
		Warning("Item '%s' has an invalid itemID, the ID must be greater than 0!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	const DataInventoryItem_Base_t *data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(m_iItemID, m_bIsMapItem);
	if (data == NULL)
	{
		Warning("Item '%s' doesn't exist!\nRemoving!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	if (data->bGlobalGlow)
	{
		Color glowColor = data->clGlowColor;
		color32 col32 = { (byte)glowColor.r(), (byte)glowColor.g(), (byte)glowColor.b(), (byte)glowColor.a() };
		m_GlowColor = col32;
		SetGlowMode(GLOW_MODE_GLOBAL);
	}

	bool bRenderObjIcon = data->bEnableObjectiveIcon;
	if (bRenderObjIcon)
	{
		CObjectiveIcon *pObjIcon = (CObjectiveIcon*)CreateEntityByName("objective_icon");
		if (pObjIcon)
		{
			const char *szTexture = data->szObjectiveIconTexture;

			Vector vecOrigin = this->GetAbsOrigin();
			vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;

			pObjIcon->SetAbsOrigin(vecOrigin);
			pObjIcon->SetAbsAngles(this->GetAbsAngles());
			pObjIcon->SetObjectiveIconTexture(szTexture, true);
			pObjIcon->Spawn();
			m_pObjIcon = pObjIcon;
		}
	}

	m_nSkin = data->iSkin;

	EnableRotationEffect();

	AddSolidFlags(FSOLID_TRIGGER);
}

void CInventoryItem::Precache(void)
{
	PrecacheModel(STRING(szModel));
}

void CInventoryItem::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer() || !pActivator->IsHuman())
		return;

	if (!HL2MPRules()->m_bRoundStarted && HL2MPRules()->ShouldHideHUDDuringRoundWait())
		return;

	const DataInventoryItem_Base_t *data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(m_iItemID, m_bIsMapItem);
	if (data == NULL)
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pActivator);
	if (!pPlayer)
		return;

	if (pPlayer->IsPlayerInfected())
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_ITEM_DENY_INFECTION", 0, pActivator->entindex());
		return;
	}

	CSingleUserRecipientFilter filter(pPlayer);

	bool bAutoConsume = data->bAutoConsume;
	if (!bAutoConsume && !m_bExcludeFromInventory && (GameBaseShared()->GetInventoryItemCountForPlayer(pActivator->entindex()) >= MAX_INVENTORY_ITEM_COUNT))
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_INVENTORY_FULL", 0, pActivator->entindex());
		EmitSound(filter, pPlayer->entindex(), data->szSoundScriptFailure);
		return;
	}

	const char *szLink = (szEntityLink == NULL_STRING) ? "" : STRING(szEntityLink);

	int iRequiredLevel = data->iLevelReq;
	if (pPlayer->GetPlayerLevel() < iRequiredLevel)
	{
		char pchArg1[16];
		Q_snprintf(pchArg1, 16, "%i", iRequiredLevel);
		GameBaseServer()->SendToolTip("#TOOLTIP_ITEM_DENY_LEVEL", 0, pActivator->entindex(), pchArg1);
		EmitSound(filter, pPlayer->entindex(), data->szSoundScriptFailure);
		return;
	}

	if (!bAutoConsume)
	{
		if (!m_bExcludeFromInventory)
		{
			// We have enough space!! Get it before it is 2 late!
			GameBaseShared()->AddInventoryItem(pActivator->entindex(), m_iItemID, szLink, m_bIsMapItem);
		}
	}
	else
	{
		if (!GameBaseShared()->UseInventoryItem(pActivator->entindex(), m_iItemID, m_bIsMapItem, true))
		{
			EmitSound(filter, pPlayer->entindex(), data->szSoundScriptFailure);
			return;
		}
	}

	EmitSound(filter, pPlayer->entindex(), data->szSoundScriptSuccess);
	m_OnUse.FireOutput(this, this);
	UTIL_Remove(this);
}

void CInventoryItem::DelayedUse(CBaseEntity *pActivator)
{
	if (!pActivator)
		return;

	if (!pActivator->IsPlayer() || !pActivator->IsHuman())
		return;

	if (!HL2MPRules()->m_bRoundStarted && HL2MPRules()->ShouldHideHUDDuringRoundWait())
		return;

	const DataInventoryItem_Base_t *data = GameBaseShared()->GetSharedGameDetails()->GetInventoryData(m_iItemID, m_bIsMapItem);
	if (data == NULL)
		return;

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(pActivator);
	if (!pPlayer)
		return;

	if (pPlayer->IsPlayerInfected())
	{
		GameBaseServer()->SendToolTip("#TOOLTIP_ITEM_DENY_INFECTION", 0, pActivator->entindex());
		return;
	}

	bool bAutoConsume = data->bAutoConsume;
	if (!bAutoConsume)
		return;

	CSingleUserRecipientFilter filter(pPlayer);

	int iRequiredLevel = data->iLevelReq;
	if (pPlayer->GetPlayerLevel() < iRequiredLevel)
	{
		char pchArg1[16];
		Q_snprintf(pchArg1, 16, "%i", iRequiredLevel);
		GameBaseServer()->SendToolTip("#TOOLTIP_ITEM_DENY_LEVEL", 0, pActivator->entindex(), pchArg1);
		EmitSound(filter, pPlayer->entindex(), data->szSoundScriptFailure);
		return;
	}

	if (!GameBaseShared()->UseInventoryItem(pActivator->entindex(), m_iItemID, m_bIsMapItem, true, true))
	{
		EmitSound(filter, pPlayer->entindex(), data->szSoundScriptFailure);
		return;
	}

	EmitSound(filter, pPlayer->entindex(), data->szSoundScriptSuccess);
	m_OnUse.FireOutput(this, this);
	UTIL_Remove(this);
}

void CInventoryItem::SetItem(const char *model, uint iID, const char *entityLink, bool bMapItem)
{
	szModel = FindPooledString(model);
	if (szModel == NULL_STRING)
		szModel = AllocPooledString(model);

	m_iItemID = iID;
	m_bIsMapItem = bMapItem;

	if (entityLink != NULL && strlen(entityLink) > 0)
	{
		szEntityLink = FindPooledString(entityLink);
		if (szEntityLink == NULL_STRING)
			szEntityLink = AllocPooledString(entityLink);
	}
	else
		szEntityLink = NULL_STRING;
}

void CInventoryItem::OnRotationEffect(void)
{
	BaseClass::OnRotationEffect();

	if ((EnablePhysics() == false) && m_bHasDoneLateUpdate)
		return;

	CBaseEntity *pEnt = m_pObjIcon.Get();
	if (pEnt)
	{
		Vector vecOrigin = this->GetAbsOrigin();
		vecOrigin.z += OBJECTIVE_ICON_EXTRA_HEIGHT;
		pEnt->SetAbsOrigin(vecOrigin);
	}

	m_bHasDoneLateUpdate = true;
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
		pItem->SetAbsAngles(QAngle(0, 0, 0));
		pItem->SetItem(data->szModelPath, iItemID, NULL, bMapItem);
		pItem->Spawn();
		UTIL_DropToFloor(pItem, MASK_SOLID_BRUSHONLY, pPlayer);
	}
}
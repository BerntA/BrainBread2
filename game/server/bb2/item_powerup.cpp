//=========       Copyright © Reperio Studios 2016 @ Bernt Andreas Eide!       ============//
//
// Purpose: Powerup Item
//
//========================================================================================//

#include "cbase.h"
#include "gamerules.h"
#include "items.h"
#include "hl2_player.h"
#include "hl2mp_gamerules.h"
#include "GameBase_Shared.h"

class CItemPowerup : public CItem
{
public:
	DECLARE_CLASS(CItemPowerup, CItem);
	DECLARE_DATADESC();

	CItemPowerup();

	void Spawn(void);
	bool MyTouch(CBasePlayer *pPlayer);
	void SetParam(const char *param);
	void SetParam(float param);

private:

	float m_flPowerUpDuration;
	string_t czPowerupName;
};

LINK_ENTITY_TO_CLASS(item_powerup, CItemPowerup);

BEGIN_DATADESC(CItemPowerup)
DEFINE_KEYFIELD(czPowerupName, FIELD_STRING, "PowerupName"),
END_DATADESC()

CItemPowerup::CItemPowerup()
{
	color32 col32 = { 15, 255, 15, 245 };
	m_GlowColor = col32;
	czPowerupName = NULL_STRING;
	m_flPowerUpDuration = 0.0f;
}

void CItemPowerup::Spawn(void)
{
	if (!HL2MPRules()->IsPowerupsAllowed())
	{
		Warning("Powerups is not available in this gamemode!\n");
		UTIL_Remove(this);
		return;
	}

	if (czPowerupName == NULL_STRING)
	{
		Warning("Powerup '%s' with no powerup name, removing!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(STRING(czPowerupName));
	if (data == NULL)
	{
		Warning("Powerup '%s' is invalid, removing!\n", STRING(GetEntityName()));
		UTIL_Remove(this);
		return;
	}

	SetModel(data->pchModelPath);
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	BaseClass::Spawn();

	EnableRotationEffect();

	AddSpawnFlags(SF_NORESPAWN);

	m_flPowerUpDuration = data->flPerkDuration;
}

bool CItemPowerup::MyTouch(CBasePlayer *pPlayer)
{
	CHL2MP_Player *pClient = ToHL2MPPlayer(pPlayer);
	if (!pClient)
		return false;

	const DataPlayerItem_Player_PowerupItem_t *data = GameBaseShared()->GetSharedGameDetails()->GetPlayerPowerupData(STRING(czPowerupName));
	if (!data)
		return false;

	if (!pClient->CanEnablePowerup(data->iFlag, m_flPowerUpDuration))
		return false;

	Vector vecOrigin = pClient->GetAbsOrigin();
	CRecipientFilter filter;
	filter.AddRecipientsByPAS(vecOrigin);
	EmitSound(filter, entindex(), data->pchActivationSoundScript);
	return true;
}

void CItemPowerup::SetParam(const char *param)
{
	czPowerupName = FindPooledString(param);
	if (czPowerupName == NULL_STRING)
		czPowerupName = AllocPooledString(param);
}

void CItemPowerup::SetParam(float param)
{
	m_flPowerUpDuration = param;
}

CON_COMMAND_F(bb2_create_powerup_item, "Create some powerup item.", FCVAR_CHEAT)
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
		return;

	if (args.ArgC() != 2)
	{
		Warning("This command requires 2 arguments!\n");
		return;
	}

	const char *powerupName = args[1];
	if (!powerupName || !powerupName[0])
		return;

	int indexPowerup = GameBaseShared()->GetSharedGameDetails()->GetIndexForPowerup(powerupName);
	if (indexPowerup == -1)
	{
		Warning("Invalid powerup name!\n");
		return;
	}

	CItem *pPowerupEnt = (CItem*)CreateEntityByName("item_powerup");
	if (pPowerupEnt)
	{
		Vector vecOrigin = pPlayer->GetAbsOrigin();
		vecOrigin.z += 30.0f;
		pPowerupEnt->SetParam(powerupName);
		pPowerupEnt->SetAbsOrigin(vecOrigin);
		pPowerupEnt->SetAbsAngles(QAngle(0, 0, 0));
		pPowerupEnt->Spawn();
		UTIL_DropToFloor(pPowerupEnt, MASK_SOLID_BRUSHONLY, pPlayer);
	}
}
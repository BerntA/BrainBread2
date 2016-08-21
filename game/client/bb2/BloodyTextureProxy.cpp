//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Bloody Hands Shader Effect - Proxy!
//
//========================================================================================//

#include "cbase.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialproxy.h"
#include "baseviewmodel_shared.h"
#include "c_hl2mp_player.h"
#include "toolframework_client.h"
#include "GameEventListener.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_BloodyTextureProxy : public IMaterialProxy, public CGameEventListener
{
public:

	C_BloodyTextureProxy();
	virtual ~C_BloodyTextureProxy();

	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);
	C_BaseEntity *BindArgToEntity(void *pArg);
	virtual void OnBind(void *pC_BaseEntity);
	virtual void Release(void) { delete this; }
	IMaterial *GetMaterial(void);

protected:
	virtual void FireGameEvent(IGameEvent *event);

private:

	IMaterialVar* blendFactor;
};

C_BloodyTextureProxy::C_BloodyTextureProxy()
{
	blendFactor = NULL;
	ListenForGameEvent("round_start");
	ListenForGameEvent("death_notice");
}

C_BloodyTextureProxy::~C_BloodyTextureProxy()
{
}

void C_BloodyTextureProxy::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();

	if (!strcmp(type, "round_start"))
	{
		blendFactor->SetFloatValue(0.0f);
	}
	else if (!strcmp(type, "death_notice"))
	{
		int victimID = event->GetInt("victimID");
		if (victimID == GetLocalPlayerIndex())
			blendFactor->SetFloatValue(0.0f);
	}
}

bool C_BloodyTextureProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	bool found;

	blendFactor = pMaterial->FindVar("$detailblendfactor", &found, false);
	if (!found)
		return false;

	return true;
}

C_BaseEntity *C_BloodyTextureProxy::BindArgToEntity(void *pArg)
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;
	return pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
}

void C_BloodyTextureProxy::OnBind(void* pC_BaseEntity)
{
	if (!pC_BaseEntity)
		return;

	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	C_BaseEntity *pEntity = BindArgToEntity(pC_BaseEntity);

	bool bShouldDrawBlood = false;

	C_BaseViewModel *pViewModel = dynamic_cast<C_BaseViewModel*> (pEntity);
	if (pViewModel)
	{
		C_HL2MP_Player *pPlayer = ToHL2MPPlayer(pViewModel->GetOwner());
		if (pPlayer)
		{
			if ((pPlayer == pLocal) || (GetSpectatorTarget() == pPlayer->entindex()))
			{
				if (pViewModel->IsPlayerHands())
				{
					for (int i = 0; i < 4; i++)
					{
						C_BaseCombatWeapon *pWeapon = pPlayer->GetAllWeapons(i);
						if (!pWeapon)
							continue;

						if (pWeapon->m_bIsBloody)
						{
							bShouldDrawBlood = true;
							break;
						}
					}
				}
				else
				{
					C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
					if (pWeapon)
					{
						if (pWeapon->m_bIsBloody)
							bShouldDrawBlood = true;
						else
						{
							bShouldDrawBlood = false;
							blendFactor->SetFloatValue(0.0f);
						}
					}
				}
			}
		}
	}

	if (bShouldDrawBlood)
		blendFactor->SetFloatValue(1.0f);

	if (ToolsEnabled())
		ToolFramework_RecordMaterialParams(GetMaterial());
}

IMaterial *C_BloodyTextureProxy::GetMaterial()
{
	return blendFactor->GetOwningMaterial();
}

EXPOSE_INTERFACE(C_BloodyTextureProxy, IMaterialProxy, "BloodyTexture" IMATERIAL_PROXY_INTERFACE_VERSION);
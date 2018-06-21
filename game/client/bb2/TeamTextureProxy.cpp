//=========       Copyright © Reperio Studios 2013-2019 @ Bernt Andreas Eide!       ============//
//
// Purpose: A simple team based texture proxy, hide for non team members!
//
//=============================================================================================//

#include "cbase.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialproxy.h"
#include "baseviewmodel_shared.h"
#include "c_hl2mp_player.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_TeamTextureProxy : public IMaterialProxy
{
public:
	C_TeamTextureProxy();
	virtual ~C_TeamTextureProxy();

	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);
	virtual void OnBind(void *pC_BaseEntity);
	virtual void Release(void) { delete this; }
	virtual IMaterial *GetMaterial(void);

private:
	IMaterialVar *alpha;
	int team;
};

C_TeamTextureProxy::C_TeamTextureProxy()
{
	alpha = NULL;
	team = TEAM_UNASSIGNED;
}

C_TeamTextureProxy::~C_TeamTextureProxy()
{
}

bool C_TeamTextureProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	bool found;

	alpha = pMaterial->FindVar("$alpha", &found, false);
	if (!found)
	{
		alpha = NULL;
		return false;
	}

	team = pKeyValues->GetInt("team");
	return true;
}

void C_TeamTextureProxy::OnBind(void* pC_BaseEntity)
{
	if (alpha == NULL)
		return;

	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pLocal)
		return;

	alpha->SetFloatValue(((pLocal->GetTeamNumber() == team) ? 1.0f : 0.0f));

	if (ToolsEnabled())
		ToolFramework_RecordMaterialParams(GetMaterial());
}

IMaterial *C_TeamTextureProxy::GetMaterial()
{
	if (alpha == NULL)
		return NULL;

	return alpha->GetOwningMaterial();
}

EXPOSE_INTERFACE(C_TeamTextureProxy, IMaterialProxy, "TeamLinkTexture" IMATERIAL_PROXY_INTERFACE_VERSION);
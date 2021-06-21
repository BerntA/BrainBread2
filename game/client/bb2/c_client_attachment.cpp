//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Attachments is used for player models. Such as attaching a helmet, weapon, etc to the player model ( thirdperson model ).
//
//========================================================================================//

#include "cbase.h"
#include "c_client_attachment.h"
#include "c_hl2mp_player.h"
#include "hl2mp_gamerules.h"
#include "physpropclientside.h"
#include "vcollide_parse.h"
#include "mapentities_shared.h"
#include "gamestringpool.h"
#include "GameBase_Client.h"
#include "props_shared.h"
#include "c_te_effect_dispatch.h"
#include "datacache/imdlcache.h"
#include "GameBase_Shared.h"
#include "weapon_hl2mpbase.h"
#include "iinput.h"
#include "c_playerresource.h"
#include "c_playermodel.h"
#include "model_types.h"
#include "GlobalRenderEffects.h"
#include "view.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar bb2_render_weapon_attachments("bb2_render_weapon_attachments", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Enable/Disable weapon attachment rendering.");

ConVar attachment_debugging("attachment_debugging", "0", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Allow forcing the position and angles of the weapon attachment(back)");
ConVar attachment_weapon_back_forward("attachment_weapon_back_forward", "0", FCVAR_CLIENTDLL, "Find the proper position for your weapon on the back.");
ConVar attachment_weapon_back_right("attachment_weapon_back_right", "0", FCVAR_CLIENTDLL, "Find the proper position for your weapon on the back.");
ConVar attachment_weapon_back_up("attachment_weapon_back_up", "0", FCVAR_CLIENTDLL, "Find the proper position for your weapon on the back.");
ConVar attachment_weapon_back_pitch("attachment_weapon_back_pitch", "0", FCVAR_CLIENTDLL, "Find the proper position for your weapon on the back.");
ConVar attachment_weapon_back_yaw("attachment_weapon_back_yaw", "0", FCVAR_CLIENTDLL, "Find the proper position for your weapon on the back.");
ConVar attachment_weapon_back_roll("attachment_weapon_back_roll", "0", FCVAR_CLIENTDLL, "Find the proper position for your weapon on the back.");

extern ConVar r_propsmaxdist;
#define UPDATE_TIME 0.5f // How often should we check if our equipment index has changed?

static CUtlVector<C_ClientAttachment*> s_ClientAttachmentList;

bool RemoveAllClientAttachments()
{
	if (!s_ClientAttachmentList.Count())
		return false;

	while (s_ClientAttachmentList.Count() > 0)
	{
		C_ClientAttachment *p = s_ClientAttachmentList[0];
		p->Release();
	}

	return true;
}

C_ClientAttachment *CreateClientAttachment(C_HL2MP_Player *pParent, int type, int param, bool bonemerge)
{
	if (!pParent)
		return NULL;

	C_ClientAttachment *pAttachment = new C_ClientAttachment();
	if (!pAttachment->Initialize(type, param))
	{
		pAttachment->Release();
		return NULL;
	}

	pAttachment->m_nBody = 0;
	pAttachment->m_nSkin = 0;
	pAttachment->SetEffects(0);
	pAttachment->SetOwnerEntity(pParent);
	pAttachment->SetPlayerLink(pParent);

	if (type != CLIENT_ATTACHMENT_WEAPON)
		pAttachment->FollowEntity(pParent, bonemerge);

	return pAttachment;
}

C_ClientAttachment::C_ClientAttachment(void)
{
	m_iAttachmentType = m_iParameter = 0;
	m_flUpdateTime = 0.0f;
	m_pPlayer = NULL;
	m_pOther = NULL;
	m_bShouldDelete = m_bNoModelParticles = m_bShouldHide = false;
	m_bNeedsUpdate = true;
	m_takedamage = DAMAGE_EVENTS_ONLY;

	s_ClientAttachmentList.AddToTail(this);
}

C_ClientAttachment::~C_ClientAttachment()
{
	s_ClientAttachmentList.FindAndRemove(this);
}

bool C_ClientAttachment::Initialize(int type, int param)
{
	if (InitializeAsClientEntity(STRING(GetModelName()), RENDER_GROUP_OPAQUE_ENTITY) == false)
		return false;

	m_iAttachmentType = type;
	m_iParameter = param;

	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);

	// start fading out at 75% of r_propsmaxdist
	m_fadeMaxDist = r_propsmaxdist.GetFloat();
	m_fadeMinDist = r_propsmaxdist.GetFloat() * 0.75f;

	UpdatePartitionListEntry();
	CollisionProp()->UpdatePartition();
	SetBlocksLOS(false);
	UpdateVisibility();
	SetNextClientThink(CLIENT_THINK_ALWAYS);
	return true;
}

void C_ClientAttachment::Release(void)
{
	if (GetMoveParent())
		FollowEntity(NULL);

	BaseClass::Release();
}

void C_ClientAttachment::ClientThink(void)
{
	if (m_bShouldDelete == true)
	{
		Release();
		return;
	}

	if (!bb2_render_weapon_attachments.GetBool() && !attachment_debugging.GetBool())
		return;

	if (m_flUpdateTime < gpGlobals->curtime)
	{
		m_flUpdateTime = gpGlobals->curtime + UPDATE_TIME;
		C_HL2MP_Player *pPlayer = ToHL2MPPlayer(GetPlayerLink());
		if (pPlayer && g_PR)
		{
			int teamNum = g_PR->GetTeam(pPlayer->entindex());
			bool bAlive = g_PR->IsAlive(pPlayer->entindex());

			switch (m_iAttachmentType)
			{

			case CLIENT_ATTACHMENT_WEAPON:
			{
				int index = GetOtherLinkEntindex();
				C_BaseCombatWeapon *pWeapon = pPlayer->Weapon_GetBySlot(m_iParameter);
				m_bShouldHide = (pWeapon == NULL || (pWeapon == pPlayer->GetActiveWeapon()) || !bAlive || (teamNum != TEAM_HUMANS));
				if (pWeapon && !pWeapon->VisibleInWeaponSelection())
					m_bShouldHide = true;

				if (pWeapon && ((pWeapon->entindex() != index) || m_bNeedsUpdate))
				{
					StopFollowingEntity();
					m_bNeedsUpdate = false;
					m_pOther = pWeapon;
					SetModel(pWeapon->GetWorldModel());
					AttachmentFollow(pPlayer, false, GetOverridenParentEntity(pPlayer)->GetBaseAnimating()->LookupAttachment(pWeapon->GetAttachmentLink()));
				}

				if (!m_bShouldHide)
				{
					if (attachment_debugging.GetBool())
					{
						SetLocalOrigin(Vector(attachment_weapon_back_forward.GetFloat(), attachment_weapon_back_right.GetFloat(), attachment_weapon_back_up.GetFloat()));
						SetLocalAngles(QAngle(attachment_weapon_back_pitch.GetFloat(), attachment_weapon_back_yaw.GetFloat(), attachment_weapon_back_roll.GetFloat()));
					}
					else
					{
						SetLocalOrigin(pWeapon->GetAttachmentPositionOffset());
						SetLocalAngles(pWeapon->GetAttachmentAngleOffset());
					}
				}

				break;
			}

			}
		}
	}
}

void C_ClientAttachment::PerformUpdateCheck(void)
{
	m_bNeedsUpdate = true;
	UpdateVisibility();
}

bool C_ClientAttachment::IsDormant(void)
{
	if (GetPlayerLink())
		return GetPlayerLink()->IsDormant();

	return false;
}

int C_ClientAttachment::DrawModel(int flags)
{
	if (attachment_debugging.GetBool())
		return BaseClass::DrawModel(flags);

	if (m_bShouldHide || !bb2_render_weapon_attachments.GetBool())
		return 0;

	C_HL2MP_Player *pOwner = ToHL2MPPlayer(GetPlayerLink());
	C_HL2MP_Player *pLocal = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if ((pOwner == pLocal) && !g_bShouldRenderLocalPlayerExternally && !input->CAM_IsThirdPerson())
		return 0;

	if ((pOwner == pLocal) && (!bb2_render_client_in_mirrors.GetBool() && g_bShouldRenderLocalPlayerExternally) && !input->CAM_IsThirdPerson())
		return 0;

	// If the local player is spectating us then hide attachments!
	if ((pOwner && pLocal) && (pOwner != pLocal))
	{
		int specIndex = GetSpectatorTarget();
		if ((pOwner->entindex() == specIndex) && (pLocal->GetTeamNumber() <= TEAM_SPECTATOR) && (pLocal->GetObserverMode() == OBS_MODE_IN_EYE))
			return 0;
	}

	if (pOwner && pOwner->IsPerkFlagActive(PERK_POWERUP_PREDATOR))
	{
		modelrender->ForcedMaterialOverride(GlobalRenderEffects->GetCloakOverlay());
		int retVal = BaseClass::DrawModel(STUDIO_RENDER | STUDIO_TRANSPARENCY);
		modelrender->ForcedMaterialOverride(0);
		return retVal;
	}

	return BaseClass::DrawModel(flags);
}
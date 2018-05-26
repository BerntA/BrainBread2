//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: Attachments is used for player models. Such as attaching a helmet, weapon, etc to the player model ( thirdperson model ).
//
//========================================================================================//

#ifndef C_CLIENT_ATTACHMENT_H
#define C_CLIENT_ATTACHMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "takedamageinfo.h"
#include "c_hl2mp_player.h"

class C_ClientAttachment : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_ClientAttachment, C_BaseAnimating);

	C_ClientAttachment(void);
	virtual ~C_ClientAttachment();

	virtual bool Initialize(int type, int param);
	virtual void Spawn();
	virtual void ReleaseSafely(void) { m_bShouldDelete = true; }
	virtual void Release(void);
	virtual void ClientThink(void);
	virtual void PerformUpdateCheck(void);

	virtual bool IsDormant(void);
	virtual int DrawModel(int flags);
	virtual CollideType_t GetCollideType(void) { return ENTITY_SHOULD_NOT_COLLIDE; }
	virtual bool IsClientCreated(void) const { return true; }
	virtual ShadowType_t ShadowCastType() { return ShadowType_t::SHADOWS_NONE; }

	virtual void SetPlayerLink(C_BaseEntity *pLink)
	{
		m_pPlayer = pLink;
	}

	virtual C_BaseEntity *GetPlayerLink(void)
	{
		return m_pPlayer.Get();
	}

	virtual C_BaseEntity *GetOtherLink(void)
	{
		return m_pOther.Get();
	}

	virtual int GetOtherLinkEntindex(void)
	{
		C_BaseEntity *pLink = GetOtherLink();
		if (!pLink)
			return -1;

		return pLink->entindex();
	}

protected:
	bool m_bShouldDelete;
	bool m_bShouldHide;
	bool m_bNeedsUpdate;
	int m_iParameter;
	int m_iAttachmentType;
	float m_flUpdateTime;
	EHANDLE m_pPlayer;
	EHANDLE m_pOther;
};

extern bool RemoveAllClientAttachments();
extern bool HasAnyClientAttachments();
extern C_ClientAttachment *CreateClientAttachment(C_HL2MP_Player *pParent, int type, int param, bool bonemerge);

#endif // C_CLIENT_ATTACHMENT_H
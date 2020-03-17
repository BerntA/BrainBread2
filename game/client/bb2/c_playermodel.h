//=========       Copyright © Reperio Studios 2018 @ Bernt Andreas Eide!       ============//
//
// Purpose: Enables support for client-based playermodels like in Sven Co-Op | HL1 goldsrc...
//
//========================================================================================//

#ifndef C_PLAYERMODEL_H
#define C_PLAYERMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseflex.h"
#include "baseentity_shared.h"

class C_HL2MP_Player;
class C_Playermodel : public C_BaseFlex
{
public:
	DECLARE_CLASS(C_Playermodel, C_BaseFlex);

	C_Playermodel(void);
	virtual ~C_Playermodel();

	virtual bool Initialize(void);
	virtual void Release(void);

	virtual int ObjectCaps() { return FCAP_DONT_SAVE; }
	virtual ShadowType_t ShadowCastType();
	virtual bool ShouldDraw();
	virtual bool ShouldReceiveProjectedTextures(int flags);
	virtual void OnDormantStateChange(void);
	virtual void OnUpdate(void);
	virtual CStudioHdr *OnNewModel(void);

	virtual bool IsDormant(void);
	virtual int DrawModel(int flags);
	virtual bool IsClientCreated(void) const { return true; }

	virtual void SetPlayerOwner(C_HL2MP_Player *pPlayer) { m_pPlayer = pPlayer; }
	virtual C_HL2MP_Player *GetPlayerOwner() { return m_pPlayer; }

	virtual void UpdateModel(void);
	virtual void DrawHelmet(bool bRender = false);
	virtual void DeleteHelmet(void); // Fades out the helmet!

protected:
	C_HL2MP_Player *m_pPlayer;
	CNewParticleEffect *m_pParticleHelmet; // A fancy thing given to either high lvl plrs, admins, devs!
};

extern bool RemoveAllClientPlayermodels();
extern C_Playermodel *CreateClientPlayermodel(C_HL2MP_Player *pParent);

#endif // C_PLAYERMODEL_H
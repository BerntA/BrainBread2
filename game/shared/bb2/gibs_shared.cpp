//=========       Copyright © Reperio Studios 2015 @ Bernt Andreas Eide!       ============//
//
// Purpose: BrainBread 2 Shared Gib Details!
// Setting the bodygroups with the value 1 means gibbed, 0 is non gibbed.
// Ragdoll info will be found here as well!
//
//========================================================================================//

#include "cbase.h"
#include "gibs_shared.h"
#include "GameBase_Shared.h"
#include "hl2mp_gamerules.h"

#ifdef CLIENT_DLL
#include "c_ai_basenpc.h"
#include "c_basecombatcharacter.h"
#include "c_client_gib.h"
#else
#include "ai_basenpc.h"
#include "basecombatcharacter.h"
#endif

const char *PLAYER_BODY_ACCESSORY_BODYGROUPS[PLAYER_ACCESSORY_MAX] =
{
	"extra_head",
	"extra_body",
	"extra_leg_left",
	"extra_leg_right",
};

// WHEN WE EXPLODE THIS IS THE GIB 'LIST' TO SPAWN, IF ONE OF THESE GIBS HAVE BEEN SET ALREADY THEN IT WILL SKIP TO THE NEXT.
gibDataInfo GIB_BODYGROUPS[5] =
{
	{ "head", "gore_head", GIB_NO_HEAD, HITGROUP_HEAD },
	{ "arms_left", "gore_arm_left", GIB_NO_ARM_LEFT, HITGROUP_LEFTARM },
	{ "arms_right", "gore_arm_right", GIB_NO_ARM_RIGHT, HITGROUP_RIGHTARM },
	{ "legs_left", "gore_leg_left", GIB_NO_LEG_LEFT, HITGROUP_LEFTLEG },
	{ "legs_right", "gore_leg_right", GIB_NO_LEG_RIGHT, HITGROUP_RIGHTLEG },
};

// THIS ARRAY MUST BE IN THE SAME ORDER AS THE HITGROUPS! VALUE 0-7!
gibSharedDataItem GIB_SHARED_DATA[8] =
{
	{ "Gibs.Explode", "gore_body_lower", "", "", true, 0 },
	{ "Gibs.Head", "gore_head", GIB_BODYGROUP_BASE_HEAD, "head", false, GIB_NO_HEAD },
	{ "Gibs.Explode", "gore_body_lower", "", "", true, 0 },
	{ "Gibs.Explode", "gore_body_lower", "", "", true, 0 },
	{ "Gibs.Arm", "gore_arm_left", GIB_BODYGROUP_BASE_ARM_LEFT, "arm", false, GIB_NO_ARM_LEFT },
	{ "Gibs.Arm", "gore_arm_right", GIB_BODYGROUP_BASE_ARM_RIGHT, "arm", false, GIB_NO_ARM_RIGHT },
	{ "Gibs.Leg", "gore_leg_left", GIB_BODYGROUP_BASE_LEG_LEFT, "leg", false, GIB_NO_LEG_LEFT },
	{ "Gibs.Leg", "gore_leg_right", GIB_BODYGROUP_BASE_LEG_RIGHT, "leg", false, GIB_NO_LEG_RIGHT },
};

#ifdef CLIENT_DLL
void DispatchClientSideGib(C_ClientRagdollGib *pVictim, const Vector& velocity, const char *gib)
#else
void DispatchClientSideGib(CBaseCombatCharacter *pVictim, const char *gib)
#endif
{
	if (!pVictim)
		return;

	CPASFilter filter(pVictim->WorldSpaceCenter());
	int modelIndex = modelinfo->GetModelIndex(gib);
	if (!modelIndex)
	{
		Warning("Gib %s has an invalid model index, is it precached?\n", gib);
		return;
	}

	Vector vecNewVelocity;
#ifndef CLIENT_DLL
	vecNewVelocity = g_vecAttackDir * -1;
	vecNewVelocity.x += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity.y += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity.z += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity *= 900;
#else
	vecNewVelocity = velocity;
#endif

	if (Q_strstr(gib, "head"))
	{
		te->ClientSideGib(filter, -1, modelIndex, 0, pVictim->m_nSkin, pVictim->GetAbsOrigin(), pVictim->GetAbsAngles(), vecNewVelocity, 0, 0, CLIENT_GIB_RAGDOLL_NORMAL_PHYSICS, 0);
		return;
	}

	te->ClientSideGib(filter, -1, modelIndex, 0, pVictim->m_nSkin, pVictim->GetAbsOrigin(), pVictim->GetAbsAngles(), vecNewVelocity, 0, 0, CLIENT_GIB_RAGDOLL, 0);
}

#ifdef CLIENT_DLL
const char *GetGibModel(C_ClientRagdollGib *pVictim, const char *gib)
{
	if (pVictim && strlen(pVictim->pchNPCName) > 0)
		return GameBaseShared()->GetNPCData()->GetModelForGib(pVictim->pchNPCName, gib, STRING(pVictim->GetModelName()));
	else if (pVictim && (pVictim->GetPlayerIndexLink() > 0))
		return GameBaseShared()->GetSharedGameDetails()->GetPlayerGibForModel(gib, STRING(pVictim->GetModelName()));

	return "";
}

bool C_ClientRagdollGib::CanGibEntity(const Vector &velocity, int hitgroup, int damageType)
{
	C_HL2MP_Player *pClient = C_HL2MP_Player::GetLocalHL2MPPlayer();
	if (!pClient)
		return false;

	if (hitgroup < 0 || hitgroup >= _ARRAYSIZE(GIB_SHARED_DATA))
		return false;

	bool bIsNPC = (strlen(pchNPCName) > 0);
	bool bIsPlayer = (GetPlayerIndexLink() > 0);
	if (!bIsNPC && !bIsPlayer)
		return false;

	gibSharedDataItem gibInfo = GIB_SHARED_DATA[hitgroup];

	if (bIsNPC && !GameBaseShared()->GetNPCData()->DoesNPCHaveGibForLimb(pchNPCName, STRING(GetModelName()), gibInfo.gibFlag))
		return false;

	if (bIsPlayer && !GameBaseShared()->GetSharedGameDetails()->DoesPlayerHaveGibForLimb(STRING(GetModelName()), gibInfo.gibFlag))
		return false;

	int nGibFlag = gibInfo.gibFlag;
	bool bCanPopHead = ((damageType == DMG_BUCKSHOT) || (damageType == DMG_BULLET) || (damageType == DMG_BLAST));
	const char *pszHitGroup = gibInfo.bodygroup, *pszAttachmentPoint = gibInfo.attachmentName, *pszSoundScript = gibInfo.soundscript;

	// Spit out the rest of the gibs:
	if (damageType == DMG_BLAST)
	{
		for (int i = 0; i < _ARRAYSIZE(GIB_BODYGROUPS); i++)
		{
			bool bCanDispatch = true;
			if (GIB_BODYGROUPS[i].hitgroup == HITGROUP_HEAD)
				bCanDispatch = !bCanPopHead;

			const char *pszGib = GetGibModel(this, GIB_BODYGROUPS[i].bodygroup);
			if (strlen(pszGib) <= 0)
				continue;

			int iGibFlag = GIB_BODYGROUPS[i].flag;
			if (IsGibFlagActive(iGibFlag))
				continue;

			int gib_bodygroup = FindBodygroupByName(GIB_BODYGROUPS[i].bodygroup);
			if (gib_bodygroup == -1)
				continue;

			AddGibFlag(iGibFlag);
			SetBodygroup(gib_bodygroup, 1);

			if (bCanDispatch)
				DispatchClientSideGib(this, velocity, pszGib);

			if (strlen(pszAttachmentPoint) > 0)
			{
				int iAttachment = LookupAttachment(GIB_BODYGROUPS[i].attachmentName);
				if (iAttachment != -1)
					UTIL_GibImpact(this, iAttachment, BLOOD_COLOR_RED, GIB_BODYGROUPS[i].hitgroup);
			}
		}

		if (strlen(pszSoundScript) > 0)
			EmitSound(pszSoundScript);

		OnGibbedGroup(0, true);
		return true;
	}

	if ((strlen(pszHitGroup) <= 0) || nGibFlag <= 0 || IsGibFlagActive(nGibFlag))
		return false;

	const char *pszGib = GetGibModel(this, pszHitGroup);
	if (strlen(pszGib) <= 0)
		return false;

	int gib_bodygroup = FindBodygroupByName(pszHitGroup);
	if (gib_bodygroup == -1)
		return false;

	bool bCanDispatch = true;
	if (hitgroup == HITGROUP_HEAD)
		bCanDispatch = !bCanPopHead;

	AddGibFlag(nGibFlag);
	SetBodygroup(gib_bodygroup, 1);

	if (bCanDispatch)
		DispatchClientSideGib(this, velocity, pszGib);

	if (strlen(pszAttachmentPoint) > 0)
	{
		int iAttachment = LookupAttachment(pszAttachmentPoint);
		if (iAttachment != -1)
			UTIL_GibImpact(this, iAttachment, BLOOD_COLOR_RED, hitgroup);
	}

	if (strlen(pszSoundScript) > 0)
		EmitSound(pszSoundScript);

	OnGibbedGroup(hitgroup, false);
	return true;
}
#else
const char *GetGibModel(CBaseCombatCharacter *pVictim, const char *gib)
{
	if (pVictim && pVictim->IsNPC() && pVictim->MyNPCPointer())
		return GameBaseShared()->GetNPCData()->GetModelForGib(pVictim->MyNPCPointer()->GetFriendlyName(), gib, STRING(pVictim->GetModelName()));
	else if (pVictim && pVictim->IsPlayer())
		return GameBaseShared()->GetSharedGameDetails()->GetPlayerGibForModel(gib, STRING(pVictim->GetModelName()));

	return "";
}

void CBaseCombatCharacter::OnSetGibHealth(void)
{
	// Setup Gib Health:
	const char *limbnames[4] =
	{
		"arm", // Left
		"arm", // Right
		"leg", // Left
		"leg", // Right
	};

	for (int i = 0; i < _ARRAYSIZE(m_flGibHealth); i++)
	{
		if (IsPlayer())
			m_flGibHealth[i] = GameBaseShared()->GetSharedGameDetails()->GetPlayerLimbData(limbnames[i], GetTeamNumber(), true);
		else if (IsNPC() && MyNPCPointer() && (MyNPCPointer()->GetFriendlyName()[0]))
			m_flGibHealth[i] = GameBaseShared()->GetNPCData()->GetLimbData(MyNPCPointer()->GetFriendlyName(), limbnames[i], true);
	}
}

bool CBaseCombatCharacter::CanGibEntity(const CTakeDamageInfo &info)
{
	if (!AllowEntityToBeGibbed())
		return false;

	int iHitGroup = LastHitGroup();
	if (iHitGroup < 0 || iHitGroup >= _ARRAYSIZE(GIB_SHARED_DATA))
		return false;

	gibSharedDataItem gibInfo = GIB_SHARED_DATA[iHitGroup];

	float flGibHealth = GetExplodeFactor();
	float flDamage = info.GetDamage();

	bool bCanPopHead = ((info.GetDamageType() & DMG_BUCKSHOT) || (info.GetDamageType() & DMG_BULLET) || (info.GetDamageType() & DMG_BLAST));
	bool bCanExplode = gibInfo.bCanExplode;
	int nGibFlag = gibInfo.gibFlag;

	const char *pszFriendlyName = NULL, *pszHitGroup = gibInfo.bodygroup, *pszAttachmentPoint = gibInfo.attachmentName, *pszSoundScript = gibInfo.soundscript;
	if (IsNPC() && MyNPCPointer())
		pszFriendlyName = MyNPCPointer()->GetFriendlyName();

	if (!bCanExplode)
	{
		flGibHealth = 0.0f;
		int gibHPIndex = -1;
		if (iHitGroup == HITGROUP_LEFTARM)
			gibHPIndex = 0;
		else if (iHitGroup == HITGROUP_RIGHTARM)
			gibHPIndex = 1;
		else if (iHitGroup == HITGROUP_LEFTLEG)
			gibHPIndex = 2;
		else if (iHitGroup == HITGROUP_RIGHTLEG)
			gibHPIndex = 3;

		if (gibHPIndex != -1)
		{
			m_flGibHealth[gibHPIndex] -= info.GetDamage();
			flGibHealth = m_flGibHealth[gibHPIndex];
		}
	}

	// Spit out the rest of the gibs:
	if (bCanExplode && (flDamage > flGibHealth) && (flDamage > m_iHealth) && (m_iHealth <= 0))
	{
		AddGibFlag(GIB_FULL_EXPLODE);

		for (int i = 0; i < _ARRAYSIZE(GIB_BODYGROUPS); i++)
		{
			bool bCanDispatch = true;
			if (GIB_BODYGROUPS[i].hitgroup == HITGROUP_HEAD)
				bCanDispatch = !bCanPopHead;

			const char *pszGib = GetGibModel(this, GIB_BODYGROUPS[i].bodygroup);
			if (strlen(pszGib) <= 0)
				continue;

			int iGibFlag = GIB_BODYGROUPS[i].flag;
			if (IsGibFlagActive(iGibFlag))
				continue;

			int gib_bodygroup = FindBodygroupByName(GIB_BODYGROUPS[i].bodygroup);
			if (gib_bodygroup == -1)
				continue;				

			AddGibFlag(iGibFlag);
			SetBodygroup(gib_bodygroup, 1);

			if (bCanDispatch)
				DispatchClientSideGib(this, pszGib);
		}

		if (strlen(pszSoundScript) > 0)
			EmitSound(pszSoundScript);

		OnGibbedGroup(0, true);
		return true;
	}

	if ((strlen(pszHitGroup) <= 0) || bCanExplode || (AllowEntityToBeGibbed() != GIB_FULL_GIBS))
		return false;

	if (nGibFlag <= 0 || IsGibFlagActive(nGibFlag))
		return false;

	const char *pszGib = GetGibModel(this, pszHitGroup);
	if (strlen(pszGib) <= 0)
		return false;

	int gib_bodygroup = FindBodygroupByName(pszHitGroup);
	if (gib_bodygroup == -1)
		return false;

	if (nGibFlag == GIB_NO_HEAD && (m_iHealth > 0))
		return false;

	if (flGibHealth <= 0.0f)
	{
		bool bCanDispatch = true;
		if (iHitGroup == HITGROUP_HEAD)
			bCanDispatch = !bCanPopHead;

		AddGibFlag(nGibFlag);
		SetBodygroup(gib_bodygroup, 1);

		if (bCanDispatch)
			DispatchClientSideGib(this, pszGib);

		if ((strlen(pszAttachmentPoint) > 0) && (iHitGroup != HITGROUP_HEAD))
		{
			int iAttachment = LookupAttachment(pszAttachmentPoint);
			if (iAttachment != -1)
				UTIL_GibImpact(this, iAttachment, BLOOD_COLOR_RED, iHitGroup);
		}

		if (strlen(pszSoundScript) > 0)
			EmitSound(pszSoundScript);

		OnGibbedGroup(iHitGroup, false);
		return true;
	}

	return false;
}
#endif
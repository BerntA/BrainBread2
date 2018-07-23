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
#include "te_player_gib.h"
#endif

const char *PLAYER_BODY_ACCESSORY_BODYGROUPS[PLAYER_ACCESSORY_MAX] =
{
	"extra_head",
	"extra_body",
	"extra_leg_left",
	"extra_leg_right",
};

// WHEN WE EXPLODE THIS IS THE GIB 'LIST' TO SPAWN, IF ONE OF THESE GIBS HAVE BEEN SET ALREADY THEN IT WILL SKIP TO THE NEXT.
gibDataInfo GIB_BODYGROUPS[PLAYER_GIB_GROUPS_MAX] =
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
void DispatchClientSideGib(C_ClientRagdollGib *pVictim, const Vector& velocity, const char *gib, int gibType)
#else
void DispatchClientSideGib(CBaseCombatCharacter *pVictim, const char *gib, int gibType)
#endif
{
	if (!pVictim)
		return;

	Vector vecNewVelocity;
#ifndef CLIENT_DLL
	vecNewVelocity = g_vecAttackDir * -1;
	vecNewVelocity.x += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity.y += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity.z += random->RandomFloat(-0.15, 0.15);
	vecNewVelocity *= 900;

	if (pVictim->IsPlayer())
	{
		TE_PlayerGibRagdoll(
			pVictim->entindex(),
			gibType,
			(gibType == GIB_NO_HEAD) ? CLIENT_GIB_RAGDOLL_NORMAL_PHYSICS : CLIENT_GIB_RAGDOLL,
			pVictim->WorldSpaceCenter(),
			pVictim->GetAbsOrigin(),
			vecNewVelocity,
			pVictim->GetAbsAngles());
		return;
	}
#else
	vecNewVelocity = velocity;

	if (pVictim->GetPlayerLinkTeam() > 0)
	{
		SpawnGibOrRagdollForPlayer(
			pVictim,
			0, pVictim->GetPlayerLinkTeam(),
			pVictim->GetPlayerLinkSurvivor(),
			gibType,
			(gibType == GIB_NO_HEAD) ? CLIENT_GIB_RAGDOLL_NORMAL_PHYSICS : CLIENT_GIB_RAGDOLL,
			pVictim->GetAbsOrigin(), vecNewVelocity, pVictim->GetAbsAngles());
		return;
	}
#endif

	CPASFilter filter(pVictim->WorldSpaceCenter());
	int modelIndex = modelinfo->GetModelIndex(gib);
	if (!modelIndex)
	{
		Warning("Gib %s has an invalid model index, is it precached?\n", gib);
		return;
	}

	if (gibType == GIB_NO_HEAD)
	{
		te->ClientSideGib(filter, -1, modelIndex, 0, pVictim->m_nSkin, pVictim->GetAbsOrigin(), pVictim->GetAbsAngles(), vecNewVelocity, 0, 0, CLIENT_GIB_RAGDOLL_NORMAL_PHYSICS);
		return;
	}

	te->ClientSideGib(filter, -1, modelIndex, 0, pVictim->m_nSkin, pVictim->GetAbsOrigin(), pVictim->GetAbsAngles(), vecNewVelocity, 0, 0, CLIENT_GIB_RAGDOLL);
}

#ifdef CLIENT_DLL
const char *GetGibModel(C_ClientRagdollGib *pVictim, const char *gib)
{
	if (pVictim && strlen(pVictim->pchNPCName) > 0)
		return GameBaseShared()->GetNPCData()->GetModelForGib(pVictim->pchNPCName, gib, STRING(pVictim->GetModelName()));
	else if (pVictim && (pVictim->GetPlayerLinkTeam() > 0))
		return GameBaseShared()->GetSharedGameDetails()->GetPlayerGibForModel(pVictim->GetPlayerLinkSurvivor(), !(pVictim->GetPlayerLinkTeam() == TEAM_DECEASED), gib);

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
	bool bIsPlayer = (GetPlayerLinkTeam() > 0);
	if (!bIsNPC && !bIsPlayer)
		return false;

	gibSharedDataItem *gibInfo = &GIB_SHARED_DATA[hitgroup];

	if (bIsNPC && !GameBaseShared()->GetNPCData()->DoesNPCHaveGibForLimb(pchNPCName, STRING(GetModelName()), gibInfo->gibFlag))
		return false;

	if (bIsPlayer && !GameBaseShared()->GetSharedGameDetails()->DoesPlayerHaveGibForLimb(GetPlayerLinkSurvivor(), !(GetPlayerLinkTeam() == TEAM_DECEASED), gibInfo->gibFlag))
		return false;	

	int nGibFlag = gibInfo->gibFlag;
	bool bCanPopHead = ((damageType == DMG_BUCKSHOT) || (damageType == DMG_BULLET) || (damageType == DMG_BLAST));
	const char *pszHitGroup = gibInfo->bodygroup, *pszAttachmentPoint = gibInfo->attachmentName, *pszSoundScript = gibInfo->soundscript;

	// Spit out the rest of the gibs:
	if (damageType == DMG_BLAST)
	{
		for (int i = 0; i < _ARRAYSIZE(GIB_BODYGROUPS); i++)
		{
			bool bCanDispatch = true;
			if (GIB_BODYGROUPS[i].hitgroup == HITGROUP_HEAD)
				bCanDispatch = !bCanPopHead;

			int iGibFlag = GIB_BODYGROUPS[i].flag;
			if (IsGibFlagActive(iGibFlag))
				continue;

			const char *pszGib = GetGibModel(this, GIB_BODYGROUPS[i].bodygroup);
			if (strlen(pszGib) <= 0)
				continue;

			int gib_bodygroup = FindBodygroupByName(GIB_BODYGROUPS[i].bodygroup);
			if (gib_bodygroup == -1)
				continue;

			AddGibFlag(iGibFlag);
			SetBodygroup(gib_bodygroup, 1);

			if (bCanDispatch)
				DispatchClientSideGib(this, velocity, pszGib, iGibFlag);

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

	if ((nGibFlag <= 0) || IsGibFlagActive(nGibFlag) || (strlen(pszHitGroup) <= 0))
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
		DispatchClientSideGib(this, velocity, pszGib, nGibFlag);

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

void OnClientPlayerRagdollSpawned(C_ClientRagdollGib *ragdoll, int gibFlags)
{
	if (ragdoll == NULL)
		return;

	if (ragdoll->IsGibFlagActive(GIB_FULL_EXPLODE))
	{
		ragdoll->m_nBody = 0;
		for (int i = 0; i < _ARRAYSIZE(GIB_BODYGROUPS); i++)
		{
			int gib_bodygroup = ragdoll->FindBodygroupByName(GIB_BODYGROUPS[i].bodygroup);
			if (gib_bodygroup == -1)
				continue;

			ragdoll->SetBodygroup(gib_bodygroup, 1);
		}

		return;
	}

	for (int i = 0; i < _ARRAYSIZE(GIB_BODYGROUPS); i++)
	{
		if (ragdoll->IsGibFlagActive(GIB_BODYGROUPS[i].flag) == false)
			continue;

		int gib_bodygroup = ragdoll->FindBodygroupByName(GIB_BODYGROUPS[i].bodygroup);
		if (gib_bodygroup == -1)
			continue;

		ragdoll->SetBodygroup(gib_bodygroup, 1);
		ragdoll->OnGibbedGroup(GIB_BODYGROUPS[i].hitgroup, false);
	}
}
#else
const char *GetGibModel(CBaseCombatCharacter *pVictim, const char *gib)
{
	if (pVictim && pVictim->IsNPC() && pVictim->MyNPCPointer())
		return GameBaseShared()->GetNPCData()->GetModelForGib(pVictim->MyNPCPointer()->GetFriendlyName(), gib, STRING(pVictim->GetModelName()));

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

	gibSharedDataItem *gibInfo = &GIB_SHARED_DATA[iHitGroup];

	float flGibHealth = GetExplodeFactor();
	float flDamage = info.GetDamage();

	bool bIsPlayer = IsPlayer();
	bool bCanPopHead = ((info.GetDamageType() & DMG_BUCKSHOT) || (info.GetDamageType() & DMG_BULLET) || (info.GetDamageType() & DMG_BLAST));
	bool bCanExplode = gibInfo->bCanExplode;
	int nGibFlag = gibInfo->gibFlag;

	const char *pszFriendlyName = NULL, *pszHitGroup = gibInfo->bodygroup, *pszAttachmentPoint = gibInfo->attachmentName, *pszSoundScript = gibInfo->soundscript;
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

			int iGibFlag = GIB_BODYGROUPS[i].flag;
			if (IsGibFlagActive(iGibFlag))
				continue;

			const char *pszGib = "";
			if (bIsPlayer == false)
			{
				pszGib = GetGibModel(this, GIB_BODYGROUPS[i].bodygroup);
				if (strlen(pszGib) <= 0)
					continue;

				int gib_bodygroup = FindBodygroupByName(GIB_BODYGROUPS[i].bodygroup);
				if (gib_bodygroup == -1)
					continue;

				SetBodygroup(gib_bodygroup, 1);
			}

			AddGibFlag(iGibFlag);
			if (bCanDispatch)
				DispatchClientSideGib(this, pszGib, iGibFlag);
		}

		if (strlen(pszSoundScript) > 0)
			EmitSound(pszSoundScript);

		OnGibbedGroup(0, true);
		return true;
	}

	if ((nGibFlag <= 0) || IsGibFlagActive(nGibFlag) || bCanExplode || (AllowEntityToBeGibbed() != GIB_FULL_GIBS) || (flGibHealth > 0.0f) || (strlen(pszHitGroup) <= 0))
		return false;

	if ((nGibFlag == GIB_NO_HEAD) && (m_iHealth > 0))
		return false;

	const char *pszGib = "";
	if (bIsPlayer == false)
	{
		pszGib = GetGibModel(this, pszHitGroup);
		if (strlen(pszGib) <= 0)
			return false;

		int gib_bodygroup = FindBodygroupByName(pszHitGroup);
		if (gib_bodygroup == -1)
			return false;

		SetBodygroup(gib_bodygroup, 1);
	}

	bool bCanDispatch = true;
	if (iHitGroup == HITGROUP_HEAD)
		bCanDispatch = !bCanPopHead;

	AddGibFlag(nGibFlag);

	if (bCanDispatch)
		DispatchClientSideGib(this, pszGib, nGibFlag);

	if ((iHitGroup != HITGROUP_HEAD) && (strlen(pszAttachmentPoint) > 0))
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
#endif